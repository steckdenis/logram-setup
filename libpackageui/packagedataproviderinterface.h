/*
 * packagedataproviderinterface.h
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

#ifndef __PACKAGEDATAPROVIDERINTERFACE_H__
#define __PACKAGEDATAPROVIDERINTERFACE_H__

#include <QString>
#include <QByteArray>
#include <QVector>

namespace Logram
{
    class Depend;
    class PackageFile;
    struct ChangeLogEntry;
}

namespace LogramUi
{

class PackageDataProviderInterface
{
    public:
        PackageDataProviderInterface() {}
        virtual ~PackageDataProviderInterface() {};
        
        virtual QString name() const = 0;
        virtual QString version() const = 0;
        
        virtual int flags() const = 0;
        virtual void setFlags(int flags) = 0;
        
        virtual QString website() const = 0;
        virtual QString title() const = 0;
        virtual QString shortDesc() const = 0;
        virtual QString longDesc() const = 0;
        virtual QString license() const = 0;
        
        virtual QByteArray iconData() const = 0;
        
        virtual QString repository() const = 0;
        virtual QString distribution() const = 0;
        virtual QString section() const = 0;
        
        virtual int downloadSize() const = 0;
        virtual int installSize() const = 0;
        
        virtual QVector<PackageDataProviderInterface *> versions() const = 0;
        virtual QVector<Logram::Depend *> depends() const = 0;
        virtual QVector<Logram::ChangeLogEntry *> changelog() const = 0;
        virtual QVector<Logram::PackageFile *> files() const = 0;
};

}

#endif