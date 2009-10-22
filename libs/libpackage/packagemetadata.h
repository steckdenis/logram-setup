/*
 * packagemetadata.h
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

#ifndef __PACKAGEMETADATA_H__
#define __PACKAGEMETADATA_H__

#include <QtXml/QDomDocument>

#include <QObject>

class Package;
class PackageSystem;

class PackageMetaData : public QDomDocument, public QObject
{
    public:
        PackageMetaData(Package *pkg, PackageSystem *ps);
        ~PackageMetaData();
        
        QString primaryLang() const;
        QString stringOfKey(const QDomElement &element) const;
        QString packageDescription() const;
        QString packageTitle() const;
        QString currentPackage() const;
        
        void setCurrentPackage(const QString &name);
        
    private:
        struct Private;
        Private *d;
};

#endif