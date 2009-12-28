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

#include "app.h"

#include <logram/solver.h>
#include <logram/databasepackage.h>
#include <logram/packagelist.h>

#include <iostream>
using namespace std;

using namespace Logram;

void App::add(const QStringList &packages)
{
    Solver *solver = ps->newSolver();
    
    foreach(const QString &package, packages)
    {
        QString name = package;
        Solver::Action action;

        if (package.startsWith('-'))
        {
            action = Solver::Remove;
            name.remove(0, 1);
        }
        else if (package.startsWith('+'))
        {
            action = Solver::Purge;
            name.remove(0, 1);
        }
        else
        {
            action = Solver::Install;
        }

        solver->addPackage(name, action);
    }

    if (!solver->solve())
    {
        error();
        delete solver;
        return;
    }

    // Afficher et gérer les résultats
    manageResults(solver);
    
    delete solver;
}

void App::getsource(const QString &name)
{
    QString n = name.section('=', 0, 0);
    QString v;

    if (name.contains('='))
    {
        v = name.section('=', 1, -1);
    }

    Package *pkg;
    
    if (!ps->package(n, v, pkg))
    {
        error();
        return;
    }
    
    // Caster le paquet en DatabasePackage
    if (pkg->origin() != Package::Database)
    {
        cout << COLOR(tr("ERREUR : "), "31");
        cout << qPrintable(tr("Le paquet dont vous souhaitez récupérer la source doit provenir d'un dépôt"));
        cout << endl;
        return;
    }
    
    DatabasePackage *dpkg = (DatabasePackage *)pkg;
    
    // Télécharger le fichier
    // bool download(Repository::Type type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs);
    QString url = dpkg->url(DatabasePackage::Source);
    QString fname = url.section('/', -1, -1);
    Repository repo;
    ManagedDownload *md = 0;
    
    if (!ps->repository(dpkg->repo(), repo))
    {
        error();
        return;
    }
    
    if (!ps->download(repo.type, dpkg->url(DatabasePackage::Source), fname, true, md))
    {
        error();
        return;
    }
}

void App::upgrade()
{
    QList<DatabasePackage *> pkgs = ps->upgradePackages();
    int instSize, dlSize;
    
    // Vérifier qu'il y a bien quelque-chose à faire
    if (pkgs.count() == 0)
    {
        cout << COLOR(tr("Aucune mise à jour n'est disponible. En cas de doutes, essayez de lancer l'opération «update»."), "34") << endl << endl;
        
        qDeleteAll(pkgs);
        return;
    }
    
    // Afficher la liste des paquets
    cout << COLOR(tr("Paquets pour lesquels une mise à jour est disponible :"), "32") << endl << endl;
    
    displayPackages(&pkgs, instSize, dlSize, false);
    
    // Demander si ça convient
    cout << endl;
    
    cout << qPrintable(tr("Entrez la liste des paquets à mettre à jour, ou \"n\" pour abandonner"))
         << endl;
         
    // Créer la liste des nombres
    QByteArray nums;
    
    for (int i=0; i<pkgs.count(); ++i)
    {
        if (i != 0)
        {
            nums += ',';
        }
        
        nums += QByteArray::number(i + 1);
    }
    
    // Demander l'entrée
    char *buffer = new char[nums.size() + 1];
    bool ok = false;
    QList<int> numPkgs;
    
    while (!ok)
    {
        cout << "> ";
        cout.flush();
        
        getString(buffer, nums.size() + 1, nums.data(), true);
        
        // Lire l'entrée
        QString in(buffer);

        if (in == "n")
        {
            // Abandonner
            return;
        }
        
        // Créer la liste des nombres
        ok = true;
        foreach(const QString &s, in.split(','))
        {
            int n = s.toInt(&ok);
            
            if (!ok || n < 1 || n > pkgs.count())
            {
                // Redemander l'entrée
                numPkgs.clear();
                ok = false;
                break;
            }
            
            numPkgs.append(n - 1);
        }
    }
    
    delete[] buffer;
    
    // Remplir la liste des paquets
    Solver *solver = ps->newSolver();
    
    foreach(int index, numPkgs)
    {
        DatabasePackage *pkg = pkgs.at(index);
        DatabasePackage *other = pkg->upgradePackage();
            
        if (other != 0)
        {
            solver->addPackage(other->name() + "=" + other->version(), Solver::Install);
        }
    }
    
    if (!solver->solve())
    {
        error();
        delete solver;
        return;
    }

    // Afficher et gérer les résultats
    cout << endl;
    manageResults(solver);
    
    delete solver;
}

static QString actionString(Solver::Action act)
{
    switch (act)
    {
        case Solver::Install:
            return App::tr("installé");
            
        case Solver::Remove:
            return App::tr("supprimé");
            
        case Solver::Purge:
            return App::tr("supprimé complètement");
            
        case Solver::Update:
            return App::tr("mis à jour");
            
        default:
            return QString();
    }
}

void App::displayPackages(QList<Logram::DatabasePackage *> *packages, int &instSize, int &dlSize, bool showType)
{
    QList<Package *> pkgs;
    
    for (int i=0; i<packages->count(); ++i)
    {
        pkgs.append(packages->at(i));
    }
    
    displayPackages(&pkgs, instSize, dlSize, showType);
}

void App::displayPackages(QList<Package *> *packages, int &instSize, int &dlSize, bool showType)
{
    instSize = 0;
    dlSize = 0;
    
    for (int i=0; i<packages->count(); ++i)
    {
        Package *pkg = packages->at(i);
        
        QString name = pkg->name().leftJustified(20, ' ', false);

        if (pkg->action() == Solver::Install)
        {
            if (showType)
            {
                cout << " I: ";
            }
            else
            {
                cout << ' ' << (i+1) << ". ";
            }
            
            if (pkg->origin() != Package::File)
            {
                dlSize += pkg->downloadSize();
            }
            instSize += pkg->installSize();
            cout << COLOR(name, "34");
        }
        else if (pkg->action() == Solver::Remove)
        {
            if (showType)
            {
                cout << " R: ";
            }
            else
            {
                cout << ' ' << (i+1) << ". ";
            }
            
            instSize -= pkg->installSize();
            cout << COLOR(name, "31");
        }
        else if (pkg->action() == Solver::Update)
        {
            if (showType)
            {
                cout << " U: ";
            }
            else
            {
                cout << ' ' << (i+1) << ". ";
            }
            
            if (pkg->origin() != Package::File)
            {
                dlSize += pkg->downloadSize();
            }
            
            // Différence entre la version installée et la version qu'on va télécharger
            Package *other = pkg->upgradePackage();
            
            if (other != 0)
            {
                instSize += other->installSize() - pkg->installSize();
            }
            
            cout << COLOR(name, "33");
        }
        else if (pkg->action() == Solver::Purge)
        {
            if (showType)
            {
                cout << " P: ";
            }
            else
            {
                cout << ' ' << (i+1) << ". ";
            }
            
            instSize -= pkg->installSize();
            cout << COLOR(name, "35");
        }

        if (pkg->action() != Solver::Update)
        {
            cout << ' '
                 << COLOR(pkg->version(), "32")
                 << endl
                 << "        "
                 << qPrintable(pkg->shortDesc())
                 << endl;
        }
        else
        {
            Package *opkg = pkg->upgradePackage();
            
            cout << ' '
                 << COLOR(pkg->version(), "32")
                 << " → "
                 << COLOR(opkg->version(), "32")
                 << endl
                 << "        "
                 << qPrintable(pkg->shortDesc())
                 << endl;
        }
    }
}

void App::manageResults(Solver *solver)
{
    // Boucle pour demander son avis à l'utilisateur
    PackageList *packages;
    int index = 0;
    int tot = solver->results();
    int dlSize, instSize;
    bool allempty = true;
    bool oneok = false;
    char in[2];

    // Vérifier qu'au moins une solution est bonne
    for (int i=0; i<tot; ++i)
    {
        packages = solver->result(i);
        
        if (!packages->wrong())
        {
            oneok = true;
            break;
        }
    }
    
    // Si aucune des solutions n'est bonne, présenter le problème à l'utilisateur
    if (!oneok)
    {
        cout << COLOR(tr("Setup n'a pas trouvé comment appliquer les changements que vous souhaitez"), "31") << endl;
        cout << qPrintable(tr("Voici la liste des tentatives et les raisons pour lesquelles elles ont échoué")) << endl;
        cout << endl;
        
        for (int i=0; i<tot; ++i)
        {
            packages = solver->result(i);
            
            cout << COLOR(tr("Tentative numéro %1 :").arg(i+1), "33") << endl;;
            
            for (int j=0; j<packages->errors(); ++j)
            {
                PackageList::Error *err = packages->error(j);
                
                cout << "  * ";
                
                switch (err->type)
                {
                    case PackageList::Error::SameNameSameVersionDifferentAction:
                        cout << qPrintable(tr("Le paquet %1 à la version %2 devait être %3,\n    mais un autre paquet lui a demandé d'être %4")
                            .arg(err->package)
                            .arg(err->version)
                            .arg(actionString(err->action))
                            .arg(actionString(err->otherAction)));
                        break;
                        
                    case PackageList::Error::SameNameDifferentVersionSameAction:
                        cout << qPrintable(tr("Le paquet %1 doit être %2\n    à la version %3 ET à la version %4")
                            .arg(err->package)
                            .arg(actionString(err->action))
                            .arg(err->version)
                            .arg(err->otherVersion));
                        break;
                        
                    case PackageList::Error::NoPackagesMatchingPattern:
                        cout << qPrintable(tr("Le paquet %1 à la version %2\n    dépend de «%3», qui est introuvable")
                            .arg(err->package)
                            .arg(err->version)
                            .arg(err->pattern));
                        break;
                }
                
                cout << endl;
            }
        }
        
        cout << endl;
        return;
    }
    
    // Sinon, lui montrer le choix
    cout << COLOR(tr("Paquets qui seront installés ou supprimés :"), "32") << endl;
    cout << qPrintable(tr("    Légende : "))
         << COLOR(tr("I: Installé "), "34")
         << COLOR(tr("R: Supprimé "), "31")
         << COLOR(tr("U: Mis à jour "), "33")
         << COLOR(tr("P: Supprimé totalement "), "35")
         << endl;
    
    while (true)
    {
        cout << endl;

        packages = solver->result(index);
        
        // Si la liste est vide, c'est qu'on n'a rien à faire
        // TODO (p.e dans solver) : si la liste est vide et qu'on a demandé une installation, passer le paquet en installé manuellement
        if (packages->count() == 0)
        {
            index++;
            
            if (index >= tot)
            {
                // Reboucler
                if (allempty == true)
                {
                    // Sauf que rien ne contient la solution, donc quitter
                    cout << COLOR(tr("Les changements que vous souhaitez appliquer semblent déjà avoir été appliqués. Votre système ne sera pas modifié."), "34") << endl << endl;
                    return;
                }
                
                index = 0;
                continue;
            }
            
            // Passer à la solution suivante
            continue;
        }
        
        allempty = false;

        displayPackages(packages, instSize, dlSize, true);

        cout << endl;

        // Demander si c'est bon
        if (instSize >= 0)
        {
            cout << qPrintable(tr("Solution %1 sur %2, de poids %3. Téléchargement de %4, installation de %5\n"
                                "Accepter (y), Suivante (n), Précédante (p) ou Annuler (c) ? ")
                                    .arg(QString::number(index+1))
                                    .arg(QString::number(tot))
                                    .arg(packages->weight())
                                    .arg(PackageSystem::fileSizeFormat(dlSize))
                                    .arg(PackageSystem::fileSizeFormat(instSize)));
        }
        else
        {
            cout << qPrintable(tr("Solution %1 sur %2, de poids %3. Téléchargement de %4, libération de %5\n"
                              "Accepter (y), Suivante (n), Précédante (p) ou Annuler (c) ? ")
                                  .arg(QString::number(index+1))
                                  .arg(QString::number(tot))
                                  .arg(packages->weight())
                                  .arg(PackageSystem::fileSizeFormat(dlSize))
                                  .arg(PackageSystem::fileSizeFormat(-instSize)));
        }
        
        getString(in, 2, "y", true);

        if (in[0] == 'y')
        {
            if (packages->count() == 0)
            {
                return;
            }
            
            break;
        }
        else if (in[0] == 'n')
        {
            index = qMin(index + 1, tot - 1);
        }
        else if (in[0] == 'p')
        {
            index = qMax(index - 1, 0);
        }
        else if (in[0] == 'c')
        {
            return;
        }
        else
        {
            break;
        }
    }

    cout << endl;
    cout << COLOR(tr("Installation des paquets..."), "32") << endl;
    cout << endl;
    
    // Connecter un signal
    connect(packages, SIGNAL(communication(Logram::Package *, Logram::Communication *)), this, 
                        SLOT(communication(Logram::Package *, Logram::Communication *)));

    // Installer les paquets
    if (!packages->process())
    {
        error();
        return;
    }   

    cout << endl;
    cout << COLOR(tr("Opérations sur les paquets appliquées !"), "32") << endl;
    cout << endl;
}