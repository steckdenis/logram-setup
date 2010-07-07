/*
 * packagelist.h
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
 * @file packagelist.h
 * @brief Gestion des listes de paquets et des opérations dessus
 */

#ifndef __PACKAGELIST_H__
#define __PACKAGELIST_H__

#include <QObject>
#include <QVector>
#include <QProcess>

#include "solver.h"

namespace Logram
{

class PackageSystem;
class Package;
class Communication;

/**
 * @brief Gestion des listes de paquets et des opérations dessus
 * 
 * Cette classe est utilisée par Solver pour vous fournir la liste des paquets
 * qui pourraient être installés, supprimés ou mis à jour.
 * 
 * Elle s'utilise comme une simple liste de Package . De plus, une fois les
 * paquets voulus insérés dans la liste, il est possible de lancer leur
 * installation.
 */
class PackageList : public QObject, public QVector<Package *>
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         */
        PackageList(PackageSystem *ps);
        
        /**
         * @brief Destructeur
         */
        ~PackageList();
        
        /**
         * @brief Ajoute un paquet à la liste
         * 
         * Équivalent à QList::add, mais effectue des opérations en plus sur
         * le paquet. Par exemple, cette fonction enregistre si ce paquet
         * nécessitera un redémarrage.
         * 
         * @param pkg Package à installer
         */
        void addPackage(Package *pkg);
        
        /**
         * @brief Spécifie si les paquets de cette liste seront supprimés à la suppression de la liste
         * 
         * Cette fonction permet de choisir si PackageList doit supprimer
         * (en utilisant @b delete) les paquets qu'elle contient. Par 
         * défaut, elle les supprime.
         * 
         * @param enable true si PackageList doit supprimer ses paquets à sa destruction.
         */
        void setDeletePackagesOnDelete(bool enable);
        
        /**
         * @brief Ajoute une ligne au fichier safeRemoves
         * 
         * Ce fichier contient des informations sur les fichiers qui devront
         * être installés ou supprimés après un redémarrage, une fois les
         * systèmes de fichiers montés, avant le démarrage des services.
         * 
         * Cette fonction permet d'y écrire des lignes depuis ProcessThread, 
         * de manière <em>thread-safe</em>.
         * 
         * @param line Ligne à ajouter au fichier, sans compter le retour à la ligne.
         */
        void appendSafeRemoveLine(const QByteArray &line);
        
        /**
         * @brief Paquet en cours d'installation
         * 
         * Cette fonction utilitaire permet de récupérer le dernier paquet
         * dont l'installation à commencé, pour par exemple afficher son
         * nom, sa description ou son icône.
         * 
         * @return Dernier Package dont l'installation a commencé.
         */
        Package *installingPackage() const;
        
        /**
         * @brief Détermine si un redémarrage est nécessaire
         * 
         * Certains paquets peuvent nécessiter un redémarrage. Si l'un d'entre
         * eux est passé à addPackage, alors cette fonction renverra @b true.
         * 
         * Sinon, elle renvoie @b false
         * 
         * @return True si un redémarrage est demandé par au moins un paquet.
         */
        bool needsReboot() const;
        
        /**
         * @brief Nombre de licences à accepter
         * 
         * Certains paquets peuvent nécessiter l'approbation d'une licence par
         * l'utilisateur final. Cette fonction retourne le nombre de paquets
         * qui possèdent ce type de licence (généralement non-libre).
         * 
         * @return Nombre de paquets ayant une licence spéciale à accepter.
         */
        int numLicenses() const;
        
        /**
         * @brief Paquets rendus orphelins par cette liste
         * 
         * Si cette liste contient des suppressions ou des mises à jour, alors
         * elle peut rendre inutile d'anciennes dépendances.
         * 
         * Cette fonction renvoie la liste des ID des paquets rendus orphelins
         * par l'exécution de cette liste.
         * 
         * @return Liste des ID des paquets rendus orphelins par la liste.
         */
        QVector<int> orphans() const;
        
        /**
         * @brief Exécute la liste
         * 
         * Cette fonction non-bloquante lance le téléchargement et
         * l'installation des paquets.
         * 
         * @return True si tout s'est bien passé, false sinon.
         */
        bool process();
        
    private slots:
        void packageProceeded(bool success);
        void packageDownloaded(bool success);
        void communication(Logram::Package *pkg, Logram::Communication *comm);
        
        void triggerFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void triggerOut();
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif