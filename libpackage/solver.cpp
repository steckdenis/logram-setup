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

#include "solver_p.h"
#include "packagesystem.h"
#include "databasereader.h"
#include "databasepackage.h"
#include "filepackage.h"
#include "packagelist.h"

#include <QList>
#include <QFile>

#include <QtDebug>
#include <QtScript>

using namespace Logram;

struct Solver::Private
{
    PackageSystem *ps;
    DatabaseReader *psd;
    bool installSuggests, useDeps, useInstalled;

    struct WantedPackage
    {
        QString pattern;
        Solver::Action action;
    };
    
    struct Level
    {
        int nodeIndex;  // Index dans nodeList
        int childIndex; // Index du premier enfant de ce noeud à explorer
    };
    
    QList<WantedPackage> wantedPackages;
    
    QList<Solver::Node *> nodes;
    Solver::Node *rootNode, *errorNode;
    
    QList<Solver::Node *> nodeList; // Liste des index dans nodes des noeuds qu'on va traiter (futur packageList)
    QList<Level> levels;
    Solver::Node::Child *choiceChild;

    // Fonctions
    bool addNode(Package *package, Solver::Node *node);
    Node *checkPackage(int index, Solver::Action action, bool &ok);
    bool addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child, bool revdep = false);
    
    bool exploreNode(Solver::Node* node, int firstChild, int choice, bool& ended);
};

Solver::Solver(PackageSystem *ps, DatabaseReader *psd)
{
    d = new Private;
    
    d->psd = psd;
    d->ps = ps;
    d->useDeps = true;
    d->useInstalled = true;
    d->errorNode = 0;

    d->installSuggests = ps->installSuggests();
}

Solver::~Solver()
{
    delete d;
}

void Solver::setUseDeps(bool enable)
{
    d->useDeps = enable;
}

void Solver::setUseInstalled(bool enable)
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

static void weightChildren(Solver::Node *node, Solver::Node *mainNode, bool weightMin)
{
    // node = le noeud qu'on va peser, explorer ses enfants, les peser, etc
    // mainNode = le noeud principal auquel on va ajouter le poids des noeuds qu'on pèse
    // weightMin = true si on s'intéresse au minimum, false si on s'intéresse au maximum
    
    // Si on a déjà pesé ce noeud, on a fini
    if (node->weightedBy == mainNode && ((node->flags & Solver::Node::WeightMin) != 0) == weightMin)
    {
        return;
    }
    
    // Dire qu'on l'a pesé
    node->weightedBy = mainNode;
    
    if (weightMin)
    {
        node->flags |= Solver::Node::WeightMin;
    }
    else
    {
        node->flags &= ~Solver::Node::WeightMin;
    }
    
    // Explorer ses enfants pour le pesage
    Solver::Node::Child *children = node->children, *child;
    
    for (int i=0; i<node->childcount; ++i)
    {
        child = &children[i];
        
        if (child->count == 1)
        {
            weightChildren(child->node, mainNode, weightMin);
        }
        else
        {
            // Prendre soit maxNode ou minNode;
            Solver::Node **nodes = child->nodes;
            Solver::Node *nd = nodes[(weightMin ? child->minNode : child->maxNode)];
            
            // Peser ce noeud
            weightChildren(nd, mainNode, weightMin);
        }
    }
    
    // Ajouter le poids de ce noeud au noeud principal
    if (weightMin)
    {
        mainNode->minWeight += node->weight;
        mainNode->minDlSize += (node->package ? node->package->downloadSize() : 0);
        mainNode->minInstSize += (node->package ? node->package->installSize() : 0);
    }
    else
    {
        mainNode->maxWeight += node->weight;
        mainNode->maxDlSize += (node->package ? node->package->downloadSize() : 0);
        mainNode->maxInstSize += (node->package ? node->package->installSize() : 0);
    }
}

static void minMaxWeight(Solver::Node *node)
{
    // Si le noeud est déjà pesé, c'est bon
    if ((node->flags & Solver::Node::MinMaxWeighted) != 0)
    {
        return;
    }
    
    // Directement dire qu'on a pesé ce node pour éviter les problèmes de dépendance en boucle
    node->flags |= Solver::Node::MinMaxWeighted;
    
    // Explorer les enfantes de ce noeud, et les peser
    Solver::Node::Child *children = node->children, *child;
    
    for (int i=0; i<node->childcount; ++i)
    {
        child = &children[i];
        
        if (child->count == 1)
        {
            minMaxWeight(child->node);
        }
        else
        {
            // Plus compliqué : on ne retient que les plus petits min et les plus gars max
            // Attention, car les tuples (weight, dlSize, instSize) douvent appartenir au
            // même noeud
            //
            // Il faut donc une priorité. Si on a un noeud de poids plus faible que le précédant,
            // on le garde. Si c'est le même, alors on départage avec dlSize. Si c'est le même, 
            // on départage avec installSize. Si c'est le même, on laisse l'autre (c'est juste
            // informatif).
            
            Solver::Node **nodes = child->nodes;
            Solver::Node *nd;
            
            for (int j=0; j<child->count; ++j)
            {
                nd = nodes[j];
                
                minMaxWeight(nd);
                
                // Voir si ce noeud est plus petit que les autres
                if (child->minNode == -1)
                {
                    child->minNode = j;
                }
                else
                {
                    Solver::Node *minNode = nodes[child->minNode];
                    
                    if (nd->minWeight < minNode->minWeight ||
                       (nd->minWeight == minNode->minWeight && nd->minDlSize < minNode->minDlSize) ||
                       (nd->minWeight == minNode->minWeight && nd->minDlSize == minNode->minDlSize && nd->minInstSize < minNode->minInstSize))
                    {
                        child->minNode = j;
                    }
                }
                
                // Voir si c'est le plus grand des autres
                if (child->maxNode == -1)
                {
                    child->maxNode = j;
                }
                else
                {
                    Solver::Node *maxNode = nodes[child->maxNode];
                    
                    if (nd->maxWeight > maxNode->maxWeight ||
                       (nd->maxWeight == maxNode->maxWeight && nd->maxDlSize > maxNode->maxDlSize) ||
                       (nd->maxWeight == maxNode->maxWeight && nd->maxDlSize == maxNode->maxDlSize && nd->maxInstSize > maxNode->maxInstSize))
                    {
                        child->maxNode = j;
                    }
                }
            }
            
            // On a obligatoirement un enfant plus grand et plus petit
            Q_ASSERT(child->minNode != -1 && child->maxNode != -1);
        }
    }
    
    // On a exploré tous les enfants de ce paquets, ils sont pesés. On peut
    // maintenant les explorer à nouveau pour trouver le poids minimum et maximum de ce paquet.
    
    weightChildren(node, node, false);
    weightChildren(node, node, true);
}

bool Solver::weight()
{
    QScriptValue func, global;
    QScriptValueList args;
    
    // Lire le programme QtScript
    QFile fl(d->ps->confRoot() + "/etc/lgrpkg/scripts/weight.qs");
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    // Parser le script
    QScriptEngine engine;
    engine.evaluate(fl.readAll(), fl.fileName());
    fl.close();
    
    if (engine.hasUncaughtException())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ScriptException;
        err->info = fl.fileName();
        err->more = engine.uncaughtExceptionLineNumber() + ": " + engine.uncaughtException().toString();
          
        d->ps->setLastError(err);
        
        return false;
    }
    
    global = engine.globalObject();
    func = global.property("weight");
    
    global.setProperty("solver", engine.newQObject(this));
    
    // Créer la liste de ScriptNode
    QObjectList olist;
    ScriptNode *nd;
    
    foreach (Node *node, d->nodes)
    {
        nd = new ScriptNode(node, this);
        olist.append(nd);
    }
    
    // Appeler la fonction
    args << qScriptValueFromSequence(&engine, olist);
    
    func.call(QScriptValue(), args);
    
    // Maintenant qu'on a le poids des paquets, calculer les min/max
    minMaxWeight(d->rootNode);
    
    return true;
}

bool Solver::beginList(bool &ended)
{
    ended = true; // Mis à false si on a un choix
    
    d->errorNode = 0;
    d->choiceChild = 0;
    
    return d->exploreNode(d->rootNode, -1, -1, ended);
}

bool Solver::continueList(int choice, bool &ended)
{
    // Vérifier qu'on a bien quelque-chose à continuer
    if (d->levels.count() == 0)
    {
        return false;
    }
    
    // Prendre le dernier niveau
    Private::Level level = d->levels.takeLast();
    
    // Continuer l'exploration
    ended = true;
    d->errorNode = 0;
    d->choiceChild = 0;
    
    return d->exploreNode((d->nodeList.count() ? d->nodeList.last() : d->rootNode), level.childIndex, choice, ended);
}

QList<Solver::Node *> Solver::choices()
{
    QList<Node *> rs;
    
    if (d->choiceChild == 0)
    {
        // Pas de choix
        return rs;
    }
    
    // Explorer les enfants de choiceChild
    Q_ASSERT(d->choiceChild->count > 1);
    
    Node **nodes = d->choiceChild->nodes;
    Node *node;
    
    for (int i=0; i<d->choiceChild->count; ++i)
    {
        node = nodes[i];
        rs.append(node);
    }
    
    return rs;
}

bool Solver::upList()
{
    // Vérifier que la liste des niveaux contient au moins deux éléments,
    // puisqu'on va en supprimer un et utiliser l'autre. Sinon, ne rien faire
    if (d->levels.count() < 2)
    {
        return false; // Rien à faire
    }
    
    // Supprimer le dernier élément
    d->levels.removeLast();
    
    // Se servier du dernier (de maintenant) pour supprimer les noeuds qui
    // viennent après dans d->nodeList
    const Private::Level &level = d->levels.last();
    
    while (d->nodeList.count() > (level.nodeIndex + 1))
    {
        d->nodeList.removeLast();
    }
    
    // Liste ok. Il faut maintenant re-peupler d->choiceChild, puisqu'on retourne à un choix
    Node *node = d->nodeList.last();
    d->choiceChild = &node->children[level.childIndex];
    
    return true; // Il y a du nouveau
}

PackageList *Solver::list()
{
    // Créer la PackageList pour les noeuds
    PackageList *rs = new PackageList(d->ps);
    
    // Explorer les noeuds
    foreach (Node *node, d->nodeList)
    {
        rs->addPackage(node->package);
    }
    
    return rs;
}

Solver::Node *Solver::errorNode()
{
    return d->errorNode;
}

bool Solver::Private::exploreNode(Solver::Node* node, int firstChild, int choice, bool& ended)
{
    // Vérifier que le noeud est correct
    if (node->error != 0)
    {
        errorNode = node;
        return false;
    }
    
    // Explorer les noeuds déjà présents dans la liste
    if (node->package && firstChild == -1)
    {
        Solver::Action myAction;
        QString myName = node->package->name();
        QString myVersion = node->package->version();
        
        foreach (Solver::Node *nd, nodeList)
        {
            Solver::Action otherAction = nd->package->action();
            
            if (nd == node)
            {
                // Ce noeud est déjà dans la liste, retourner
                return true;
            }
            
            if (nd->package->name() == myName)
            {
                if (nd->package->version() == myVersion)
                {
                    if (otherAction == myAction)
                    {
                        // Hack : on peut installer un paquet depuis un fichier
                        // qui a le même nom et la même version qu'un paquet de
                        // la BDD.
                        return true;
                    }
                    else
                    {
                        Solver::Error *err = new Solver::Error;
                        err->type = Solver::Error::SameNameSameVersionDifferentAction;
                        err->other = nd;
                        
                        node->error = err;
                        errorNode = node;
                        
                        return false;
                    }
                }
                else if (otherAction == myAction)
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::SameNameSameActionDifferentVersion;
                    err->other = nd;
                    
                    node->error = err;
                    errorNode = node;
                    
                    return false;
                }
            }
        }
        
        nodeList.append(node);
    }
    
    // Explorer les enfants
    Solver::Node::Child *children = node->children, *child;
    firstChild = (firstChild == -1 ? 0 : firstChild);
    
    for (int i=firstChild; i<node->childcount; ++i)
    {
        child = &children[i];
        
        if (child->count == 1)
        {
            // Explorer cet enfant, simplement
            if (!exploreNode(child->node, -1, -1, ended))
            {
                return false;
            }
            
            if (!ended) return true;
        }
        else
        {
            // Ah, on a un choix à faire. Vérifier qu'on n'a pas déjà les instructions
            if (choice != -1)
            {
                // On a un choix, simplement le prendre
                if (choice >= child->count)
                {
                    return false;
                }
                
                if (!exploreNode(child->nodes[choice], -1, -1, ended))
                {
                    return false;
                }
                
                if (!ended) return true;
            }
            else
            {
                // Demander à l'utilisateur
                choiceChild = child;
                ended = false;
                
                // Ajouter un niveau de choix
                Level level;
                
                level.nodeIndex = nodeList.count() - 1;
                level.childIndex = i;
                
                levels.append(level);
                
                // Retourner
                return true;
            }
        }
    }
    
    return true;
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
    
    node->weight = 0;
    node->minWeight = 0;
    node->maxWeight = 0;
    node->minDlSize = 0;
    node->maxDlSize = 0;
    node->minInstSize = 0;
    node->maxInstSize = 0;
    
    node->weightedBy = 0;
    
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
    QList<Depend *> fdeps;
    
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
        // Paquet fichier
        fdeps = package->depends();
        
        foreach(Depend *dep, fdeps)
        {
            if ((dep->type() == DEPEND_TYPE_DEPEND && action == Solver::Install) ||
                (dep->type() == DEPEND_TYPE_SUGGEST && action == Solver::Install && installSuggests))
            {
                node->childcount++;
            }
            else if ((dep->type() == DEPEND_TYPE_CONFLICT || dep->type() == DEPEND_TYPE_REPLACE) && action == Solver::Install)
            {
                node->childcount++;
            }
            else if (dep->type() == DEPEND_TYPE_REVDEP && action == Solver::Remove)
            {
                node->childcount++;
            }
        }
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
                FilePackage *fpkg = new FilePackage(wp.pattern, ps, psd, Solver::Install); // On ne peut qu'installer des .lpk
                
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
        else
        {
            Solver::Action act = Solver::None;
            _Depend *ddep;
            Depend *fdep;
            int dtype;
            
            // Type de la dépendance en fonction de l'origine du paquet
            if (package->origin() == Package::Database)
            {
                ddep = deps.at(di);
                dtype = ddep->type;
            }
            else
            {
                fdep = fdeps.at(di);
                dtype = fdep->type();
            }
            
            // Trouver l'action
            if ((dtype == DEPEND_TYPE_DEPEND && action == Solver::Install) ||
                (dtype == DEPEND_TYPE_SUGGEST && action == Solver::Install && installSuggests))
            {
                act = Solver::Install;
            }
            else if ((dtype == DEPEND_TYPE_CONFLICT || dtype == DEPEND_TYPE_REPLACE) && action == Solver::Install)
            {
                act = Solver::Remove;
            }
            else if (dtype == DEPEND_TYPE_REVDEP && action == Solver::Remove)
            {
                // Créer plusieurs noeuds pour cet enfant : un pour supprimer cette revdep, un pour chaque provide
                // qu'on peut installer à la place.
                QList<int> pkgIndexes;
                
                // Premier enregistrement : supprimer la revdep
                pkgIndexes = psd->packagesOfString(ddep->pkgver, ddep->pkgname, ddep->op);
                
                // pkgIndexes contient obligatoirement un paquet, c'est databasewriter qui s'en assure.
                // C'est une dépendance de type paquet=version.
                Q_ASSERT(pkgIndexes.count() == 1);
                
                // Paquet dans la base de donnée
                DatabasePackage *dpkg = (DatabasePackage *)package;
                int pindex = dpkg->index();
                _Package *mpkg = psd->package(pindex);
                
                // Ajouter à pkgIndexes les index des provides de ce paquet
                foreach(int pkgIndex, psd->packagesOfString(0, mpkg->name, DEPEND_OP_NOVERSION))
                {
                    if (pkgIndex != pindex)
                    {
                        pkgIndexes.append(pkgIndex);
                    }
                }
                
                // Ajouter les paquets
                if (!addPkgs(pkgIndexes, node, Solver::Install, &children[i], true))
                {
                    // addPkgs s'occupe de l'erreur
                    return false;
                }
                
                // On a géré les revdeps, on a fini
                continue;
            }
            
            if (act == Solver::None)
            {
                // Rien à faire. Décrémenter i pour ne pas sortir du tableau children, laisser
                // di comme il est car il sert à récupérer la dépendance suivante.
                i--;
                continue;
            }
            
            QList<int> pkgIndexes;
            
            // Trouver les indexes des paquets à installer en fonction de l'origine du paquet
            if (package->origin() == Package::Database)
            {
                pkgIndexes = psd->packagesOfString(ddep->pkgver, ddep->pkgname, ddep->op);
            }
            else
            {
                pkgIndexes = psd->packagesByVString(fdep->name(), fdep->version(), fdep->op());
            }
            
            // Dépendre de paquets qui n'existent pas n'est pas bon
            if (pkgIndexes.count() == 0 && act == Solver::Install)
            {
                Solver::Error *err = new Solver::Error;
                err->type = Solver::Error::NoDeps;
                err->other = 0;
                err->pattern = (package->origin() == Package::Database ?
                                    PackageSystem::dependString(
                                    psd->string(false, ddep->pkgname),
                                    psd->string(false, ddep->pkgver),
                                    ddep->op)
                                :   PackageSystem::dependString(
                                    fdep->name(), 
                                    fdep->version(), 
                                    fdep->op()));
                                    
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
    }
    
    return true;
}

bool Solver::Private::addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child, bool revdep)
{
    // Si on n'a qu'un enfant, c'est simple
    if (pkgIndexes.count() == 1)
    {
        bool ok = true;
        Node *nd = checkPackage(pkgIndexes.at(0), (revdep ? Solver::Remove : action), ok);
        
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
        child->maxNode = -1;
        child->minNode = -1;
        
        for (int j=0; j<pkgIndexes.count(); ++j)
        {
            int pkgIndex = pkgIndexes.at(j);
            
            bool ok = true;
            Node *nd = checkPackage(pkgIndex, (revdep && j == 0 ? Solver::Remove : action), ok);
            
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

/* Intégration QtScript */

struct ScriptNode::Private
{
    Solver::Node *node;
    ScriptError *error;
};

ScriptNode::ScriptNode(Logram::Solver::Node *node, QObject *parent) : QObject(parent)
{
    d = new Private;
    d->node = node;
    d->error = 0;
}

ScriptNode::~ScriptNode()
{
    delete d;
}

QObject *ScriptNode::package() const
{
    return d->node->package;
}

int ScriptNode::flags() const
{
    return (int)d->node->flags;
}

void ScriptNode::setFlags(int value)
{
    d->node->flags = Solver::Node::Flags((Solver::Node::Flag)value);
}

bool ScriptNode::hasError() const
{
    return (d->node->error != 0);
}

QObject *ScriptNode::error()
{
    if (d->error == 0)
    {
        if (d->node->error != 0)
        {
            d->error = new ScriptError(d->node->error, this);
        }
        else
        {
            d->error = (ScriptError *)(new QObject(this));
        }
    }
    
    return d->error;
}

int ScriptNode::weight() const
{
    return d->node->weight;
}

void ScriptNode::setWeight(int value)
{
    d->node->weight = value;
}

QObjectList ScriptNode::children()
{
    QObjectList rs;
    ScriptChild *child;
    
    if (d->node->childcount == 0) return rs;
    
    // Explorer les enfants de ce noeud
    Solver::Node::Child *children = d->node->children;
    
    for (int i=0; i<d->node->childcount; ++i)
    {
        child = new ScriptChild(&children[i], this);
        rs.append(child);
    }
    
    return rs;
}

struct ScriptChild::Private
{
    ScriptNode *node;
    Solver::Node::Child *child;
};

ScriptChild::ScriptChild(Solver::Node::Child *child, ScriptNode *parent) : QObject(parent)
{
    d = new Private;
    d->child = child;
    d->node = parent;
}

ScriptChild::~ScriptChild()
{
    delete d;
}

QObjectList ScriptChild::nodes()
{
    QObjectList rs;
    ScriptNode *node;
    
    if (d->child->count == 1)
    {
        // Utiliser le champs "node"
        node = new ScriptNode(d->child->node, this);
        rs.append(node);
        return rs;
    }
    
    // Sinon, explorer les enfants
    Solver::Node **nodes = d->child->nodes;
    
    for (int i=0; i<d->child->count; ++i)
    {
        node = new ScriptNode(nodes[i], this);
        rs.append(node);
    }
    
    return rs;
}

QObject *ScriptChild::node() const
{
    return d->node;
}

struct ScriptError::Private
{
    Solver::Error *error;
    ScriptNode *node, *parent;
};

ScriptError::ScriptError(Solver::Error *error, ScriptNode *parent) : QObject(parent)
{
    d = new Private;
    d->error = error;
    d->node = 0;
    d->parent = parent;
}

ScriptError::~ScriptError()
{
    delete d;
}

int ScriptError::type() const
{
    return (int)d->error->type;
}

QObject *ScriptError::other()
{
    if (d->node == 0)
    {
        if (d->error->other)
        {
            d->node = new ScriptNode(d->error->other, this);
        }
        else
        {
            d->node = (ScriptNode *)(new QObject(this));
        }
    }
    
    return d->node;
}

QString ScriptError::pattern() const
{
    return d->error->pattern;
}

QObject *ScriptError::node() const
{
    return d->parent;
}

#include "solver_p.moc"
#include "solver.moc"
