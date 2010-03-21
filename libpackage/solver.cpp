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
#include "packagesystem.h"
#include "databasereader.h"
#include "databasepackage.h"
#include "filepackage.h"
#include "packagelist.h"

#include <QVector>
#include <QList>
#include <QHash>
#include <QCoreApplication>

#include <QtDebug>
#include <QtAlgorithms>

// DEBUG
#include <iostream>
using namespace std;

using namespace Logram;

struct Pkg
{
    int index;
    Solver::Action action;
    bool reallyWanted;
    
    bool operator==(const Pkg &other);
};

// Utilisé par QList::contains
bool Pkg::operator==(const Pkg &other)
{
    return (index == other.index) && (action == other.action);
}

struct Solver::Private
{
    PackageSystem *ps;
    DatabaseReader *psd;
    bool installSuggests;
    bool error;

    // Liste pour la résolution des dépendances
    QList<QVector<Pkg> > packages;
    QHash<int, QList<PackageList::Error *> > listErrors;
    QHash<QString, Solver::Action> wantedPackages;
    QList<int> wrongLists;
    
    // Permet de gérer correctement les suggestions
    QList<int> topLevelPackages;
    QList<FilePackage *> filePackages;

    // Liste publique
    QList<PackageList *> lists;

    // Fonctions
    void addPkg(Pkg &pkg, int listIndex, QList<int> &plists);
    void addPkgs(const QVector<Pkg> &pkgsToAdd, QList<int> &lists);
};

Solver::Solver(PackageSystem *ps, DatabaseReader *psd)
{
    d = new Private;
    
    d->error = false;
    d->psd = psd;
    d->ps = ps;

    d->installSuggests = ps->installSuggests();
}

Solver::~Solver()
{
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

PackageList *Solver::result(int index) const
{
    return d->lists.at(index);
}

static bool listWeightLessThan(PackageList *l1, PackageList *l2)
{
    return l1->weight() < l2->weight();
}

bool Solver::solve()
{
    QList<int> lists;
    QVector<Pkg> pkgsToAdd;
    QList<int> pkgsIndexes;
    Pkg p;

    // Créer la première liste
    d->packages.append(QVector<Pkg>());
    lists.append(0);

    // Explorer les paquets voulus
    foreach(const QString &pkg, d->wantedPackages.keys())
    {
        if (pkg.endsWith(".lpk"))
        {
            // Paquet local
            FilePackage *fpkg = new FilePackage(pkg, d->ps, d->psd, Solver::None);
        
            if (!fpkg->isValid())
            {
                PackageError *err = new PackageError;
                err->type = PackageError::PackageNotFound;
                err->info = pkg;
                
                d->ps->setLastError(err);
                
                return false;
            }
            
            // Explorer les dépendances du paquet
            foreach (Depend *dep, fpkg->depends())
            {   
                // Mapper les DEPEND_TYPE en Action
                Solver::Action act = Solver::None;

                if (dep->type() == DEPEND_TYPE_DEPEND
                || (dep->type() == DEPEND_TYPE_SUGGEST && d->installSuggests))
                {
                    act = Solver::Install;
                }
                else if (dep->type() == DEPEND_TYPE_CONFLICT || dep->type() == DEPEND_TYPE_REPLACE)
                {
                    act = Solver::Remove;
                }
                else
                {
                    continue;
                }
                
                pkgsIndexes = d->psd->packagesByVString(dep->name(), dep->version(), dep->op());

                if (pkgsIndexes.count() == 0)
                {
                    // Aucun paquet ne correspond
                    PackageError *err = new PackageError;
                    err->type = PackageError::PackageNotFound;
                    err->info = pkg;
                    
                    d->ps->setLastError(err);
                    
                    return false;
                }
                
                // Créer le pkgsToAdd
                Pkg p;
                
                foreach(int i, pkgsIndexes)
                {
                    p.index = i;
                    p.action = act;
                    p.reallyWanted = true;
                    
                    pkgsToAdd.append(p);
                }
                
                d->addPkgs(pkgsToAdd, lists);
                
                pkgsToAdd.clear();
            }
            
            // Ajouter le paquet aux paquets à ajouter dans toutes les listes
            fpkg->setAction(Solver::Install);
            fpkg->setWanted(true);              // Installé manuellement
            d->filePackages.append(fpkg);
        }
        else
        {
            Action act = d->wantedPackages.value(pkg);
            pkgsIndexes = d->psd->packagesByVString(pkg);
            
            if (pkgsIndexes.count() == 0)
            {
                // Aucun paquet ne correspond
                PackageError *err = new PackageError;
                err->type = PackageError::PackageNotFound;
                err->info = pkg;
                
                d->ps->setLastError(err);
                
                return false;
            }
            
            foreach(int i, pkgsIndexes)
            {
                p.index = i;
                p.action = act;
                p.reallyWanted = true;
                
                pkgsToAdd.append(p);
            }

            d->addPkgs(pkgsToAdd, lists);
            
            pkgsToAdd.clear();
            
            d->topLevelPackages << pkgsIndexes;
        }
    }

#if 0
    qDebug() << d->wrongLists;
    for (int i=0; i<d->packages.count(); ++i)
    {
        const QVector<Pkg> &pkgs = d->packages.at(i);
        qDebug() << "{" << d->wrongLists.contains(i);
        foreach(const Pkg &pkg, pkgs)
        {
            qDebug() << "    " << d->psd->packageName(pkg.index) << d->psd->packageVersion(pkg.index) << pkg.action;
        }
        qDebug() << "}";
    }
#endif

    // Créer les véritables listes de Package pour les listes trouvées, et étant correctes
    for (int i=0; i<d->packages.count(); ++i)
    {
        PackageList *plist = new PackageList(d->ps);
        
        if (plist->error())
        {
            return false;
        }
        
        plist->setWrong(d->wrongLists.contains(i));
        
        // Ajouter les éventuels paquets fichiers enregistrés comme tel
        foreach(FilePackage *fpkg, d->filePackages)
        {
            plist->addPackage(new FilePackage(*fpkg));
        }

        // La liste est bonne, créer les paquets
        const QVector<Pkg> &pkgs = d->packages.at(i);
        QList<int> updatedPackages; // Liste des paquets à ignorer car faisant partie d'une mise à jour

        foreach(const Pkg &pkg, pkgs)
        {
            // Un paquet peut ne pas être voulu (paquet non-installé sélectionné pour suppression histoire de faire échouer la liste si un paquet nécessitant de l'installer est installé, mais ça l'utilisateur n'a pas à savoir)
            if (pkg.reallyWanted && !updatedPackages.contains(pkg.index))
            {
                Package *package = 0;
                
                // Explorer à nouveau les paquets pour voir si ce n'est pas une mise à jour
                foreach(const Pkg &opkg, pkgs)
                {
                    if (!opkg.reallyWanted) continue;
                    
                    _Package *my = d->psd->package(pkg.index);
                    _Package *other = d->psd->package(opkg.index);
                    
                    if (my->name == other->name && my->version != other->version)
                    {
                        updatedPackages.append(opkg.index);
                        
                        // Savoir quel paquet est l'ancien, et quel paquet est le nouveau
                        if (opkg.action == Solver::Install)
                        {
                            // Nous sommes l'ancien, l'autre le nouveau    
                            package = new DatabasePackage(pkg.index, d->ps, d->psd, Solver::Update);
                            package->setUpgradePackage(opkg.index);
                        }
                        else
                        {
                            // Nous sommes le nouveau, l'autre l'ancien
                            package = new DatabasePackage(opkg.index, d->ps, d->psd, Solver::Update);
                            package->setUpgradePackage(pkg.index);
                        }
                        
                        break;
                    }
                }
                
                if (package == 0)
                {
                    // Pas de mise à jour
                    package = new DatabasePackage(pkg.index, d->ps, d->psd, pkg.action);
                }
                else if (package->flags() & PACKAGE_FLAG_DONTUPDATE)
                {
                    // On a un paquet pour mise à jour, mais ce paquet ne veut pas
                    plist->setWrong(true);
                    
                    PackageList::Error *err = new PackageList::Error;
                    err->type = PackageList::Error::UnupdateablePackageUpdated;
                    
                    err->package = package->name();
                    err->version = package->version();
                    
                    err->otherVersion = package->upgradePackage()->version();
                    
                    d->listErrors[i].append(err);
                }
                
                // Savoir si le paquet a été installé manuellement
                if (d->topLevelPackages.contains(pkg.index) && pkg.action == Solver::Install)
                {
                    package->setWanted(true);
                }

                plist->addPackage(package);
            }
        }
        
        // Ajouter les éventuelles erreurs
        if (plist->wrong())
        {
            foreach(PackageList::Error *err, d->listErrors.value(i))
            {
                plist->addError(err);
            }
        }

        // Ajouter la liste
        d->lists.append(plist);
    }
    
    // Trier la liste par poids
    qSort(d->lists.begin(), d->lists.end(), listWeightLessThan);

    // On peut supprimer les tables temporaires (pas d->lists !)
    d->wrongLists.clear();
    d->wantedPackages.clear();
    d->packages.clear();
    qDeleteAll(d->filePackages);
    
    return true;
}

void Solver::Private::addPkg(Pkg &pkg, int listIndex, QList<int> &plists)
{
    QList<int> lists;

    // Première liste
    lists.append(listIndex);

    QVector<Pkg> &list = packages[listIndex];
    _Package *mpkg = psd->package(pkg.index);
    
    // Vérifier que l'action demandée est compatible avec ce que le paquet veut
    if (pkg.action == Solver::Install && (mpkg->flags & PACKAGE_FLAG_DONTINSTALL))
    {
        PackageList::Error *err = new PackageList::Error;
        err->type = PackageList::Error::UninstallablePackageInstalled;
        
        err->package = psd->string(false, mpkg->name);
        err->version = psd->string(false, mpkg->version);
        
        listErrors[listIndex].append(err);
        
        wrongLists.append(listIndex);
        return;
    }
    else if (pkg.action == Solver::Remove && (mpkg->flags & PACKAGE_FLAG_DONTREMOVE))
    {
        PackageList::Error *err = new PackageList::Error;
        err->type = PackageList::Error::UnremovablePackageRemoved;
        
        err->package = psd->string(false, mpkg->name);
        err->version = psd->string(false, mpkg->version);
        
        listErrors[listIndex].append(err);
        
        wrongLists.append(listIndex);
        return;
    }
    
    // Explorer la liste actuelle pour voir si le paquet demandé n'est pas déjà dedans
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
                    PackageList::Error *err = new PackageList::Error;
                    err->type = PackageList::Error::SameNameSameVersionDifferentAction;
                    
                    err->package = psd->string(false, mpkg->name);
                    err->version = psd->string(false, mpkg->version);
                    err->action = pkg.action;
                    
                    err->otherPackage = psd->string(false, lpkg->name);
                    err->otherVersion = psd->string(false, lpkg->version);
                    err->otherAction = pk.action;
                    
                    listErrors[listIndex].append(err);
                    
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
                    PackageList::Error *err = new PackageList::Error;
                    err->type = PackageList::Error::SameNameDifferentVersionSameAction;
                    
                    err->package = psd->string(false, mpkg->name);
                    err->version = psd->string(false, mpkg->version);
                    err->action = pkg.action;
                    
                    err->otherPackage = psd->string(false, lpkg->name);
                    err->otherVersion = psd->string(false, lpkg->version);
                    err->action = pk.action;
                    
                    listErrors[listIndex].append(err);
                    
                    wrongLists.append(listIndex);
                    return;
                }
            }
        }
    }

    // Si on veut supprimer le paquet et que le paquet n'est pas installé, quitter
    bool ok = false;
    
    if (pkg.action == Solver::Remove && !(mpkg->flags & PACKAGE_FLAG_INSTALLED))
    {
        pkg.reallyWanted = false;
        ok = true;
    }

    // Si on veut installer un paquet déjà installé, quitter
    if (pkg.action == Solver::Install && mpkg->flags & PACKAGE_FLAG_INSTALLED)
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
            if ((opkg->flags & PACKAGE_FLAG_INSTALLED) && opkg->version != mpkg->version && opkg->name == mpkg->name)
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
         || (dep->type == DEPEND_TYPE_SUGGEST && pkg.action == Solver::Install && installSuggests && topLevelPackages.contains(pkg.index)))
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
            
            if (revdepPackage.count() != 0 && myVersions.count() == 1)
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

        if (act == Solver::None)
        {
            continue;
        }
        
        QList<int> mpkgsToAdd = psd->packagesOfString(dep->pkgver, dep->pkgname, dep->op);

        if (mpkgsToAdd.count() == 0 && act == Solver::Install)
        {
            // Aucun des paquets demandés ne convient, distribution cassée. On quitte la branche
            PackageList::Error *err = new PackageList::Error;
            err->type = PackageList::Error::NoPackagesMatchingPattern;
            
            err->pattern = PackageSystem::dependString(
                psd->string(false, dep->pkgname),
                psd->string(false, dep->pkgver),
                dep->op);
                
            err->package = psd->string(false, mpkg->name);
            err->version = psd->string(false, mpkg->version);
            
            listErrors[listIndex].append(err);
            
            wrongLists << lists;
            return;
        }
        
        // Créer le pkgsToAdd
        QVector<Pkg> pkgsToAdd;
        Pkg p;
        
        foreach(int i, mpkgsToAdd)
        {
            if (i != pkg.index)
            {
                p.index = i;
                p.action = act;
                p.reallyWanted = true;
                
                pkgsToAdd.append(p);
            }
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
    QHash<int, int> lcounts;
    int count = lists.count();
    
#if 0
    if (pkgsToAdd.count() > 1)
    {
        bool f = true;
        foreach(const Pkg &pkg, pkgsToAdd)
        {
            if (f)
            {
                qDebug() << "Un choix :";
                f = false;
            }
            
            qDebug() << "    " << psd->packageName(pkg.index) << psd->packageVersion(pkg.index);
        }
    }
#endif

    // Si on a plusieurs choix, créer les sous-listes nécessaires
    for (int j=0; j<pkgsToAdd.count(); ++j)
    {
        /* NOTE: La variable suivante permet de stocker le nombre d'éléments dans lists.
                 Ainsi, quand on ajoute des éléments dedans, ils ne sont plus re-traités
                 (puisqu'ils l'ont déjà été). Au prochain tour de foreach(), ils seront
                 traités comme il faut
        */
        Pkg pkg = pkgsToAdd.at(j);
        
        for (int i=0; i<count; ++i)
        {
            int lindex = lists.at(i);

            if (first)
            {
                const QVector<Pkg> &pkgs = packages.at(lindex);
                
                lcounts[lindex] = pkgs.count();
                
                // Ne rien ajouter si la liste contient déjà ce paquet, exactement le même,
                // avec la même action
                if (!pkgs.contains(pkg))
                {
                    addPkg(pkg, lindex, lists);
                }
            }
            else
            {
                // On a une variante, créer une nouvelle liste, et y copier <lcount> éléments dedans
                // (pour en faire une copie de la liste originelle)
                const QVector<Pkg> &pkgs = packages.at(lindex);
                
                // Ne rien ajouter si la liste contient déjà ce paquet, exactement le même,
                // avec la même action
                if (!pkgs.contains(pkg))
                {
                    int lcount = lcounts[lindex];
                    
                    packages.append(QVector<Pkg>());
                    lindex = packages.count()-1;
                    lists.append(lindex);

                    QVector<Pkg> &mpkgs = packages[lindex];
                    
                    for (int k=0; k<lcount; ++k)
                    {
                        mpkgs.append(pkgs.at(k));
                    }
                    
                    addPkg(pkg, lindex, lists);
                }
            }
        }

        first = false;
    }
}
