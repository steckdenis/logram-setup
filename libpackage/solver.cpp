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
    
    QList<WantedPackage> wantedPackages;
    
    QList<Solver::Node *> nodes;
    Solver::Node *rootNode, *errorNode;
    
    struct Level
    {
        int choiceNodeIndex;    // Index du noeud pour lequel on a un choix
        int lastNodeIndex;      // Index du dernier noeud dans la liste qui doit rester
        int choiceChildIndex;   // Index de l'enfant dans choiceNodeIndex qui présente un choix
    };
    
    QList<Solver::Node *> nodeList;
    QList<Level> levels;          // Liste des niveaux (int --> index dans nodeList)
    Solver::Node::Child *choiceChild;
    Solver::Node *choiceNode;

    // Fonctions
    bool addNode(Package *package, Solver::Node *node);
    Node *checkPackage(int index, Solver::Action action, bool &ok, bool userWanted);
    bool addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child, bool revdep = false);
    
    bool exploreNode(Solver::Node *node, bool &ended);
    bool verifyNode(Solver::Node *node, Solver::Error* &error);
};

Solver::Solver(PackageSystem *ps, DatabaseReader *psd)
{
    d = new Private;
    
    d->psd = psd;
    d->ps = ps;
    d->useDeps = true;
    d->useInstalled = true;
    d->errorNode = 0;
    d->rootNode = 0;

    d->installSuggests = ps->installSuggests();
}

Solver::~Solver()
{
    // Supprimer les noeuds
    foreach (Node *node, d->nodes)
    {
        if (node->package)
        {
            delete node->package;
        }
        
        if (node->error)
        {
            delete node->error;
        }
        
        if (node->childcount)
        {
            delete[] node->children;
        }
        
        delete node;
    }
    
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
    d->ps->setLastError(0); // Effacer l'erreur
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
    if (
        (node->flags & Solver::Node::Wanted) == 0 ||
        (node->flags & Solver::Node::MinMaxDone) == 0 || 
        (node->weightedBy == mainNode && 
        ((node->flags & Solver::Node::WeightMin) != 0) == weightMin
        ))
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
    
    if (node->error != 0) return;
    
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
        
        if (node->package)
        {
            switch (node->package->action())
            {
                case Solver::Install:
                    mainNode->minInstSize += node->package->installSize();
                    mainNode->minDlSize += node->package->downloadSize();
                    break;
                case Solver::Remove:
                case Solver::Purge:
                    mainNode->minInstSize -= node->package->installSize();
                    break;
            }
        }
    }
    else
    {
        mainNode->maxWeight += node->weight;
        
        if (node->package)
        {
            switch (node->package->action())
            {
                case Solver::Install:
                    mainNode->maxInstSize += node->package->installSize();
                    mainNode->maxDlSize += node->package->downloadSize();
                    break;
                case Solver::Remove:
                case Solver::Purge:
                    mainNode->maxInstSize -= node->package->installSize();
                    break;
            }
        }
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
    
    if (node->error != 0) return;
    
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
                
                // Si le noeud est en erreur, le sauter (s'ils sont tous en erreur, alors on
                // est en erreur, donc on n'arrive pas ici)
                if (nd->error != 0)
                {
                    continue;
                }
                
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
    node->flags |= Solver::Node::MinMaxDone;
    
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
    d->ps->setLastError(0); // Effacer l'erreur
    ended = true; // Mis à false si on a un choix
    
    d->errorNode = 0;
    d->choiceChild = 0;
    d->choiceNode = 0;
    
    return d->exploreNode(d->rootNode, ended);
}

bool Solver::continueList(int choice, bool &ended)
{
    d->ps->setLastError(0); // Effacer l'erreur
    
    // On doit avoir un enfant à choix
    if (d->choiceChild == 0)
    {
        return false;
    }
    
    // Vérifier que le choix est bien dans les limites
    if (choice < 0 || choice >= d->choiceChild->count)
    {
        return false;
    }
    
    // Définir son choix. Pour cela, explorer les noeuds de cet enfant et trouver le bon
    Solver::Error *error = 0;
    
    for (int i=0, index=0; i<d->choiceChild->count; ++i)
    {
        Solver::Node *nd = d->choiceChild->nodes[i];
        
        if (!d->verifyNode(nd, error))
        {
            if (error != 0)
            {
                // On n'a pas besoin de l'erreur
                delete error;
            }
            
            continue;
        }
        
        // Le noeud est bon, voir s'il correspond au choix
        if (index == choice)
        {
            // Choix fait sur i
            d->choiceChild->chosenNode = i;
            break;
        }
        
        // Compter le noeud
        index++;
    }
    
    // Continuer l'exploration
    return beginList(ended);
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
    
    Solver::Error *error = 0;
    
    for (int i=0; i<d->choiceChild->count; ++i)
    {
        node = nodes[i];
        
        if (!d->verifyNode(node, error))
        {
            if (error != 0)
            {
                // On n'a pas besoin de l'erreur
                delete error;
            }
            
            continue;
        }
        
        rs.append(node);
    }
    
    return rs;
}

bool Solver::upList()
{
    d->ps->setLastError(0); // Effacer l'erreur
    
    // Vérifier que la liste des niveaux contient au moins deux éléments,
    // puisqu'on va en supprimer un et utiliser l'autre. Sinon, ne rien faire
    if (d->levels.count() < 2)
    {
        return false; // Rien à faire
    }
    
    // Supprimer le dernier élément
    d->levels.removeLast();
    
    // Se servir du dernier (de maintenant) pour supprimer les noeuds qui
    // viennent après dans d->nodeList
    const Private::Level &level = d->levels.last();
    
    while (d->nodeList.count() >= level.lastNodeIndex)
    {
        Node *nd = d->nodeList.last();
        
        nd->nodeListIndex = -1;
        nd->currentChild = 0;
        nd->flags &= ~Node::Explored;
        
        d->nodeList.removeLast();
    }
    
    // Liste ok. Il faut maintenant re-peupler d->choiceChild, puisqu'on retourne à un choix
    Node *node = d->nodeList.at(level.choiceNodeIndex);
    d->choiceChild = &node->children[level.choiceChildIndex];
    d->choiceNode = node;
    
    // choiceChild a pour le moment un choix déjà fait, le lui retirer
    d->choiceChild->chosenNode = -1;
    
    return true; // Il y a du nouveau
}

PackageList *Solver::list()
{
    // Créer la PackageList pour les noeuds
    PackageList *rs = new PackageList(d->ps);
    rs->setDeletePackagesOnDelete(false);
    
    // Explorer les noeuds
    for (int i=0; i<d->nodeList.count(); ++i)
    {
        Node *node = d->nodeList.at(i);
        
        if (node->flags & Solver::Node::Wanted)
        {
            // Explorer à nouveau nodeList pour voir s'il contient un paquet
            // pouvant être une mise à jour de ce paquet.
            bool upgrade = false;
            
            if (node->package->origin() == Package::Database)
            {
                for (int j=0; j<d->nodeList.count(); ++j)
                {
                    Node *other = d->nodeList.at(j);
                    
                    if (other != node &&
                        other->package->origin() == Package::Database &&
                        other->package->name() == node->package->name() &&
                        other->package->action() != node->package->action()
                    )
                    {
                        // Deux paquet du même nom, mise à jour si les actions sont différentes
                        DatabasePackage *delpkg, *instpkg;
                        
                        delpkg  = (DatabasePackage *)(other->package->action() == Solver::Install ? node->package : other->package);
                        instpkg = (DatabasePackage *)(other->package->action() == Solver::Install ? other->package : node->package);
                        
                        // l'ancien est delpkg, le nouveau instpkg
                        delpkg->setUpgradePackage(instpkg);
                        delpkg->setAction(Solver::Update);
                        
                        upgrade = true;
                        
                        // L'autre est forcément plus loin dans la liste. Retirer son wanted pour
                        // ne pas mettre à jour deux fois
                        other->flags &= ~Node::Wanted;
                        
                        // Ajouter le paquet à la liste
                        rs->addPackage(delpkg);
                        
                        // Pas besoin de continuer l'exploration
                        break;
                    }
                }
            }
            
            if (!upgrade)
            {
                rs->addPackage(node->package);
            }
        }
    }
    
    return rs;
}

Solver::Node *Solver::errorNode()
{
    return d->errorNode;
}

Solver::Node *Solver::choiceNode()
{
    return d->choiceNode;
}

bool Solver::Private::verifyNode(Solver::Node *node, Solver::Error* &error)
{
    error = 0;
    
    // Vérifier que le noeud est correct
    if (node->error != 0)
    {
        error = node->error;
        
        if (error->type == Solver::Error::SameNameSameVersionDifferentAction ||
            error->type == Solver::Error::InstallSamePackageDifferentVersion)
        {
            // Erreur créée dans une précédante tentative (choix malheureux de l'utilisateur).
            // On n'en a plus besoin.
            // (note: et si on en a besoin, elle sera recrée plus tard dans cette fonction).
            delete error;
            error = 0;
            node->error = 0;
        }
        else
        {
            // Vraie erreur
            return false;
        }
    }
    
    // Si le noeud est déjà complètement exploré, on n'en a plus besoin
    if ((node->flags & Solver::Node::Explored) != 0)
    {
        return false;
    }
    else if (node->nodeListIndex != -1)
    {
        // Éviter les récursions en boucle : ce noeud est déjà dans un
        // exploreNode si son nodeListIndex != -1 mais que ses flags
        // ne contiennent pas Node::Explored
        return false;
    }
    
    // Explorer les noeuds déjà présents dans la liste
    if (node->package)
    {
        Solver::Action myAction = node->package->action();
        QString myName = node->package->name();
        QString myVersion = node->package->version();
        
        foreach (Solver::Node *nd, nodeList)
        {
            Solver::Action otherAction = nd->package->action();
            
            if (nd == node) continue;
            
            if (nd->package->name() == myName)
            {
                if (nd->package->version() == myVersion)
                {
                    if (otherAction == myAction)
                    {
                        return false;
                    }
                    else
                    {
                        error = new Solver::Error;
                        error->type = Solver::Error::SameNameSameVersionDifferentAction;
                        error->other = nd;
                        
                        return false;
                    }
                }
                else if (otherAction == myAction && myAction == Solver::Install)
                {
                    // Uniquement l'installation qui plante, on peut supprimer plusieurs
                    // versions d'un même paquet (avec les reallyWanted).
                    error = new Solver::Error;
                    error->type = Solver::Error::InstallSamePackageDifferentVersion;
                    error->other = nd;
                    
                    return false;
                }
            }
        }
    }
    
    return true;    // Bon noeud, on en a besoin
}

bool Solver::Private::exploreNode(Solver::Node* node, bool& ended)
{
    // Ajouter le noeud, s'il le faut, à nodeList
    if (node != rootNode && node->nodeListIndex == -1)
    {
        node->nodeListIndex = nodeList.count();
        nodeList.append(node);
    }
    
    // Si le noeud n'est pas voulu, alors on a fini (ils doivent quand-même se retrouver
    // dans nodeList
    if ((node->flags & Solver::Node::Wanted) == 0)
    {
        return true;
    }
    
    // Explorer les enfants
    Solver::Node::Child *children = node->children, *child;
    Solver::Error *error = 0;
    
    for (; node->currentChild < node->childcount; node->currentChild++)
    {
        child = &children[node->currentChild];
        
        if (child->count == 1)
        {
            // Si pour une raison ou pour une autre, on n'a pas besoin de l'enfant, le sauter
            if (!verifyNode(child->node, error))
            {
                if (error != 0)
                {
                    // VerifyNode a détecté une erreur dans le noeud
                    child->node->error = error;
                    errorNode = child->node;
                    return false;
                }
                
                // Passer au suivant
                continue;
            }
            
            // Explorer cet enfant, simplement
            if (!exploreNode(child->node, ended))
            {
                return false;
            }
            
            // Si un choix s'est présenté, retourner pour permettre à l'utilisateur de choisir.
            if (!ended) return true;
        }
        else
        {
            // On a un choix. Vérifier si l'utilisateur n'a pas déjà choisi
            int goodCount = 0, autoCount = 0;
            
            if (child->chosenNode != -1)
            {
                Solver::Node *nd = child->nodes[child->chosenNode];
                
                if (!exploreNode(nd, ended))
                {
                    return false;
                }
                
                // Si un choix s'est présenté, retourner pour permettre à l'utilisateur de choisir.
                if (!ended) return true;
            }
            else
            {
                // Explorer les noeuds de l'enfant.
                // Il est possible qu'il n'y en ai que 0 ou 1 de possible, ce qui serait facile
                Solver::Node *nd, *goodNode;
                
                for (int j=0; j<child->count; ++j)
                {
                    nd = child->nodes[j];
                    
                    // Voir si le noeud convient
                    if (!verifyNode(nd, error))
                    {
                        if (error != 0)
                        {
                            nd->error = error;
                        }
                        else
                        {
                            // Noeud déjà dans la liste, sans erreur.
                            // Bon candidat pour un choix automatique
                            autoCount++;
                        }
                    }
                    else
                    {
                        goodCount++;
                        goodNode = nd;
                    }
                }
            
                if (autoCount != 0)
                {
                    // Il y a moyen de ne pas installer de noeuds en plus
                    continue;
                }
                else if (goodCount == 0)
                {
                    // Tous les noeuds sont en erreur
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::ChildError;
                    err->other = 0;
                    
                    node->error = err;
                    errorNode = node;
                    
                    return false;
                }
                else if (goodCount == 1)
                {
                    // Un choix où on n'a qu'une possibilité n'est plus un choix
                    if (!exploreNode(goodNode, ended))
                    {
                        return false;
                    }
                    
                    // Si un choix s'est présenté, retourner pour permettre à l'utilisateur de choisir.
                    if (!ended) return true;
                }
                else
                {
                    // Pas de choix auto possible
                    choiceChild = child;
                    choiceNode = node;
                    ended = false;
                    
                    Level level;
                    
                    level.choiceNodeIndex = node->nodeListIndex;
                    level.choiceChildIndex = node->currentChild;
                    level.lastNodeIndex = nodeList.count() - 1;
                    
                    levels.append(level);
                    
                    return true;
                }
            }
        }
    }
    
    // On a exploré ce noeud en entier
    node->flags |= Solver::Node::Explored;
    
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
    node->currentChild = 0;
    node->nodeListIndex = -1;
    
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
        errorNode = node;
        
        return false;
    }
    
    // Vérifier que ce qu'on demande est bon
    if (package && ((package->flags() & PACKAGE_FLAG_DONTINSTALL) != 0) && action == Solver::Install)
    {
        Solver::Error *err = new Solver::Error;
        err->type = Solver::Error::UninstallablePackageInstalled;
        err->other = 0;
        
        node->error = err;
        errorNode = node;
        
        return false;
    }
    else if (package && ((package->flags() & PACKAGE_FLAG_DONTREMOVE) != 0) && (action == Solver::Remove || action == Solver::Purge))
    {
        Solver::Error *err = new Solver::Error;
        err->type = Solver::Error::UnremovablePackageRemoved;
        err->other = 0;
        
        node->error = err;
        errorNode = node;
        
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
    if (action == Solver::Install && package && package->origin() == Package::Database && useDeps)
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
                suppl = checkPackage(otherVersion, Solver::Remove, ok, false);
                
                // Tout doit s'être bien passé
                if (!ok)
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::ChildError;
                    err->other = suppl;
                    
                    node->error = err;
                    errorNode = node;
                    
                    return false;
                }
                
                // Vérifier que nd est updatable
                if (suppl->package && ((suppl->package->flags() & PACKAGE_FLAG_DONTUPDATE) != 0))
                {
                    Solver::Error *err = new Solver::Error;
                    err->type = Solver::Error::UnupdatablePackageUpdated;
                    err->other = suppl;
                    
                    node->error = err;
                    errorNode = node;
                    
                    return false;
                }
                
                // Paquet en plus à gérer
                node->childcount++;
                
                // Une seule autre version
                break;
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
                fpkg->setWanted(true); // Demandé par l'utilisateur
                
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
                    errorNode = node;
                    
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
                    err->other = 0;
                    
                    node->error = err;
                    errorNode = node;
                    
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
            
            if (i == (node->childcount - 1) && suppl)
            {
                // Il est temps d'ajouter suppl
                children[i].count = 1;
                children[i].maxNode = -1;
                children[i].minNode = -1;
                children[i].node = suppl;
                
                // Boucle de toute façon finie
                break;
            }
            
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
                errorNode = node;
                
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
        Node *nd = checkPackage(pkgIndexes.at(0), (revdep ? Solver::Remove : action), ok, (node->package == 0));
        
        // Enregistrer le noeud
        child->count = 1;
        child->node = nd;
        
        if (!ok)
        {
            Solver::Error *err = new Solver::Error;
            err->type = Solver::Error::ChildError;
            err->other = nd;
            
            node->error = err;
            errorNode = node;
            
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
        child->chosenNode = -1;
        
        for (int j=0; j<pkgIndexes.count(); ++j)
        {
            int pkgIndex = pkgIndexes.at(j);
            
            bool ok = true;
            Node *nd = checkPackage(pkgIndex, (revdep && j == 0 ? Solver::Remove : action), ok, (node->package == 0));
            
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
            errorNode = node;
            
            return false;
        }
    }
    
    return true;
}

Solver::Node *Solver::Private::checkPackage(int index, Solver::Action action, bool &ok, bool userWanted)
{
    // Explorer tous les noeuds à la recherche d'un éventuel qui correspondrait à l'index et à l'action
    for (int i=0; i<nodes.count(); ++i)
    {
        Solver::Node *node = nodes.at(i);
        
        if (node->package && node->package->action() == action && node->package->origin() == Package::Database && ((DatabasePackage *)node->package)->index() == index)
        {
            // Le noeud existe déjà, le retourner
            ok = (node->error == 0);
            if (userWanted) node->package->setWanted(true);
            
            return node;
        }
    }
    
    // Pas de noeud correspondant trouvé
    DatabasePackage *package = new DatabasePackage(index, ps, psd, action);
    package->setWanted(userWanted);     // Savoir si c'est un paquet explicitement demandé par l'utilisateur
    
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
