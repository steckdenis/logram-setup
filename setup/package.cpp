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
        // TODO: Gérer les erreurs de solver->root()
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
        cout << "label=\"" << qPrintable(node->package->name() + "~" + node->package->version() + " (" + QString::number(node->weight) + ")\\n" + treeLinkWeight(node)) << '"';
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

void App::manageResults(Solver *solver)
{
    if (depsTree)
    {
        // Créer l'arbre des dépendances
        cout << "digraph G {" << endl;
        cout << "    node [shape=rectangle];" << endl;
        
        printTree(solver->root());
        
        cout << "}" << endl;
    }
#if 0
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
#endif
}