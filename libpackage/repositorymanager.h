/*
 * repositorymanager.h
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

#ifndef __REPOSITORYMANAGER_H__
#define __REPOSITORYMANAGER_H__

#include <QObject>

namespace Logram
{
    
class PackageSystem;

class RepositoryManager : public QObject
{
    public:
        RepositoryManager(PackageSystem *ps);
        ~RepositoryManager();
        
        bool loadConfig(const QString &fileName);
        
        bool includePackage(const QString &fileName);
        bool includeSource(const QString &fileName, bool appendHistory = true);
        bool exp(const QStringList &distros);
        
        struct Private; // Nécessaire pour passphrase_cb
        
    private:
        Private *d;
};

} /* Namespace */

#define SOURCEPACKAGE_FLAG_LATEST               0b00000001
#define SOURCEPACKAGE_FLAG_MANUAL               0b00000010
#define SOURCEPACKAGE_FLAG_FAILED               0b00000100
#define SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG   0b00001000
#define SOURCEPACKAGE_FLAG_REBUILD              0b00010000
#define SOURCEPACKAGE_FLAG_CONTINUOUS           0b00100000
#define SOURCEPACKAGE_FLAG_WARNINGS             0b01000000
#define SOURCEPACKAGE_FLAG_BUILDING             0b10000000

#endif