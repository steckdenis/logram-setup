/*
 * packagelist.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include <QRegExp>
#include <QVector>
#include <QIcon>
#include <QtDebug>

#include "mainwindow.h"

#include <packagesystem.h>
#include <databasereader.h>
#include <databasepackage.h>

using namespace Logram;

void MainWindow::addPackage(DatabasePackage *pkg)
{
    // Explorer les paquets dans la liste des changements pour voir s'il n'y est pas déjà
    QTreeWidgetItem *root = treeActions->invisibleRootItem();
    
    for (int i=0; i<root->childCount(); ++i)
    {
        PackageItem *candidate = static_cast<PackageItem *>(root->child(i));
        
        if (!candidate) continue;
        
        if (candidate->package()->index() == pkg->index())
        {
            pkg->setAction(candidate->package()->action());
            break;
        }
    }
    
    // Créer l'élément
    PackageItem *item = new PackageItem(pkg, treePackages, PackageItem::PackageList);
    treePackages->addTopLevelItem(item);
    
    if (treePackages->currentItem() == 0)
    {
        treePackages->setCurrentItem(item);
        itemActivated(item);
    }
}

void MainWindow::displayPackages(PackageFilter filter, const QString &pattern)
{
    QVector<int> ids, newids;
    DatabaseReader *dr = ps->databaseReader();
    
    // Vider la vue
    treePackages->clear();
    
    // Si on ne prend que les paquets qu'on peut mettre à jour, c'est facile
    if (filter == Updateable || filter == Orphan)
    {
        QVector<DatabasePackage *> pkgs;
        
        if (filter == Updateable)
        {
            pkgs = ps->upgradePackages();
        }
        else
        {
            pkgs = ps->orphans();
        }
        
        // Filtrer en fonction de pattern
        if (pattern.isEmpty())
        {
            for (int i=0; i<pkgs.count(); ++i)
            {
                addPackage(pkgs.at(i));
            }
        }
        else
        {
            QRegExp regex(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
            
            for (int i=0; i<pkgs.count(); ++i)
            {
                DatabasePackage *pkg = pkgs.at(i);
                
                if (regex.exactMatch(pkg->name()))
                {
                    addPackage(pkg);
                }
                else
                {
                    delete pkg;
                }
            }
        }
        
        // Fini
        return;
    }
    
    // Trouver les IDs en fonction de ce qu'on demande
    if (pattern.isEmpty())
    {
        // Tous les paquets
        ids.reserve(ps->packages());
        
        for (int i=0; i<ps->packages(); ++i)
        {
            ids.append(i);
        }
    }
    else
    {
        if (!ps->packagesByName(QRegExp(pattern, Qt::CaseSensitive, QRegExp::Wildcard), ids))
        {
            return;
        }
    }
    
    // Filtrer les paquets
    if (filter != NoFilter && filter != Updateable)
    {
        for (int i=0; i<ids.count(); ++i)
        {
            int index = ids.at(i);
            _Package *pkg = dr->package(index);
            
            if (filter == Installed && (pkg->flags & PACKAGE_FLAG_INSTALLED) != 0)
            {
                newids.append(index);
            }
            else if (filter == NotInstalled && (pkg->flags & PACKAGE_FLAG_INSTALLED) == 0)
            {
                newids.append(index);
            }
        }
        
        ids = newids;
    }
    
    // Créer l'arbre des paquets
    QHash<int, bool> subVersions;       // Permet de n'avoir qu'un paquet par nom
    
    for (int i=0; i<ids.count(); ++i)
    {
        int index = ids.at(i);
        
        // Si on a déjà ce paquet quelque-part dans la liste, le laisser
        if (subVersions.contains(index))
        {
            continue;
        }
        
        // Ajouter ce paquet aux listes
        DatabasePackage *dpkg = ps->package(index);
        addPackage(dpkg);
        
        // Trouver les paquets ayant le même nom, donc des versions différentes
        _Package *pkg = dr->package(index);
        QVector<int> otherVersions = dr->packagesOfString(0, pkg->name, DEPEND_OP_NOVERSION);
        
        for (int j=0; j<otherVersions.count(); ++j)
        {
            _Package *opkg = dr->package(otherVersions.at(j));
            
            // Vérifier qu'il a le même nom, donc pas un provide
            if (opkg && pkg->version != opkg->version && pkg->name == opkg->name)
            {
                subVersions.insert(otherVersions.at(j), true);
            }
        }
    }
}

void MainWindow::searchPackages()
{
    QString pattern = txtSearch->text();
    
    if (!pattern.contains('*'))
    {
        pattern = "*" + pattern + "*";
    }
    
    displayPackages((PackageFilter)cboFilter->currentIndex(), pattern);
}