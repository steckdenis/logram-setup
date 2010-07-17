/*
 * actionpage.cpp
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

#include "actionpage.h"
#include "installwizard.h"
#include "mainwindow.h"

#include <databasepackage.h>

using namespace Logram;

ActionPage::ActionPage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    wizard = _wizard;

    setupUi(this);
}

void ActionPage::initializePage()
{
    MainWindow *win = wizard->mainWindow();
    QTreeWidgetItem *root = win->treeActions->invisibleRootItem();
    
    treeActions->clear();
    
    for (int i=0; i<root->childCount(); ++i)
    {
        PackageItem *item = static_cast<PackageItem *>(root->child(i));
        
        if (!item) continue;
        
        DatabasePackage *pkg = win->duplicatePackage(win->packageSystem(), item->package());
        PackageItem *newItem = new PackageItem(pkg, treeActions, PackageItem::LargeActionList, false);
        
        treeActions->addTopLevelItem(newItem);
    }
}
