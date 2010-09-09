/*
 * utils.h
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

#ifndef __UTILS_H__
#define __UTILS_H__

/**
 * @file utils.h
 * @brief Utilitaires partagés pouvant être utilisés dans les GUI
 */

#include <QString>
#include <QByteArray>
#include <QPixmap>

#include <solver.h>

namespace Logram
{
    class PackageSystem;
}

namespace LogramUi
{

/**
 * @brief Utilitaires partagés pouvant être utilisés dans les GUI
 * 
 * Cette classe ne peut être instanciée mais contient un ensemble de fonctions
 * statiques utiles à la majorité des interfaces utilisateurs graphiques de
 * Logram.
 */
class Utils
{
    public:
        /**
         * @brief Pixmap lue à partir de données brutes
         * 
         * Cette fonction permet par exemple d'obtenir l'icône d'un paquet à
         * partir de son fichier metadata.xml.
         * 
         * @param data Données brutes, contenu d'un fichier PNG valide par exemple
         * @param width Largeur de l'image qu'on souhaite obtenir
         * @param height Heuteur de l'image
         * @return Image lue et redimensionnée
         */
        static QPixmap pixmapFromData(const QByteArray &data, int width, int height);
        
        /**
         * @brief Formatte un texte en Markdown
         * 
         * Les descriptions et entrées de changelog de Logram sont écrites en
         * Markdown. Cette fonction permet de formater un texte en Markdown
         * et retoure sa représentation en HTML.
         * 
         * @param source Texte en Markdown
         * @return Texte HTML
         */
        static QString markdown(const QString &source);
        
        /**
         * @brief Nom d'une action, infinitif
         * 
         * Cette fonction permet d'obtenir le nom traduit d'une fonction, sous
         * la forme d'un infinitif, comme «Installer» ou «Supprimer».
         * 
         * @param action Action dont le nom est demandé
         * @return Infinitif du verbe de cette action
         */
        static QString actionNameInf(Logram::Solver::Action action);
        
        /**
         * @brief Boîte de dialogue d'erreur
         * 
         * Quand une opération échoue dans la classe PackageSystem, la fonction
         * appelée retourne @b false, et PackageSystem::lastError a un résultat
         * non-nul.
         * 
         * Cette fonction crée une boîte de dialogue et l'affiche, avec le texte
         * d'erreur correspondant à ce qui s'est produit.
         * 
         * @param ps PackageSystem en état d'erreur
         */
        static void packageSystemError(Logram::PackageSystem *ps);
        
    private:
        Utils(){}
};

}

#endif