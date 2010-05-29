/*
 * packagelist.h
 * This file is part of Logram
 *
 * Copyright (C) 2009 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#ifndef __PACKAGELIST_H__
#define __PACKAGELIST_H__

#include <QObject>
#include <QList>
#include <QProcess>

#include "solver.h"

namespace Logram
{

class PackageSystem;
class Package;
class Communication;

class PackageList : public QObject, public QList<Package *>
{
    Q_OBJECT
    
    public:
        PackageList(PackageSystem *ps);
        ~PackageList();
        
        void addPackage(Package *pkg);
        void setDeletePackagesOnDelete(bool enable);
        
        Package *installingPackage() const;
        bool needsReboot() const;
        int numLicenses() const;
        QList<int> orphans() const;
        
        bool process();
        
    private slots:
        void packageProceeded(bool success);
        void packageDownloaded(bool success);
        void communication(Logram::Package *pkg, Logram::Communication *comm);
        
        void triggerFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void triggerOut();
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif