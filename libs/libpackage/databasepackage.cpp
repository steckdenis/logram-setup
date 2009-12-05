/*
 * databasepackage.cpp
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

    bool depok;
    QList<Depend *> deps;

    // Téléchargement
    QString waitingDest;
};

struct DatabaseDepend::Private
{
    _Depend *dep;
    DatabaseReader *psd;

    QList<Depend *> subdeps;
};

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

void DatabasePackage::registerState(int idate, int iby, int state)
{
    _Package *pkg = d->psd->package(d->index);
    pkg->idate = idate;
    pkg->iby = iby;
    pkg->state = state;
}

bool DatabasePackage::download()
{
    QString fname, r, type, u;
    
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
        r = repo();
        type = d->ps->repoType(r);
        u = d->ps->repoUrl(r) + "/" + url();
    }
    else /* Update */
    {
        DatabasePackage *other = upgradePackage();
        
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + other->url().section('/', -1, -1);
        r = other->repo();
        type = d->ps->repoType(r);
        u = d->ps->repoUrl(r) + "/" + other->url();
    }
    
    d->waitingDest = fname;
        
    ManagedDownload *md;

    return d->ps->download(type, u, fname, false, md); // Non-bloquant
        
}

void DatabasePackage::downloadEnded(ManagedDownload *md)
{
    if (md != 0)
    {
        if (md->destination == d->waitingDest)
        {
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
            emit downloaded(true);
        }
    }
}

bool DatabasePackage::isValid()
{
    return d->index != -1;
}

QList<Package *> DatabasePackage::versions()
{
    QList<int> pkgs = d->psd->packagesOfString(0, d->psd->package(d->index)->name, DEPEND_OP_NOVERSION);
    QList<Package *> rs;

    foreach(int pkg, pkgs)
    {
        Package *pk = new DatabasePackage(pkg, d->ps, d->psd);
        rs.append(pk);
    }

    return rs;
}

QList<Depend *> DatabasePackage::depends()
{
    if (d->depok)
    {
        // On a les dépendances en cache
        return d->deps;
    }

    // Trouver les dépendances
    QList<_Depend *> mdeps = d->psd->depends(d->index);
    QList<Depend *> rs;
    
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
    _Package *pkg = d->psd->package(d->index); \
    \
    if (pkg == 0) return QString(); \
    \
    return QString(d->psd->string(translate, pkg->attr));

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

QString DatabasePackage::packageHash()
{
    PKG_STR_ATTR(false, pkg_hash)
}

QString DatabasePackage::metadataHash()
{
    PKG_STR_ATTR(false, mtd_hash)
}

QString DatabasePackage::url(UrlType type)
{
    // Construire la partie premières_lettres/nom
    QString n = name();
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
    if (type == Binary)
    {
        return "pool/" + d + "/" + n + "/" + n + "~" + version() + "." + arch() + ".tlz";
    }
    else
    {
        return "metadata/" + d + "/" + n + "/" + n + "~" + version() + ".metadata.xml.lzma";
    }
}

bool DatabasePackage::isGui()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->is_gui;
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
    
    if (pkg == 0) return false;
    
    return pkg->iby;
}

int DatabasePackage::status()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->state;
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
    return d->psd->string(false, d->dep->pkgname);
}

QString DatabaseDepend::version()
{
    return d->psd->string(false, d->dep->pkgver);
}

int8_t DatabaseDepend::type()
{
    return d->dep->type;
}

int8_t DatabaseDepend::op()
{
    return d->dep->op;
}