/*
 * repositorymanager.h
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
 * @file repositorymanager.h
 * @brief Gestion des dépôts
 */

#ifndef __REPOSITORYMANAGER_H__
#define __REPOSITORYMANAGER_H__

#include <QObject>

namespace Logram
{
    
class PackageSystem;

/**
 * @brief Gestion des dépôts
 * 
 * Cette classe permet de gérer un dépôt de paquets Logram. Elle utilise une
 * base de donnée pour stocker les paquets, et gère l'arbre des mirroirs.
 */
class RepositoryManager : public QObject
{
    Q_OBJECT
    
    public:
        
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         */
        RepositoryManager(PackageSystem *ps);
        
        /**
         * @brief Destructeur
         */
        ~RepositoryManager();
        
        /**
         * @brief Charge la configuration
         * 
         * RepositoryManager est une classe largement configurable par
         * l'utilisateur. Cette fonction lui fournit un fichier au format
         * Ini (QSettings::IniFormat) renseignant tout ce qu'elle a besoin
         * 
         *  - @b Distributions : liste des distributions gérées (séparées
         *    par des espaces
         *  - @b Languages : liste des langues (fichiers translate.x) gérées
         *  - @b WebsiteIntegration : @b true s'il faut également modifier
         *    des tables du Site Web Logram (pages de wiki pour les
         *    descriptions, topics de commentaires, etc)
         *  - @b Database/Hostname : nom d'hôte du serveur de base de donnée
         *    utilisé (MySQL seulement pour le moment)
         *  - @b Database/Name : nom de la base de donnée
         *  - @b Database/User : nom d'utilisateur
         *  - @b Database/Password : mot de passe
         *  - @b Sign/Enabled : @b true pour activer la signature
         *  - @b Sign/Key : si nécessaire, ID de la clef GPG à utiliser
         *    (nombre court de quelques lettres et chiffres)
         * 
         * @param fileName Nom du fichier de configuration
         * @return True si tout s'est bien passé (on se connecte à MySQL dans cette fonction), false sinon
         */
        bool loadConfig(const QString &fileName);
        
        /**
         * @brief Inclus un paquet binaire
         * 
         * Copie un paquet dans le dossier @b pool/ et l'enregistre en base
         * de donnée.
         * 
         * Tables touchées :
         * 
         *  - @b packages_package : insertion ou mise à jour
         *  - @b packages_file : suppression des enregistrement de ce paquet,
         *    puis ajout des nouveaux
         *  - @b packages_directory : ajout des dossiers encore inconnus
         *    jusqu'à présent
         *  - @b wiki_page : si @b websiteIntegration, mise à jour ou ajout
         *    d'une page pour ce paquet.
         *  - @b wiki_logentry : Ajout d'un enregistrement, author_ip =
         *    "Setup import" (hack, author_ip est un VARCHAR). Seulement si
         *    @b websiteIntegration.
         *  - @b packages_changelog : Ajout si nécessaire des enregistrement
         *    trouvés dans metadata.xml mais pas dans la BDD
         *  - @b packages_string : synchronisation des descriptions longues
         *    et courtes, ainsi que du titre.
         * 
         * Les tables @b packages_distribution, @b packages_arch et
         * @b packages_section doivent contenir les éléments nécessaires au
         * paquet.
         * 
         * @param fileName Nom du fichier du paquet (*.lpk)
         * @return True si tout s'est bien passé, false sinon.
         */
        bool includePackage(const QString &fileName);
        
        /**
         * @brief Inclus un paquet source
         * 
         * Copie un paquet source dans le dossier @b pool/ et effectue
         * des opérations en base de donnée
         * 
         * Tables touchées :
         * 
         *  - @b packages_sourcepackage : Ajout d'un enregistrement si
         *    nécessaire
         *  - @b packages_sourcelog : Ajout d'un enregistrement, flag
         *    SOURCEPACKAGE_FLAG_MANUAL positionné. Un enregistrement
         *    par architecture.
         * 
         * Les tables @b packages_distribution et @b packages_arch
         * doivent être remplies correctement.
         * 
         * @param fileName Nom du fichier du paquet source (*.src.lpk)
         * @param appendHistory True s'il faut créer un enregistrement dans sourcelog, false sinon (utilisé par le serveur de construction qui insère lui-même son enregistrement).
         * @return True si tout s'est bien passé, false sinon.
         */
        bool includeSource(const QString &fileName, bool appendHistory = true);
        
        /**
         * @brief Exporte les dépôts
         * 
         * Créer les fichiers dans @b dists, en fonction du contenu de la
         * base de donnée.
         * 
         * @param distros Liste des distributions à exporter, liste vide pour toutes.
         */
        bool exp(const QStringList &distros);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#define SOURCEPACKAGE_FLAG_LATEST               0b00000001  /*!< Dernière version du paquet dans sa distribution */
#define SOURCEPACKAGE_FLAG_MANUAL               0b00000010  /*!< Importation manuelle */
#define SOURCEPACKAGE_FLAG_FAILED               0b00000100  /*!< Construction ratée */
#define SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG   0b00001000  /*!< Demande au serveur de construction d'ajouter une entrée automatique au changelog */
#define SOURCEPACKAGE_FLAG_REBUILD              0b00010000  /*!< Reconstruction demandée */
#define SOURCEPACKAGE_FLAG_CONTINUOUS           0b00100000  /*!< Reconstruction continue : laisser SOURCEPACKAGE_FLAG_REBUILD après la reconstruction */
#define SOURCEPACKAGE_FLAG_WARNINGS             0b01000000  /*!< Des alertes ont été générées */
#define SOURCEPACKAGE_FLAG_BUILDING             0b10000000  /*!< Paquet en cours de construction */

#endif