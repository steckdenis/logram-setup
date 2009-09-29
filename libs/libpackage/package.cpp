/*
 * package.cpp
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
#include "libpackage.h"
#include "libpackage_p.h"

#include <QtDebug>

#include <QCoreApplication>

/*************************************
******* Privates *********************
*************************************/

struct Package::Private
{
    int index;
    PackageSystem *ps;
    PackageSystemPrivate *psd;

    bool depok;
    QList<Depend *> deps;

    Solver::Action action;

    // Téléchargement
    QString waitingDest;
};

struct Depend::Private
{
    _Depend *dep;
    PackageSystemPrivate *psd;

    QList<Depend *> subdeps;
};

/*************************************
******* Package **********************
*************************************/

Package::Package(int index, PackageSystem *ps, PackageSystemPrivate *psd, Solver::Action _action) : QObject(ps)
{
    d = new Private;
    d->index = index;
    d->ps = ps;
    d->psd = psd;
    d->depok = false;
    d->action = _action;

    connect(ps, SIGNAL(downloadEnded(ManagedDownload *)), this, SLOT(downloadEnded(ManagedDownload *)));
}

Package::~Package()
{
    qDeleteAll(d->deps);
    delete d;
}

void Package::process()
{
    // Télécharger le paquet
    QString fname = "/var/cache/lgrpkg/download/" + url().section('/', -1, -1);
    QString r = repo();
    QString type = d->ps->repoType(r);
    QString u = d->ps->repoUrl(r) + "/" + url();

    d->waitingDest = fname;
    
    d->ps->download(type, u, fname, false); // Non-bloquant
}

void Package::downloadEnded(ManagedDownload *md)
{
    if (md->destination == d->waitingDest)
    {
        // Envoyer le signal comme quoi on va installer
        emit installing();

        // TODO: Installer le paquet

        // Envoyer le signal comme quoi on a fini
        emit installed();
    }
}

Solver::Action Package::action()
{
    return d->action;
}

bool Package::isValid()
{
    return d->index != -1;
}

QList<Depend *> Package::depends()
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
        dep = new Depend(mdeps.at(i), d->psd);
        rs.append(dep);
    }

    // Mettre en cache
    d->deps = rs;
    d->depok = true;

    return rs;
}

QString Package::name()
{
    return d->psd->packageName(d->index);
}

QString Package::version()
{
    return d->psd->packageVersion(d->index);
}

QString Package::shortDesc()
{
    return d->psd->packageShortDesc(d->index);
}

QString Package::longDesc()
{
    return d->psd->packageLongDesc(d->index);
}

QString Package::title()
{
    return d->psd->packageTitle(d->index);
}

QString Package::source()
{
    return d->psd->packageSource(d->index);
}

QString Package::repo()
{
    return d->psd->packageRepo(d->index);
}

QString Package::section()
{
    return d->psd->packageSection(d->index);
}

QString Package::distribution()
{
    return d->psd->packageDistribution(d->index);
}

QString Package::license()
{
    return d->psd->packageLicense(d->index);
}

QString Package::url()
{
    return d->psd->packageUrl(d->index);
}

/*************************************
******* Depend ***********************
*************************************/

Depend::Depend(_Depend *dep, PackageSystemPrivate *psd)
{
    d = new Private;
    d->dep = dep;
    d->psd = psd;
}

Depend::~Depend()
{
    qDeleteAll(d->subdeps);
    delete d;
}

QString Depend::name()
{
    return d->psd->string(0, d->dep->pkgname);
}

QString Depend::version()
{
    return d->psd->string(0, d->dep->pkgver);
}

int8_t Depend::type()
{
    return d->dep->type;
}

int8_t Depend::op()
{
    return d->dep->op;
}