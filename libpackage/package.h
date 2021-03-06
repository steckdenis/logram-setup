/*
 * package.h
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

class PackageFile;
class DatabasePackage;

/**
 * @brief Dépendance d'un paquet
 */
class Depend
{
    public:
        Depend();               /*!< @brief Constructeur */
        virtual ~Depend() {}    /*!< @brief Destructeur */
        
        /** @brief Type de dépendance d'un paquet envers un autre */
        enum Type
        {
            Invalid = 0,    /*!< @brief Invalide */
            DependType = 1, /*!< @brief Le paquet dépend d'un autre */
            Suggest = 2,    /*!< @brief Le paquet en suggère un autre */
            Conflict = 3,   /*!< @brief Le paquet ne peut être installé sans l'autre */
            Provide = 4,    /*!< @brief Le paquet fournit un autre (pas spécialement existant, bash fournit "shell" par exemple) */
            Replace = 5,    /*!< @brief Le paquet fournit l'autre (conflit + fournit) */
            RevDep = 6      /*!< @brief L'autre dépend de ce paquet (dépendance inverse, «est requis par»). 
                              
                              Dans ce cas, dans la base de donnée, Logram::_Package::name vaut l'index du
                              paquet qui dépend de ce paquet, et version vaut 0. Op ignoré */
        };
        
        /** @brief Opération de comparaison pour trouver la version des autres paquets concernés par la dépendance */
        enum Operation
        {
            NoVersion = 0,      /*!< @brief Pas de version spécifiée */
            Equal = 1,          /*!< @brief = */
            GreaterOrEqual = 2, /*!< @brief >= */
            Greater = 3,        /*!< @brief > */
            LowerOrEqual = 4,   /*!< @brief <= */
            Lower = 5,          /*!< @brief < */
            NotEqual = 10       /*!< @brief != */
        };
        
        virtual QString name() = 0;     /*!< @brief Nom du paquet */
        virtual QString version() = 0;  /*!< @brief Version */
        virtual Type type() = 0;      /*!< @brief Type de dépendance (DEPEND_TYPE*) */
        virtual Operation op() = 0;        /*!< @brief Opération (DEPEND_OP*) */
};

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
            Database,   /*!< @brief Le paquet provient de la base de donnée */
            File        /*!< @brief Le paquet provient d'un fichier */
        };
        
        enum Flag
        {
            KDEIntegration =        0b000000000011,  /*!< @brief Intégration à KDE (0 à 3) */
            GUI =                   0b000000000100,  /*!< @brief Paquet graphique */
            DontUpdate =            0b000000001000,  /*!< @brief Ne pas mettre à jour */
            DontInstall =           0b000000010000,  /*!< @brief Ne pas installer */
            DontRemove =            0b000000100000,  /*!< @brief Ne pas supprimer */
            Eula =                  0b000001000000,  /*!< @brief Nécessite l'approbation d'une licence */
            NeedsReboot =           0b000010000000,  /*!< @brief Nécessite un redémarrage */
            Wanted =                0b000100000000,  /*!< @brief Installé manuellement */
            Installed =             0b001000000000,  /*!< @brief Installé */
            Removed =               0b010000000000,  /*!< @brief Supprimé */
            Primary =               0b100000000000   /*!< @brief Paquet primaire (bash, pas bash-doc) */
        };

        // Interface
        /**
         * @brief Télécharge le paquet
         *  
         * Télécharge le fichier .tlz du paquet binaire, et le place dans le
         * cache LPM. 
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
        
        virtual QString name() = 0;         /*!< @brief Nom du paquet */
        virtual QString version() = 0;      /*!< @brief Version du paquet */
        virtual QString maintainer() = 0;   /*!< @brief Mainteneur du paquet */
        virtual QString shortDesc() = 0;    /*!< @brief Description courte */
        virtual QString source() = 0;       /*!< @brief Nom du paquet source */
        virtual QString upstreamUrl() = 0;  /*!< @brief Url du site web du projet à la base de l'application empaquetée */
        virtual QString repo() = 0;         /*!< @brief Dépôt duquel vient le paquet */
        virtual QString section() = 0;      /*!< @brief Section du paquet (base, devel, games, etc) */
        virtual QString distribution() = 0; /*!< @brief Distribution du paquet (experimental, stable, old, testing) */
        virtual QString license() = 0;      /*!< @brief License du paquel (GPLv2, GPLv3, BSD, Apache, etc) */
        virtual QString arch() = 0;         /*!< @brief Architecture du paquet (i686, x86_64, all, src) */
        virtual QByteArray metadataHash() = 0; /*!< @brief Hash SHA1 des métadonnées du paquet, binaire */
        virtual QByteArray packageHash() = 0; /*!< @brief Hash SHA1 du fichier .tlz, binaire */
        virtual Flag flags() = 0;            /*!< @brief Flags du paquet */
        virtual int used() = 0;             /*!< @brief Compteur d'utilisation (nombre de paquets en dépendant installés) */
        
        virtual int downloadSize() = 0;     /*!< @brief Taille à télécharger */
        virtual int installSize() = 0;      /*!< @brief Taille installée */
        
        virtual QVector<Depend *> depends() = 0; /*!< @brief Liste des dépendances */
        virtual QVector<PackageFile *> files() = 0; /*!< @brief Liste des fichiers */
        virtual QDateTime installedDate() = 0; /*!< @brief Date d'installation du paquet, indéfini si non-installé */
        virtual int installedBy() = 0;      /*!< @brief UID de l'utilisateur ayant installé le paquet */
        
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
        virtual bool fastVersionCompare(Package *other) = 0; /*!< @brief Comparaison rapide de la version */
        virtual bool fastNameVersionCompare(Package *other) = 0; /*!< @brief Comparaison rapide du nom et de la version */
        
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
        virtual void registerState(int idate, int iby, Flag flags) = 0;
        
        // Commun à tous les types de paquets
        /**
         * @brief Procède à l'installation/suppression du paquet
         * 
         * Lance un QThread chargé d'installer le paquet. Ce paquet doit
         * avoir été téléchargé à l'aide de download() pour que ça fonctionne.
         */
        void process();
        Solver::Action action();        /*!< @brief Action demandée au paquet */
        void setAction(Solver::Action act); /*!< @brief Définit l'action du paquet */
        
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
         * QString rs = Package::dependsToString(pkg->depends(), Depend::Conflict);
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
        static QString dependsToString(const QVector<Depend *> &deps, Depend::Type type);
        
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

/**
 * @brief Fichier d'un paquet
 */
class PackageFile
{
    public:
        /**
         * @brief Constructeur 
         * @param ps PackageSystem utilisé
         */
        PackageFile(PackageSystem *ps);
        virtual ~PackageFile();             /*!< @brief Destructeur */
        
        enum Flag
        {
            Installed =     0b00000001,     /*!< @brief Installé */
            Directory =     0b00000010,     /*!< @brief Dossier */
            DontRemove =    0b00000100,     /*!< @brief Ne pas supprimer */
            DontPurge =     0b00001000,     /*!< @brief Ne pas supprimer, même si on purge */
            Backup =        0b00010000,     /*!< @brief Ne pas remplacer. Garder le fichier de l'utilisateur, placer le nouveau dans un .new */
            Overwrite =     0b00100000,     /*!< @brief Toujours écraser ce fichier en cas de mise à jour */
            Virtual =       0b01000000,     /*!< @brief Fichier virtuel, non-installé par un paquet, mais créé par le programme qu'il contient */
            CheckBackup =   0b10000000      /*!< @brief Ne sauvegarder ce fichier que s'il a été modifié par l'utilisateur */
        };
        
        virtual QString path() = 0;         /*!< @brief Chemin */
        virtual Flag flags() = 0;           /*!< @brief Flags */
        virtual uint installTime() = 0;     /*!< @brief Timestamp UNIX de la date d'installation */
        virtual Package *package() = 0;     /*!< @brief Paquet auquel le fichier appartient */
        
        virtual void setFlags(Flag flags) = 0;              /*!< @brief Modifie les flags du fichier */
        virtual void setInstallTime(uint timestamp) = 0;    /*!< @brief Définit la date d'installation */
        
    protected:
        void saveFile();                    /*!< @brief Enregistre ce fichier comme devant être écrit dans installed_files.list */
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif