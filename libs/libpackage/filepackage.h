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

#ifndef __FILEPACKAGE_H__
#define __FILEPACKAGE_H__

#include "solver.h"
#include "package.h"

namespace Logram
{

class PackageSystem;
class DatabaseReader;
    
class FileDepend;

class FilePackage : public Package
{
    Q_OBJECT
    
    public:
        FilePackage(const QString &fileName, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        FilePackage(const FilePackage &other);
        ~FilePackage();

        bool download();
        QString tlzFileName();
        bool isValid();
        Package::Origin origin();
        
        QString name();
        QString version();
        QString maintainer();
        QString shortDesc();
        QString source();
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString arch();
        QByteArray packageHash();
        QByteArray metadataHash();
        int flags();
        
        QDateTime installedDate();
        int installedBy();
        int status();
        
        int downloadSize();
        int installSize();

        QList<Depend *> depends();
        
        void registerState(int idate, int iby, int state);
        
        QByteArray metadataContents();
        QStringList files();

    signals:
        void downloaded(bool success);

    private:
        struct Private;
        Private *d;
};

class FileDepend : public Depend
{
    public:
        FileDepend(int8_t type, int8_t op, const QString &name, const QString &version);
        ~FileDepend();

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