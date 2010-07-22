/*
 * packagesource.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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
#include <QDirIterator>
#include <QPluginLoader>

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <QtXml>
#include <QtDebug>

using namespace Logram;

struct PackageSource::Private
{
    PackageSystem *ps;
    PackageMetaData *md;
    PackageSource *src;
    
    QHash<Option, QVariant> options;
    QHash<QString, PackageSourceInterface *> plugins;
    
    QVector<PackageRemark *> remarks;
};

void PackageSource::listFiles(const QString &dir, const QString &prefix, QStringList &list)
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
            int oldcount = list.count();
            
            listFiles(fi.absoluteFilePath(), prefix + fi.fileName() + '/', list);

            // Si on n'a pas ajouté d'entrées, alors le dossier est vide et il faut le gérer
            if (oldcount == list.count())
            {
                list.append(prefix + fi.fileName());
            }
        }
    }
}

/*****************************************************************************/

PackageSource::PackageSource(PackageSystem *ps) : Templatable(ps)
{
    d = new Private;
    
    d->ps = ps;
    d->src = this;
    
    // Charger les plugins
    foreach (const QString &pluginPath, ps->pluginPaths())
    {
        QDir pluginDir(pluginPath);
        
        foreach (const QString &fileName, pluginDir.entryList(QDir::Files))
        {
            QPluginLoader loader(pluginDir.absoluteFilePath(fileName));
            PackageSourceInterface *plugin = qobject_cast<PackageSourceInterface *>(loader.instance());
            
            if (plugin)
            {
                d->plugins.insert(plugin->name(), plugin);
            }
        }
    }
}

PackageSource::~PackageSource()
{
    delete d->md;
    
    for (int i=0; i<d->remarks.count(); ++i)
    {
        delete d->remarks.at(i);
    }
    
    delete d;
}

void PackageSource::addPlugins(const QVector<PackageSourceInterface *> plugins)
{
    foreach (PackageSourceInterface *plugin, plugins)
    {
        d->plugins.insert(plugin->name(), plugin);
    }
}

PackageSystem *PackageSource::packageSystem()
{
    return d->ps;
}

bool PackageSource::setMetaData(const QString &fileName)
{
    d->md = new PackageMetaData(d->ps);
    
    d->md->loadFile(fileName, QByteArray(), false);
    d->md->setTemplatable(this);
    
    return !d->md->error();
}

bool PackageSource::setMetaData(const QByteArray &data)
{
    d->md = new PackageMetaData(d->ps);
    
    d->md->loadData(data);

    return !d->md->error();
}

void PackageSource::setOption(Option opt, const QVariant &value)
{
    d->options.insert(opt, value); // Remplace une éventuelle valeur déjà prévue
}

QVariant PackageSource::option(Option opt, const QVariant &defaultValue)
{
    if (!d->options.contains(opt))
    {
        d->options.insert(opt, defaultValue);
        return defaultValue;
    }
    
    return d->options.value(opt);
}

QVector<PackageRemark *> PackageSource::remarks()
{
    return d->remarks;
}

void PackageSource::addRemark(PackageRemark *remark)
{
    d->remarks.append(remark);
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
    addKey("arch", SETUP_ARCH);
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

struct _PackageFile
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
        bool devel = false;
        
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
            
            addKey("develver", d->md->scriptOut());
            devel = true;
        }
        
        // Templater la version
        version = templateString(changelog.attribute("version"));
        
        addKey("version", version);
        
        // Si version de développement, mettre le vraie version sans le {{develver}} à des fins de documentation
        if (devel)
        {
            changelog.setAttribute("realversion", version);
        }
    }
    
    // Voir si on a un enregistrement précédant qu'il faut gérer
    changelog = changelog.nextSiblingElement("entry");
    
    if (!changelog.isNull() && changelog.hasAttribute("realversion"))
    {
        changelog.setAttribute("version", changelog.attribute("realversion"));
        changelog.removeAttribute("realversion");
    }
    
    // Nom du fichier qui va contenir les nouvelles métadonnées
    QString metaFileName = "/tmp/%1~%2.metadata.xml";
    metaFileName = metaFileName.arg(d->md->documentElement().firstChildElement("source").attribute("name"), version);
    
    // Initialiser les plugins
    QHash<QString, PackageSourceInterface *>::const_iterator it;
        
    for (it = d->plugins.constBegin(); it != d->plugins.constEnd(); ++it)
    {
        it.value()->init(d->md, this);
    }
    
    // Première passe d'exploration des paquets : trouver les fichiers et lancer les plugins
    QList<QStringList> packageFiles;
    QDomElement package = d->md->documentElement()
                            .firstChildElement();
    
    while (!package.isNull())
    {
        bool isSource = false;
        
        if (package.tagName() == "source")
        {
            isSource = true;
        }
        else if (package.tagName() != "package")
        {
            package = package.nextSiblingElement();
            continue;
        }
        
        QString packageName = package.attribute("name");
        packageFiles.append(QStringList());
        QStringList &okFiles = packageFiles.last();
        
        // Explorer les <files pattern="" /> et ajouter à okFiles les fichiers qui conviennent
        // Si source, alors on a juste besoin du metadata.xml et du dossier control, s'il existe
        if (isSource)
        {
            if (QFile::exists("control"))
            {
                QDirIterator it("control", QDirIterator::Subdirectories);
                
                while (it.hasNext())
                {
                    it.next();
                    
                    if (it.fileInfo().isFile())
                    {
                        okFiles.append(it.filePath());
                    }
                }
            }
        }
        else
        {
            QDomElement pattern = package.firstChildElement("files");
            
            while (!pattern.isNull())
            {
                QRegExp regex(pattern.attribute("pattern", "*"), Qt::CaseSensitive, QRegExp::Wildcard);
                bool exclude = (pattern.attribute("exclude", "false") == "true");
                
                if (!exclude)
                {
                    foreach (const QString &fname, files)
                    {
                        if (regex.exactMatch(fname))
                        {
                            okFiles.append(fname);
                        }
                    }
                }
                else
                {
                    // Retirer d'okFiles les fichiers qui correspondent
                    for (int i=0; i<okFiles.count(); ++i)
                    {
                        if (regex.exactMatch(okFiles.at(i)))
                        {
                            okFiles.removeAt(i);
                            --i;
                        }
                    }
                }
                
                pattern = pattern.nextSiblingElement("files");
            }
        }
        
        // Explorer les plugins, construire la liste de ceux à lancer sur le paquet, et les lancer
        QStringList plugins;
        QHash<QString, PackageSourceInterface *>::const_iterator it;
        
        for (it = d->plugins.constBegin(); it != d->plugins.constEnd(); ++it)
        {
            if (it.value()->byDefault())
            {
                plugins.append(it.value()->name());
            }
        }
        
        QDomElement plugin = package.firstChildElement("plugin");
        
        while (!plugin.isNull())
        {
            int index = plugins.indexOf(plugin.attribute("name"));
            
            bool contains = (index != -1);
            bool enable = (plugin.attribute("enable", "true") == "true");
            
            if (contains && !enable)
            {
                plugins.removeAt(index);
            }
            else if (!contains && enable)
            {
                plugins.append(plugin.attribute("name"));
            }
            
            plugin = plugin.nextSiblingElement("plugin");
        }
        
        foreach (const QString &pluginName, plugins)
        {
            PackageSourceInterface *p = d->plugins.value(pluginName, 0);
            
            if (p == 0)
            {
                PackageRemark *remark = new PackageRemark;
                
                remark->severity = PackageRemark::Warning;
                remark->packageName = packageName;
                remark->message = tr("Le plugin %1 est activé pour ce paquet mais n'existe pas").arg(pluginName);
                
                addRemark(remark);
            }
            else
            {
                p->processPackage(packageName, okFiles, isSource);
            }
        }
        
        package = package.nextSiblingElement();
    }
    
    // Écrire les métadonnées
    QFile fl(metaFileName);

    if (!fl.open(QIODevice::WriteOnly))
    {
        return false;
    }

    fl.write(d->md->toByteArray(4));
    fl.close();
    
    // Deuxième passe : compression des paquets
    int curPkg = 0;
    int totPkg = d->md->documentElement().elementsByTagName("package").count() + 1; // Aussi la source
    
    int progress = d->ps->startProgress(Progress::GlobalCompressing, totPkg);
    
    package = d->md->documentElement().firstChildElement();
    
    while (!package.isNull())
    {
        bool isSource = false;
        
        if (package.tagName() == "source")
        {
            isSource = true;
        }
        else if (package.tagName() != "package")
        {
            package = package.nextSiblingElement();
            continue;
        }
        
        QString packageName = package.attribute("name");
        const QStringList &okFiles = packageFiles.at(curPkg);
        
        // Obenir la version
        QString arch;
        
        if (isSource)
        {
            arch = "src";
        }
        else
        {
            arch = package.attribute("arch", "any");
            
            if (arch == "any")
            {
                arch = SETUP_ARCH;
            }
            else if (arch != "all" && arch != SETUP_ARCH)
            {
                // On ne doit pas construire ce paquet
                package = package.nextSiblingElement();
                continue;
            }
        }
        
        // Obtenir le nom de fichier
        QString packageFile = packageName + "~" + version + "." + arch + ".lpk";
        
        // Envoyer le signal de progression
        if (!d->ps->sendProgress(progress, curPkg, packageFile))
        {
            return false;
        }
        
        curPkg++;
        
        // Créer la liste des _PackageFiles permettant de créer l'archive
        QVector<_PackageFile> packageFiles;
        _PackageFile pf;
        
        // D'abord, le fichier de métadonnées
        pf.from = metaFileName;
        
        if (isSource)
        {
            pf.to = "metadata.xml";
        }
        else
        {
            pf.to = "control/metadata.xml";
        }
        
        packageFiles.append(pf);
        
        // Ensuite, ce qu'on veut inclure
        if (!isSource)
        {
            QDomElement embed = package.firstChildElement("embed");
            
            while (!embed.isNull())
            {
                pf.from = templateString(embed.attribute("from"));
                pf.to = templateString(embed.attribute("to"));
                
                packageFiles.append(pf);
                
                embed = embed.nextSiblingElement("embed");
            }
        }
        
        // Pour finir, le reste des fichiers du paquet
        foreach (const QString &fname, okFiles)
        {
            if (!isSource)
            {
                pf.from = buildDir + fname;
                pf.to = "data" + fname;
            }
            else
            {
                pf.from = fname;
                pf.to = fname;
            }
            
            packageFiles.append(pf);
        }
        
        // Créer l'archive .lpk (tar + xz) (code largement inspiré de l'exemple de "man archive_write")
        struct archive *a;
        struct archive *disk;
        struct archive_entry *entry;
        char *buff = new char[8192];
        int len;
        int fd;
        
        a = archive_write_new();
        archive_write_set_compression_xz(a);
        archive_write_set_format_ustar(a);
        archive_write_open_filename(a, qPrintable(packageFile));
        
        disk = archive_read_disk_new();
        archive_read_disk_set_standard_lookup(disk);
        
        int mp = d->ps->startProgress(Progress::Compressing, packageFiles.count());
        
        for (int i=0; i<packageFiles.count(); ++i)
        {
            const _PackageFile &pf = packageFiles.at(i);
            
            if (!d->ps->sendProgress(mp, i, pf.to))
            {
                return false;
            }
            
            // Ajouter l'élément
            fd = open(qPrintable(pf.from), O_RDONLY);
            entry = archive_entry_new();
            
            struct stat st;
            lstat(qPrintable(pf.from), &st);
            
            archive_entry_set_pathname(entry, qPrintable(pf.to));
            archive_entry_copy_sourcepath(entry, qPrintable(pf.from));
            archive_read_disk_entry_from_file(disk, entry, fd, &st);
            
            archive_write_header(a, entry);
            len = read(fd, buff, 8192);
            
            while (len > 0)
            {
                archive_write_data(a, buff, len);
                len = read(fd, buff, 8192);
            }
            
            close(fd);
            archive_entry_free(entry);
        }
        
        d->ps->endProgress(mp);
        
        archive_write_close(a);
        archive_write_finish(a);
        delete[] buff;
        
        package = package.nextSiblingElement();
    }
    
    // Informer les plugins qu'on a fini
    for (it = d->plugins.constBegin(); it != d->plugins.constEnd(); ++it)
    {
        it.value()->end();
    }
    
    d->ps->endProgress(progress);
    
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

#include "packagesource.moc"