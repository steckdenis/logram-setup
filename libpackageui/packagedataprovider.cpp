/*
 * packagedataprovider.cpp
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

#include "packagedataprovider.h"
#include "utils.h"

#include <databasepackage.h>
#include <packagemetadata.h>

using namespace Logram;
using namespace LogramUi;

PackageDataProvider::PackageDataProvider(DatabasePackage* package, PackageSystem* _ps)
{
    pkg = package;
    md = pkg->metadata();
    ps = _ps;
    
    if (md == 0)
    {
        Utils::packageSystemError(ps);
    }
    else
    {
        md->setCurrentPackage(pkg->name());
    }
}

PackageDataProvider::~PackageDataProvider()
{

}

QString PackageDataProvider::name() const
{
    return pkg->name();
}

QString PackageDataProvider::version() const
{
    return pkg->version();
}

int PackageDataProvider::flags() const
{
    return pkg->flags();
}

void PackageDataProvider::setFlags(int flags)
{
    pkg->setFlags(flags);
}

QString PackageDataProvider::website() const
{
    if (md)
        return md->upstreamUrl();
    else
        return QString();
}

QString PackageDataProvider::title() const
{
    if (md)
        return md->packageTitle();
    else
        return pkg->name();
}

QString PackageDataProvider::shortDesc() const
{
    return pkg->shortDesc();
}

QString PackageDataProvider::longDesc() const
{
    if (md)
        return md->packageDescription();
    else
        return QString();
}

QString PackageDataProvider::license() const
{
    return pkg->license();
}

QByteArray PackageDataProvider::iconData() const
{
    if (md)
        return md->packageIconData();
    else
        return QByteArray();
}

QString PackageDataProvider::repository() const
{
    return pkg->repo();
}

QString PackageDataProvider::distribution() const
{
    return pkg->distribution();
}

QString PackageDataProvider::section() const
{
    return pkg->section();
}

int PackageDataProvider::downloadSize() const
{
    return pkg->downloadSize();
}

int PackageDataProvider::installSize() const
{
    return pkg->installSize();
}

QVector<ChangeLogEntry *> PackageDataProvider::changelog() const
{
    if (md)
        return md->changelog();
    else
        return QVector<ChangeLogEntry *>();
}

QVector<Depend *> PackageDataProvider::depends() const
{
    return pkg->depends();
}

QVector<PackageFile *> PackageDataProvider::files() const
{
    return pkg->files();
}

QVector<PackageDataProviderInterface *> PackageDataProvider::versions() const
{
    QVector<PackageDataProviderInterface *> rs;
    
    if (pkg->origin() == Package::Database)
    {
        DatabasePackage *dpkg = static_cast<DatabasePackage *>(pkg);
        
        QVector<DatabasePackage *> v = dpkg->versions();
        
        for (int i=0; i<v.count(); ++i)
        {
            PackageDataProvider *provider = new PackageDataProvider(v.at(i), ps);
            
            rs.append(provider);
        }
    }
    else
    {
        PackageDataProvider *provider = new PackageDataProvider(pkg, ps);
        
        rs.append(provider);
    }
    
    return rs;
}
