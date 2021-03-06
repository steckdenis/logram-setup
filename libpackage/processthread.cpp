/*
 * processthread.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include "processthread.h"
#include "package.h"
#include "packagesystem.h"
#include "packagemetadata.h"
#include "templatable.h"
#include "databasepackage.h"

#include <QDir>
#include <QFile>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QFileInfo>

#include <QtDebug>

#include <archive.h>
#include <archive_entry.h>

using namespace Logram;

struct ProcessThread::Private
{
    PackageSystem *ps;
    Package *pkg, *oldpkg;
    
    bool error;
    
    struct archive *a;
    
    // Fonctions
    bool install_files();
    bool remove_files();
};

ProcessThread::ProcessThread(PackageSystem *ps, Package *pkg) : QThread(pkg)
{
    d = new Private;
    d->ps = ps;
    d->error = false;
    d->a = 0;
    
    if (pkg->action() == Solver::Update && pkg->upgradePackage() != 0)
    {
        d->pkg = pkg->upgradePackage();
        d->oldpkg = pkg;
    }
    else
    {
        d->pkg = pkg;
        d->oldpkg = pkg;
    }
}

ProcessThread::~ProcessThread()
{
    delete d;
}

bool ProcessThread::error() const
{
    return d->error;
}

/* Copie conforme de copy_data se trouvant dans 
   http://code.google.com/p/libarchive/source/browse/trunk/examples/untar.c */
static int copy_data(struct archive *ar, struct archive *aw)
{
    int r;
    const void *buff;
    size_t size;
    off_t offset;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        
        if (r == ARCHIVE_EOF)
        {
            return ARCHIVE_OK;
        }
        
        if (r != ARCHIVE_OK)
        {
            return r;
        }
        
        r = archive_write_data_block(aw, buff, size, offset);
        
        if (r != ARCHIVE_OK) 
        {
            return r;
        }
    }
}

/** Permet de renommer un fichier qui existe déjà en un fichier de sauvegarde **/
static void backup_file(const QByteArray &_path)
{
    QString path(_path), newpath;
    int num = 0;
    
    if (QFile::exists(path))
    {
        // Trouver le premier nom de fichier .bak<nombre> correspondant
        do
        {
            newpath = path + ".bak";
            
            if (num != 0)
            {
                newpath += QString::number(num);
            }
            
            num++;
        } while (QFile::exists(newpath));
            
        QFile::rename(path, newpath);
    }
}

/** Permet de trouver un nom de fichier pour _path pour qu'il n'existe pas encore **/
static QByteArray new_file_name(const QByteArray &_path)
{
    QByteArray path(_path);
    int num = 0;
    
    while (QFile::exists(path))
    {
        path = _path + ".lgrnew";
        
        if (num != 0)
        {
            path += QByteArray::number(num);
        }
        
        num++;
    }
    
    return path;
}

bool ProcessThread::Private::install_files()
{
    // Sauvegarder le dossier courant
    QString currentDir = QDir::currentPath();
    QByteArray instroot = ps->installRoot().toUtf8();
    uint itime = QDateTime::currentDateTime().toTime_t();
    
    if (!instroot.endsWith('/'))
    {
        instroot += '/';
    }
    
    // Se placer à la racine, libarchive en a besoin
    QDir::setCurrent("/");
    
    // Créer une archive pour écrire les fichiers sur le disque
    struct archive *ext;
    struct archive_entry *entry;
    int r;
    
    ext = archive_write_disk_new();
    entry = archive_entry_new();
    
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS | ARCHIVE_EXTRACT_XATTR);
    archive_write_disk_set_standard_lookup(ext);
    
    // Explorer les fichiers de a, sachant que control/metadata est déjà passé
    QByteArray path, filePath;
    QVector<PackageFile *> files;
    
    for (;;)
    {
        r = archive_read_next_header2(a, entry);
        
        if (r == ARCHIVE_EOF)
        {
            break;
        }
        
        if (r != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::InstallError;
            err->more = ProcessThread::tr("Erreur lors de la lecture de l'archive");
            err->info = pkg->name() + '~' + pkg->version();
            
            ps->setLastError(err);
            
            archive_write_finish(ext);
            return false;
        }
        
        // Liste des fichiers et métadonnées
        path = QByteArray(archive_entry_pathname(entry));
        
        if (!path.startsWith("data/"))
        {
            archive_read_data_skip(a);
            continue;
        }
        
        path.remove(0, 5);  // retirer "data/"
        
        // Calculer le chemin du fichier
        filePath = instroot + path;
        
        // Ne pas gérer les fichiers des dossiers
        mode_t mode = archive_entry_filetype(entry);
        
        if (!S_ISDIR(mode))
        {
            // Trouver les PackageFiles allant avec ce fichier
            files = ps->files(QString(path));
            
            if (files.count() == 0)
            {
                PackageError *err = new PackageError;
                err->type = PackageError::InstallError;
                err->more = ProcessThread::tr("Aucun fichier dans la base de donnée LPM ne correspond à %1").arg(QString(path));
                err->info = pkg->name() + '~' + pkg->version();
                
                ps->setLastError(err);
                
                archive_write_finish(ext);
                return false;
            }
            
            bool conflict_handled = false; // TODO: Dans le for, si mis à true, une communication Status serait bien.
            
            // Explorer ces fichiers
            for (int i=0; i<files.count(); ++i)
            {
                PackageFile *file = files.at(i);
                
                if (file->flags() & PackageFile::Installed)
                {
                    if (file->package()->name() == pkg->name())
                    {
                        if (file->flags() & PackageFile::Backup)
                        {
                            // Fichier au paquet, normalement on ne sauvegarde pas, mais ici on le fait
                            // Par exemple, fichier de configuration. On garde celui de l'utilisateur, on installe un .new
                            filePath = new_file_name(filePath);
                        }
                        else if (file->flags() & PackageFile::CheckBackup)
                        {
                            // Vérifier que ce fichier a été modifié depuis
                            QFileInfo fi(filePath);
                            
                            if (fi.lastModified().toTime_t() > file->installTime())
                            {
                                filePath = new_file_name(filePath);
                            }
                        }
                    }
                    else
                    {
                        if (!(file->flags() & PackageFile::Overwrite))
                        {
                            // Fichier d'un autre paquet, on sauvegarde sauf si OVERWRITE est utilisé
                            // Comme ce paquet ne peut pas être cassé, il faut installer son fichier et bouger celui de l'autre
                            backup_file(filePath);
                        }
                    }
                    
                    conflict_handled = true;
                }
                
                // Supprimer le fichier, plus besoin
                delete file;
            }
            
            if (!conflict_handled)
            {
                // On n'a pas vérifié si ce fichier est déjà présent, bien que ne provenant pas d'un paquet dans la BDD.
                // Il faut le sauvegarder.
                backup_file(filePath);
            }
        }
        
        // Écrire le fichier sur le disque
        archive_entry_set_pathname(entry, filePath.constData());
        
        if (archive_write_header(ext, entry) != ARCHIVE_OK ||
            copy_data(a, ext) != ARCHIVE_OK || 
            archive_write_finish_entry(ext) != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::InstallError;
            err->more = ProcessThread::tr("Impossible d'installer le fichier %1").arg(QString(filePath));
            err->info = pkg->name() + '~' + pkg->version();
            
            ps->setLastError(err);
            
            archive_write_finish(ext);
            return false;
        }
    }
    
    // Nettoyage
    archive_entry_free(entry);
    archive_write_finish(ext);
    
    // Restoration du dossier courant
    QDir::setCurrent(currentDir);
    
    // Marquage des fichiers du paquet comme étant installés
    files = pkg->files();
    
    for (int i=0; i<files.count(); ++i)
    {
        PackageFile *file = files.at(i);
        
        file->setFlags((PackageFile::Flag)(file->flags() | PackageFile::Installed));
        file->setInstallTime(itime);
        
        delete file;
    }
    
    return true;
}

bool ProcessThread::Private::remove_files()
{
    // Explorer les fichiers du paquet
    QVector<PackageFile *> files = oldpkg->files();
    QString instroot = ps->installRoot();
    
    if (!instroot.endsWith('/'))
    {
        instroot += '/';
    }
    
    for (int i=0; i<files.count(); ++i)
    {
        PackageFile *file = files.at(i);
        
        // Ne pas supprimer un fichier dontpurge ou dontremove
        if (
            (file->flags() & PackageFile::DontPurge) ||
            (oldpkg->action() == Solver::Remove && (file->flags() & PackageFile::DontRemove))
        )
        {
            // Ne pas supprimer ce fichier
            continue;
        }
        else
        {
            // Supprimer le fichier
            QFile::remove(instroot + file->path());
            
            // Ne plus le déclarer comme installé
            file->setFlags((PackageFile::Flag)(file->flags() & ~PackageFile::Installed));
        }
    }
    
    return true;
}

void ProcessThread::run()
{
    QByteArray metadata;
    QFile fl;
    
    // Créer l'archive permettant de lire le paquet, si on installe ou met à jour
    if (d->oldpkg->action() == Solver::Install || d->oldpkg->action() == Solver::Update)
    {
        struct archive_entry *entry;
        
        d->a = archive_read_new();
        entry = archive_entry_new();
        
        archive_read_support_compression_lzma(d->a);
        archive_read_support_compression_xz(d->a);
        archive_read_support_format_all(d->a);
        
        // NOTE: L'ancien paquet contient le fichier .tlz du nouveau, pas le nouveau
        if (archive_read_open_filename(d->a, qPrintable(d->oldpkg->tlzFileName()), 10240))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = d->oldpkg->tlzFileName();
            
            d->ps->setLastError(err);
            
            archive_read_finish(d->a);
            d->error = true;
            return;
        }
        
        // Lire les métadonnées (premier fichier)
        QByteArray path;
        char *buffer;
        int r, size;
        
        r = archive_read_next_header2(d->a, entry);
        
        if (r != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::InstallError;
            err->info = tr("Impossible de lire le fichier metadata.xml du paquet");
            err->more = d->pkg->name() + '~' + d->pkg->version();
            
            d->ps->setLastError(err);
            
            archive_read_finish(d->a);
            d->error = true;
            return;
        }
        
        path = QByteArray(archive_entry_pathname(entry));
        
        if (path != "control/metadata.xml")
        {
            PackageError *err = new PackageError;
            err->type = PackageError::InstallError;
            err->info = tr("Paquet invalide : son premier fichier n'est pas control/metadata.xml");
            err->more = d->pkg->name() + '~' + d->pkg->version();
            
            d->ps->setLastError(err);
            
            archive_read_finish(d->a);
            d->error = true;
            return;
        }
        
        size = archive_entry_size(entry);
        buffer = new char[size];
        archive_read_data(d->a, buffer, size);
        
        metadata = QByteArray(buffer, size);
        
        delete[] buffer;
        archive_entry_free(entry);
    }
    else
    {
        fl.setFileName(d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + "~" + d->pkg->version() + ".xml");
        
        if (!fl.open(QIODevice::ReadOnly))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = fl.fileName();
            
            d->ps->setLastError(err);
            
            d->error = true;
            return;
        }
        
        metadata = fl.readAll();
        fl.close();
    }
    
    // Charger les métadonnées
    PackageMetaData *md = new PackageMetaData(d->ps, 0);
    md->loadData(metadata);
    md->setCurrentPackage(d->pkg->name());
    connect(md, SIGNAL(processLineOut(QProcess *, const QByteArray &)), 
          d->pkg, SLOT(processLineOut(QProcess *, const QByteArray &)));
    
    // Lancer les scripts pre* et post*
    Templatable *tpl = new Templatable(0);
    
    tpl->addKey("instroot", d->ps->installRoot());
    tpl->addKey("varroot", d->ps->varRoot());
    tpl->addKey("confroot", d->ps->confRoot());
    tpl->addKey("name", d->pkg->name());
    tpl->addKey("version", d->pkg->version());
    tpl->addKey("date", QDate::currentDate().toString(Qt::ISODate));
    tpl->addKey("time", QTime::currentTime().toString(Qt::ISODate));
    tpl->addKey("arch", SETUP_ARCH);
    tpl->addKey("purge", (d->pkg->action() == Solver::Purge ? "true" : "false"));
    
    md->setTemplatable(tpl);
    
    #define SCRIPT(script_name) \
        if (!md->runScript(d->pkg->name(), script_name, d->ps->installRoot(), QStringList())) \
        { \
            d->error = true; \
            return; \
        }
        
    // NOTE: oldpkg est toujours le paquet avec l'action (l'ancien), on ne l'utilise
    // que pour ça, c'est bien le nouveau (en cas de mise à jour) qui est utilisé
    // pour le reste.
    switch (d->oldpkg->action())
    {
        case Solver::Install:
            SCRIPT("preinst")
            
            if (!d->install_files())
            {
                archive_read_finish(d->a);
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            // Écrire les métadonnées dans le fichier de sauvegarde
            fl.setFileName(d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + "~" + d->pkg->version() + ".xml");
            
            if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                archive_read_finish(d->a);
                PackageError *err = new PackageError;
                err->type = PackageError::OpenFileError;
                err->info = fl.fileName();
                
                d->ps->setLastError(err);
                
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            fl.write(metadata);
            fl.close();
            
            SCRIPT("postinst")
            break;
            
        case Solver::Purge:
        case Solver::Remove:
            SCRIPT("prerm")
            
            if (!d->remove_files())
            {
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            // Supprimer le cache
            QFile::remove(d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + "~" + d->pkg->version() + ".xml");
            
            SCRIPT("postrm")
            break;
            
        case Solver::Update:
            // Variables supplémentaires
            tpl->addKey("oldversion", d->oldpkg->version());
            
            SCRIPT("preupd")
            
            // Une mise à jour est simplement la suppression de l'ancien paquet et l'installation du nouveau.
            // remove_files s'occupe de oldpkg, install_files de pkg.
            
            // Suppression de l'ancien
            if (!d->remove_files())
            {
                archive_read_finish(d->a);
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            // Supprimer le cache
            QFile::remove(d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->oldpkg->name() + "~" + d->oldpkg->version() + ".xml");
            
            // Installation du nouveau
            if (!d->install_files())
            {
                archive_read_finish(d->a);
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            // Écrire les métadonnées dans le fichier de sauvegarde
            fl.setFileName(d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + "~" + d->pkg->version() + ".xml");
            
            if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                archive_read_finish(d->a);
                PackageError *err = new PackageError;
                err->type = PackageError::OpenFileError;
                err->info = fl.fileName();
                
                d->ps->setLastError(err);
                
                d->error = true;
                delete tpl;
                delete md;
                return;
            }
            
            fl.write(metadata);
            fl.close();
            
            SCRIPT("postupd")
            break;
            
        default:
            d->error = true;
            break;
    }
    
    
    // Nettoyage
    if (d->a)
    {
        archive_read_finish(d->a);
    }
    
    delete tpl;
    delete md;
}

#include "processthread.moc"
