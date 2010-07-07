/*
 * databasepackage.h
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
    @file databasepackage.h
    @brief Contient la classe DatabasePackage, permettant de gérer les paquets en base de donnée
*/

#ifndef __DATABASEPACKAGE_H__
#define __DATABASEPACKAGE_H__

#include <QObject>
#include <QProcess>
#include <QDateTime>

#include "solver.h"
#include "package.h"

#include <stdint.h>

namespace Logram
{

class DatabaseReader;
struct ManagedDownload;

struct _Depend;
struct _File;
class DatabaseDepend;
class PackageFile;

/**
    @brief Paquet dans la base de donnée binaire
    
    Hérite de la classe Package et réimplémente son interface.
    
    Cette classe permet d'accéder facilement aux paquets contenus dans la base
    de donnée binaire, ainsi que d'en extraire leurs informations, leurs 
    dépendances, etc.
    
    Cette classe contient également quelques fonctions supplémentaires
    permettant de faires plus de choses avec un paquet en base de donnée
    qu'avec un paquet générique.
*/
class DatabasePackage : public Package
{
    Q_OBJECT
    
    public:
        /**
            @brief Construit un nouveau DatabasePackage
            @param index Index du paquet dans la base de donnée (fichier @b packages)
            @param ps PackageSystem permettant de lier cette classe au système de gestion des paquets de Logram
            @param psd Lecteur de base de donnée binaire permettant de récupérer les informations nécessaires
            @param _action Action demandée au paquet, passée à Package::Package
        */
        DatabasePackage(int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        
        /** 
            @overload
        */
        DatabasePackage(QObject *parent, int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        
        /**
            @brief Destructeur
        */
        ~DatabasePackage();

        
        bool download();
        QString tlzFileName();
        bool isValid();
        Package::Origin origin();

        /**
            @brief Type d'url d'un paquet
            
            Un paquet dans la base de donnée doit être téléchargé. Il possède néanmoins plusieurs
            urls différentes (paquet .tlz, métadonnées, source). Cette énumération vous permet
            de demander la bonne url à url().
            
            @sa url
        */
        enum UrlType
        {
            Binary,         /*!< @brief Url du paquet binaire .tlz */
            Metadata,       /*!< @brief Url des métadonnées .xml.xz */
            Source          /*!< @brief Url de la source .src.tlz */
        };
        
        QString name();
        QString version();
        QString newerVersion();     /*!< @brief Version d'une éventuelle mise à jour, QString() si paquet à jour */
        QString maintainer();
        QString shortDesc();        
        QString source();           
        QString upstreamUrl();      
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString arch();
        
        /** 
            @brief Récupère l'url d'un paquet
            
            Un paquet dans la base de donnée doit être téléchargé. Il possède néanmoins plusieurs
            urls différentes (paquet .tlz, métadonnées, source). Cette fonction vous permet de la
            calculer en fonction du nom, de la version et de l'architecture du paquet, ainsi
            que de l'architecture standard d'un dépôt Logram.
            
            @warning L'url ne contient pas le dépôt. Elle se présente sous la forme "pool/i/initng/...".
                     Vous devez vous-même récupérer l'url racine du dépôt du paquet, avec ses mirroirs.
            
            @sa UrlType
            @return Url du paquet, calculée correctement.
        */
        QString url(UrlType type = DatabasePackage::Binary);
        QByteArray packageHash();
        QByteArray metadataHash();
        int flags();
        
        /**
            @brief Définis les flags d'un paquet
            
            Une fois un paquet en base de donnée (donc venant des dépôts ou d'un fichier installé),
            l'utilisateur a la possibilité de lui attribuer des flags (ne pas supprimer, etc).
            
            Cette fonction vous permet de définir les flags d'un paquet, en utilisant les définitions
            se trouvant dans le fichier package.h.
            
            @code
                DatabasePackage *pkg = getPackage();
                
                int flag = PACKAGE_FLAG_DONTUPDATE;
                
                pkg->setFlags(pkg->flags() | flag);
            @endcode
            
            @param flags nouveau flags du paquet
            @sa flags
        */
        void setFlags(int flags);
        
        QDateTime installedDate();
        int installedBy();
        int used();
        int index() const;          /*!< @internal */
        
        int downloadSize();
        int installSize();

        QVector<Package *> versions();    /*!< @brief Liste de Package donc chacun est une version différente de ce paquet */
        QVector<Depend *> depends();
        QVector<PackageFile *> files();
        
        bool fastNameCompare(Package *other);
        bool fastVersionCompare(Package *other);
        bool fastNameVersionCompare(Package *other);
        
        void registerState(int idate, int iby, int flags);

    signals:
        void downloaded(bool success);

    private slots:
        void downloadEnded(Logram::ManagedDownload *md);

    private:
        struct Private;
        Private *d;
};

/**
    @brief Dépendance d'un paquet en base de donnée
    
    Cette classe implémente l'interface Depend et s'utilise de la même manière
*/
class DatabaseDepend : public Depend
{
    public:
        DatabaseDepend(_Depend *dep, DatabaseReader *psd);
        ~DatabaseDepend();

        QString name();
        QString version();
        int8_t type();
        int8_t op();

    private:
        struct Private;
        Private *d;
};

/**
 * @brief Fichier d'un paquet en provenance de la base de donnée
 */
class DatabaseFile : public PackageFile
{
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         * @param dr DatabaseReader permettant de lire le fichier
         * @param file Fichier dans la base de donnée
         * @param pkg Paquet auquel ce fichier appartient
         * @param packagebinded True si ~DatabasePackage() doit supprimer @p pkg
         */
        DatabaseFile(PackageSystem *ps, DatabaseReader *dr, _File *file, DatabasePackage *pkg, bool packagebinded);
        ~DatabaseFile();    /*!< @brief Destructeur */
        
        QString path();     /*!< @brief Chemin d'accès */
        int flags();        /*!< @brief Flags */
        uint installTime(); /*!< @brief Timestamp d'installation */
        Package *package(); /*!< @brief Paquet auquel appartient ce fichier */
        
        void setFlags(int flags);               /*!< @brief Définit les flags du paquet et les enregistre dans installed_files.list */
        void setInstallTime(uint timestamp);    /*!< @brief Définit la date d'installation */
        void setPackageBinded(bool binded);     /*!< @brief Définit si le destructeur doit supprimer le paquet de ce fichier */
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif