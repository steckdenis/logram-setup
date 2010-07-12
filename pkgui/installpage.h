/*
 * installpage.h
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

#ifndef __INSTALLPAGE_H__
#define __INSTALLPAGE_H__

#include <QWizardPage>

#include "ui_installpage.h"
#include "progresslist.h"

class InstallWizard;
class MainWindow;

namespace Logram
{
    class PackageSystem;
    class PackageList;
    class Package;
    class Communication;
}

class InstallPage : public QWizardPage, public Ui_installPage
{
    Q_OBJECT
    
    public:
        InstallPage(InstallWizard *_wizard);
        
        void initializePage();
        
    private slots:
        void communication(Logram::Package *sender, Logram::Communication *comm);
        void progress(Logram::Progress *progress);
        void wizardRejected();

    private:
        InstallWizard *wizard;
        ProgressList *progressList;
        MainWindow *win;
        
        Logram::PackageSystem *ps;
        Logram::PackageList *packageList;
        bool _cancel;
};

#endif
