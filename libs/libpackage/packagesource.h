/*
 * packagesource.h
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

#ifndef __PACKAGESOURCE_H__
#define __PACKAGESOURCE_H__

#include <QVariant>
#include <QProcess>
#include <QStringList>

#include "templatable.h"

namespace Logram
{

class PackageSystem;
class FilePackage;
class PackageMetaData;

class PackageSource : public Templatable
{
    Q_OBJECT
    
    public:
        PackageSource(PackageSystem *ps);
        ~PackageSource();
        
        enum Option
        {
            SourceDir,
            BuildDir,
            ControlDir
        };
        
        bool setMetaData(const QString &fileName);
        bool setMetaData(const QByteArray &data);
        void setOption(Option opt, const QVariant &value);
        QVariant option(Option opt, const QVariant &defaultValue);
        
        void loadKeys();     // Une fois toutes les options définies
        
        bool getSource();
        bool checkSource(const QString &dir, bool fail);
        bool build();
        bool binaries();
        
    private:
        struct Private;
        Private *d;
};
    
} /* Namespace */

#endif