/*
 * installwizard.h
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

#ifndef __INSTALLWIZARD_H__
#define __INSTALLWIZARD_H__

#include <QWizard>

enum PageIds
{
    ActionList,
    Branches,
    Licenses,
    Install,
    Done
};

#include "ui_installwizard.h"
#include <package.h>

class MainWindow;
class ProgressList;

namespace Logram
{
    class Solver;
    class PackageList;
    class PackageSystem;
    class Progress;
}

struct PackageMessage
{
    Logram::Package *pkg;
    QString title;
    QString message;
};

class InstallWizard : public QWizard
{
    Q_OBJECT
    
    public:
        InstallWizard(MainWindow *parent);
        
        void solverError();
        
        MainWindow *mainWindow() const;
        Logram::Solver *solver() const;
        Logram::PackageList *packageList() const;
        Logram::PackageSystem *packageSystem() const;
        
        void setSolver(Logram::Solver *_solver);
        void setPackageList(Logram::PackageList *_packageList);
        
        void addMessage(const PackageMessage &message);
        QList<PackageMessage> packageMessages() const;
        
        enum PageIds
        {
            Actions,
            Branches,
            Licenses,
            Install,
            Done
        };
        
    private slots:
        void pageChanged(int id);
        
    private:
        MainWindow *win;
        
        Logram::Solver *_solver;
        Logram::PackageList *_packageList;
        Logram::PackageSystem *ps;
        
        QList<PackageMessage> messages;
};

#endif