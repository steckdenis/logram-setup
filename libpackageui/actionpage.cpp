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
#include "utils.h"

#include <package.h>
#include <packagesystem.h>

#include <QVector>

using namespace Logram;
using namespace LogramUi;

ActionPage::ActionPage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    wizard = _wizard;

    setupUi(this);
}

void ActionPage::initializePage()
{
    QVector<Package *> packages = wizard->packages();
    
    treeActions->clear();
    
    for (int i=0; i<packages.count(); ++i)
    {
        Package *pkg = packages.at(i);
        
        QTreeWidgetItem *item = new QTreeWidgetItem(treeActions);
        
        // Icône
        if (pkg->action() == Solver::Install)
        {
            item->setIcon(0, QIcon(":/images/pkg-install.png"));
        }
        else if (pkg->action() == Solver::Remove || pkg->action() == Solver::Purge)
        {
            item->setIcon(0, QIcon(":/images/pkg-remove.png"));
        }
        
        // Texte
        item->setText(0, pkg->name());
        item->setText(1, pkg->version());
        item->setText(2, Utils::actionNameInf(pkg->action()));
        item->setText(3, PackageSystem::fileSizeFormat(pkg->downloadSize()));
        item->setText(4, PackageSystem::fileSizeFormat(pkg->installSize()));
        item->setText(5, pkg->shortDesc());
        
        treeActions->addTopLevelItem(item);
    }
}
