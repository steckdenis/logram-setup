/*
 * categoryview.h
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

#ifndef __CATEGORYVIEW_H__
#define __CATEGORYVIEW_H__

/**
 * @file categoryview.h
 * @brief Affichage des catégories et distributions
 */

#include <QToolBox>

class QTreeWidget;

namespace Logram
{
    class PackageSystem;
}

namespace LogramUi
{

class FilterInterface;


/**
 * @brief Affichage des catégories et distributions
 * 
 * Ce widget, découpé en deux parties, permet de filtrer les paquets en
 * fonction de leur section et de leur distribution.
 * 
 * La liste des sections est affichée avec leur icône, titre et
 * description.
 * 
 * Ces informations sont obtenues à partir des fichiers de métadonnées
 * des dépôts. Appelez Logram::PackageSystem::update() avec comme paramètre
 * Logram::PackageSystem::Sections en plus de Logram::PackageSystem::Minimal.
 */
class CategoryView : public QToolBox
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         * @param interface Interface de filtrage
         * @param parent QWidget parent
         */
        CategoryView(Logram::PackageSystem *ps, FilterInterface *interface, QWidget* parent = 0);
        
        /**
         * @brief Destructeur
         */
        ~CategoryView();
        
        /**
         * @brief Efface le contenu des listes et relit les fichiers de métadonnées
         * 
         * Cette fonction doit être appelée après Logram::PackageSystem::update,
         * si votre application appelle cette fonction. Cela permet de garder
         * le widget synchronisé avec les dépôts.
         */
        void reload();
        
        QTreeWidget *sections();        /*!< @brief Arbre des sections */
        QTreeWidget *distributions();   /*!< @brief Arbre des distributions */
        
        QString sectionTitle(const QString &name) const;        /*!< @brief Titre de la section donnée */
        QString sectionDescription(const QString &name) const;  /*!< @brief Description de la section donnée */
        QIcon sectionIcon(const QString &name) const;           /*!< @brief Icône de la section donnée */
        
    private slots:
        void updateFilter();
        
    private:
        struct Private;
        Private *d;
};

}

#endif