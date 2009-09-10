/*
 * packagesystem.cpp
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

#include "libpackage.h"
#include "libpackage_p.h"
#include "package.h"

PackageSystem::PackageSystem(QObject *parent) : QObject(parent)
{
    // Rien à faire
}

void PackageSystem::init()
{
    d = new PackageSystemPrivate(this);
}

QList<Package *> PackageSystem::packagesByName(const QString &regex)
{
    QList<Package *> rs;
    Package *pkg;
    
    QList<int> pkgs = d->packagesByName(regex);

    foreach(int i, pkgs)
    {
        pkg = new Package(i, this, d);

        rs.append(pkg);
    }

    return rs;
}

Package *PackageSystem::package(const QString &name, const QString &version)
{
    Package *pkg;
    int i = d->package(name, version);

    pkg = new Package(i, this, d);

    return pkg;
}

void PackageSystem::raise(Error err, const QString &info)
{
    emit error(err, info);
}

void PackageSystem::sendMessage(const QString &msg)
{
    emit message(msg);
}