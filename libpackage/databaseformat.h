/*
 * databaseformat.h
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

/**
    @file databaseformat.h
    @brief Description du format binaire de la base de donnée
    
    La base de donnée binaire de LPM est composée de plusieurs fichiers :
    
     - @b packages    : Contient la liste des paquets connus par LPM. Il contient
                        une liste de _Package
     - @b strings     : Contient une liste de _String. Ainsi, dans @b packages, toute
                        référence à une chaîne se fait grâce à son index. La comparaison
                        entre deux chaînes en est largement facilitée et accélérée.
     - @b translate   : Même chose que @b strings, mais contient les chaînes traduites
                        (descriptions des paquets)
     - @b depends     : Liste de _Depend, permet de savoir quels paquets en nécessitent 
                        d'autres
     - @b strpackages : Permet de savoir à quels paquets appartient une chaîne. Par exemple,
                        <em>libinitng</em> peut appartenir au paquet numéro 22 et au paquet
                        numéro 45, c'est à dire par exemple libinitng~0.7.0 et
                        libinitng~0.7.1. Ainsi, la résolution des dépendances est largement
                        accélérée
                                    
*/

#ifndef __DATABASEFORMAT_H__
#define __DATABASEFORMAT_H__

#include <stdint.h>

namespace Logram
{

/**
    @brief Paquet dans la base de donnée binaire
    
    Cette grosse structure contient toutes les informations nécessaires à la gestion
    des paquets dans la base de donnée binaire
*/
struct _Package
{
    int32_t name;       /*!< @brief Index de la chaîne du nom */
    int32_t version;    /*!< @brief Index de la chaîne de version */
    int32_t maintainer; /*!< @brief Index de la chaîne du nom du mainteneur */
    int32_t short_desc; /*!< @brief Index de la chaîne de description courte */
    int32_t source;     /*!< @brief Nom du paquet source */
    int32_t uurl;       /*!< @brief Url upstream (site web, etc) */
    int32_t repo;       /*!< @brief Nom du mirroir qui fourni le paquet (sources.list) */
    int32_t arch;       /*!< @brief Url de téléchargement */
    int32_t section;    /*!< @brief Section */
    int32_t distribution; /*!< @brief Distribution */
    int32_t license;    /*!< @brief Licence */
    int32_t pkg_hash;   /*!< @brief Index de la chaîne représentant le hash sha1 du paquet .tlz */
    int32_t mtd_hash;   /*!< @brief Index de la chaîne représentant le hash sha1 du fichier de métadonnées */
    int32_t flags;      /*!< @brief Flags */
    int32_t deps;       /*!< @brief Index du tableau de dépendances */
    int32_t isize;      /*!< @brief Taille de l'installation */
    int32_t dsize;      /*!< @brief Taille du téléchargement */
    int32_t used;       /*!< @brief Nombre de paquets installés qui dépendent de ce paquet */
    int32_t first_file; /*!< @brief Premier des fichiers installés par ce paquet */

    int32_t idate;      /*!< @brief Timestamp de l'installation */
    int32_t iby;        /*!< @brief UID de l'utilisateur ayant installé le paquet */
    int32_t index;      /*!< @brief Index du paquet (utilisé par databasewriter) */
};

/**
 * @brief Fichier ou dossier d'un paquet
 */
struct _File
{
    int32_t parent_dir; /*!< @brief Index du _File représentant le dossier parent, -1 si racine */
    int32_t package;    /*!< @brief Index du paquet contenant le fichier, ignoré si dossier */
    int32_t flags;      /*!< @brief Flags du fichier (voir PACKAGE_FILE_* dans package.h) */
    int32_t name_ptr;   /*!< @brief Pointeur du nom de ce fichier dans la zone de donnée de @b files */
    int32_t next_file_dir;  /*!< @brief Index du fichier suivant appartenant au même dossier, ou -1 */
    int32_t next_file_pkg;  /*!< @brief Index du fichier suivant appartenant au même paquet, ou -1 */
    int32_t first_child;    /*!< @brief Premier enfant d'un dossier */
    uint32_t itime;     /*!< @brief Timestamp UNIX de la date d'installation */
};

/**
    @brief Chaîne de caractère
*/
struct _String
{
    int32_t ptr;        /*!< @brief Pointeur à partir du début de la table des données */
    int32_t strpkg;     /*!< @brief Index d'un StrPackagePtr dans strpackages */
};

/**
    @brief Permet, pour un paquet, d'avoir sa liste de dépendances
    
    En effet, _Package a un champs @b deps, qui est l'index d'un _DependPtr.
    Ce _DependPtr permet d'accéder à la liste de toutes les dépendances du paquet
*/
struct _DependPtr
{
    int32_t ptr;        /*!< @brief Pointeur sur le premier Depend à partir du début des données */
    int32_t count;      /*!< @brief Nombre de Depend dans le tableau */
};

/**
    @brief Dépendance d'un paquet
*/
struct _Depend
{
    int8_t type;        /*!< @brief Type de dépendace, voir DEPEND_TYPE_* */
    int8_t op;          /*!< @brief Opération (=, >=, etc), voir DEPEND_OP_* */
    int32_t pkgname;    /*!< @brief Index de la chaîne du nom du paquet de la dépendance, ou index du paquet si REVDEP */
    int32_t pkgver;     /*!< @brief Index de la chaîne de la version du paquet de la dépendance, ou 0 si REVDEP */
};

/**
    @brief Permet de trouver le paquet à la bonne version possédant cette chaîne
*/
struct _StrPackage
{
    int32_t version;    /*!< @brief Index de la chaîne de version nécessaire pour qu'on ait le bon paquet */
    int32_t package;    /*!< @brief Index du paquet */
};

/**
    @brief Permet, pour une chaîne, d'avoir la liste des paquets qui l'utilisent
*/
struct _StrPackagePtr
{
    int32_t ptr;        /*!< @brief Pointeur sur un _StrPackage dans la zone de données */
    int32_t count;      /*!< @brief Nombre de StrPackages dedans */
};

} /* Namespace */

#endif