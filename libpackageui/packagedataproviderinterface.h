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

/**
 * @file packagedataproviderinterface.h
 * @brief Interface d'information sur les paquets
 */

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

/**
 * @brief Interface d'information sur les paquets
 * 
 * Le widget InfoPane permet d'afficher des informations sur un paquet. Il est
 * concu pour ne pas dépendre de la @b libpackage, ce qui lui permet d'afficher
 * des informations aussi bien sur les paquets Logram que sur d'autres types de 
 * paquets.
 * 
 * Le but premier de ce découpage est de permettre à des programmes de fournir
 * des informations modifiées ou complétées sur des paquets. Par exemple, Pkgui
 * utilise les informations en provenance directe de @b libpackage, alors que
 * AppManager pourrait préférer celles venant de fichiers XML de métadonnées,
 * afin d'éviter de télécharger trop de fichiers metadata.xml.
 */
class PackageDataProviderInterface
{
    public:
        PackageDataProviderInterface() {}               /*!< @brief Constructeur */
        virtual ~PackageDataProviderInterface() {};     /*!< @brief Destructeur */
        
        virtual QString name() const = 0;               /*!< @brief Nom du paquet */
        virtual QString version() const = 0;            /*!< @brief Version du paquet */
        
        virtual int flags() const = 0;                  /*!< @brief Flags (PACKAGE_FLAG_*) */
        virtual void setFlags(int flags) = 0;           /*!< @brief Définit les flags */
        virtual bool flagsEditable() const = 0;         /*!< @brief Permet de savoir s'il est possible de modifier les flags avec setFlags() */
        
        virtual QString website() const = 0;            /*!< @brief Site web de l'auteur du paquet */
        virtual QString title() const = 0;              /*!< @brief Titre */
        virtual QString shortDesc() const = 0;          /*!< @brief Description courte */
        virtual QString longDesc() const = 0;           /*!< @brief Description longue */
        virtual QString license() const = 0;            /*!< @brief Licence */
        
        virtual QByteArray iconData() const = 0;        /*!< @brief Données de l'icône (contenu d'un fichier PNG par exemple) */
        
        virtual QString repository() const = 0;         /*!< @brief Nom du dépôt */
        virtual QString distribution() const = 0;       /*!< @brief Nom de la distribution */
        virtual QString section() const = 0;            /*!< @brief Nom de la section */
        
        virtual int downloadSize() const = 0;           /*!< @brief Taille à télécharger, en octets */
        virtual int installSize() const = 0;            /*!< @brief Taille à installer, en octets */
        
        virtual QVector<PackageDataProviderInterface *> versions() const = 0;   /*!< @brief Versions disponibles */
        virtual QVector<Logram::Depend *> depends() const = 0;                  /*!< @brief Dépendances */
        virtual QVector<Logram::ChangeLogEntry *> changelog() const = 0;        /*!< @brief Entrées de changelog */
        virtual QVector<Logram::PackageFile *> files() const = 0;               /*!< @brief Fichiers */
};

}

#endif