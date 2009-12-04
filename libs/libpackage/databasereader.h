/*
 * databasereader.h
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

#ifndef __PACKAGESYSTEM_P_H__
#define __PACKAGESYSTEM_P_H__



/**********************************************
******* Classe ********************************
**********************************************/

#include <QString>
#include <QList>
#include <QEventLoop>
#include <QHash>

#include "databaseformat.h"

class QFile;
class QNetworkAccessManager;
class QSettings;

namespace Logram
{
    
class PackageSystem;

struct UpgradeInfo
{
    int installedPackage;
    int newPackage;
};

class DatabaseReader
{
    public:
        DatabaseReader(PackageSystem *_ps);
        ~DatabaseReader();
        bool init();

        bool package(const QString &name, const QString &version, int &rs);
        bool packagesByName(const QString &regex, QList<int> &rs);
        QList<int> packagesByVString(const QString &verStr);
        QList<_Depend *> depends(int pkgIndex);
        QList<int> packagesOfString(int stringIndex, int nameIndex, int op);
        QList<UpgradeInfo> upgradePackages();

        const char *string(bool translate, int index);
        _Package *package(int index);
        _Depend *depend(int32_t ptr);
        
    private:
        bool mapFile(const QString &file, QFile **ptr, uchar **map);

    private:
        QFile *f_packages, *f_strings, *f_translate, *f_depends, *f_strpackages;
        uchar *m_packages, *m_strings, *m_translate, *m_depends, *m_strpackages;

        PackageSystem *ps;
};

} /* Namespace */

#endif