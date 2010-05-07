/*
 * databasepackage.h
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

        /**
            @brief Télécharge le paquet
            
            Télécharge le fichier .tlz du paquet binaire, et le place dans le cache Setup. 
            
            Cette fonction n'est pas bloquante, elle retourne sitôt le téléchargement commencé.
            
            Une fois le paquet téléchargé (downloadEnded() est émis), vous pouvez récupérer
            le nom complet du fichier .tlz local en appelant tlzFileName().
            
            @sa tlzFileName
            @return true si le téléchargement est bien lancé, false sinon
        */
        bool download();
        
        /**
            @brief Nom du fichier .tlz récupéré
            
            Une fois le fichier .tlz du paquet téléchargé par download(), appelez cette fonction
            pour récupérer son nom.
            
            @sa download
            @return Nom du fichier .tlz téléchargé
        */
        QString tlzFileName();
        
        /**
            @brief Permet de savoir si le paquet est valide
            
            Il se peut que des erreurs se soient produites dans le constructeur, qui ne peut
            pas retourner une valeur (false ici). Cette fonction renvoie false si le paquet
            n'a pu être construit. PackageSystem::lastError() est placé de manière appropriée.
            
            @return true si le paquet est prêt à être utilisé, false sinon
        */
        bool isValid();
        
        /**
            @brief Renvoie l'origine du paquet, Package::Database ici
            
            Permet de savoir si un Package vient de la base de donnée ou d'un fichier.
            L'implémentation dans cette classe renvoie toujours Package::Database.
            
            @return Package::Database
        */
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
            Binary,         /*!< Url du paquet binaire .tlz */
            Metadata,       /*!< Url des métadonnées .xml.xz */
            Source          /*!< Url de la source .src.tlz */
        };
        
        QString name();             /*!< Nom du paquet */
        QString version();          /*!< Version du paquet */
        QString newerVersion();     /*!< Version d'une éventuelle mise à jour, QString() si paquet à jour */
        QString maintainer();       /*!< Mainteneur du paquet */
        QString shortDesc();        /*!< Description courte */
        QString source();           /*!< Nom du paquet source (libinitng vient d'initng) */
        QString upstreamUrl();      /*!< Url du site web du projet à la base de l'application empaquetée */
        QString repo();             /*!< Dépôt duquel vient le paquet */
        QString section();          /*!< Section du paquet (base, devel, games, etc) */
        QString distribution();     /*!< Distribution du paquet (experimental, stable, old, testing) */
        QString license();          /*!< License du paquel (GPLv2, GPLv3, BSD, Apache, etc) */
        QString arch();             /*!< Architecture du paquet (i686, x86_64, all, src) */
        
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
        QByteArray packageHash();   /*!< Hash SHA1 du fichier .tlz, pour vérifier son authenticité */
        QByteArray metadataHash();  /*!< Hash SHA1 des métadonnées du paquet */
        int flags();                /*!< Flags du paquet */
        
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
        
        QDateTime installedDate();  /*!< Date d'installation du paquet, indéfini si non-installé */
        int installedBy();          /*!< UID de l'utilisateur ayant installé le paquet */
        int used();                 /*!< Nombre de paquets installés qui dépendent de ce paquet */
        int index() const;          /*!< @internal */
        
        int downloadSize();         /*!< Taille du fichier .tlz à télécharger, en octets */
        int installSize();          /*!< Taille du paquet une fois installée. PackageSystem::fileSizeFormat pour formatter tout ça) */

        QList<Package *> versions();    /*!< Liste de Package donc chacun est une version différente de ce paquet */
        QList<Depend *> depends();      /*!< Liste de Depend représentant les dépendances de ce paquet */
        QList<PackageFile *> files();   /*!< Liste des fichiers du paquet */
        
        /**
            @brief Enregistrer l'état du paquet
            @internal
            
            Quand un paquet est installé, il doit être enregistré dans la base de donnée binaire comme l'étant.
            Cette fonction le fait
            
            @param idate : TimeStamp UNIX de la date d'installation
            @param iby : UID de l'utilisateur ayant installé le paquet
            @param flags : Flags du paquet à enregistrer
        */
        void registerState(int idate, int iby, int flags);

    signals:
        /**
            @brief Le paquet est téléchargé
            
            Émis lorsque le paquet a fini d'être télécharge. tlzFileName contient alors le nom d'un fichier
            .tlz dans le système de fichier local.
            
            @sa tlzFileName
            @param success true si le téléchargement a réussi en entier, false sinon
        */
        void downloaded(bool success);

    private slots:
        /**
            @brief Téléchargement du fichier terminé
            @internal
            
            Le fichier .tlz est téléchargé. Ce slot émet downloaded
            
            @param md Logram::ManagedDownload représentant le téléchargement
        */
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

class DatabaseFile : public PackageFile
{
    public:
        DatabaseFile(PackageSystem *ps, DatabaseReader *dr, _File *file, DatabasePackage *pkg, bool packagebinded);
        ~DatabaseFile();
        
        QString path();
        int flags();
        uint installTime();
        Package *package();
        
        void setFlags(int flags);
        void setInstallTime(uint timestamp);
        void setPackageBinded(bool binded);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif