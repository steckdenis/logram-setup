/*
 * package.h
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
 * @file package.h
 * @brief Déclaration de la classe Package
 */

#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include <QObject>
#include <QProcess>
#include <QDateTime>

#include "solver.h"

#include <stdint.h>

namespace Logram
{

class PackageSystem;
class PackageMetaData;
class Communication;
class DatabaseReader;

class Depend;
class PackageFile;
class DatabasePackage;

/**
 * @brief Classe de base de la gestion des paquets
 * 
 * Dans LPM, les paquets peuvent provenir de la base de donnée ou de fichiers.
 * Cette classe abstraite permet de gérer un paquet d'où qu'il vienne. Elle
 * contient des propriétés permettant aux scripts QtScript d'utiliser les 
 * paquets.
 * 
 * Cette classe a avant tout un but informatif, et ne permet que peu de choses
 * modifiant le paquet ou son état (téléchargement, installation).
 */
class Package : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(Logram::Solver::Action action READ action)
    
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QString maintainer READ maintainer)
    Q_PROPERTY(QString shortDesc READ shortDesc)
    Q_PROPERTY(QString source READ source)
    Q_PROPERTY(QString repo READ repo)
    Q_PROPERTY(QString section READ section)
    Q_PROPERTY(QString distribution READ distribution)
    Q_PROPERTY(QString license READ license)
    Q_PROPERTY(QString arch READ arch)
    Q_PROPERTY(Origin origin READ origin)
    
    Q_PROPERTY(int flags READ flags)
    
    Q_PROPERTY(int downloadSize READ downloadSize)
    Q_PROPERTY(int installSize READ installSize)
    
    public:
        /**
         * @brief Constructeur par défaut
         * 
         * Constructeur par défaut de Package. Ne peut être appelé que par une
         * classe dérivée.
         * 
         * @param ps PackageSystem en cours d'utilisation
         * @param psd DatabaseReader de ce PackageSystem
         * @param _action Action sur le paquet (installation, etc)
         */
        Package(PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        
        /**
         * @overload
         */
        Package(QObject *parent, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        
        /**
         * @brief Constructeur de copie
         */
        Package(const Package &other);
        
        /**
         * @brief Destructeur
         */
        virtual ~Package();
        
        /**
         * @brief Origine du paquet
         */
        enum Origin
        {
            Database,   /*!< Le paquet provient de la base de donnée */
            File        /*!< Le paquet provient d'un fichier */
        };

        // Interface
        /**
         * @brief Télécharge le paquet
         *  
         * Télécharge le fichier .tlz du paquet binaire, et le place dans le
         * cache Setup. 
         *  
         * Cette fonction n'est pas bloquante, elle retourne sitôt le
         * téléchargement commencé.
         *  
         * Une fois le paquet téléchargé (downloadEnded() est émis), vous
         * pouvez récupérer le nom complet du fichier .tlz local en appelant
         * tlzFileName().
         *  
         * @sa tlzFileName
         * @return true si le téléchargement est bien lancé, false sinon
         */
        virtual bool download() = 0;
        
        /**
         * @brief Nom du fichier .tlz récupéré
         *  
         * Une fois le fichier .tlz du paquet téléchargé par download(),
         * appelez cette fonction pour récupérer son nom.
         *  
         * @sa download
         * @return Nom du fichier .tlz téléchargé
         */
        virtual QString tlzFileName() = 0;
        
        /**
         * @brief Permet de savoir si le paquet est valide
         *  
         * Il se peut que des erreurs se soient produites dans le
         * constructeur, qui ne peut pas retourner une valeur (false ici).
         * Cette fonction renvoie false si le paquet n'a pu être construit.
         * PackageSystem::lastError() est placé de manière appropriée.
         *  
         * @return true si le paquet est prêt à être utilisé, false sinon
         */
        virtual bool isValid() = 0;
        
        /**
         * @brief Renvoie l'origine du paquet, Package::Database ici
         *  
         * Permet de savoir si un Package vient de la base de donnée ou d'un
         * fichier.
         *  
         * @return Origine du paquet
         */
        virtual Origin origin() = 0;
        
        virtual QString name() = 0;         /*!< Nom du paquet */
        virtual QString version() = 0;      /*!< Version du paquet */
        virtual QString maintainer() = 0;   /*!< Mainteneur du paquet */
        virtual QString shortDesc() = 0;    /*!< Description courte */
        virtual QString source() = 0;       /*!< Nom du paquet source */
        virtual QString upstreamUrl() = 0;  /*!< Url du site web du projet à la base de l'application empaquetée */
        virtual QString repo() = 0;         /*!< Dépôt duquel vient le paquet */
        virtual QString section() = 0;      /*!< Section du paquet (base, devel, games, etc) */
        virtual QString distribution() = 0; /*!< Distribution du paquet (experimental, stable, old, testing) */
        virtual QString license() = 0;      /*!< License du paquel (GPLv2, GPLv3, BSD, Apache, etc) */
        virtual QString arch() = 0;         /*!< Architecture du paquet (i686, x86_64, all, src) */
        virtual QByteArray metadataHash() = 0; /*!< Hash SHA1 des métadonnées du paquet */
        virtual QByteArray packageHash() = 0; /*!< Hash SHA1 du fichier .tlz, pour vérifier son authenticité */
        virtual int flags() = 0;            /*!< Flags du paquet */
        virtual int used() = 0;             /*!< Compteur d'utilisation (nombre de paquets en dépendant installés) */
        
        virtual int downloadSize() = 0;     /*!< Taille à télécharger */
        virtual int installSize() = 0;      /*!< Taille installée */
        
        virtual QList<Depend *> depends() = 0; /*!< Liste des dépendances */
        virtual QList<PackageFile *> files() = 0; /*!< Liste des fichiers */
        virtual QDateTime installedDate() = 0; /*!< Date d'installation du paquet, indéfini si non-installé */
        virtual int installedBy() = 0;      /*!< UID de l'utilisateur ayant installé le paquet */
        
        /**
         * @brief Comparaison rapide du nom
         * 
         * Pour les paquets en base de donnée, il existe une manière très
         * rapide de comparer leur nom ou version : comparer les index de
         * leurs chaînes. Cela évite des appels répétitifs à
         * QString::operator==().
         * 
         * @param other Package à comparer. Comparaison rapide effectuée si other->origin() == Package::Database.
         * @return true si les noms sont les mêmes, false sinon
         */
        virtual bool fastNameCompare(Package *other) = 0;
        virtual bool fastVersionCompare(Package *other) = 0; /*!< Comparaison rapide de la version */
        virtual bool fastNameVersionCompare(Package *other) = 0; /*!< Comparaison rapide du nom et de la version */
        
        /**
         * @brief Enregistrer l'état du paquet
         * @internal
         *  
         * Quand un paquet est installé, il doit être enregistré dans la base
         * de donnée binaire comme l'étant. Cette fonction le fait.
         *  
         * @param idate : TimeStamp UNIX de la date d'installation
         * @param iby : UID de l'utilisateur ayant installé le paquet
         * @param flags : Flags du paquet à enregistrer
         */
        virtual void registerState(int idate, int iby, int flags) = 0;
        
        // Commun à tous les types de paquets
        /**
         * @brief Procède à l'installation/suppression du paquet
         * 
         * Lance un QThread chargé d'installer le paquet. Ce paquet doit
         * avoir été téléchargé à l'aide de download() pour que ça fonctionne.
         */
        void process();
        Solver::Action action();        /*!< Action demandée au paquet */
        void setAction(Solver::Action act); /*!< Définit l'action du paquet */
        
        /**
         * @brief Métadonnées du paquet
         * 
         * Fonction utilitaire permettant d'obtenir les métadonnées. Cette
         * fonction est équivalente à
         * 
         * @code
         * PackageMetaData *md = new PackageMetaData(packageSystem);
         * md->bindPackage(package);
         * 
         * Q_ASSERT(md->error() == false);
         * @endcode
         * @return Métadonnées du paquet
         */
        PackageMetaData *metadata();
        
        // Attributs communs
        /**
         * @brief Renseigne si l'utilisateur veut ce paquet
         * 
         * LPM supporte la suppression automatique des paquets installés
         * comme dépendance et non pas explicitement par l'utilisateur.
         * 
         * La propriété wanted de Package permet de renseigner ou de savoir
         * si un paquet est installé comme dépendance automatique (@b false)
         * ou manuellement par l'utilisateur (@b true).
         * 
         * @param wanted true si l'utilisateur installe ce paquet, false si
         *               installé comme dépendance
         */
        void setWanted(bool wanted);
        
        /**
         * Permet de savoir si l'utilisateur veut ce paquet
         * @sa setWanted
         * @return true si le paquet est voulu.
         */
        bool wanted() const;

        // Utilitaire
        /**
         * @brief Affichage lisible par un humain des dépendances
         * 
         * Transforme une liste de Depend en une chaîne de caractère. Chaque
         * dépendance, dont le type correspond à @p type, est ajoutée à cette
         * chaîne, séparée de la précédante par un ; .
         * 
         * Chaque dépendance est au format «[pattern]([op][version])».
         * 
         * Exemples :
         * 
         *  - libncurses5\>=5.7
         *  - mesa=git; qt\>=4.7.2; foobar
         * 
         * @code
         * Package *pkg = package();
         * 
         * QString rs = Package::dependsToString(pkg->depends(), DEPEND_TYPE_CONFLICT);
         * 
         * qDebug() << rs; // Affiche les conflits de pkg
         * @endcode
         * 
         * @note Ce format est celui utilisé par les fichiers texte récupérés
         *       des serveurs de Logram, ainsi que par
         *       installed_packages.list.
         * 
         * @param deps liste des dépendances
         * @param type type que doivent avoir les dépendances pour être
         *             retenues dans la liste
         * @return liste des dépendances formattée.
         */
        static QString dependsToString(const QList<Depend *> &deps, int type);
        
        // Mise à jour
        /**
         * @brief Paquet en version plus récente
         * 
         * Lorsque action() vaut Solver::Update, ce champs permet d'obtenir
         * le paquet vers lequel ce paquet va être mis à jour.
         * 
         * @return Paquet en version plus récente
         */
        DatabasePackage *upgradePackage();
        
        /**
         * @brief Définit le paquet en version plus récente
         * 
         * Cette fonction est utilisée par DatabaseReader quand on lui
         * demande de trouver les paquets pouvant être mis à jour.
         * 
         * @param i index du paquet dans la base de donnée
         * @internal
         */
        void setUpgradePackage(int i);
        
        /**
         * @brief Définir le paquet en version plus récente
         * 
         * Cette fonction est utilisée par Solver pour informer un paquet
         * qu'il sera mis à jour vers une autre version.
         * 
         * @param pkg Paquet en version plus récente
         * @internal
         */
        void setUpgradePackage(DatabasePackage *pkg);

    signals:
        /**
         * @brief Le paquet est installé
         * 
         * Ce signal est émis lorsque le paquet est installé ou supprimé.
         * Le thread qui a été lancé par process() est alors supprimé.
         * 
         * @param success true si tout s'est bien passé, false sinon.
         */
        void proceeded(bool success);
        
        /**
         * @brief Le paquet est téléchargé
         *  
         * Émis lorsque le paquet a fini d'être télécharge. tlzFileName
         * contient alors le nom d'un fichier .tlz dans le système de fichier
         * local.
         *  
         * @sa tlzFileName
         * @param success true si le téléchargement a réussi en entier, 
         *                false sinon
         */
        void downloaded(bool success);
        
        /**
         * @brief Communication
         * @internal
         * 
         * Ce signal permet de savoir si un paquet émmet une communication. Il
         * est préférable d'utiliser PackageSystem::communication qui est
         * global à tous les paquets.
         * 
         * @param sender ce paquet
         * @param comm communication
         */
        void communication(Logram::Package *sender, Logram::Communication *comm);

    private slots:
        void processEnd();
        void processLineOut(QProcess *process, const QByteArray &line);

    private:
        struct Private;
        Private *d;
};

class Depend
{
    public:
        Depend();
        virtual ~Depend() {}

        virtual QString name() = 0;
        virtual QString version() = 0;
        virtual int8_t type() = 0;
        virtual int8_t op() = 0;
};

class PackageFile
{
    public:
        PackageFile(PackageSystem *ps);
        virtual ~PackageFile();
        
        virtual QString path() = 0;
        virtual int flags() = 0;
        virtual uint installTime() = 0;
        virtual Package *package() = 0;
        
        virtual void setFlags(int flags) = 0;
        virtual void setInstallTime(uint timestamp) = 0;
        
    protected:
        void saveFile();
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

// Constantes

#define DEPEND_TYPE_INVALID  0
#define DEPEND_TYPE_DEPEND   1
#define DEPEND_TYPE_SUGGEST  2
#define DEPEND_TYPE_CONFLICT 3
#define DEPEND_TYPE_PROVIDE  4
#define DEPEND_TYPE_REPLACE  5
#define DEPEND_TYPE_REVDEP   6   // Dans ce cas, name = index du paquet dans packages, version = 0

#define DEPEND_OP_NOVERSION  0   // Pas de version spécifiée
#define DEPEND_OP_EQ         1   // =
#define DEPEND_OP_GREQ       2   // >=
#define DEPEND_OP_GR         3   // >
#define DEPEND_OP_LOEQ       4   // <=
#define DEPEND_OP_LO         5   // <
#define DEPEND_OP_NE         6   // !=

#define PACKAGE_FLAG_KDEINTEGRATION         0b00000000000011
#define PACKAGE_FLAG_GUI                    0b00000000000100
#define PACKAGE_FLAG_DONTUPDATE             0b00000000001000
#define PACKAGE_FLAG_DONTINSTALL            0b00000000010000
#define PACKAGE_FLAG_DONTREMOVE             0b00000000100000
#define PACKAGE_FLAG_EULA                   0b00000001000000
#define PACKAGE_FLAG_NEEDSREBOOT            0b00000010000000
#define PACKAGE_FLAG_WANTED                 0b00000100000000
#define PACKAGE_FLAG_INSTALLED              0b00001000000000
#define PACKAGE_FLAG_REMOVED                0b00010000000000
#define PACKAGE_FLAG_REBUILD                0b00100000000000
#define PACKAGE_FLAG_RECOMPILE              0b01000000000000
#define PACKAGE_FLAG_CONTINUOUSRECOMPILE    0b10000000000000

#define PACKAGE_FILE_INSTALLED              0b00000001
#define PACKAGE_FILE_DIR                    0b00000010
#define PACKAGE_FILE_DONTREMOVE             0b00000100
#define PACKAGE_FILE_DONTPURGE              0b00001000
#define PACKAGE_FILE_BACKUP                 0b00010000
#define PACKAGE_FILE_OVERWRITE              0b00100000
#define PACKAGE_FILE_VIRTUAL                0b01000000
#define PACKAGE_FILE_CHECKBACKUP            0b10000000


#endif