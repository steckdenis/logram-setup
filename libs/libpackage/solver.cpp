/*
 * solver.cpp
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

#include "solver.h"
#include "libpackage.h"
#include "libpackage_p.h"
#include "package.h"

#include <QVector>
#include <QList>
#include <QHash>
#include <QMap>
#include <QFile>
#include <QEventLoop>
#include <QCoreApplication>

#include <QtDebug>
#include <QtScript>

#include <iostream>

struct Pkg
{
    int index;
    Solver::Action action;
    bool reallyWanted;
};

struct Solver::Private
{
    PackageSystem *ps;
    PackageSystemPrivate *psd;
    bool installSuggests;
    int parallelInstalls, parallelDownloads;

    // Liste pour la résolution des dépendances
    QList<QVector<Pkg> > packages;
    QHash<QString, Solver::Action> wantedPackages;
    QList<int> wrongLists;

    // Liste publique (QMap car on trie sur le poid. Il faut ensuite retourner les résultats à l'envers, pour un tri décroissant)
    QMap<int, QList<Package *> > lists;

    // Programme QtScript
    QScriptValue weightFunction;

    // Pour l'installation
    QEventLoop loop;
    int ipackages, dpackages, pipackages;
    bool idone;
    Package *installingPackage;
    QList<Package *> list;

    // Fonctions
    void addPkg(Pkg &pkg, int listIndex, QList<int> &plists);
    void addPkgs(const QVector<Pkg> &pkgsToAdd, QList<int> &lists);
    int listWeight(QList<Package *> *list);
};

Solver::Solver(PackageSystem *ps, PackageSystemPrivate *psd)
{
    d = new Private;
    d->psd = psd;
    d->ps = ps;

    d->installSuggests = ps->installSuggests();
    d->parallelInstalls = ps->parallelInstalls();
    d->parallelDownloads = ps->parallelDownloads();

    // Lire le programme QtScript
    QFile fl("/etc/lgrpkg/scripts/weight.qs");

    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError err;
        err.type = PackageError::OpenFileError;
        err.info = fl.fileName();
        
        throw err;
    }

    // Parser le script
    QScriptEngine *engine = new QScriptEngine();
    engine->evaluate(fl.readAll(), fl.fileName());

    if (engine->hasUncaughtException())
    {
        PackageError err;
        err.type = PackageError::ScriptException;
        err.info = engine->uncaughtExceptionLineNumber() + ": " + engine->uncaughtException().toString();
        
        throw err;
    }

    QScriptValue global = engine->globalObject();
    d->weightFunction = global.property("weight");

    fl.close();
}

Solver::~Solver()
{
    if (d->weightFunction.engine() != 0)
    {
        delete d->weightFunction.engine();
    }

    delete d;
}

void Solver::addPackage(const QString &nameStr, Action action)
{
    d->wantedPackages.insert(nameStr, action);
}

int Solver::results() const
{
    return d->lists.count();
}

QList<Package *> Solver::result(int index, int &weight) const
{
    QList<Package *> rs = d->lists.values().at(index);

    weight = d->lists.key(rs);

    return rs;
}

void Solver::solve()
{
    QList<int> lists;
    QList<int> mpkgsToAdd;
    QVector<Pkg> pkgsToAdd;
    Pkg p;

    // Créer la première liste
    d->packages.append(QVector<Pkg>());
    lists.append(0);

    // Explorer les paquets voulus
    foreach(const QString &pkg, d->wantedPackages.keys())
    {
        Action act = d->wantedPackages.value(pkg);
        mpkgsToAdd = d->psd->packagesByVString(pkg);
        
        foreach(int i, mpkgsToAdd)
        {
            p.index = i;
            p.action = act;
            p.reallyWanted = true;
            
            pkgsToAdd.append(p);
        }

        d->addPkgs(pkgsToAdd, lists);
        
        pkgsToAdd.clear();
    }

#if 0
    for (int i=0; i<d->packages.count(); ++i)
    {
        const QVector<Pkg> &pkgs = d->packages.at(i);
        qDebug() << "{" << d->wrongLists.contains(i);
        foreach(const Pkg &pkg, pkgs)
        {
            qDebug() << "    " << d->psd->packageName(pkg.index) << d->psd->packageVersion(pkg.index);
        }
        qDebug() << "}";
    }
#endif

    // Créer les véritables listes de Package pour les listes trouvées, et étant correctes
    for (int i=0; i<d->packages.count(); ++i)
    {
        if (d->wrongLists.contains(i))
        {
            continue;
        }

        // La liste est bonne, créer les paquets
        const QVector<Pkg> &pkgs = d->packages.at(i);
        QList<Package *> list;

        foreach(const Pkg &pkg, pkgs)
        {
            // Un paquet peut ne pas être voulu (paquet non-installé sélectionné pour suppression histoire de faire échouer la liste si un paquet nécessitant de l'installer est installé, mais ça l'utilisateur n'a pas à savoir)
            if (pkg.reallyWanted)
            {
                Package *package = new Package(pkg.index, d->ps, d->psd, pkg.action);

                list.append(package);
            }
        }

        // Peser la liste
        int weight = d->listWeight(&list);

        // Ajouter la liste
        d->lists.insertMulti(weight, list);
    }

    // On peut supprimer les tables temporaires (pas d->lists !)
    d->wrongLists.clear();
    d->wantedPackages.clear();
}

void Solver::process(int index)
{
    // Installer les paquets d'une liste, donc explorer ses paquets
    d->list = d->lists.values().at(index);
    d->ipackages = 0;
    d->pipackages = 0;

    int maxdownloads = qMin(d->parallelDownloads, d->list.count());
    d->dpackages = maxdownloads;
    
    for(int i=0; i<maxdownloads; ++i)
    {
        Package *pkg = d->list.at(i);

        connect(pkg, SIGNAL(installed()), this, SLOT(packageInstalled()));
        connect(pkg, SIGNAL(downloaded()), this, SLOT(packageDownloaded()), Qt::QueuedConnection);

        // Progression
        d->ps->sendProgress(PackageSystem::GlobalDownload, i, d->list.count(), pkg->name());

        // Téléchargement
        pkg->download();
    }

    // Attendre
    d->loop.exec();

    // Nettoyage
    disconnect(this, SLOT(packageInstalled()));
    disconnect(this, SLOT(packageDownloaded()));
}

void Solver::packageInstalled()
{
    Package *pkg = static_cast<Package *>(sender());
    
    // Voir si on a un paquet suivant
    if (d->ipackages < d->dpackages)
    {
        Package *next = d->list.at(d->ipackages);

        // Progression
        d->ps->sendProgress(PackageSystem::Install, d->ipackages, d->list.count(), next->name());

        // Installation
        d->ipackages++;
        d->installingPackage = next;

        next->install();
    }
    else if (d->ipackages < d->list.count())
    {
        // On n'a pas tout installé, mais les téléchargements ne suivent pas, arrêter cette branche et dire à
        // packageDownloaded qu'on l'attend
        d->pipackages--;
        return;
    }
    else if (d->ipackages < (d->list.count() + (d->pipackages - 1)))
    {
        // Laisser le dernier se finir
        d->ipackages++;
    }
    else
    {
        // On a tout installé et fini
        d->ps->endProgress(PackageSystem::Install, d->list.count());

        d->loop.exit();
    }
}

void Solver::packageDownloaded()
{
    Package *pkg = qobject_cast<Package *>(sender());

    if (!pkg) return;

    // Installer le paquet si le nombre de paquets installés en parallèle n'est pas dépasse
    if (d->pipackages < d->parallelInstalls)
    {
        // Progression
        d->ps->sendProgress(PackageSystem::Install, d->ipackages, d->list.count(), pkg->name());
        
        // Installer
        d->ipackages++;
        d->pipackages++;
        d->installingPackage = pkg;
        pkg->install();
    }

    // Si disponible, télécharger un autre paquet
    if (d->dpackages < d->list.count())
    {
        Package *next = d->list.at(d->dpackages);

        // Connexion de signaux
        connect(next, SIGNAL(installed()), this, SLOT(packageInstalled()));
        connect(next, SIGNAL(downloaded()), this, SLOT(packageDownloaded()), Qt::QueuedConnection);

        // Progression
        d->ps->sendProgress(PackageSystem::GlobalDownload, d->dpackages, d->list.count(), next->name());

        // Téléchargement
        d->dpackages++;
        next->download();
    }
    else
    {
        // On a tout téléchargé
        d->ps->endProgress(PackageSystem::GlobalDownload, d->list.count());
    }
}

Package *Solver::installingPackage() const
{
    return d->installingPackage;
}

void Solver::Private::addPkg(Pkg &pkg, int listIndex, QList<int> &plists)
{
    QList<int> lists;

    // Première liste
    lists.append(listIndex);

    // Explorer la liste actuelle pour voir si le paquet demandé n'est pas déjà dedans
    QVector<Pkg> &list = packages[listIndex];
    _Package *mpkg = psd->package(pkg.index);

    for(int i=0; i<list.count(); ++i)
    {
        const Pkg &pk = list.at(i);
        _Package *lpkg = psd->package(pk.index);

        // Voir si on a déjà dans la liste un paquet du même nom que nous
        if (lpkg->name == mpkg->name)
        {
            // Si les versions sont les mêmes
            if (lpkg->version == mpkg->version)
            {
                // Si on veut faire la même action, on retourne true (dépendance en boucle ok)
                if (pk.action == pkg.action)
                {
                    return;
                }
                else
                {
                    // L'action n'est pas la même, c'est une contradiction, on quitte
                    wrongLists.append(listIndex);
                    return;
                }
            }
            else
            {
                // Les versions ne sont pas les mêmes
                if (pk.action == pkg.action)
                {
                    // Contradiction, on ne peut pas par exemple installer deux versions les mêmes
                    wrongLists.append(listIndex);
                    return;
                }
            }
        }
    }

    // Si on veut supprimer le paquet et que le paquet n'est pas installé, quitter
    bool ok = false;
    
    if (pkg.action == Solver::Remove && mpkg->state != PACKAGE_STATE_INSTALLED)
    {
        pkg.reallyWanted = false;
        ok = true;
    }

    // Si on veut installer un paquet déjà installé, quitter
    if (pkg.action == Solver::Install && mpkg->state == PACKAGE_STATE_INSTALLED)
    {
        pkg.reallyWanted = false;
        ok = true;
    }
    
    // Ajouter le paquet dans la liste
    packages[listIndex].append(pkg);
    
    if (ok)
    {
        return;
    }

    // Si on installe le paquet, vérifier que d'autres versions ne sont pas installées
    if (pkg.action == Solver::Install)
    {
        QList<int> otherVersions = psd->packagesOfString(0, mpkg->name, DEPEND_OP_NOVERSION);
        
        foreach(int otherVersion, otherVersions)
        {
            _Package *opkg = psd->package(otherVersion);
            
            // NOTE: le "&& opkg->name == mpkg->name" permet d'avoir deux paquets fournissant le même provide ensemble
            if (opkg->state == PACKAGE_STATE_INSTALLED && opkg->version != mpkg->version && opkg->name == mpkg->name)
            {
                // Supprimer le paquet
                Pkg pkgToAdd;
                
                pkgToAdd.index = otherVersion;
                pkgToAdd.action = Solver::Remove;
                pkgToAdd.reallyWanted = true;
                
                QVector<Pkg> pkgsToAdd;
                pkgsToAdd.append(pkgToAdd);

                addPkgs(pkgsToAdd, lists);

                break;  // On n'a qu'une seule autre version d'installée, normalement
            }
        }
    }

    // Explorer les dépendances du paquet
    QList<_Depend *> deps = psd->depends(pkg.index);

    foreach (_Depend *dep, deps)
    {
        // Mapper les DEPEND_TYPE en Action
        Solver::Action act = Solver::None;

        if ((dep->type == DEPEND_TYPE_DEPEND && pkg.action == Solver::Install)
         || (dep->type == DEPEND_TYPE_SUGGEST && pkg.action == Solver::Install && installSuggests))
        {
            act = Solver::Install;
        }
        else if ((dep->type == DEPEND_TYPE_CONFLICT || dep->type == DEPEND_TYPE_REPLACE) && pkg.action == Solver::Install)
        {
            act = Solver::Remove;
        }
        else if (dep->type == DEPEND_TYPE_REVDEP && pkg.action == Solver::Remove)
        {
            // Gestion des dépendances inverses. Créer n branches. Dans la première,
            // supprimer dep. Dans les autres, installer les provides
            QList<int> myVersions = psd->packagesOfString(0, mpkg->name, DEPEND_OP_NOVERSION);
            QList<int> revdepPackage = psd->packagesOfString(dep->pkgver, dep->pkgname, dep->op);
            QVector<Pkg> pkgsToAdd;
            
            // Ajout de la dépendance inverse à supprimer
            Pkg p;
            
            if (revdepPackage.count() != 0)
            {
                p.index = revdepPackage.at(0);  // N'en contient qu'un
                p.action = Solver::Remove;
                p.reallyWanted = true;
                
                pkgsToAdd.append(p);
            }
            
            // Ajouter les versions ne notre paquet
            foreach(int i, myVersions)
            {
                if (i != pkg.index)
                {
                    p.index = i;
                    p.action = Solver::Install;
                    p.reallyWanted = true;
                    
                    pkgsToAdd.append(p);
                }
            }
            
            // Ajouter les paquets
            if (pkgsToAdd.count() != 0)
            {
                addPkgs(pkgsToAdd, lists);
                continue;
            }
        }

        if (act == Solver::None) continue;
        
        QList<int> mpkgsToAdd = psd->packagesOfString(dep->pkgver, dep->pkgname, dep->op);

        if (mpkgsToAdd.count() == 0 && act == Solver::Install)
        {
            // Aucun des paquets demandés ne convient, distribution cassée. On quitte la branche
            wrongLists << lists;
            return;
        }
        
        // Créer le pkgsToAdd
        QVector<Pkg> pkgsToAdd;
        Pkg p;
        
        foreach(int i, mpkgsToAdd)
        {
            p.index = i;
            p.action = act;
            p.reallyWanted = true;
            
            pkgsToAdd.append(p);
        }
        
        addPkgs(pkgsToAdd, lists);
    }

    // Rajouter les listes nouvellement créées dans plists
    lists.removeFirst();
    
    plists << lists;
}

void Solver::Private::addPkgs(const QVector<Pkg> &pkgsToAdd, QList<int> &lists)
{
    bool first = true;

    QList<int> lcounts;

    foreach(int list, lists)
    {
        lcounts.append(packages.at(list).count());
    }

    // Si on a plusieurs choix, créer les sous-listes nécessaires
    for (int j=0; j<pkgsToAdd.count(); ++j)
    {
        /* NOTE: La variable suivante permet de stocker le nombre d'éléments dans lists.
                 Ainsi, quand on ajoute des éléments dedans, ils ne sont plus re-traités
                 (puisqu'ils l'ont déjà été). Au prochain tour de foreach(), ils seront
                 traités comme il faut
        */
        int count = lists.count();
        Pkg pkg = pkgsToAdd.at(j);
        
        for (int i=0; i<count; ++i)
        {
            int lindex = lists.at(i);

            if (first)
            {
                addPkg(pkg, lindex, lists);
            }
            else
            {
                // On a une variante, créer une nouvelle liste, et y copier <lcount> éléments dedans
                // (pour en faire une copie de la liste originelle)
                const QVector<Pkg> &pkgs = packages.at(lindex);

                packages.append(QVector<Pkg>());
                lindex = packages.count()-1;
                lists.append(lindex);

                QVector<Pkg> &mpkgs = packages[lindex];

                int lcount = lcounts.at(i);
                
                for (int k=0; k<lcount; ++k)
                {
                    mpkgs.append(pkgs.at(k));
                }
                
                addPkg(pkg, lindex, lists);
            }
        }

        if (first) first = false;
    }
}

int Solver::Private::listWeight(QList<Package *> *list)
{
    QScriptValueList args;

    args << qScriptValueFromSequence(weightFunction.engine(), *(QObjectList *)list);
    args << 0; // TODO: savoir si on installe, supprime, etc
    
    return weightFunction.call(QScriptValue(), args).toInt32();
}
