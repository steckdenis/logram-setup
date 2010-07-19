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
#include <QVector>

namespace Logram
{
    class Solver;
    class PackageList;
    class PackageSystem;
    class Progress;
    class Package;
}

class ActionPage;
class BranchePage;
class LicensePage;
class InstallPage;
class DonePage;

namespace LogramUi
{

struct PackageMessage
{
    Logram::Package *pkg;
    QString title;
    QString message;
};

class InstallWizard : public QWizard
{
    Q_OBJECT
    
    friend class ::ActionPage;
    friend class ::BranchePage;
    friend class ::LicensePage;
    friend class ::InstallPage;
    friend class ::DonePage;
    
    public:
        InstallWizard(Logram::PackageSystem *ps, QWidget *parent);
        ~InstallWizard();
        
        void addPackage(Logram::Package *package);
        QVector<Logram::Package *> packages() const;
        
        enum PageIds
        {
            Actions,
            Branches,
            Licenses,
            Install,
            Done
        };
        
    private:
        void solverError();
        
        Logram::Solver *solver() const;
        Logram::PackageSystem *packageSystem() const;
        Logram::PackageList *packageList() const;
        
        void setSolver(Logram::Solver *solver);
        void setPackageList(Logram::PackageList *packageList);
        
        void addMessage(const PackageMessage &message);
        QList<PackageMessage> messages() const;
        
    private slots:
        void pageChanged(int id);
        
    private:
        struct Private;
        Private *d;
};

}

#endif
