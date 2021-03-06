/*
 * branchepage.h
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

#ifndef __BRANCHEPAGE_H__
#define __BRANCHEPAGE_H__

#include <QWizardPage>
#include "ui_branchepage.h"

namespace LogramUi
{
    class InstallWizard;
}

namespace Logram
{
    class PackageSystem;
    class Solver;
    class PackageList;
}

class BranchePage : public QWizardPage, public Ui_branchePage
{
    Q_OBJECT
    
    public:
        BranchePage(LogramUi::InstallWizard *_wizard);
        
        void initializePage();
        bool isComplete();
        int nextId() const;
        
        void updateChoiceList();
        
    private slots:
        void choiceUp();
        void choiceSelect();

    private:
        LogramUi::InstallWizard *wizard;
        
        Logram::PackageSystem *ps;
        Logram::Solver *solver;
        Logram::PackageList *packageList;
        
        bool nextOk;
        int level;
};

#endif
