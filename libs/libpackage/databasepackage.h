/*
 * databasepackage.h
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

#ifndef __DATABASEPACKAGE_H__
#define __DATABASEPACKAGE_H__

#include <QObject>
#include <QProcess>
#include <QDateTime>

#include "solver.h"
#include "package.h"

#include <stdint.h>

namespace Logram
{

class DatabaseReader;
struct ManagedDownload;

class _Depend;
class DatabaseDepend;

class DatabasePackage : public Package
{
    Q_OBJECT
    
    public:
        DatabasePackage(int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        ~DatabasePackage();

        bool download();
        QString tlzFileName();
        bool isValid();
        Package::Origin origin();

        enum UrlType
        {
            Binary,
            Metadata
        };
        
        QString name();
        QString version();
        QString newerVersion();
        QString maintainer();
        QString shortDesc();
        QString source();
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString arch();
        QString url(UrlType type = DatabasePackage::Binary);
        QString packageHash();
        QString metadataHash();
        bool isGui();
        
        QDateTime installedDate();
        int installedBy();
        int status();
        
        int downloadSize();
        int installSize();

        QList<Package *> versions();
        QList<Depend *> depends();
        
        void registerState(int idate, int iby, int state);

    signals:
        void downloaded(bool success);

    private slots:
        void downloadEnded(Logram::ManagedDownload *md);

    private:
        struct Private;
        Private *d;
};

class DatabaseDepend : public Depend
{
    public:
        DatabaseDepend(_Depend *dep, DatabaseReader *psd);
        ~DatabaseDepend();

        QString name();
        QString version();
        int8_t type();
        int8_t op();

    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif