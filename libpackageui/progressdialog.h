/*
 * progressdialog.h
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

#ifndef __PROGRESSDIALOG_H__
#define __PROGRESSDIALOG_H__

/**
 * @file progressdialog.h
 * @brief Boîte de dialogue des progressions
 */

#include <QHash>
#include <QDialog>

#include <packagesystem.h>

class QVBoxLayout;
class QPushButton;
class QTimer;

namespace LogramUi
{

class ProgressList;

/**
 * @brief Affichage des progressions
 * 
 * Cette classe permet d'afficher les progressions envoyées par PackageSystem
 * dans une boîte de dialogue, affichée seulement quand nécessaire.
 * 
 * @sa ProgressList
 */
class ProgressDialog : public QDialog
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param parent Parent
         */
        ProgressDialog(QWidget *parent);
        
        /**
         * @brief Destructeur
         */
        ~ProgressDialog();
        
        /**
         * @brief Ajoute une progression
         * 
         * Cette fonction peut être appelée pour ajouter toute progression
         * que vous décidez de faire gérer par cette classe.
         * 
         * @code
         * if (progress->action == Progress::Create)
         * {
         *     progressDialog->addProgress(progress);
         * }
         * else if (progress->action == Progress::Update)
         * {
         *     progressDialog->updateProgress(progress);
         * }
         * else
         * {
         *     progressDialog->endProgress(progress);
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
        
        bool canceled();        /*!< @brief Retourne true si le bouton «Annuler» a été pressé */
        
    private slots:
        void cancelClicked();
        void hideElapsed();
        void showElapsed();
        
    protected:
        void closeEvent(QCloseEvent *event);
        
    private:
        struct Private;
        Private *d;
};

}

#endif