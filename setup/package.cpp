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
#include <logram/packagemetadata.h>

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
    
    if (!ps->download(repo.type, repo.mirrors.at(0) + "/" + dpkg->url(DatabasePackage::Source), fname, true, md))
    {
        error();
        return;
    }
    
    // Suggérer les dépendances qui seraient utiles
    cout << COLOR(tr("Ce paquet pourrait nécessiter ceci pour être compilé :"), "35") << endl;
    
    cout << qPrintable(tr("Légende : "))
         << COLOR(tr("D: Dépend "), "31")
         << COLOR(tr("S: Suggère "), "32")
         << COLOR(tr("C: Conflit "), "33")
         << COLOR(tr("P: Fourni "), "34")
         << COLOR(tr("R: Remplace "), "35")
         << COLOR(tr("N: Est requis par"), "37")
    << endl << endl;
    
    PackageMetaData *meta = dpkg->metadata();
    QList<SourceDepend *> deps = meta->sourceDepends();
    
    foreach (SourceDepend *dep, deps)
    {
        switch (dep->type)
        {
            case DEPEND_TYPE_DEPEND:
                cout << " D: ";
                cout << COLOR(dep->string, "31");
                break;
            case DEPEND_TYPE_SUGGEST:
                cout << " S: ";
                cout << COLOR(dep->string, "32");
                break;
            case DEPEND_TYPE_CONFLICT:
                cout << " C: ";
                cout << COLOR(dep->string, "33");
                break;
            case DEPEND_TYPE_PROVIDE:
                cout << " P: ";
                cout << COLOR(dep->string, "34");
                break;
            case DEPEND_TYPE_REPLACE:
                cout << " R: ";
                cout << COLOR(dep->string, "35");
                break;
            case DEPEND_TYPE_REVDEP:
                cout << " N: ";
                cout << COLOR(dep->string, "37");
                break;
        }
        
        cout << endl;
    }
    
    cout << endl;
}

void App::autoremove()
{
    QList<DatabasePackage *> pkgs = ps->orphans();
    int instSize = 0, dlSize = 0;
    
    if (pkgs.count() == 0)
    {
        cout << COLOR(tr("Il n'y a aucun paquet orphelin dans votre système."), "34") << endl << endl;
        
        qDeleteAll(pkgs);
        return;
    }
    
    cout << COLOR(tr("Paquets installés automatiquement et qui ne sont plus nécessaires :"), "32") << endl << endl;
    
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
            
        solver->addPackage(pkg->name() + "=" + pkg->version(), Solver::Remove);
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
    qDeleteAll(pkgs);
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
    qDeleteAll(pkgs);
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

void App::displayPackage(Package *pkg, int i, int &instSize, int &dlSize, bool showType)
{
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
                << COLOR(pkg->version(), "32");
    }
    else
    {
        Package *opkg = pkg->upgradePackage();
        
        cout << ' '
                << COLOR(pkg->version(), "32")
                << " → "
                << COLOR(opkg->version(), "32");
    }
    
    // Afficher les flags importants du paquet (eula et reboot)
    if (pkg->flags() & PACKAGE_FLAG_EULA)
    {
        cout << ' ' << COLOR(tr("(license à accepter)"), "31");
    }
    
    if (pkg->flags() & PACKAGE_FLAG_NEEDSREBOOT)
    {
        cout << ' ' << COLOR(tr("(redémarrage)"), "31");
    }
    
    cout << endl
            << "        "
            << qPrintable(pkg->shortDesc())
            << endl;
}

void App::displayPackages(QList<Package *> *packages, int &instSize, int &dlSize, bool showType)
{
    instSize = 0;
    dlSize = 0;
    
    for (int i=0; i<packages->count(); ++i)
    {
        Package *pkg = packages->at(i);
        
        displayPackage(pkg, i, instSize, dlSize, showType);
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
                        
                    case PackageList::Error::UninstallablePackageInstalled:
                        cout << qPrintable(tr("Le paquet %1 à la version %2\n    devait être installé mais a été tagué comme étant non-installable")
                            .arg(err->package)
                            .arg(err->version));
                        break;
                        
                    case PackageList::Error::UnremovablePackageRemoved:
                        cout << qPrintable(tr("Le paquet %1 à la version %2\n    devait être supprimé mais a été tagué comme étant non-supprimable")
                            .arg(err->package)
                            .arg(err->version));
                        break;
                        
                    case PackageList::Error::UnupdateablePackageUpdated:
                        cout << qPrintable(tr("Le paquet %1 à la version %2\n    devait être mis à jour à la version %3,\n    mais a été tagué comme ne pouvant être mis à jour")
                            .arg(err->package)
                            .arg(err->version)
                            .arg(err->otherVersion));
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
        instSize = dlSize = 0;

        displayPackages(packages, instSize, dlSize, true);

        cout << endl;

        // Demander si c'est bon
        cout << qPrintable(tr("Solution %1 sur %2, de poids %3, %4 licence(s) à accepter%5.")
                                .arg(index + 1)
                                .arg(tot)
                                .arg(packages->weight())
                                .arg(packages->numLicenses())
                                .arg( (packages->needsReboot() ? tr(", nécessite un redémarrage") : QString())))
            << endl;
                                
        if (instSize >= 0)
        {
            cout << qPrintable(tr("Téléchargement de %1, installation de %2")
                                    .arg(COLORS(PackageSystem::fileSizeFormat(dlSize), "33"))
                                    .arg(COLORS(PackageSystem::fileSizeFormat(instSize), "33")));
        }
        else
        {
            cout << qPrintable(tr("Téléchargement de %1, libération de %2")
                                    .arg(COLORS(PackageSystem::fileSizeFormat(dlSize), "33"))
                                    .arg(COLORS(PackageSystem::fileSizeFormat(-instSize), "33")));
        }
        
        cout << endl
             << COLOR(tr("Accepter (y), Suivante (n), Précédante (p) ou Annuler (c) ? "), "32")
             << std::flush;
        
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
    
    // La liste a été validée, vérifier que toutes les CLUF sont acceptées
    if (packages->numLicenses() != 0)
    {
        for (int i=0; i<packages->count(); ++i)
        {
            Package *p = packages->at(i);
            
            if (p->flags() & PACKAGE_FLAG_EULA)
            {
                cout << endl
                     << COLOR(tr("License du paquet %1").arg(p->name()), "35")
                     << endl << endl;
                
                PackageMetaData *md = p->metadata();
                md->setCurrentPackage(p->name());
                
                printIndented(md->packageEula().toUtf8(), 4);
                
                cout << endl << endl
                     << COLOR(tr("Accepter cette license (y/n) ? "), "32");
                     
                getString(in, 2, "n", true);
                
                if (in[0] != 'y')
                {
                    // Abandon de l'installation
                    return;
                }
            }
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
    
    // Paquets devenus orphelins
    if (packages->orphans().count() != 0)
    {
        cout << endl;
        cout << COLOR(tr("Les paquets suivants ont été installés automatiquement et ne sont plus nécessaires :"), "31") << endl;
        cout << endl;
        
        instSize = dlSize = 0;
        int i = 0;
        
        foreach (int p, packages->orphans())
        {
            DatabasePackage *dp = ps->package(p);
            dp->setAction(Solver::Remove);
            
            displayPackage(dp, i, instSize, dlSize, false);
            ++i;
        }
        
        cout << endl;
        cout << qPrintable(tr("Pour une suppression totale de %1. Utilisez setup autoremove pour supprimer ces paquets.").arg(PackageSystem::fileSizeFormat(-instSize))) << endl;
    }
    
    // Savoir si on doit dire à l'utilisateur de redémarrer
    if (packages->needsReboot())
    {
        cout << endl;
        cout << COLOR(tr("Un ou plusieurs des paquets que vous avez installés nécessitent un redémarrage"), "31") << endl;
    }

    cout << endl;
    cout << COLOR(tr("Opérations sur les paquets appliquées !"), "32") << endl;
    cout << endl;
}