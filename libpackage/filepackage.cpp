/*
 * filepackage.cpp
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

#include "filepackage.h"
#include "packagesystem.h"
#include "templatable.h"
#include "packagemetadata.h"

// Qt
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QLocale>
#include <QDir>
#include <QCryptographicHash>

#include <QtDebug>
#include <QtXml>

// LibArchive
#include <archive.h>
#include <archive_entry.h>

using namespace Logram;

struct FilePackage::Private
{
    PackageSystem *ps;
    QString fileName;
    bool valid;
    
    int flags;
    qint64 size, isize;
    QString name;
    QString version;
    QString maintainer;
    QString shortDesc;
    QString source;
    QString upstream_url;
    QString section;
    QString distribution;
    QString license;
    QString arch;
    QString primaryLang;
    
    QVector<PackageFile *> files;
    QByteArray metadataContents;
    
    QByteArray packageHash, metadataHash;
    
    QVector<Depend *> depends;
    
    void addDeps(const QByteArray& str, Depend::Type type);
};

/**** FileFile ****/

struct FileFile::Private
{
    QString path;
    int flags;
    uint itime;
};

FileFile::FileFile(PackageSystem* ps, const QString& path, Flag flags) : PackageFile(ps)
{
    d = new Private;
    d->path = path;
    d->flags = flags;
}

FileFile::~FileFile()
{
    delete d;
}
        
QString FileFile::path()
{
    return d->path;
}

PackageFile::Flag FileFile::flags()
{
    return (Flag)d->flags;
}

uint FileFile::installTime()
{
    return d->itime;
}

Package *FileFile::package()
{
    return 0;
}
        
void FileFile::setFlags(PackageFile::Flag flags)
{
    d->flags = flags;
    saveFile();
}

void FileFile::setFlagsNoSave(PackageFile::Flag flags)
{
    d->flags = flags;
}

void FileFile::setPath(const QString &path)
{
    d->path = path;
}

void FileFile::setInstallTime(uint timestamp)
{
    d->itime = timestamp;
    saveFile();
}

/**** FilePackage ****/

FilePackage::FilePackage(const QString &fileName, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action)
    : Package(ps, psd, _action)
{
    d = new Private;
    d->ps = ps;
    d->isize = 0;
    d->flags = 0;
    
    if (QDir::isAbsolutePath(fileName))
    {
        d->fileName = fileName;
    }
    else
    {
        d->fileName = QDir::currentPath() + "/" + fileName;
    }
    d->valid = true;
    
    // Architecture et taille
    d->arch = fileName.section('.', -2, -2);
    QFileInfo fi(fileName);
    d->size = fi.size();
    
    d->name = fileName.section('/', -1, -1).section('~', 0, 0);
    d->version = fileName.section('/', -1, -1).section('~', 1, -1).section('.', 0, -3);
    
    // Hash du paquet
    QFile fl(fileName);
    
    if (fl.open(QIODevice::ReadOnly))
    {
        d->packageHash = QCryptographicHash::hash(fl.readAll(), QCryptographicHash::Sha1);
        
        fl.close();
    }
    
    // Lire l'archive .tar.tlz qu'est un paquet, et récupérer les métadonnées et le fichier .control
    struct archive *a;
    struct archive_entry *entry;
    int r;
    
    a = archive_read_new();
    archive_read_support_compression_lzma(a);
    archive_read_support_compression_xz(a);
    archive_read_support_format_all(a);
    
    r = archive_read_open_filename(a, qPrintable(fileName), 10240);
    
    if (r != ARCHIVE_OK)
    {
        d->valid = false;
        return;
    }
    
    // Lire les entrées
    QByteArray path;
    int size;
    char *buffer;
    
    QString lang = QLocale::system().name().section('_', 0, 0);
    
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        path = QByteArray(archive_entry_pathname(entry));
        d->isize += archive_entry_size(entry);
        
        if (path.startsWith("data/"))
        {
            // Ignorer les dossiers
            mode_t mode = archive_entry_filetype(entry);
            
            if (S_ISDIR(mode))
            {
                continue;
            }
            
            path.remove(0, 5);
            FileFile *file = new FileFile(d->ps, path, (PackageFile::Flag)0);
            
            if (path.startsWith("etc"))
            {
                file->setFlagsNoSave(PackageFile::CheckBackup);
            }
            
            d->files.append(file);
        }
        
        // Savoir quel type de fichier on a lu
        if (path == "control/metadata.xml")
        {
            // Lire le fichier
            size = archive_entry_size(entry);
            buffer = new char[size];
            archive_read_data(a, buffer, size);
            
            d->metadataContents = QByteArray(buffer, size);
            
            delete[] buffer;
        }
    }
    
    r = archive_read_finish(a);
    
    if (r != ARCHIVE_OK)
    {
        d->valid = false;
    }
    
    // Créer la template qui sera utilisée pour la suite
    Templatable tpl(0);
    
    tpl.addKey("version", d->version);
    tpl.addKey("package", d->name);
    tpl.addKey("arch", d->arch);
    
    // Charger les informations contenues dans les métadonnées
    PackageMetaData doc(ps);
    doc.loadData(d->metadataContents);
    
    d->primaryLang = doc.primaryLang();
    
    // Paquet principal
    
    if (doc.primaryPackages().contains(d->name))
    {
        d->flags |= Package::Primary;
    }
    
    // Attributs du paquet
    QDomElement package = doc.documentElement().firstChildElement();
    
    while (!package.isNull())
    {
        if (package.tagName() == "package" && package.attribute("name") == d->name)
        {
            // Le bon paquet
            
            d->section = package.attribute("section");
            
            // Explorer ses enfants, qui contiennent ce qu'on veut
            QDomElement el = package.firstChildElement();
            
            while (!el.isNull())
            {
                if (el.tagName() == "depend")
                {
                    // Dépendance
                    Depend::Type type = Depend::DependType;
                    QString stype = el.attribute("type", "depend");
                    
                    if (stype == "suggest")
                    {
                        type = Depend::Suggest;
                    }
                    else if (stype == "conflict")
                    {
                        type = Depend::Conflict;
                    }
                    else if (stype == "provide")
                    {
                        type = Depend::Provide;
                    }
                    else if (stype == "replace")
                    {
                        type = Depend::Replace;
                    }
                    
                    d->addDeps(tpl.templateString(el.attribute("string")).toUtf8(), type);
                }
                else if (el.tagName() == "flag")
                {
                    QString name = el.attribute("name");
                    int val = el.attribute("value", "1").toInt();
                    
                    if (name == "kdeintegration")
                    {
                        d->flags |= (val & 3);
                    }
                    else if (name == "gui")
                    {
                        d->flags |= ((val & 1) << 2);
                    }
                    else if (name == "eula")
                    {
                        d->flags |= ((val & 1) << 6);
                    }
                    else if (name == "needsreboot")
                    {
                        d->flags |= ((val & 1) << 7);
                    }
                }
                else if (el.tagName() == "shortdesc")
                {
                    d->shortDesc = PackageMetaData::stringOfKey(el, d->primaryLang);
                }
                else if (el.tagName() == "file")
                {
                    // Un fichier (pour la liste des fichiers du paquet, pas la création, qui utilise <files>)
                    QString path = el.attribute("path");
                    path.remove(0, 1);  // Retirer le premier /
                    int flags = 0;
                    FileFile *file = 0;
                    
                    // Trouver le FileFile correspondant, ou le créer si nécessaire
                    for (int i=0; i<d->files.count(); ++i)
                    {
                        FileFile *fl = (FileFile *)d->files.at(i);
                        
                        if (fl->path() == path)
                        {
                            file = fl;
                            break;
                        }
                    }
                    
                    if (file == 0)
                    {
                        // On n'a pas trouvé le fichier
                        flags |= PackageFile::Virtual;
                        file = new FileFile(d->ps, path, (PackageFile::Flag)flags);
                        d->files.append(file);
                        
                        // NOTE: On fait tout ceci avant de parser les éléments <flag>
                        // pour leur permettre de désactiver le flag virtual, au cas où.
                    }
                    
                    // Explorer ses enfants pour voir si ce fichier a des flags
                    QDomElement flag = el.firstChildElement("flag");
                    
                    while (!flag.isNull())
                    {
                        QString name = flag.attribute("name");
                        bool enable = (flag.attribute("value", "1") == "1");
                        
                        #define APPLY_FLAG(flag_name) \
                            if (enable) flags |= flag_name; \
                            else flags &= ~flag_name;
                        
                        if (name == "backup")
                        {
                            APPLY_FLAG(PackageFile::Backup)
                        }
                        else if (name == "checkbackup")
                        {
                            APPLY_FLAG(PackageFile::CheckBackup)
                        }
                        else if (name == "dontremove")
                        {
                            APPLY_FLAG(PackageFile::DontRemove)
                        }
                        else if (name == "dontpurge")
                        {
                            APPLY_FLAG(PackageFile::DontPurge)
                        }
                        else if (name == "overwrite")
                        {
                            APPLY_FLAG(PackageFile::Overwrite)
                        }
                        
                        flag = flag.nextSiblingElement("flag");
                    }
                    
                    // Placer les flags dans l'enregistrement
                    file->setFlagsNoSave((PackageFile::Flag)flags);
                }
                
                el = el.nextSiblingElement();
            }
        }
        else if (package.tagName() == "source")
        {
            d->license = package.attribute("license");
            d->source = package.attribute("name");
            d->upstream_url = package.attribute("upstreamurl");
            
            QDomElement maintainer = package.firstChildElement("maintainer");
            
            d->maintainer = maintainer.attribute("name") + " <" 
                          + maintainer.attribute("email") + '>';
        }
        else if (package.tagName() == "changelog")
        {
            d->distribution = package.firstChildElement("entry").attribute("distribution");
        }
        
        package = package.nextSiblingElement();
    }
    
    // Hash des métadonnées non-compressées
    d->metadataHash = QCryptographicHash::hash(d->metadataContents, QCryptographicHash::Sha1);
}

FilePackage::FilePackage(const FilePackage &other) : Package(other)
{
    d = new Private;
    
    d->ps = other.d->ps;
    d->fileName = other.d->fileName;
    d->valid = other.d->valid;
    
    d->flags = other.d->flags;
    d->size = other.d->size;
    d->name = other.d->name;
    d->version = other.d->version;
    d->maintainer = other.d->maintainer;
    d->shortDesc = other.d->shortDesc;
    d->source = other.d->source;
    d->section = other.d->section;
    d->distribution = other.d->distribution;
    d->license = other.d->license;
    d->arch = other.d->arch;
    d->primaryLang = other.d->primaryLang;
    
    d->metadataContents = other.d->metadataContents;
    
    foreach(Depend *dep, other.d->depends)
    {
        d->depends.append(new FileDepend(dep->type(), dep->op(), dep->name(), dep->version()));
    }
    
    foreach(PackageFile *file, other.d->files)
    {
        d->files.append(new FileFile(d->ps, file->path(), file->flags()));
    }
}

FilePackage::~FilePackage()
{
    qDeleteAll(d->depends);
    qDeleteAll(d->files);
    
    delete d;
}

void FilePackage::Private::addDeps(const QByteArray &str, Depend::Type type)
{
    if (str.isEmpty())
    {
        return;
    }
    
    // Parser la chaîne
    QList<QByteArray> deps = str.split(';');
    QByteArray dep;
    Depend *depend;
    
    foreach (const QByteArray &_dep, deps)
    {
        dep = _dep.trimmed();
        
        QByteArray name, version;
        Depend::Operation op = ps->parseVersion(dep, name, version);

        if (op == Depend::NoVersion)
        {
            // Dépendance non-versionnée
            depend = new FileDepend(type, Depend::NoVersion, dep, QString());

            // Ajouter la dépendance
            depends.append(depend);
        }
        else
        {
            // Créer le depend
            depend = new FileDepend(type, op, name, version);

            depends.append(depend);
        }
    }
}

bool FilePackage::download()
{
    // Pas besoin de télécharger
    emit downloaded(true);
    
    return true;
}

QString FilePackage::tlzFileName()
{
    return d->fileName;
}

bool FilePackage::isValid()
{
    return d->valid;
}

Package::Origin FilePackage::origin()
{
    return Package::File;
}

QVector<PackageFile *> FilePackage::files()
{
    return d->files;
}

QString FilePackage::name()
{
    return d->name;
}

QString FilePackage::version()
{
    return d->version;
}

QString FilePackage::maintainer()
{
    return d->maintainer;
}

QString FilePackage::shortDesc()
{
    return d->shortDesc;
}

QString FilePackage::source()
{
    return d->source;
}

QString FilePackage::upstreamUrl()
{
    return d->upstream_url;
}

QString FilePackage::repo()
{
    return "filesystem";
}

QString FilePackage::section()
{
    return d->section;
}

QString FilePackage::distribution()
{
    return d->distribution;
}

QString FilePackage::license()
{
    return d->license;
}

QString FilePackage::arch()
{
    return d->arch;
}

QByteArray FilePackage::packageHash()
{
    return d->packageHash;
}

QByteArray FilePackage::metadataHash()
{
    return d->metadataHash;
}

Package::Flag FilePackage::flags()
{
    return (Flag)d->flags;
}

QDateTime FilePackage::installedDate()
{
    return QDateTime();
}

int FilePackage::installedBy()
{
    return 0;
}

int FilePackage::used()
{
    return 0;
}

bool FilePackage::fastNameCompare(Package *other)
{
    return (name() == other->name());
}

bool FilePackage::fastVersionCompare(Package *other)
{
    return (version() == other->version());
}

bool FilePackage::fastNameVersionCompare(Package *other)
{
    return (name() == other->name() && version() == other->version());
}

int FilePackage::downloadSize()
{
    return d->size;
}

int FilePackage::installSize()
{
    return d->isize;
}

QVector<Depend *> FilePackage::depends()
{
    return d->depends;
}

void FilePackage::registerState(int idate, int iby, Package::Flag flags)
{
    // TODO: Pas possible d'ajouter un élément dans la BDD. Peut-être déclencher un update()
    (void) idate;
    (void) iby;
    (void) flags;
}

QByteArray FilePackage::metadataContents()
{
    return d->metadataContents;
}

/* Dépendances */

struct FileDepend::Private
{
    Depend::Type type;
    Depend::Operation op;
    QString name, version;
};

FileDepend::FileDepend(Depend::Type type, Depend::Operation op, const QString& name, const QString& version)
{
    d = new Private;
    
    d->type = type;
    d->op = op;
    d->name = name;
    d->version = version;
}

FileDepend::~FileDepend()
{
    delete d;
}

QString FileDepend::name()
{
    return d->name;
}

QString FileDepend::version()
{
    return d->version;
}

Depend::Type FileDepend::type()
{
    return d->type;
}

Depend::Operation FileDepend::op()
{
    return d->op;
}

#include "filepackage.moc"