/*
 * solver.h
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

#ifndef __SOLVER_H__
#define __SOLVER_H__

#include <QObject>

class Package;
class PackageSystem;
class PackageSystemPrivate;

class Solver : public QObject
{
    Q_OBJECT
    
    public:
        Solver(PackageSystem *ps, PackageSystemPrivate *psd);
        ~Solver();

        enum Action
        {
            None,
            Install,
            Remove,
            Purge,
            Update
        };

        void addPackage(const QString &nameStr, Action action);
        void solve();
        void process(int index);

        int results() const;
        QList<Package *> result(int index, int &weight) const;
        Package *installingPackage() const;

    private slots:
        void packageInstalled();
        void packageDownloaded();

    private:
        struct Private;
        Private *d;
};

#endif