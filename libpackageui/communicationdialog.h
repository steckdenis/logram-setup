/*
 * communicationdialog.h
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

#ifndef __COMMUNICATIONDIALOG_H__
#define __COMMUNICATIONDIALOG_H__

/**
 * @file communicationdialog.h
 * @brief Affiche une communication
 */

#include <QDialog>
#include <QVector>

class QAbstractButton;

namespace Logram
{
    class Communication;
    class Package;
}

namespace LogramUi
{

/**
 * @brief Affiche une communication
 * 
 * Les communications de LPM peuvent être compliquées à gérer, surtout
 * graphiquement.
 * 
 * Cette boîte de dialogue se charge d'afficher à l'utilisateur la
 * communication devant être de type Logram::Communication::Question .
 * 
 * @code
 * void communication(Logram::Package *sender, Logram::Communication *comm)
 * {
 *     if (comm->type() != Communication::Question)
 *     {
 *         return;
 *     }
 *     
 *     CommunicationDialog dialog(sender, comm, this);
 *     
 *     dialog.exec();
 * }
 * @endcode
 * 
 * @note CommunicationDialog se charge d'appeler Logram::Communication::setValue.
 */
class CommunicationDialog : public QDialog
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param _pkg Paquet de la communication, celui passé en paramètre au slot @b communication
         * @param _comm Communication à afficher
         * @param parent Widget parent de la boîte de dialogue
         */
        CommunicationDialog(Logram::Package *_pkg, Logram::Communication *_comm, QWidget* parent = 0);
        
        /**
         * @brief Destructeur
         */
        ~CommunicationDialog();
        
    private slots:
        void updateValues();
        
    private:
        struct Private;
        Private *d;
};

}

#endif