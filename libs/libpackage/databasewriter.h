/*
 * databasewriter.h
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

#ifndef __DATABASEWRITER_H__
#define __DATABASEWRITER_H__

#include <QObject>
#include <QEventLoop>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkReply;
class QIODevice;

namespace Logram {
    
class PackageSystem;

struct _String;
struct _Package;
struct _StrPackage;
struct _Depend;

struct knownEntry
{
    QByteArray version;
    _Package *pkg;
    int index;
    bool ignore;        // True si c'est un paquet présent dans les dépôts et installé à la même version
};

class DatabaseWriter : public QObject
{
    Q_OBJECT
    public:
        DatabaseWriter(PackageSystem *_parent);

        bool download(const QString &source, const QString &url, const QString &type, bool isTranslations);
        bool rebuild();

    private:
        PackageSystem *parent;
        
        QStringList cacheFiles;

        QList<_Package *> packages;
        
        //QHash<int, int> packagesIndexes;
        QList<_String *> strings;
        QList<_String *> translate;
        QHash<QByteArray, int> stringsIndexes;
        QHash<QByteArray, int> translateIndexes;
        QList<QByteArray> stringsStrings;
        QList<QByteArray> translateStrings;

        int strPtr, transPtr;

        QList<QList<_StrPackage *> > strPackages;
        QList<QList<_Depend *> > depends;
        
        QHash<QByteArray, QList<knownEntry *> > knownPackages; // (nom, [(version, _Package)])
        QList<knownEntry *> knownEntries;

        void handleDl(QIODevice *device);
        int stringIndex(const QByteArray &str, int pkg, bool isTr, bool create = true);
        void setDepends(_Package *pkg, const QByteArray &str, int type);
        void revdep(_Package *pkg, const QByteArray &name, const QByteArray &version, int op, int type);
};

} /* Namespace */

#endif