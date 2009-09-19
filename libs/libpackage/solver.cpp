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

#include <QtDebug>

struct Pkg
{
    int index;
    Solver::Action action;
};

struct Solver::Private
{
    PackageSystem *ps;
    PackageSystemPrivate *psd;
    bool installSuggests;

    // Liste pour la résolution des dépendances
    QList<QVector<Pkg> > packages;
    QHash<QString, Solver::Action> wantedPackages;
    QList<int> wrongLists;

    // Fonctions
    void addPkg(int packageIndex, int listIndex, Solver::Action action, QList<int> &plists);
    void addPkgs(const QList<int> &pkgsToAdd, QList<int> &lists, Solver::Action act);
};

Solver::Solver(PackageSystem *ps, PackageSystemPrivate *psd)
{
    d = new Private;
    d->psd = psd;
    d->ps = ps;

    d->installSuggests = false; // TODO
}

Solver::~Solver()
{
    delete d;
}

void Solver::addPackage(const QString &nameStr, Action action)
{
    // nameStr = nom ou nom=version ou nom>=version, etc.
    d->wantedPackages.insert(nameStr, action);
}

void Solver::solve()
{
    QList<int> lists;
    QList<int> pkgsToAdd;

    // Créer la première liste
    d->packages.append(QVector<Pkg>());
    lists.append(0);

    // Explorer les paquets voulus
    foreach(const QString &pkg, d->wantedPackages.keys())
    {
        Action act = d->wantedPackages.value(pkg);
        pkgsToAdd = d->psd->packagesByVString(pkg);

        d->addPkgs(pkgsToAdd, lists, act);
    }

    //NOTE: Debug
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
}

void Solver::Private::addPkg(int packageIndex, int listIndex, Solver::Action action, QList<int> &plists)
{
    QList<int> lists;

    // Première liste
    lists.append(listIndex);

    // Explorer la liste actuelle pour voir si le paquet demandé n'est pas déjà dedans
    QVector<Pkg> &list = packages[listIndex];
    _Package *mpkg = psd->package(packageIndex);

    for(int i=0; i<list.count(); ++i)
    {
        Pkg pk = list.at(i);
        _Package *lpkg = psd->package(pk.index);

        // Voir si on a déjà dans la liste un paquet du même nom que nous
        if (lpkg->name == mpkg->name)
        {
            // Si les versions sont les mêmes
            if (lpkg->version == mpkg->version)
            {
                // Si on veut faire la même action, on retourne true (dépendance en boucle ok)
                if (pk.action == action)
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
                if (pk.action != action)
                {
                    // Par exemple, supprimer foo-1.2 et installer foo-1.3, c'est ok
                    return;
                }
                else
                {
                    // Contradiction, on ne peut pas par exemple installer deux versions les mêmes
                    wrongLists.append(listIndex);
                    return;
                }
            }
        }
    }

    // Ajouter le paquet dans la liste
    Pkg pkg;
    pkg.index = packageIndex;
    pkg.action = action;
    packages[listIndex].append(pkg);

    // Explorer les dépendances du paquet
    QList<_Depend *> deps = psd->depends(packageIndex);

    foreach (_Depend *dep, deps)
    {
        // Mapper les DEPEND_TYPE en Action
        Solver::Action act = Solver::None;

        if ((dep->type == DEPEND_TYPE_DEPEND && action == Solver::Install)
         || (dep->type == DEPEND_TYPE_SUGGEST && action == Solver::Install && installSuggests))
        {
            act = Solver::Install;
        }
        else if ((dep->type == DEPEND_TYPE_CONFLICT || dep->type == DEPEND_TYPE_REPLACE) && action == Solver::Install)
        {
            act = Solver::Remove;
        }
        else if (dep->type == DEPEND_TYPE_REVDEP && action == Solver::Remove)
        {
            act = Solver::Remove;
            // TODO: Vérifier que la dépendance inverse est installée avant de surcharger l'arbre
        }

        if (act == Solver::None) continue;
        
        QList<int> pkgsToAdd = psd->packagesOfString(dep->pkgver, dep->pkgname, dep->op);

        if (pkgsToAdd.count() == 0)
        {
            // Aucun des paquets demandés ne convient, distribution cassée. On quitte la branche
            wrongLists << lists;
            return;
        }
            
        addPkgs(pkgsToAdd, lists, act);
    }

    // Rajouter les listes nouvellement créées dans plists
    lists.removeFirst();
    
    plists << lists;
}

void Solver::Private::addPkgs(const QList<int> &pkgsToAdd, QList<int> &lists, Solver::Action act)
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
        int pindex = pkgsToAdd.at(j);
        
        for (int i=0; i<count; ++i)
        {
            int lindex = lists.at(i);

            if (first)
            {
                addPkg(pindex, lindex, act, lists);
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
                
                addPkg(pindex, lindex, act, lists);
            }
        }

        if (first) first = false;
    }
}
