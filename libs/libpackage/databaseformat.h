/*
 * databaseformat.h
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

#ifndef __DATABASEFORMAT_H__
#define __DATABASEFORMAT_H__

#include <stdint.h>

namespace Logram
{

struct _Package
{
    // TODO: Maintainer
    int32_t name;       // Index de la chaîne du nom
    int32_t version;    // Index de la chaîne de version
    int32_t maintainer; // Index de la chaîne du nom du mainteneur
    int32_t short_desc; // Index de la chaîne de description courte
    int32_t source;     // Nom du paquet source
    int32_t repo;       // Nom du mirroir qui fourni le paquet (sources.list)
    int32_t arch;       // Url de téléchargement
    int32_t section;    // Section
    int32_t distribution; // Distribution
    int32_t license;    // Licence
    int32_t pkg_hash;   // Index de la chaîne représentant le hash sha1 du paquet .tlz
    int32_t mtd_hash;   // Index de la chaîne représentant le hash sha1 du fichier de métadonnées
    int32_t is_gui;     // Programme en GUI (NOTE: 32 bits disponibles, on peut en faire des flags)
    int32_t deps;       // Index du tableau de dépendances
    int32_t isize;      // Taille de l'installation
    int32_t dsize;      // Taille du téléchargement

    int32_t idate;      // Timestamp de l'installation
    int32_t iby;        // UID de l'utilisateur ayant installé le paquet
    int32_t state;      // Status du paquet (PACKAGE_STATE)
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

} /* Namespace */

#define PACKAGESYSTEM_OPT_INSTALLSUGGESTS    1
#define PACKAGESYSTEM_OPT_PARALLELINSTALLS   2
#define PACKAGESYSTEM_OPT_PARALLELDOWNLOADS  4
#define PACKAGESYSTEM_OPT_INSTALLROOT        8
#define PACKAGESYSTEM_OPT_CONFROOT          16
#define PACKAGESYSTEM_OPT_VARROOT           32

#endif