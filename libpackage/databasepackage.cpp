/*
 * databasepackage.cpp
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

#include "package.h"
#include "databasepackage.h"
#include "packagesystem.h"
#include "databasereader.h"
#include "packagemetadata.h"
#include "communication.h"

#include <QtDebug>

#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QFile>
#include <QCryptographicHash>

using namespace Logram;

/*************************************
******* Privates *********************
*************************************/

struct DatabasePackage::Private
{
    int index;
    PackageSystem *ps;
    DatabaseReader *psd;
    _Package *dbpkg;

    bool depok;
    QVector<Depend *> deps;

    // Téléchargement
    QString waitingDest, usedMirror;
    ManagedDownload *waitingMd;
    Repository usedRepo;
};

struct DatabaseDepend::Private
{
    _Depend *dep;
    DatabaseReader *psd;

    QVector<Depend *> subdeps;
};

/*************************************
******* File *************************
*************************************/

struct DatabaseFile::Private
{
    DatabaseReader *dr;
    int id;
    DatabasePackage *pkg;
    bool packagebinded;
    QString path;
    _File *file;
};

DatabaseFile::DatabaseFile(PackageSystem *ps, DatabaseReader *dr, _File *file, DatabasePackage *pkg, bool packagebinded)
    : PackageFile(ps)
{
    d = new Private;
    d->dr = dr;
    d->pkg = pkg;
    d->packagebinded = packagebinded;
    
    // Trouver le chemin
    d->file = file;
    
    _File *dir = dr->file(file->parent_dir);
    QString path(dr->fileString(file->name_ptr));
    
    while (dir)
    {
        path = QString(dr->fileString(dir->name_ptr)) + '/' + path;
        
        dir = dr->file(dir->parent_dir);
    }
    
    d->path = path;
}

DatabaseFile::~DatabaseFile()
{
    if (d->packagebinded)
    {
        delete d->pkg;
    }
    
    delete d;
}
        
QString DatabaseFile::path()
{
    return d->path;
}

PackageFile::Flag DatabaseFile::flags()
{
    return (Flag)d->file->flags;
}

uint DatabaseFile::installTime()
{
    return d->file->itime;
}

Package *DatabaseFile::package()
{
    return d->pkg;
}

void DatabaseFile::setPackageBinded(bool binded)
{
    d->packagebinded = binded;
}
        
void DatabaseFile::setFlags(PackageFile::Flag flags)
{
    d->file->flags = flags;
    saveFile();
}

void DatabaseFile::setInstallTime(uint timestamp)
{
    d->file->itime = timestamp;
    saveFile();
}

/*************************************
******* Package **********************
*************************************/

DatabasePackage::DatabasePackage(int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action) 
    : Package(ps, psd, _action)
{
    d = new Private;
    d->index = index;
    d->ps = ps;
    d->psd = psd;
    d->depok = false;
    d->waitingMd = 0;
    d->dbpkg = psd->package(index);
    
    connect(ps, SIGNAL(downloadEnded(Logram::ManagedDownload *)), this, SLOT(downloadEnded(Logram::ManagedDownload *)));
}

DatabasePackage::DatabasePackage(QObject *parent, int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action) 
    : Package(parent, ps, psd, _action)
{
    d = new Private;
    d->index = index;
    d->ps = ps;
    d->psd = psd;
    d->depok = false;
    d->waitingMd = 0;
    d->dbpkg = psd->package(index);
    
    connect(ps, SIGNAL(downloadEnded(Logram::ManagedDownload *)), this, SLOT(downloadEnded(Logram::ManagedDownload *)));
}

DatabasePackage::~DatabasePackage()
{
    qDeleteAll(d->deps);
    
    delete d;
}

Package::Origin DatabasePackage::origin()
{
    return Package::Database;
}

QString DatabasePackage::tlzFileName()
{
    return d->waitingDest;
}

void DatabasePackage::registerState(int idate, int iby, Package::Flag flags)
{
    d->dbpkg->idate = idate;
    d->dbpkg->iby = iby;
    d->dbpkg->flags = flags;
}

bool DatabasePackage::download()
{
    QString fname, u;
    Repository::Type type;
    
    if (action() == Solver::Remove || action() == Solver::Purge)
    {
        // Pas besoin de télécharger
        emit downloaded(true);
        return true;
    }
    else if (action() == Solver::Install)
    {
        // Télécharger le paquet
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + url().section('/', -1, -1);
        
        d->ps->repository(repo(), d->usedRepo);
        type = d->usedRepo.type;
        
        d->usedMirror = d->ps->bestMirror(d->usedRepo);
        
        u = d->usedMirror + "/" + url();
    }
    else /* Update */
    {
        DatabasePackage *other = upgradePackage();
        
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + other->url().section('/', -1, -1);
        
        d->ps->repository(other->repo(), d->usedRepo);
        type = d->usedRepo.type;
        
        d->usedMirror = d->ps->bestMirror(d->usedRepo);
        
        u = d->usedMirror + "/" + other->url();
    }
    
    d->waitingDest = fname;
    
    return d->ps->download(type, u, d->waitingDest, false, d->waitingMd); // Non-bloquant
}

void DatabasePackage::downloadEnded(ManagedDownload *md)
{
    if (md == d->waitingMd)
    {
        // Libérer le mirroir
        d->ps->releaseMirror(d->usedMirror);
        
        if (md->error)
        {
            delete md;
            
            emit downloaded(false);
            return;
        }
        
        // Vérifier le sha1sum du paquet
        QFile fl(d->waitingDest);
        
        if (!fl.open(QIODevice::ReadOnly))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = d->waitingDest;
            
            d->ps->setLastError(err);
        
            delete md;
            emit downloaded(false);
            return;
        }
        
        QByteArray contents = fl.readAll();
        fl.close();
        
        QString sha1sum = QCryptographicHash::hash(contents, QCryptographicHash::Sha1).toHex();
        
        // Comparer les hashs
        QString myHash;
        
        if (action() == Solver::Update)
        {
            myHash = upgradePackage()->packageHash();
        }
        else
        {
            myHash = packageHash();
        }
        
        if (sha1sum != myHash)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::SHAError;
            err->info = name();
            err->more = md->url;
            
            d->ps->setLastError(err);
            
            delete md;
            emit downloaded(false);
            return;
        }
        
        // On a téléchargé le paquet !
        delete md;
        d->waitingMd = 0;
        emit downloaded(true);
    }
}

bool DatabasePackage::isValid()
{
    return (d->index != -1 && d->dbpkg != 0);
}

int DatabasePackage::index() const
{
    return d->index;
}

QVector<DatabasePackage *> DatabasePackage::versions()
{
    QVector<int> pkgs = d->psd->packagesOfString(0, d->dbpkg->name, Depend::NoVersion);
    QVector<DatabasePackage *> rs;

    foreach(int pkg, pkgs)
    {
        DatabasePackage *pk = new DatabasePackage(pkg, d->ps, d->psd);
        rs.append(pk);
    }

    return rs;
}

QVector<Depend *> DatabasePackage::depends()
{
    if (d->depok)
    {
        // On a les dépendances en cache
        return d->deps;
    }

    // Trouver les dépendances
    QVector<_Depend *> mdeps = d->psd->depends(d->index);
    QVector<Depend *> rs;
    
    // Créer les dépendances pour tout ça
    Depend *dep;
    
    for (int i=0; i<mdeps.count(); ++i)
    {
        dep = new DatabaseDepend(mdeps.at(i), d->psd);
        rs.append(dep);
    }

    // Mettre en cache
    d->deps = rs;
    d->depok = true;

    return rs;
}

#define PKG_STR_ATTR(translate, attr) \
    return QString(d->psd->string(translate, d->dbpkg->attr));

#define PKG_QBA_ATTR(translate, attr) \
    return QByteArray(d->psd->string(translate, d->dbpkg->attr));

QString DatabasePackage::name()
{
    PKG_STR_ATTR(false, name)
}

QString DatabasePackage::version()
{
    PKG_STR_ATTR(false, version)
}

QString DatabasePackage::maintainer()
{
    PKG_STR_ATTR(false, maintainer)
}

QString DatabasePackage::shortDesc()
{
    PKG_STR_ATTR(true, short_desc)
}

QString DatabasePackage::source()
{
    PKG_STR_ATTR(false, source)
}

QString DatabasePackage::upstreamUrl()
{
    PKG_STR_ATTR(false, uurl)
}

QString DatabasePackage::repo()
{
    PKG_STR_ATTR(false, repo)
}

QString DatabasePackage::section()
{
    PKG_STR_ATTR(false, section)
}

QString DatabasePackage::distribution()
{
    PKG_STR_ATTR(false, distribution)
}

QString DatabasePackage::license()
{
    PKG_STR_ATTR(false, license)
}

QString DatabasePackage::arch()
{
    PKG_STR_ATTR(false, arch)
}

QByteArray DatabasePackage::packageHash()
{
    PKG_QBA_ATTR(false, pkg_hash)
}

QByteArray DatabasePackage::metadataHash()
{
    PKG_QBA_ATTR(false, mtd_hash)
}

bool DatabasePackage::fastNameCompare(Package *other)
{
    if (other->origin() == Package::Database)
    {
        DatabasePackage *dpkg = (DatabasePackage *)other;
        
        return (d->dbpkg->name == dpkg->d->dbpkg->name);
    }
    else
    {
        return (name() == other->name());
    }
}

bool DatabasePackage::fastVersionCompare(Package *other)
{
    if (other->origin() == Package::Database)
    {
        DatabasePackage *dpkg = (DatabasePackage *)other;
        
        return (d->dbpkg->version == dpkg->d->dbpkg->version);
    }
    else
    {
        return (version() == other->version());
    }
}

bool DatabasePackage::fastNameVersionCompare(Package *other)
{
    if (other->origin() == Package::Database)
    {
        DatabasePackage *dpkg = (DatabasePackage *)other;
        
        return (d->dbpkg->name == dpkg->d->dbpkg->name && d->dbpkg->version && dpkg->d->dbpkg->version);
    }
    else
    {
        return (name() == other->name() && version() == other->version());
    }
}

QVector<PackageFile *> DatabasePackage::files()
{
    QVector<PackageFile *> rs;
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return rs;
    
    _File *file = d->psd->file(pkg->first_file);
    DatabaseFile *pf;
    
    QString path;
    
    while (file)
    {
        // Trouver le chemin du fichier. Ça se fait simplement en remontant dans l'arborescence
        path = QString(d->psd->fileString(file->name_ptr));
        
        pf = new DatabaseFile(d->ps, d->psd, file, this, false);
        rs.append(pf);
        
        file = d->psd->file(file->next_file_pkg);
    }
    
    return rs;
}

QString DatabasePackage::url(UrlType type)
{
    // Construire la partie premières_lettres/nom
    QString n = (type == Source ? source() : name());
    QString d;
    
    if (n.startsWith("lib"))
    {
        d = n.left(4);
    }
    else
    {
        d = n.left(1);
    }
    
    // Compléter en fonction du type demandé
    if (type == Binary || type == Source)
    {
        QString a = (type == Binary ? arch() : "src");
        
        return "pool/" + d + "/" + n + "/" + n + "~" + version() + "." + a + ".lpk";
    }
    else
    {
        return "metadata/" + d +  "/" + n + "~" + version() + ".metadata.xml.xz";
    }
}

void DatabasePackage::setFlags(Flag flags)
{
    QSettings *set = d->ps->installedPackagesList();
    
    // Enregistrer les nouveaux flags dans le fichier de sauvegarde
    if (!(this->flags() & (Package::Installed | Package::Removed)))
    {
        // Paquet pas dans installed_packages.list, utiliser une sauvegarde annexe
        set->beginGroup(name() + "_" + version());
        set->setValue("Name", name());
        set->setValue("Version", version());
    }
    else
    {
        set->beginGroup(name());
    }
    set->setValue("Flags", (int)flags);
    set->endGroup();
    
    // Enregistrer également dans la base de donnée binaire
    _Package *pkg = d->psd->package(d->index);
    
    pkg->flags = flags;
}

Package::Flag DatabasePackage::flags()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return (Flag)0;
    
    return (Flag)pkg->flags;
}

int DatabasePackage::downloadSize()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return 0;
    
    return pkg->dsize;
}

int DatabasePackage::installSize()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return 0;
    
    return pkg->isize;
}


QDateTime DatabasePackage::installedDate()
{
    QDateTime rs;
    _Package *pkg = d->psd->package(d->index);

    rs.setTime_t(pkg->idate);
    
    return rs;
}

int DatabasePackage::installedBy()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return 0;
    
    return pkg->iby;
}

int DatabasePackage::used()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return 0;
    
    return pkg->used;
}

/*************************************
******* Depend ***********************
*************************************/

DatabaseDepend::DatabaseDepend(_Depend *dep, DatabaseReader *psd)
{
    d = new Private;
    d->dep = dep;
    d->psd = psd;
}

DatabaseDepend::~DatabaseDepend()
{
    qDeleteAll(d->subdeps);
    delete d;
}

QString DatabaseDepend::name()
{
    if (d->dep->type != Depend::RevDep)
    {
        return d->psd->string(false, d->dep->pkgname);
    }
    else
    {
        // dep->pkgname = index du paquet de la revdep
        _Package *pkg = d->psd->package(d->dep->pkgname);
        return d->psd->string(false, pkg->name);
    }
}

QString DatabaseDepend::version()
{
    if (d->dep->type != Depend::RevDep)
    {
        return d->psd->string(false, d->dep->pkgver);
    }
    else
    {
        _Package *pkg = d->psd->package(d->dep->pkgname);
        return d->psd->string(false, pkg->version);
    }
}

Depend::Type DatabaseDepend::type()
{
    return (Type)d->dep->type;
}

Depend::Operation DatabaseDepend::op()
{
    return (Operation)d->dep->op;
}

#include "databasepackage.moc"