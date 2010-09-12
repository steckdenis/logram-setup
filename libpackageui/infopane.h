/*
 * infopane.h
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

#ifndef __INFOPANE_H__
#define __INFOPANE_H__

/**
 * @file infopane.h
 * @brief Affichage d'informations sur un paquet
 */

#include <QTabWidget>

class Ui_infoPane;

namespace LogramUi
{

class PackageDataProviderInterface;

/**
 * @brief Affichage d'informations sur un paquet
 * 
 * Ce widget affiche un ensemble d'onglets comprenant diverses informations sur
 * le paquet qu'on lui demande d'afficher.
 * 
 *  - Détails simples (nom, version, titre, versions disponibles, dépôt, etc)
 *  - Description longue
 *  - Dépendances
 *  - Historique
 *  - Fichiers (sous forme d'arbre, avec leurs flags)
 * 
 * Les informations sur le paquet sont obtenues à partir d'un
 * PackageDataProviderInterface , ce qui vous permet d'afficher des
 * informations même pour ce qui n'est pas vraiment un paquet.
 */
class InfoPane : public QTabWidget
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param parent Parent
         */
        InfoPane(QWidget *parent);
        
        /**
         * @brief Destructeur
         */
        ~InfoPane();
        
        /**
         * @brief Affiche les informations sur un paquet
         * @param data Fournisseur de données
         */
        void displayData(PackageDataProviderInterface *data);
        
        /**
         * @brief Définit s'il faut afficher le panneau des versions dans le premier onglet
         * 
         * Ce panneau peut être inutile si vous savez qu'il n'y aura toujours
         * qu'une seule version d'affichée.
         * 
         * @param show True pour afficher le paneau, false sinon.
         */
        void setShowVersions(bool show);
        
    private slots:
        void websiteActivated(const QString &url);
        void licenseActivated(const QString &url);
        void showFlags();
        
    private:
        struct Private;
        Private *d;
};

}

#endif