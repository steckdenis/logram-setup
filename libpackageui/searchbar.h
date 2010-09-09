/*
 * searchbar.h
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

#ifndef __SEARCHBAR_H__
#define __SEARCHBAR_H__

/**
 * @file searchbar.h
 * @brief Barre de recherche
 */

#include <QWidget>

class Ui_searchBar;
class QAction;

namespace LogramUi
{

class FilterInterface;

/**
 * @brief Barre de recherche
 * 
 * Ce widget permet à l'utilisateur d'entrer un mot ou une expression régulière
 * ainsi que de filtrer les paquets suivant leur état.
 * 
 * Il s'utilise avec un objet de type FilterInterface.
 */
class SearchBar : public QWidget
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param interface Interface de recherche utilisée
         * @param parent Parent
         */
        SearchBar(FilterInterface *interface, QWidget *parent);
        
        /**
         * @brief Destructeur
         */
        ~SearchBar();
        
        /**
         * @brief Place le focus clavier sur la zone de saisie du mot
         */
        void setFocus();
        
    private slots:
        void updateFilter();
        void actionTriggered(QAction *action);
        
    private:
        struct Private;
        Private *d;
};

}

#endif