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

#include <solver.h>
#include <databasepackage.h>
#include <packagelist.h>
#include <packagemetadata.h>

#include <iostream>
using namespace std;

using namespace Logram;

void App::add(const QStringList &packages)
{
    Solver *solver = ps->newSolver();
    
    solver->setUseDeps(useDeps);
    solver->setUseInstalled(useInstalled);
    
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

    if (!solver->solve() || !solver->weight())
    {
        cout << COLOR(tr("ERREUR : "), "31");
        cout << qPrintable(solverError(solver, tr("Bug dans Setup !"))) << endl;
        
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
    
    displayPackages((QList<Package *> *)(&pkgs), instSize, dlSize, false);
    
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
    
    displayPackages((QList<Package *> *)(&pkgs), instSize, dlSize, false);
    
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

void App::displayPackage(Package* pkg, int i, int &instSize, int &dlSize, bool showType)
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
        cout << ' ' << COLOR(tr("(licence à accepter)"), "31");
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

void App::displayPackages(QList< Package* >* packages, int &instSize, int &dlSize, bool showType)
{
    instSize = 0;
    dlSize = 0;
    
    for (int i=0; i<packages->count(); ++i)
    {
        Package *pkg = packages->at(i);
        
        displayPackage(pkg, i, instSize, dlSize, showType);
    }
}

static QString treeLinkWeight(Solver::Node *node)
{
    QString rs("%1 -> %2 (%3 -> %4, %5 -> %6)");
    
    // Label avec le poids
    rs = rs.arg(node->minWeight)
           .arg(node->maxWeight)
           .arg(PackageSystem::fileSizeFormat(node->minDlSize))
           .arg(PackageSystem::fileSizeFormat(node->maxDlSize))
           .arg(PackageSystem::fileSizeFormat(node->minInstSize))
           .arg(PackageSystem::fileSizeFormat(node->maxInstSize));
           
    return rs;
}

static void printTree(Solver::Node *node)
{
    // Ignorer un noeud déjà parcouru
    if ((node->flags & Solver::Node::Proceed) != 0) return;
    
    // Description du noeud
    cout << "    " << 'n' << node << " [";
    
    if (!node->package)
    {
        cout << "label=\"Root\\n" << qPrintable(treeLinkWeight(node)) << "\"";
    }
    else
    {
        cout << "label=\"" << qPrintable(node->package->name() + "~" + node->package->version() + " (" + QString::number(node->weight) + ") " + (node->error != 0 ? "(E)" : "") + "\\n" + treeLinkWeight(node)) << '"';
    }
    
    if ((node->flags & Solver::Node::Wanted) == 0)
    {
        cout << ", fontcolor=\"#aaaaaa\"";
    }
    
    if (node->package && (node->package->action() == Solver::Remove || node->package->action() == Solver::Purge))
    {
        cout << ", color=\"#ffaaaa\"";
    }
    
    cout << "];" << endl;
    
    // Marquer le noeud comme étant parcouru
    node->flags |= Solver::Node::Proceed;
    
    // Enfants
    Solver::Node::Child *children = node->children;
    
    for (int i=0; i<node->childcount; ++i)
    {
        Solver::Node::Child *child = &children[i];
        
        if (child->count == 1)
        {
            // Un seul enfant
            cout << "    " << 'n' << node << " -> " << 'n' << child->node << " [color=\"#000000\"];" << endl;
            
            printTree(child->node);
        }
        else
        {
            // Plusieurs enfants, créer un noeud vide qui montrera qu'il y a un choix.
            cout << "    " << 'c' << child << " [label=\" \", color=\"#444488\", shape=circle];" << endl;
            cout << "    " << 'n' << node << " -> " << 'c' << child << " [color=\"#4444cc\"];" << endl;
            
            Solver::Node **nodes = child->nodes;
            
            for (int j=0; j<child->count; ++j)
            {
                Solver::Node *nd = nodes[j];
                
                cout << "    " << 'c' << child << " -> " << 'n' << nd << " [color=\"#4444cc\"];" << endl;
                
                printTree(nd);
            }
        }
    }
}

static QString actionStringInf(Solver::Action action)
{
    switch (action)
    {
        case Solver::Install:
            return App::tr("Installer");
        case Solver::Remove:
            return App::tr("Supprimer");
        case Solver::Update:
            return App::tr("Mettre à jour");
        case Solver::Purge:
            return App::tr("Supprimer totalement");
        default:
            return QString();
    }
}

QString App::solverError(Logram::Solver *solver, const QString &defaultString)
{
    // Trouver le noeud du solveur qui a une erreur
    Solver::Node *errorNode = solver->errorNode();
    
    if (errorNode == 0)
    {
        // Ce n'est pas un noeud qui a provoqué l'erreur, voir si ce n'est pas PackageSystem
        PackageError *err = ps->lastError();
        
        if (err != 0)
        {
            return psError(err);
        }
        else
        {
            // Erreur par défaut
            return defaultString;
        }
    }
    
    // On a un noeud qui a fait des erreurs. Descendre dans son arborescence s'il est en childError
    Solver::Error *err = errorNode->error;
    
    while (err->type == Solver::Error::ChildError && err->other != 0)
    {
        if (err->other->error == 0)
        {
            return tr("Erreur inconnue. Ajoutez le paramètre -G à Setup et postez sa sortie dans votre rapport de bug");
        }
        
        errorNode = err->other;
        err = errorNode->error;
    }
    
    // err est l'erreur la plus profonde, trouver son nom
    QString pkgname, othername;
    
    if (errorNode->package)
    {
        pkgname = errorNode->package->name() + "~" + errorNode->package->version();
    }
    
    if (err->other && err->other->package)
    {
        othername = err->other->package->name() + "~" + err->other->package->version();
    }
    
    // En fonction du type
    PackageError *perr;
    
    switch (err->type)
    {
        case Solver::Error::NoDeps:
            return tr("Impossible de trouver la dépendance correspondant à %1").arg(err->pattern);
       
        case Solver::Error::InternalError:
            perr = ps->lastError();
        
            if (perr != 0)
            {
                return psError(perr);
            }
            else
            {
                // Erreur par défaut
                return defaultString;
            }
            
        case Solver::Error::ChildError:
            if (!pkgname.isNull())
            {
                return tr("Aucun des choix nécessaires à l'installation de %1 n'est possible").arg(pkgname);
                // TODO: Explorer les enfants d'errorNode pour lister les erreurs.
            }
            else
            {
                return tr("Aucun des choix permettant d'effectuer l'opération que vous demandez n'est possible");
            }
            
        case Solver::Error::SameNameSameVersionDifferentAction:
            return tr("Le paquet %1 devrait être installé et supprimé en même temps").arg(pkgname);
            
        case Solver::Error::InstallSamePackageDifferentVersion:
            return tr("Les paquets %1 et %2 devraient être installés en même temps").arg(pkgname, othername);
            
        case Solver::Error::UninstallablePackageInstalled:
            return tr("Le paquet %1 ne peut être installé mais devrait être installé").arg(pkgname);
            
        case Solver::Error::UnremovablePackageRemoved:
            return tr("Le paquet %1 ne peut être supprimé mais devrait être supprimé").arg(pkgname);
            
        case Solver::Error::UnupdatablePackageUpdated:
            return tr("Le paquet %1 ne peut être mis à jour mais devrait être mis à jour").arg(pkgname);
    }
    
    return defaultString;
}

void App::manageResults(Solver *solver)
{
    if (depsTree)
    {
        // Créer l'arbre des dépendances
        cout << "digraph G {" << endl;
        cout << "    node [shape=rectangle];" << endl;
        
        printTree(solver->root());
        
        cout << "}" << endl;
        return;
    }
    
    // Parcourir l'arbre du solveur
    bool ended = true;
    
    if (!solver->beginList(ended))
    {
        cout << COLOR(tr("Erreur : "), "31") << qPrintable(solverError(solver, tr("Erreur inconnue, sans doutes un bug"))) << endl;
        return;
    }
    
    while (!ended)
    {
        // PackageList temporaire pour afficher à l'utilisateur ce qu'on a déjà
        PackageList *ps = solver->list();
        int instSize, dlSize;
        
        if (ps->count() && solver->choiceNode() != 0 && solver->choiceNode()->package != 0)
        {
            cout << COLOR(tr("Liste des paquets pouvant être installés :"), "36") << endl;
            cout << qPrintable(tr("    Légende : "))
                << COLOR(tr("I: Installé "), "34")
                << COLOR(tr("R: Supprimé "), "31")
                << COLOR(tr("U: Mis à jour "), "33")
                << COLOR(tr("P: Supprimé totalement "), "35")
                << endl;
            cout << endl;
            
            displayPackages(ps, instSize, dlSize, true);
            
            Package *pkg = solver->choiceNode()->package;
            
            // Dire qu'un paquet propose un choix
            cout << endl;
            cout << COLOR(tr("Le paquet %1~%2 possède une dépendance permettant un choix :").arg(pkg->name(), pkg->version()), "32") << endl;
            cout << endl;
            
            // Plus besoin de la liste
            delete ps;
        }
        else
        {
            // Choix de premier niveau : on installe par exemple machin>=truc, et machin existe en deux versions.
            cout << COLOR(tr("Un des paquets que vous avez demandé peut être obtenu de plusieurs manières :"), "32") << endl;
            cout << endl;
        }
        
        // Afficher la liste des choix
        int minWeightIndex = -1;
        QList<Solver::Node *> choices = solver->choices();
        
        for (int i=0; i<choices.count(); ++i)
        {
            Solver::Node *node = choices.at(i);
            Package *choice = node->package;
            
            cout << ' ' << COLOR(QString::number(i + 1) + ". ", "34");              // 1.
            cout << qPrintable(actionStringInf(choice->action()));                  // Installer
            cout << ' ';
            cout << COLOR(choice->name(), "31") << '~' << COLOR(choice->version(), "32"); // machin~truc
            cout << endl;
            cout << "    ";
            
            if ((node->flags & Solver::Node::Wanted) == 0)
            {
                cout << qPrintable(tr("(ce choix n'aura aucune influence sur le système)")) << endl;
            }
            else
            {
                cout << qPrintable(tr("Poids entre %1 et %2 (téléchargement de %3 à %4, %5%6%7%8)")
                            .arg(node->minWeight)
                            .arg(node->maxWeight)
                            .arg(PackageSystem::fileSizeFormat(node->minDlSize))
                            .arg(PackageSystem::fileSizeFormat(node->maxDlSize))
                            .arg(node->minInstSize < 0 ? tr("suppression de ") : tr("installation de "))
                            .arg(PackageSystem::fileSizeFormat(qAbs(node->minInstSize)))
                            .arg(node->maxInstSize < 0 ? (node->minInstSize < 0 ? tr(" à ") : tr(" à suppression de ")) : (node->minInstSize < 0 ? tr(" à installation de ") : tr(" à ")))
                            .arg(PackageSystem::fileSizeFormat(node->maxInstSize))) << endl;
            }
            
            // Voir si ce choix est intéressant du côté du poids
            if (minWeightIndex == -1)
            {
                minWeightIndex = i;
            }
            else
            {
                Solver::Node *nd = choices.at(minWeightIndex);
                
                if (node->minWeight < nd->minWeight)
                {
                    // On a trouvé mieux
                    minWeightIndex = i;
                }
            }
        }
        
        // Poser la question
        cout << endl;
        
        while (true)
        {
            cout << qPrintable(tr("Solution à choisir (c : Annuler, u : Revenir au choix précédant) : "));
            cout.flush();
            
            char buffer[5]; // jusqu'à "1000\0"
            
            getString(buffer, 5, QByteArray::number(minWeightIndex + 1).constData(), true);
            
            if (buffer[0] == 'c')
            {
                // Annulation
                return;
            }
            else if (buffer[0] == 'u')
            {
                if (!solver->upList())
                {
                    // On était déjà au début
                    cout << COLOR(tr("Pas de choix précédant"), "31") << endl;
                    continue; // Reposer la question
                }
                
                // Revenir au while (!ended)
                break;
            }
            else
            {
                bool ok = true;
                int index = QByteArray(buffer).toInt(&ok);
                index--; // L'utilisateur commence à 1, nous on commence à 0
                
                if (!ok || index < 0 || index >= choices.count())
                {
                    // Entrée pas bonne, redemander
                    continue;
                }
                
                // Informer qu'on a fait un choix
                if (!solver->continueList(index, ended))
                {
                    cout << COLOR(tr("Impossible de choisir cette entrée : "), "31") << qPrintable(solverError(solver, tr("Bug dans Setup !"))) << endl;
                    cout << qPrintable(tr("Si vous avez essayé tous les choix, essayez de remonter (u).")) << endl;
                    
                    // Permettre à l'utilisateur d'essayer une autre proposition
                    continue;
                }
                
                // Continuer l'installation
                break;
            }
        }
        
        cout << endl;
    }
    
    // On a exploré tous les paquets, les afficher pour confirmation
    PackageList *packages = solver->list();
    int instSize = 0, dlSize = 0;
    char in[2];
    
    cout << COLOR(tr("Paquets qui seront installés ou supprimés :"), "36") << endl;
    cout << qPrintable(tr("    Légende : "))
         << COLOR(tr("I: Installé "), "34")
         << COLOR(tr("R: Supprimé "), "31")
         << COLOR(tr("U: Mis à jour "), "33")
         << COLOR(tr("P: Supprimé totalement "), "35")
         << endl << endl;
         
    displayPackages(packages, instSize, dlSize, true);
    
    cout << endl;
    
    if (instSize >= 0)
    {
        cout << qPrintable(tr("%1 licence(s) à accepter, installation de %2, téléchargement de %3, accepter (y/n) ? ")
                    .arg(packages->numLicenses())
                    .arg(PackageSystem::fileSizeFormat(instSize))
                    .arg(PackageSystem::fileSizeFormat(dlSize)));
    }
    else
    {
        cout << qPrintable(tr("%1 licence(s) à accepter, suppression de %2, téléchargement de %3, accepter (y/n) ? ")
                    .arg(packages->numLicenses())
                    .arg(PackageSystem::fileSizeFormat(-instSize))
                    .arg(PackageSystem::fileSizeFormat(dlSize)));
    }
    
    cout.flush();
    
    getString(in, 2, "y", true);
    
    if (in[0] != 'y')
    {
        return;
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
                     << COLOR(tr("Accepter cette licence (y/n) ? "), "32");
                     
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
        
        instSize = 0, dlSize = 0;
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