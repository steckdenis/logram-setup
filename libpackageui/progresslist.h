/*
 * progresslist.h
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

#ifndef __PROGRESSLIST_H__
#define __PROGRESSLIST_H__

/**
 * @file progresslist.h
 * @brief Liste des progressions
 */

#include <QWidget>

namespace Logram
{
    struct Progress;
}

namespace LogramUi
{

/**
 * @brief Liste des progressions
 * 
 * Les progressions dans Setup sont puissantes mais un peu complexes à gérer
 * pour les applications clientes.
 * 
 * Ce widget permet d'afficher un ensemble de barres de progressions en
 * fonction des progressions envoyées par le PackageSystem.
 * 
 * Pour les téléchargements, la vitesse de téléchargement et la taille
 * téléchargée/restante est affichée.
 * 
 * @sa ProgressDialog
 */
class ProgressList : public QWidget
{
    Q_OBJECT
    public:
        /**
         * @brief Constructeur
         * @param parent Parent
         */
        ProgressList(QWidget *parent);
        
        /**
         * @brief Destructeur
         */
        ~ProgressList();
        
        /**
         * @brief Ajoute une progression
         * 
         * Cette fonction peut être appelée pour ajouter toute progression
         * que vous décidez de faire gérer par cette classe.
         * 
         * @code
         * if (progress->action == Progress::Create)
         * {
         *     progressList->addProgress(progress);
         * }
         * else if (progress->action == Progress::Update)
         * {
         *     progressList->updateProgress(progress);
         * }
         * else
         * {
         *     progressList->endProgress(progress);
         * }
         * @endcode
         * 
         * @warning Toute progression qui sera passée comme paramètre à
         *          updateProgress() ou endProgress() doit d'abord avoir
         *          été envoyée à cette fonction.
         * 
         * @param progress Progression à ajouter
         */
        void addProgress(Logram::Progress *progress);
        
        /**
         * @brief Mettre à jour une progression
         * @param progress Progression
         */
        void updateProgress(Logram::Progress *progress);
        
        /**
         * @brief Termine une progression
         * @param progress Progression
         */
        void endProgress(Logram::Progress *progress);
        
        int count() const;      /*!< @brief Nombre de progressions gérées */
        void clear();           /*!< @brief Efface toutes les progressions */
        
    private:
        struct Private;
        Private *d;
};

}

#endif