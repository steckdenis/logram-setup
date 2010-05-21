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
    Node *rootNode;

    // Fonctions
    bool addNode(Package *package, Solver::Node *node);
    Node *checkPackage(int index, Solver::Action action, bool &ok);
    bool addPkgs(const QList<int> &pkgIndexes, Solver::Node *node, Solver::Action action, Solver::Node::Child *child, bool revdep = false);
    
};

Solver::Solver(PackageSystem *ps, DatabaseReader *psd)
{
    d = new Private;
    
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
