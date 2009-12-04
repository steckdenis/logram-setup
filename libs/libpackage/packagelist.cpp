/*
 * packagelist.cpp
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

#include "packagelist.h"
#include "packagesystem.h"
#include "package.h"

#include <QtScript>
#include <QList>
#include <QEventLoop>
#include <QFile>

using namespace Logram;

struct PackageList::Private
{
    PackageSystem *ps;
    bool error;
    
    bool wrong, weighted;
    int weight;
    QList<PackageList::Error *> errs;
    int parallelInstalls, parallelDownloads;
    
    // Pour l'installation
    QEventLoop loop;
    int ipackages, dpackages, pipackages;
    bool idone;
    Package *installingPackage;
    QList<Package *> list;
    QList<Package *> downloadedPackages;
    
    // Programme QtScript
    QScriptValue weightFunction;
};

Q_DECLARE_METATYPE(Package *)

PackageList::PackageList(PackageSystem *ps) : QObject(ps), QList<Package *>()
{
    d = new Private;
    
    d->weighted = false;
    d->error = false;
    d->ps = ps;
    
    d->parallelInstalls = ps->parallelInstalls();
    d->parallelDownloads = ps->parallelDownloads();
    
    // Lire le programme QtScript
    QFile fl(d->ps->confRoot() + "/etc/lgrpkg/scripts/weight.qs");

    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        d->ps->setLastError(err);
        
        d->error = true;
        return;
    }

    // Parser le script
    QScriptEngine *engine = new QScriptEngine();
    engine->evaluate(fl.readAll(), fl.fileName());

    if (engine->hasUncaughtException())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ScriptException;
        err->info = fl.fileName();
        err->more = engine->uncaughtExceptionLineNumber() + ": " + engine->uncaughtException().toString();
        
        d->ps->setLastError(err);
        
        d->error = true;
        delete engine;
        return;
    }

    QScriptValue global = engine->globalObject();
    d->weightFunction = global.property("weight");

    fl.close();
}

PackageList::~PackageList()
{
    if (d->weightFunction.engine() != 0)
    {
        delete d->weightFunction.engine();
    }
    
    for (int i=0; i<count(); ++i)
    {
        delete at(i);
    }
    
    delete d;
}

bool PackageList::error() const
{
    return d->error;
}
        
void PackageList::addPackage(Package *pkg)
{
    append(pkg);
}

void PackageList::addError(Error *err)
{
    d->errs.append(err);
}

void PackageList::setWrong(bool wrong)
{
    d->wrong = wrong;
}

int PackageList::errors() const
{
    return d->errs.count();
}

PackageList::Error *PackageList::error(int i) const
{
    return d->errs.at(i);
}

bool PackageList::wrong() const
{
    return d->wrong;
}

int PackageList::weight() const
{
    if (!d->weighted)
    {
        QScriptValueList args;
        
        const QList<Package *> *l = this;

        args << qScriptValueFromSequence(d->weightFunction.engine(), *l);
        args << 0; // TODO: savoir si on installe, supprime, etc
        
        d->weight = d->weightFunction.call(QScriptValue(), args).toInt32();
        d->weighted = true;
    }
    
    return d->weight;
}

Package *PackageList::installingPackage() const
{
    return d->installingPackage;
}
        
bool PackageList::process()
{
    // Installer les paquets d'une liste, donc explorer ses paquets
    d->ipackages = 0;
    d->pipackages = 0;

    int maxdownloads = qMin(d->parallelDownloads, count());
    d->dpackages = maxdownloads;
    
    for(int i=0; i<maxdownloads; ++i)
    {
        Package *pkg = at(i);

        connect(pkg, SIGNAL(proceeded(bool)), this, SLOT(packageProceeded(bool)));
        connect(pkg, SIGNAL(downloaded(bool)), this, SLOT(packageDownloaded(bool)), Qt::QueuedConnection);
        connect(pkg, SIGNAL(communication(Logram::Package *, Logram::Communication *)), this,
                     SIGNAL(communication(Logram::Package *, Logram::Communication *)));

        // Progression
        d->ps->sendProgress(PackageSystem::GlobalDownload, i, count(), pkg->name());

        // Téléchargement
        if (!pkg->download())
        {
            return false;
        }
    }

    // Attendre
    int rs = d->loop.exec();

    // Nettoyage
    disconnect(this, SLOT(packageProceeded(bool)));
    disconnect(this, SLOT(packageDownloaded(bool)));
    
    d->downloadedPackages.clear();
    
    return (rs == 0);
}

void PackageList::packageProceeded(bool success)
{
    if (!success)
    {
        // Un paquet a planté, arrêter
        d->loop.exit(1);
        return;
    }
    
    // Voir si on a un paquet suivant
    if (d->downloadedPackages.count() != 0)
    {
        Package *next = d->downloadedPackages.takeAt(0);

        // Progression
        d->ps->sendProgress(PackageSystem::PackageProcess, d->ipackages, count(), next->name() + "~" + next->version());

        // Installation
        d->ipackages++;
        d->installingPackage = next;

        next->process();
    }
    else if (d->ipackages < count())
    {
        // On n'a pas tout installé, mais les téléchargements ne suivent pas, arrêter cette branche et dire à
        // packageDownloaded qu'on l'attend
        d->pipackages--;
        return;
    }
    else if (d->ipackages < (count() + (d->pipackages - 1)))
    {
        // Laisser le dernier se finir
        d->ipackages++;
    }
    else
    {
        // On a tout installé et fini
        d->ps->endProgress(PackageSystem::PackageProcess, count());

        d->loop.exit(0);
    }
}

void PackageList::packageDownloaded(bool success)
{
    Package *pkg = qobject_cast<Package *>(sender());

    if (!success || !pkg)
    {
        // Un paquet a planté, arrêter
        d->loop.exit(1);
        return;
    }

    // Installer le paquet si le nombre de paquets installés en parallèle n'est pas dépassé
    if (d->pipackages < d->parallelInstalls)
    {
        // Progression
        d->ps->sendProgress(PackageSystem::PackageProcess, d->ipackages, count(), pkg->name() + "~" + pkg->version());
        
        // Installer
        d->ipackages++;
        d->pipackages++;
        d->installingPackage = pkg;
        pkg->process();
    }
    else
    {
        // Placer ce paquet en attente d'installation
        d->downloadedPackages.append(pkg);
    }

    // Si disponible, télécharger un autre paquet
    if (d->dpackages < count())
    {
        Package *next = at(d->dpackages);

        // Connexion de signaux
        connect(next, SIGNAL(proceeded(bool)), this, SLOT(packageProceeded(bool)));
        connect(next, SIGNAL(downloaded(bool)), this, SLOT(packageDownloaded(bool)), Qt::QueuedConnection);
        connect(next, SIGNAL(communication(Logram::Package *, Logram::Communication *)), this, 
                      SIGNAL(communication(Logram::Package *, Logram::Communication *)));
        
        // Progression
        d->ps->sendProgress(PackageSystem::GlobalDownload, d->dpackages, count(), next->name());

        // Téléchargement
        d->dpackages++;
        
        if (!next->download())
        {
            d->loop.exit(1);
            return;
        }
    }
    else
    {
        // On a tout téléchargé
        d->ps->endProgress(PackageSystem::GlobalDownload, count());
    }
}