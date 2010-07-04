/*
 * processthread.h
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
 * @file processthread.h
 * @brief Gestion des opérations sur les paquets
 */

#ifndef __PROCESSTHREAD_H__
#define __PROCESSTHREAD_H__

#include <QThread>
#include <QList>
#include <QByteArray>

namespace Logram
{

class Package;
class PackageSystem;
class PackageMetaData;

/**
 * @brief Gestion des opérations sur les paquets
 * 
 * Cette classe est un thread installant, supprimant ou mettant à jour
 * un paquet.
 */
class ProcessThread : public QThread
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         * @param pkg Package à installer, supprimer ou mettre à jour
         */
        ProcessThread(PackageSystem *ps, Package *pkg);
        
        /**
         * @brief Destructeur
         */
        ~ProcessThread();
        
        /**
         * @brief Lance l'opération
         * 
         * Lance l'opération, en se basant sur Package::action() pour savoir
         * que faire. Si installation ou mise à jour, le paquet doit déjà
         * avoir été téléchargé.
         */
        void run();
        
        bool error() const;     /*!< Retourne true en cas d'erreur, run() ne pouvant retourner de valeur */
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif