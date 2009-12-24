/*
 * packagesource.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009 - Denis Steckelmacher <steckdenis@logram-project.org>
 *
 * Logram is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Logram is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Logram; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "packagesource.h"
#include "packagemetadata.h"
#include "packagesystem.h"

#include <QHash>
#include <QProcess>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QRegExp>
#include <QVector>

#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>

#include <QtXml>
#include <QtDebug>

using namespace Logram;

struct PackageSource::Private
{
    PackageSystem *ps;
    PackageMetaData *md;
    PackageSource *src;
    QString metaFileName;
    
    QHash<Option, QVariant> options;
};

static void listFiles(const QString &dir, const QString &prefix, QStringList &list)
{
    QDir mdir(dir);
    QFileInfoList files = mdir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    
    foreach(const QFileInfo &fi, files)
    {
        // Si c'est un fichier, l'ajouter à la liste
        if (fi.isFile())
        {
            list.append(prefix + fi.fileName());
        }
        else if (fi.isDir())
        {
            // L'explorer
            listFiles(fi.absoluteFilePath(), prefix + fi.fileName() + '/', list);
        }
    }
}

/*****************************************************************************/

PackageSource::PackageSource(PackageSystem *ps) : Templatable(ps)
{
    d = new Private;
    
    d->ps = ps;
    d->src = this;
}

PackageSource::~PackageSource()
{
    delete d->md;
    
    delete d;
}

bool PackageSource::setMetaData(const QString &fileName)
{
    d->md = new PackageMetaData(d->ps);
    
    d->md->loadFile(fileName, QByteArray(), false);
    d->md->setTemplatable(this);
    d->metaFileName = fileName;
    
    return !d->md->error();
}

bool PackageSource::setMetaData(const QByteArray &data)
{
    d->md = new PackageMetaData(d->ps);
    
    d->md->loadData(data);
    d->metaFileName = "/tmp/metadata_" + d->md->documentElement() \
                                         .firstChildElement("source") \
                                         .attribute("name");

    return !d->md->error();
}

void PackageSource::setOption(Option opt, const QVariant &value)
{
    d->options.insert(opt, value); // Remplace une éventuelle valeur déjà prévue
}

QVariant PackageSource::option(Option opt, const QVariant &defaultValue)
{
    return d->options.value(opt, defaultValue);
}

void PackageSource::loadKeys()
{
    QString cDir = QDir::currentPath();
    QString sourceDir = option(SourceDir, cDir + "/src").toString();
    QString buildDir = option(BuildDir, cDir + "/build").toString();
    QString controlDir = option(ControlDir, cDir + "/control").toString();
    
    addKey("sourcedir", sourceDir);
    addKey("builddir", buildDir);
    addKey("controldir", controlDir);
}

bool PackageSource::getSource()
{
    // Récupérer les sources du paquet, donc les télécharger (plus précisément lancer le script source)
    QString sourceDir = getKey("sourcedir");
    
    if (!checkSource(sourceDir, true))
    {
        return false;
    }
    
    if (!d->md->runScript("source", "download", sourceDir, QStringList()))
    {
        return false;
    }

    return true;
}

bool PackageSource::build()
{
    // Construire la source. Le script est lancé dans sourceDir, {{destdir}} est BuildDir
    QString sourceDir = getKey("sourcedir");
    
    if (!checkSource(sourceDir, false))
    {
        return false;
    }
    
    if (!d->md->runScript("source", "build", sourceDir, QStringList()))
    {
        return false;
    }
    
    return true;
}

struct PackageFile
{
    QString from;
    QString to;
};

bool PackageSource::binaries()
{   
    // Créer la liste des fichiers dans le dossier de construction
    QStringList files;
    QString buildDir = getKey("builddir");
    
    listFiles(buildDir, "/", files);
    
    // Trouver la version
    QString version;
    QDomElement changelog = d->md->documentElement()
                                .firstChildElement("changelog")
                                .firstChildElement("entry");
    
    if (!changelog.isNull())
    {
        // Savoir s'il faut ajouter la version de développement
        if (d->md->documentElement()
            .firstChildElement("source")
            .attribute("devel", "false") == "true")
        {
            QString sourceDir = getKey("sourcedir");
    
            if (!checkSource(sourceDir, false))
            {
                return false;
            }
            
            if (!d->md->runScript("source", "version", sourceDir, QStringList()))
            {
                return false;
            }
            
            addKey("develver", d->md->scriptOut().trimmed());
        }
        
        // Templater la version
        version = templateString(changelog.attribute("version"));
        
        addKey("version", version);
        changelog.setAttribute("version", version);
    }
    
    // Explorer les paquets
    QDomElement package = d->md->documentElement()
                            .firstChildElement("package");
    
    int curPkg = 0;
    int totPkg = d->md->documentElement().elementsByTagName("package").count();
    
    while (!package.isNull())
    {
        QString packageName = package.attribute("name");
        QStringList okFiles;
        
        // Obenir la version
        QString arch = package.attribute("arch", "any");
        
        if (arch == "any")
        {
#ifdef L__ARCH
            arch = L__ARCH; // Architecture forcée à la compilation
#else
            if (sizeof(void *) == 4)
            {
                arch = "i686";
            }
            else if (sizeof(void *) == 8)
            {
                arch = "x86_64";
            }
            else
            {
                arch = "unknown";
            }
#endif
        }
        
        // Obtenir le nom de fichier
        QString packageFile = packageName + "~" + version + "." + arch + ".lpk";
        
        // Envoyer le signal de progression
        d->ps->sendProgress(PackageSystem::GlobalCompressing, curPkg, totPkg, packageFile);
        
        curPkg++;
        
        // Explorer les <files pattern="" /> et ajouter à okFiles les fichiers qui conviennent
        QDomElement pattern = package.firstChildElement("files");
        
        while (!pattern.isNull())
        {
            QRegExp regex(pattern.attribute("pattern", "*"), Qt::CaseSensitive, QRegExp::Wildcard);
            
            foreach (const QString &fname, files)
            {
                if (regex.exactMatch(fname))
                {
                    okFiles.append(fname);
                }
            }
            
            pattern = pattern.nextSiblingElement("files");
        }
        
        // Écrire les métadonnées dans /tmp
        QFile fl(d->metaFileName);
        
        if (!fl.open(QIODevice::WriteOnly))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = d->metaFileName;
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        fl.write(d->md->toByteArray(4));
        fl.close();
        
        // Créer la liste des PackageFiles permettant de créer l'archive
        QVector<PackageFile> packageFiles;
        PackageFile pf;
        
        foreach (const QString &fname, okFiles)
        {
            pf.from = buildDir + fname;
            pf.to = "data" + fname;
            packageFiles.append(pf);
        }
        
        // Inclure certains fichiers nécessaires, comme le fichier de métadonnées et ceux
        // que le paquet veut inclure
        pf.from = d->metaFileName;
        pf.to = "control/metadata.xml";
        packageFiles.append(pf);
        
        QDomElement embed = package.firstChildElement("embed");
        
        while (!embed.isNull())
        {
            pf.from = templateString(embed.attribute("from"));
            pf.to = templateString(embed.attribute("to"));
            
            packageFiles.append(pf);
            
            embed = embed.nextSiblingElement("embed");
        }
        
        // Créer l'archive .lpk (tar + xz) (code largement inspiré de l'exemple de "man archive_write")
        struct archive *a;
        struct archive *disk;
        struct archive_entry *entry;
        struct stat st;
        char *buff = new char[8192];
        int len;
        int fd;
        
        a = archive_write_new();
        archive_write_set_compression_xz(a);
        archive_write_set_format_ustar(a);
        archive_write_open_filename(a, qPrintable(packageFile));
        
        disk = archive_read_disk_new();
        archive_read_disk_set_symlink_logical(disk);
        archive_read_disk_set_standard_lookup(disk);
        
        for (int i=0; i<packageFiles.count(); ++i)
        {
            const PackageFile &pf = packageFiles.at(i);
            
            d->ps->sendProgress(PackageSystem::Compressing, i, packageFiles.count(), pf.to);
            
            // Ajouter l'élément
            stat(qPrintable(pf.from), &st);
            
            entry = archive_entry_new();
            archive_entry_set_pathname(entry, qPrintable(pf.to));
            archive_read_disk_entry_from_file(disk, entry, -1, &st);
            
            archive_write_header(a, entry);
            fd = open(qPrintable(pf.from), O_RDONLY);
            len = read(fd, buff, 8192);
            while (len > 0)
            {
                archive_write_data(a, buff, len);
                len = read(fd, buff, 8192);
            }
            close(fd);
            archive_entry_free(entry);
        }
        
        archive_write_close(a);
        archive_write_finish(a);
        delete[] buff;
        
        package = package.nextSiblingElement("package");
    }
    
    d->ps->endProgress(PackageSystem::GlobalCompressing, totPkg);
    
    return true;
}

bool PackageSource::checkSource(const QString &dir, bool fail)
{
    if (!QFile::exists(dir))
    {
        if (fail)
        {
            QDir root("/");
        
            root.mkpath(dir);
        }
        else if (!getSource())
        {
            return false;
        }
    }
    
    return true;
}