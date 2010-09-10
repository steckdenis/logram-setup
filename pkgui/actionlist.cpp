/*
 * packageinfo.cpp
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

#include "mainwindow.h"
#include <installwizard.h>

#include <QtDebug>

using namespace Logram;
using namespace LogramUi;

static DatabasePackage *itemPackage(QTreeWidget *treePackages, PackageItem **item)
{
    if (treePackages->selectedItems().count() == 0)
        return 0;
    
    *item = static_cast<PackageItem *>(treePackages->selectedItems().at(0));
    
    if (*item == 0) return 0;
    
    return (*item)->package();
}

void MainWindow::addPackageInList(DatabasePackage *pkg, QTreeWidget *treeActions)
{
    QTreeWidgetItem *root = treeActions->invisibleRootItem();
    
    // Vérifier qu'il n'y a pas déjà un enfant avec le bon paquet
    for (int i=0; i<root->childCount(); ++i)
    {
        PackageItem *candidate = static_cast<PackageItem *>(root->child(i));
        
        if (!candidate) continue;
        
        if (candidate->package()->index() == pkg->index())
        {
            // Déjà ok
            delete candidate->package();
            
            candidate->setPackage(pkg);
            candidate->updateIcon();
            return;
        }
        else if (candidate->package()->name() == pkg->name() && candidate->package()->action() == pkg->action())
        {
            // Même nom, s'assurer qu'il ait la même version
            delete candidate->package();
            
            candidate->setPackage(pkg);
            candidate->updateIcon();
            candidate->updateText(false);
            return;
        }
    }
    
    // Il faut ajouter un élément
    PackageItem *item = new PackageItem(pkg, treeActions, PackageItem::SmallActionList, false);
    root->addChild(item);
    
    // S'assurer que l'action et le bouton sont activés
    actApplyChanges->setEnabled(true);
    actCancelChanges->setEnabled(true);
    btnListApply->setEnabled(true);
    btnListClean->setEnabled(true);
}

void MainWindow::removePackageFromList(DatabasePackage *pkg, QTreeWidget *treeActions)
{
    QTreeWidgetItem *root = treeActions->invisibleRootItem();
    
    for (int i=0; i<root->childCount(); ++i)
    {
        PackageItem *candidate = static_cast<PackageItem *>(root->child(i));
        
        if (!candidate) continue;
        
        if (candidate->package()->index() == pkg->index())
        {
            // Déjà ok
            root->removeChild(candidate);
            break;
        }
    }
    
    if (root->childCount() == 0)
    {
        btnListApply->setEnabled(false);
        btnListClean->setEnabled(false);
        actApplyChanges->setEnabled(false);
        actCancelChanges->setEnabled(false);
    }
}

DatabasePackage *MainWindow::duplicatePackage(PackageSystem *ps, DatabasePackage *pkg)
{
    DatabasePackage *rs = ps->package(pkg->index());
    rs->setAction(pkg->action());
    
    return rs;
}

void MainWindow::actionsForPackage(DatabasePackage *pkg)
{
    actDeselect->setEnabled(pkg->action() != Solver::None);
    actInstallPackage->setEnabled((pkg->flags() & PACKAGE_FLAG_INSTALLED) == 0 && pkg->action() != Solver::Install);
    actRemovePackage->setEnabled(pkg->flags() & PACKAGE_FLAG_INSTALLED && pkg->action() != Solver::Remove);
    actPurge->setEnabled(pkg->flags() & PACKAGE_FLAG_INSTALLED && pkg->action() != Solver::Purge);
}

void MainWindow::installPackage()
{
    PackageItem *item;
    DatabasePackage *pkg = itemPackage(treePackages, &item);
    
    if (pkg == 0) return;
    
    pkg->setAction(Solver::Install);
    item->updateIcon();
    actionsForPackage(pkg);
    
    pkg = duplicatePackage(ps, pkg);
    
    addPackageInList(pkg, treeActions);
}

void MainWindow::removePackage()
{
    PackageItem *item;
    DatabasePackage *pkg = itemPackage(treePackages, &item);
    
    if (pkg == 0) return;
    
    pkg->setAction(Solver::Remove);
    item->updateIcon();
    actionsForPackage(pkg);
    
    pkg = duplicatePackage(ps, pkg);
    
    addPackageInList(pkg, treeActions);
}

void MainWindow::purgePackage()
{
    PackageItem *item;
    DatabasePackage *pkg = itemPackage(treePackages, &item);
    
    if (pkg == 0) return;
    
    pkg->setAction(Solver::Purge);
    item->updateIcon();
    actionsForPackage(pkg);
    
    pkg = duplicatePackage(ps, pkg);
    
    addPackageInList(pkg, treeActions);
}

void MainWindow::deselectPackage()
{
    PackageItem *item;
    DatabasePackage *pkg = itemPackage(treePackages, &item);
    
    if (pkg == 0) return;
    
    pkg->setAction(Solver::None);
    item->updateIcon();
    actionsForPackage(pkg);
    
    removePackageFromList(pkg, treeActions);
}

void MainWindow::applyList()
{
    InstallWizard wizard(ps, this);
    
    // Ajouter les paquets
    QTreeWidgetItem *item = treeActions->invisibleRootItem();
    
    for (int i=0; i<item->childCount(); ++i)
    {
        PackageItem *it = static_cast<PackageItem *>(item->child(i));
        
        if (!it) continue;
        
        wizard.addPackage(it->package());
    }
    
    disableProgressions();
    wizard.exec();
    enableProgressions();
    
    // Liste appliquée, fini
    if (wizard.result() == QDialog::Accepted)
    {
        cancelList();
    }
}

void MainWindow::cancelList()
{
    treeActions->clear();
    searchPackages();           // Besoin de mettre à jour les icônes
    
    btnListApply->setEnabled(false);
    btnListClean->setEnabled(false);
    actApplyChanges->setEnabled(false);
    actCancelChanges->setEnabled(false);
}