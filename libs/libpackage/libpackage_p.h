/*
 * libpackage_p.h
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

struct _Package
{
    int32_t name;       // Index de la chaîne du nom
    int32_t version;    // Index de la chaîne de version
    int32_t title;      // Index de la chaîne de titre
    int32_t short_desc; // Index de la chaîne de description courte
    int32_t long_desc;  // Index de la chaîne de description longue
    int32_t source;     // Nom du paquet source
    int32_t repo;       // Nom du mirroir qui fourni le paquet (sources.list)
    int32_t url;        // Url de téléchargement
    int32_t section;    // Section
    int32_t distribution; // Distribution
    int32_t license;    // Licence
    int32_t deps;       // Index du tableau de dépendances
};

struct _String
{
    int32_t ptr;        // Pointeur à partir du début de la table des données
    int32_t strpkg;     // Index d'un StrPackagePtr dans strpackages
};

struct _DependPtr
{
    int32_t ptr;        // Pointeur sur le premier Depend à partir du début des données
    int32_t count;      // Nombre de Depend dans le tableau
};

struct _Depend
{
    int8_t type;        // DEPEND_TYPE
    int8_t op;          // DEPEND_OP
    int32_t pkgname;    // Index de la chaîne du nom du paquet de la dépendance
    int32_t pkgver;     // Index de la chaîne de la version du paquet de la dépendance
};

// Structure qui quand on a un index de string (un nom), et un index de version, permet de retrouver le paquet
struct _StrPackage
{
    int32_t version;    // Index de la chaîne de version nécessaire pour qu'on ait le bon paquet
    int32_t package;    // Index du paquet
};

struct _StrPackagePtr
{
    int32_t ptr;        // Pointeur sur un StrPackage dans la zone de données
    int32_t count;      // Nombre de StrPackages dedans
};

/**********************************************
******* Classe ********************************
**********************************************/

#include <QString>
#include <QList>
#include <QEventLoop>
#include <QHash>

class QFile;
class QNetworkAccessManager;
class PackageSystem;
struct ManagedDownload;

class PackageSystemPrivate
{
    public:
        PackageSystemPrivate(PackageSystem *_ps);
        void init();

        int package(const QString &name, const QString &version);
        QList<int> packagesByName(const QString &regex);
        QList<int> packagesByVString(const QString &verStr);
        QList<_Depend *> depends(int pkgIndex);
        QList<int> packagesOfString(int stringIndex, int nameIndex, int op);

        QString packageName(int index);
        QString packageVersion(int index);
        QString packageLongDesc(int index);
        QString packageShortDesc(int index);
        QString packageTitle(int index);
        QString packageSource(int index);
        QString packageRepo(int index);
        QString packageSection(int index);
        QString packageDistribution(int index);
        QString packageLicense(int index);
        QString packageUrl(int index);

        const char *string(uchar *map, int index);
        _Package *package(int index);
        _Depend *depend(int32_t ptr);
        
    private:
        void mapFile(const QString &file, QFile **ptr, uchar **map);

    private:
        QFile *f_packages, *f_strings, *f_translate, *f_depends, *f_strpackages;
        uchar *m_packages, *m_strings, *m_translate, *m_depends, *m_strpackages;

        PackageSystem *ps;

    public:
        // Réellement privé à PackageSystem
        QEventLoop loop;
        QNetworkAccessManager *nmanager;
        QString dlDest;
        QHash<QNetworkReply *, ManagedDownload *> managedDls;
};

#endif