/*
 * solver.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include <QList>
#include <QHash>
#include <QCoreApplication>

#include <QtDebug>

using namespace Logram;

struct Solver::Private
{
    PackageSystem *ps;
    DatabaseReader *psd;
    bool installSuggests, useDeps, useInstalled;
    bool error;

    struct WantedPackage
    {
        QString pattern;
        Solver::Action action;
    };
    
    QList<WantedPackage> wantedPackages;
    
    QList<Solver::Node *> nodes;
    Node *rootNode;

    // Fonctions
    bool addNode(Package *package, Solver::Node *node);
    Node *checkPackage(int index, Solver::Action action, bool &ok);
    bool addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child);
    
};

Solver::Solver(PackageSystem *ps, DatabaseReader *psd)
{
    d = new Private;
    
    d->error = false;
    d->psd = psd;
    d->ps = ps;
    d->useDeps = true;
    d->useInstalled = true;

    d->installSuggests = ps->installSuggests();
}

Solver::~Solver()
{
    delete d;
}

bool Solver::setUseDeps(bool enable)
{
    d->useDeps = enable;
}

bool Solver::setUseInstalled(bool enable)
{
    d->useInstalled = enable;
}

void Solver::addPackage(const QString &nameStr, Action action)
{
    Private::WantedPackage pkg;
    
    pkg.pattern = nameStr;
    pkg.action = action;
    
    d->wantedPackages.append(pkg);
}

bool Solver::solve()
{
    // Créer le noeud principal
    d->rootNode = new Node;
    
    return d->addNode(0, d->rootNode);
}

Solver::Node *Solver::root()
{
    return d->rootNode;
}

bool Solver::Private::addNode(Package *package, Solver::Node *node)
{
    Solver::Action action = Solver::None;
    
    if (package) action = package->action();
    
    // Initialiser le noeud
    node->package = package;
    node->flags = Solver::Node::Wanted;
    node->error = 0;
    node->children = 0;
    node->childcount = 0;
    
    node->minWeight = 0;
    node->maxWeight = 0;
    node->minDlSize = 0;
    node->maxDlSize = 0;
    node->minInstSize = 0;
    node->maxInstSize = 0;
    
    nodes.append(node);
    
    // Vérifier la validité du paquet
    if (package && !package->isValid())
    {
        Solver::Error *err = new Solver::Error;
        err->type = Solver::Error::InternalError;
        err->other = 0;
        
        node->error = err;
        
        return false;
    }
    
    // Vérifier que ce qu'on demande est bon
    if (package && ((package->flags() & PACKAGE_FLAG_DONTINSTALL) != 0) && action == Solver::Install)
    {
        Solver::Error *err = new Solver::Error;
        err->type = Solver::Error::UninstallablePackageInstalled;
        err->other = 0;
        
        node->error = err;
        
        return false;
    }
    else if (package && ((package->flags() & PACKAGE_FLAG_DONTREMOVE) != 0) && (action == Solver::Remove || action == Solver::Purge))
    {
        Solver::Error *err = new Solver::Error;
        err->type = Solver::Error::UnremovablePackageRemoved;
        err->other = 0;
        
        node->error = err;
        
        return false;
    }
    
    // Vérifier que ce paquet est voulu, sauf si l'utilisateur veut qu'on ignore
    if (useInstalled && package && package->origin() == Package::Database)
    {
        if (
            (((package->flags() & PACKAGE_FLAG_INSTALLED) != 0) && action == Solver::Install) ||
            (((package->flags() & PACKAGE_FLAG_INSTALLED) == 0) && action != Solver::Install)
            )
        {
            node->flags &= ~Solver::Node::Wanted;
            return true;
        }
    }
    
    // Node supplémentaire en cas de mise à jour
    Node *suppl = 0;
    
    // Si on veut installer ce paquet, voir si on fini par une mise à jour
    if (action == Solver::Install && package && package->origin() == Package::Database)
    {
        // Paquet dans la base de donnée
        DatabasePackage *dpkg = (DatabasePackage *)package;
        int pindex = dpkg->index();
        _Package *mpkg = psd->package(pindex);
        
        // Explorer les autres versions
        QList<int> otherVersions = psd->packagesOfString(0, mpkg->name, DEPEND_OP_NOVERSION);
        
        foreach(int otherVersion, otherVersions)
        {
            if (otherVersion == pindex)
            {
                // Ignorer ce paquet
                continue;
            }
            
            _Package *opkg = psd->package(otherVersion);
            
            // NOTE: le "&& opkg->name == mpkg->name" permet d'avoir deux paquets fournissant le même provide ensemble
            if ((opkg->flags & PACKAGE_FLAG_INSTALLED) && opkg->version != mpkg->version && opkg->name == mpkg->name)
            {
                // Il va falloir supprimer ce paquet
                bool ok = true;
                suppl = checkPackage(otherVersion, Solver::Remove, ok);
                
                // Tout doit s'être bien passé
                if (!ok)
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::ChildError;
                    err->other = suppl;
                    
                    node->error = err;
                    
                    return false;
                }
                
                // Vérifier que nd est updatable
                if (suppl->package && ((suppl->package->flags() && PACKAGE_FLAG_DONTUPDATE) != 0))
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::UnupdatablePackageUpdated;
                    err->other = suppl;
                    
                    node->error = err;
                    
                    return false;
                }
            }
        }
    }
    
    // Créer la liste des enfants
    QList<_Depend *> deps;
    
    if (package == 0)
    {
        node->childcount = wantedPackages.count();
    }
    else if (!useDeps)
    {
        // On ne gère pas les dépendances
        return true;
    }
    else if (package->origin() == Package::Database)
    {
        deps = psd->depends(((DatabasePackage *)package)->index());
        
        // Explorer ces dépendances pour savoir combien d'enfants il faut
        foreach (_Depend *dep, deps)
        {
            if ((dep->type == DEPEND_TYPE_DEPEND && action == Solver::Install) ||
                (dep->type == DEPEND_TYPE_SUGGEST && action == Solver::Install && installSuggests))
            {
                node->childcount++;
            }
            else if ((dep->type == DEPEND_TYPE_CONFLICT || dep->type == DEPEND_TYPE_REPLACE) && action == Solver::Install)
            {
                node->childcount++;
            }
            else if (dep->type == DEPEND_TYPE_REVDEP && action == Solver::Remove)
            {
                node->childcount++;
            }
        }
    }
    else
    {
        // TODO: Dépendances des paquets locaux
    }
    
    // Si on n'a pas d'enfants, c'est déjà fini
    if (node->childcount == 0)
    {
        return true;
    }
    
    // Allouer la liste des enfants
    Node::Child *children = new Node::Child[node->childcount];
    node->children = children;
    
    // Ajouter les enfants
    for (int i=0, di=0; i<node->childcount; ++i, ++di)
    {
        if (package == 0)
        {
            const WantedPackage &wp = wantedPackages.at(i);
            
            if (wp.pattern.endsWith(".lpk"))
            {
                // On veut installer un paquet depuis un fichier
                FilePackage *fpkg = new FilePackage(wp.pattern, ps, psd, wp.action);
                
                // Nouveau Node pour ce paquet
                Node *nd = new Node;
                
                // Enregistrer ce noeud
                children[i].count = 1;
                children[i].node = nd;
                
                if (!addNode(fpkg, nd))
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::ChildError;
                    err->other = nd;
                    
                    node->error = err;
                    
                    return false;
                }
            }
            else
            {
                // Nom depuis la base de donnée
                QList<int> pkgIndexes = psd->packagesByVString(wp.pattern);
                
                if (pkgIndexes.count() == 0)
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::NoDeps;
                    err->pattern = wp.pattern;
                    
                    node->error = err;
                    
                    return false;
                }
                
                // Ajouter les paquets
                if (!addPkgs(pkgIndexes, node, wp.action, &children[i]))
                {
                    // addPkgs s'occupe de l'erreur
                    return false;
                }
            }
        }
        else if (package->origin() == Package::Database)
        {
            _Depend *dep = deps.at(di);
            Solver::Action act = Solver::None;
            
            // Trouver l'action
            if ((dep->type == DEPEND_TYPE_DEPEND && action == Solver::Install) ||
                (dep->type == DEPEND_TYPE_SUGGEST && action == Solver::Install && installSuggests))
            {
                act = Solver::Install;
            }
            else if ((dep->type == DEPEND_TYPE_CONFLICT || dep->type == DEPEND_TYPE_REPLACE) && action == Solver::Install)
            {
                act = Solver::Remove;
            }
            else if (dep->type == DEPEND_TYPE_REVDEP && action == Solver::Remove)
            {
                // TODO: Gérer les revdeps : soit installer un autre provide de ce paquet,
                //       soit supprimer la revdep.
                //
                //       Attention ! Bug dans l'ancienne version du à ceci : on installe
                //       **une seule fois** les autres provides de ce paquet, sinon on arrive dans des trucs énormes !
            }
            
            if (act == Solver::None)
            {
                // Rien à faire. Décrémenter i pour ne pas sortir du tableau children, laisser
                // di comme il est car il sert à récupérer la dépendance suivante.
                i--;
                continue;
            }
            
            // Trouver les paquets à installer
            QList<int> pkgIndexes = psd->packagesOfString(dep->pkgver, dep->pkgname, dep->op);
            
            if (pkgIndexes.count() == 0 && act == Solver::Install)
            {
                Solver::Error *err = new Solver::Error;
                err->type = Solver::Error::NoDeps;
                err->other = 0;
                err->pattern = PackageSystem::dependString(
                                    psd->string(false, dep->pkgname),
                                    psd->string(false, dep->pkgver),
                                    dep->op);
                                    
                node->error = err;
                
                return false;
            }
            
            // Ajouter les paquets
            if (!addPkgs(pkgIndexes, node, act, &children[i]))
            {
                // addPkgs s'occupe de l'erreur
                return false;
            }
        }
        else
        {
            // TODO: Gérer les dépendances des paquets en provenance de fichiers
        }
    }
    
    // TODO: Vérifier les enfants de ce noeud pour éviter tout conflit
    
    return true;
}

bool Solver::Private::addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child)
{
    // Si on n'a qu'un enfant, c'est simple
    if (pkgIndexes.count() == 1)
    {
        bool ok = true;
        Node *nd = checkPackage(pkgIndexes.at(0), action, ok);
        
        // Enregistrer le noeud
        child->count = 1;
        child->node = nd;
        
        if (!ok)
        {
            Solver::Error *err = new Solver::Error;
            err->type = Solver::Error::ChildError;
            err->other = nd;
            
            node->error = err;
            
            return false;
        }
    }
    else
    {
        // Sinon, il nous faut une liste des enfants
        int count = pkgIndexes.count();
        
        Node **nodes = new Node *[count];
        
        child->count = count;
        child->nodes = nodes;
        
        for (int j=0; j<pkgIndexes.count(); ++j)
        {
            int pkgIndex = pkgIndexes.at(j);
            
            bool ok = true;
            Node *nd = checkPackage(pkgIndex, action, ok);
            
            // Enregistrer le noeud
            nodes[j] = nd;
            
            // Si pas ok, on décrémente count, c'est tout.
            // Une fois en dehors du for, on vérifie count. S'il est arrivé à zéro, alors
            // aucun des variantes n'est bonne, et on envoie une erreur.
            if (!ok)
            {
                count--;
            }
        }
        
        if (count == 0)
        {
            Solver::Error *err = new Solver::Error;
            err->type = Solver::Error::ChildError;
            err->other = 0;
            
            node->error = err;
            
            return false;
        }
    }
    
    return true;
}

Solver::Node *Solver::Private::checkPackage(int index, Solver::Action action, bool &ok)
{
    // Explorer tous les noeuds à la recherche d'un éventuel qui correspondrait à l'index et à l'action
    for (int i=0; i<nodes.count(); ++i)
    {
        Solver::Node *node = nodes.at(i);
        
        if (node->package && node->package->action() == action && node->package->origin() == Package::Database && ((DatabasePackage *)node->package)->index() == index)
        {
            // Le noeud existe déjà, le retourner
            return node;
        }
    }
    
    // Pas de noeud correspondant trouvé
    DatabasePackage *package = new DatabasePackage(index, ps, psd, action);
    Solver::Node *node = new Solver::Node;
    ok = addNode(package, node);
    
    return node;
}
