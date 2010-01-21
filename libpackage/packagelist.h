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
        bool error() const;
        
        struct Error
        {
            enum Type
            {
                SameNameSameVersionDifferentAction,
                SameNameDifferentVersionSameAction,
                NoPackagesMatchingPattern
            };
            
            Type type;
            
            QString package, otherPackage;
            QString version, otherVersion;
            Solver::Action action, otherAction;
            
            QString pattern;
        };
        
        void addPackage(Package *pkg);
        void addError(Error *err);
        void setWrong(bool wrong);
        
        Package *installingPackage() const;
        int errors() const;
        Error *error(int i) const;
        bool wrong() const;
        int weight() const;
        bool needsReboot() const;
        
        bool process();
        
    private slots:
        void packageProceeded(bool success);
        void packageDownloaded(bool success);
        
    signals:
        void communication(Logram::Package *sender, Logram::Communication *comm);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif