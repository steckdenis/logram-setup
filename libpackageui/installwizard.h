/*
 * installwizard.h
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

#ifndef __INSTALLWIZARD_H__
#define __INSTALLWIZARD_H__

/**
 * @file installwizard.h
 * @brief Assistant d'installation de paquets
 */

#include <QWizard>
#include <QVector>

namespace Logram
{
    class Solver;
    class PackageList;
    class PackageSystem;
    class Progress;
    class Package;
}

class ActionPage;
class BranchePage;
class LicensePage;
class InstallPage;
class DonePage;

namespace LogramUi
{

/**
 * @brief Message laissé par un paquet
 * 
 * La dernière page de l'assistant d'installation contient un récapitulatif
 * de tout ce qui s'est passé lors de l'installation.
 * 
 * Cette classe permet d'ajouter un message sur cette page. Ce sont généralement
 * les autres pages de l'assistant qui font cela, mais il est également possible
 * d'ajouter des messages généraux avant de l'afficher.
 */
struct PackageMessage
{
    Logram::Package *pkg;       /*!< @brief Paquet duquel vient le message, 0 pour général */
    QString title;              /*!< @brief Titre du message */
    QString message;            /*!< @brief Contenu du message */
};

/**
 * @brief Assistant d'installation de paquets
 * 
 * Cette classe propose à l'utilisateur un assistant d'installation de paquets.
 * 
 * Il suffit à l'application cliente de lui fournir un ensemble de paquets
 * sélectionnés, et InstallWizard se chargera de les montrer à l'utilisateur, 
 * lui faire valider les licences, choisir les dépendances, afficher les
 * progressions et finalement une page de conclusion avec les différents
 * messages laissés par les paquets ou par PackageSystem.
 */
class InstallWizard : public QWizard
{
    Q_OBJECT
    
    friend class ::ActionPage;
    friend class ::BranchePage;
    friend class ::LicensePage;
    friend class ::InstallPage;
    friend class ::DonePage;
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         * @param parent QWidget parent
         */
        InstallWizard(Logram::PackageSystem *ps, QWidget *parent);
        
        /**
         * @brief Destructeur
         */
        ~InstallWizard();
        
        void addPackage(Logram::Package *package);      /*!< @brief Ajoute un paquet à la liste */
        QVector<Logram::Package *> packages() const;    /*!< @brief Liste des paquets dans la liste */
        void addMessage(const PackageMessage &message); /*!< @brief Ajoute un message à la page de résumé */
        
        /**
         * @brief IDs des pages de l'assistant
         */
        enum PageIds
        {
            Actions,                                    /*!< @brief Résumé des actions (paquets à installer) */
            Branches,                                   /*!< @brief Choix des dépendances */
            Licenses,                                   /*!< @brief Acceptation des éventuelles licences */
            Install,                                    /*!< @brief Installation (affichage des progressions) */
            Done                                        /*!< @brief Page de résumé après installation */
        };
        
    private:
        void solverError();
        
        Logram::Solver *solver() const;
        Logram::PackageSystem *packageSystem() const;
        Logram::PackageList *packageList() const;
        
        void setSolver(Logram::Solver *solver);
        void setPackageList(Logram::PackageList *packageList);
        
        QList<PackageMessage> messages() const;
        
    private slots:
        void pageChanged(int id);
        
    private:
        struct Private;
        Private *d;
};

}

#endif
