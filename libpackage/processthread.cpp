/*
 * processthread.cpp
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

#include "processthread.h"
#include "package.h"
#include "packagesystem.h"
#include "packagemetadata.h"
#include "templatable.h"
#include "databasepackage.h"

#include <QDir>
#include <QFile>

#include <QtDebug>

#include <archive.h>
#include <archive_entry.h>

using namespace Logram;

struct ProcessThread::Private
{
    PackageSystem *ps;
    Package *pkg;
    
    bool error;
};

ProcessThread::ProcessThread(PackageSystem *ps, Package *pkg) : QThread(pkg)
{
    d = new Private;
    d->ps = ps;
    d->pkg = pkg;
    d->error = false;
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

bool ProcessThread::depack(Package *pkg, QList<QByteArray> &files, QByteArray &metadataContents)
{
    QString cDir = QDir::currentPath();
    
    // Dépaqueter l'archive dans installRoot
    QDir::setCurrent(d->ps->installRoot());
    
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int r;
    
    a = archive_read_new();
    ext = archive_write_disk_new();
    
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS | ARCHIVE_EXTRACT_XATTR);
    archive_write_disk_set_standard_lookup(ext);
    
    archive_read_support_compression_lzma(a);
    archive_read_support_compression_xz(a);
    archive_read_support_format_all(a);
    
    // NOTE: d->pkg à la place de pkg, car même à l'update, c'est le paquet principal qui contient le bon fichier
    if (archive_read_open_filename(a, qPrintable(d->pkg->tlzFileName()), 10240))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = d->pkg->tlzFileName();
        
        d->ps->setLastError(err);
        
        archive_read_finish(a);
        archive_write_finish(ext);
        return false;
    }
    
    QByteArray path;
    int size;
    char *buffer;
    
    for (;;)
    {
        r = archive_read_next_header(a, &entry);
        
        if (r == ARCHIVE_EOF)
        {
            break;
        }
        
        if (r != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::InstallError;
            err->info = pkg->name() + '~' + pkg->version();
            
            d->ps->setLastError(err);
            
            archive_read_close(a);
            archive_read_finish(a);
            archive_write_finish(ext);
            return false;
        }
        
        // Liste des fichiers et métadonnées
        path = QByteArray(archive_entry_pathname(entry));
        
        if (path.startsWith("data/"))
        {
            path.remove(0, 5);  // retirer "data/"
            files.append(path);
            
            // Écrire le fichier sur le disque
            archive_entry_set_pathname(entry, path.constData());
            
            if (archive_write_header(ext, entry) != ARCHIVE_OK ||
                copy_data(a, ext) != ARCHIVE_OK || 
                archive_write_finish_entry(ext) != ARCHIVE_OK)
            {
                PackageError *err = new PackageError;
                err->type = PackageError::InstallError;
                err->info = pkg->name() + '~' + pkg->version();
                
                d->ps->setLastError(err);
                
                archive_read_close(a);
                archive_read_finish(a);
                archive_write_finish(ext);
                return false;
            }
        }
        else if (path == "control/metadata.xml")
        {
            // Lire le fichier de métadonnées
            size = archive_entry_size(entry);
            buffer = new char[size];
            archive_read_data(a, buffer, size);
            
            metadataContents = QByteArray(buffer, size);
            
            delete[] buffer;
        }
    }
    
    archive_read_close(a);
    archive_read_finish(a);
    archive_write_finish(ext);
    
    // Se remettre dans le bon dossier
    QDir::setCurrent(cDir);
    
    return true;
}

bool ProcessThread::install(Package *pkg, QList<QByteArray> &files, PackageMetaData *md, const QString &prescript)
{
    QString dir;
    QFile fl;
    QByteArray metadataContents;
    
    QDir rootDir = QDir::root();
    
    // Désempaqueter le paquet
    if (!depack(pkg, files, metadataContents))
    {
        return false;
    }
    
    // Lancer le script preinst
    md->loadData(metadataContents);
    
    if (!md->runScript(pkg->name(), prescript, d->ps->installRoot(), QStringList()))
    {
        return false;
    }
    
    // Enregistrer la liste des fichiers dans "{{varRoot}}/var/cache/lgrpkg/db/pkgs/{{name}}_{{version}}"
    dir = d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + pkg->name() + '_' + pkg->version();
    
    if (!rootDir.mkpath(dir))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = dir;
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    // Liste des fichiers : files.list
    fl.setFileName(dir + "/files.list");
    
    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    foreach (const QByteArray &file, files)
    {
        fl.write(file);
        fl.write("\n");
    }
    
    fl.close();
    
    // Métadonnées : metadata.xml
    fl.setFileName(dir + "/metadata.xml");
    
    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    fl.write(metadataContents);
    fl.close();
    
    return true;
}

void ProcessThread::run()
{
    QList<QByteArray> files;
    QByteArray metadataContents;
    QStringList sfiles;
    
    QFile fl;
    QDir rootDir = QDir::root();
    QString dir;
    
    PackageMetaData *md;
    Templatable *tpl;
    
    // On a besoin d'un templatable pour les variables du type instroot, varroot, confroot, package, version, arch, etc
    tpl = new Templatable(0);
    md = new PackageMetaData(d->ps, 0);
    
    tpl->addKey("instroot", d->ps->installRoot());
    tpl->addKey("varroot", d->ps->varRoot());
    tpl->addKey("confroot", d->ps->confRoot());
    
    md->setTemplatable(tpl);
    
    if (d->pkg->action() != Solver::Update)
    {
        // Pas de mise à jour, c'est ce paquet le maître
        tpl->addKey("package", d->pkg->name());
        tpl->addKey("version", d->pkg->version());
        tpl->addKey("arch", d->pkg->arch());
        
        // Métadonnées, chargées quand nécessaire
        md->setPackage(d->pkg);
    }
    else
    {
        // Mise à jour, c'est l'autre paquet le maître
        tpl->addKey("package", d->pkg->upgradePackage()->name());
        tpl->addKey("version", d->pkg->upgradePackage()->version());
        tpl->addKey("arch", d->pkg->upgradePackage()->arch());
        
        tpl->addKey("oldpackage", d->pkg->name());
        tpl->addKey("oldversion", d->pkg->version());
        tpl->addKey("oldarch", d->pkg->arch());
        
        // Métadonnées, chargées quand nécessaire
        md->setPackage(d->pkg->upgradePackage());
    }
    
    connect(md, SIGNAL(processLineOut(QProcess *, const QByteArray &)), 
          d->pkg, SLOT(processLineOut(QProcess *, const QByteArray &)));
    
    switch (d->pkg->action())
    {
        case Solver::Install:
            if (!install(d->pkg, files, md, "preinst"))
            {
                d->error = true;
                return;
            }
            
            // Lancer postinst
            if (!md->runScript(d->pkg->name(), "postinst", d->ps->installRoot(), QStringList()))
            {
                d->error = true;
                return;
            }
            
            // Fini !
            d->error = false;
            break;
            
        case Solver::Remove:
        case Solver::Purge:
            // Charger les métadonnées
            dir = d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + '_' + d->pkg->version();
            
            fl.setFileName(dir + "/metadata.xml");
            
            if (!fl.open(QIODevice::ReadOnly))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::OpenFileError;
                err->info = fl.fileName();
                
                d->ps->setLastError(err);
                d->error = true;
                
                return;
            }
            
            md->loadData(fl.readAll());
            fl.close();
            
            // Script prerm
            if (!md->runScript(d->pkg->name(), "prerm", d->ps->installRoot(), QStringList()))
            {
                d->error = true;
                return;
            }
            
            // Obtenir la liste des fichiers
            /*if (!d->ps->filesOfPackage(d->pkg->name(), sfiles))
            {
                d->error = true;
                return;
            }
            
            // Explorer ces fichiers et les supprimer
            foreach (const QString &file, sfiles)
            {
                // Si on ne purge pas, sauter les fichiers dans /etc (NOTE: condition fragile)
                if (d->pkg->action() != Solver::Purge && file.contains("/etc/"))
                {
                    continue;
                }
                
                QFile::remove(file);
            }*/
            
            // Supprimer l'enregistrement en base de donnée
            QFile::remove(dir + "/metadata.xml");
            QFile::remove(dir + "/files.list");
            QFile::remove(dir);
            
            // Lancer postrm
            if (!md->runScript(d->pkg->name(), "postrm", d->ps->installRoot(), QStringList()))
            {
                d->error = true;
                return;
            }
            
            break;
            
        case Solver::Update:
            // Faire comme si on installe le paquet, le nouveau paquet
            if (!install(d->pkg->upgradePackage(), files, md, "preupd"))
            {
                d->error = true;
                return;
            }
            
            // On a la liste des fichiers du nouveau paquet (files), prendre celle de l'ancien (sfiles)
            /*if (!d->ps->filesOfPackage(d->pkg->name(), sfiles))
            {
                d->error = true;
                return;
            }
            
            // Explorer les fichiers de l'ancien paquet, et si le nouveau ne les a plus, les supprimer
            foreach (const QString &file, sfiles)
            {
                bool exists = false;
                
                foreach (const QByteArray &f, files)
                {
                    if (file == (d->ps->installRoot() + QString::fromUtf8(f)))
                    {
                        exists = true;
                        break;
                    }
                }
                
                if (!exists)
                {
                    QFile::remove(file);
                }
            }*/
            
            // Supprimer l'enregistrement en base de donnée
            dir = d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + d->pkg->name() + '_' + d->pkg->version();
            
            QFile::remove(dir + "/metadata.xml");
            QFile::remove(dir + "/files.list");
            QFile::remove(dir);
            
            // Lancer postupd
            if (!md->runScript(d->pkg->upgradePackage()->name(), "postupd", d->ps->installRoot(), QStringList()))
            {
                d->error = true;
                return;
            }
            
            break;
            
        default:
            d->error = true;
            return;
    }
    
    delete tpl;
    delete md;
}