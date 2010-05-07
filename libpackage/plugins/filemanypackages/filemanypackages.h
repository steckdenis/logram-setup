/*
 * filemanypackages.h
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

#ifndef __FILEMANYPACKAGES_H__
#define __FILEMANYPACKAGES_H__

#include <QObject>
#include <QList>

#include <QtPlugin>

#include <packagesource.h>

using namespace Logram;

class FileManyPackages : public QObject, public PackageSourceInterface
{
    Q_OBJECT
    Q_INTERFACES(Logram::PackageSourceInterface)
    
    public:
        FileManyPackages();
        
        QString name() const;
        bool byDefault() const;
        
        void init(PackageMetaData *md, PackageSource *src);
        void processPackage(const QString &name, QStringList &files, bool isSource);
        
    private:
        PackageSource *src;
        
        struct Enrg
        {
            QString path;
            QString package;
        };
        
        QList<Enrg> enrgs;
};

#endif