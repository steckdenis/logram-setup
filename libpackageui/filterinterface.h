/*
 * filterinterface.h
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

#ifndef __FILTERINTERFACE_H__
#define __FILTERINTERFACE_H__

/**
 * @file filterinterface.h
 * @brief Interface de filtrage des paquets
 */

#include <QObject>
#include <QRegExp>

namespace LogramUi
{

/**
 * @brief Interface de filtrage des paquets
 * 
 * Cette classe propose une interface générique pour filtrer des paquets.
 * Elle permet aux développeurs d'applications clientes de n'avoir à 
 * surveiller qu'un seul objet pour filtrer les paquets aussi bien par
 * nom (SearchBar) que par catégorie ou distribution (CategoryView).
 * 
 * Pour utiliser cette classe, il vous suffit de créer un objet de type
 * FilterInterface, de le passer en paramètre aux constructeurs des
 * interfaces de filtre que vous voulez utiliser, puis de connecter son
 * signal dataChanged().
 * 
 * @code
 * void init()
 * {
 *     filterInterface = new FilterInterface(this);
 *     connect(filterInterface, SIGNAL(dataChanged()), this, SLOT(searchPackages()));
 *    
 *     searchBar = new SearchBar(filterInterface, this);
 *     sections = new CategoryView(ps, filterInterface, this);
 * }
 * 
 * void searchPackages()
 * {
 *     // Utiliser filterInterface->regex() par exemple
 * }
 * @endcode
 */
class FilterInterface : public QObject
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param parent Objet parent
         */
        FilterInterface(QObject *parent);
        
        /**
         * @brief Destructeur
         */
        ~FilterInterface();
        
        /**
         * @brief Filtres en fonction du status des paquets
         * 
         * Cette énumération permet de filtrer les paquets en fonction de leur
         * status, pour par exemple n'afficher que les paquets installés.
         */
        enum StatusFilter
        {
            NoFilter = 0,           /*!< @brief Ne filtre pas les paquets */
            Installed = 1,          /*!< @brief Afficher uniquement les paquets installés */
            NotInstalled = 2,       /*!< @brief Afficher uniquement les paquets non-installés */
            Updateable = 3,         /*!< @brief Afficher uniquement les paquets pouvant être mis à jour */
            Orphan = 4              /*!< @brief Afficher uniquement les paquets orphelins */
        };
        
        void setNamePattern(const QString &pattern);        /*!< @brief Définit le motif de recherche */
        void setNameSyntax(QRegExp::PatternSyntax syntax);  /*!< @brief Définit le type de recherche */
        void setStatusFilter(StatusFilter filter);          /*!< @brief Définit le filtre en fonction du status des paquets */
        void setRepository(const QString &repository);      /*!< @brief Définit le dépôt dans lequel doivent se trouver les paquets */
        void setDistribution(const QString &distribution);  /*!< @brief Définit la distribution des paquets */
        void setSection(const QString &section);            /*!< @brief Définit la section des paquets */
        
        /**
         * @brief Déclenche la mise à jour des vues
         * 
         * Demande à FilterInterface d'émettre le signal dataChanged().
         */
        void updateViews();
        
        QRegExp regex() const;                              /*!< @brief Expression régulière de recherche */
        QRegExp::PatternSyntax nameSyntax() const;          /*!< @brief Syntaxe de cette expression */
        QString namePattern() const;                        /*!< @brief Motif de cette expression (QString() si pas de filtre) */
        StatusFilter statusFilter() const;                  /*!< @brief Filtre en fonction du status */
        QString repository() const;                         /*!< @brief Dépôt des paquets (QString() si pas de filtre) */
        QString distribution() const;                       /*!< @brief Distribution des paquets (QString() si pas de filtre) */
        QString section() const;                            /*!< @brief Section des paquets (QString() si pas de filtre) */
        QString statusName(StatusFilter status) const;      /*!< @brief Nom traduit du status */
        
    signals:
        /**
         * @brief Les données ont changé
         * 
         * Ce signal est émis si un widget de modification du filtre a appelé
         * la fonction updateViews(). Les vues doivent alors re-filtrer leurs
         * paquets.
         */
        void dataChanged();
        
    private:
        struct Private;
        Private *d;
};

}

#endif