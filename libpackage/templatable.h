/*
 * templatable.h
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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
 * @file templatable.h
 * @brief Petit moteur de templates
 */

#ifndef __TEMPLATABLE_H__
#define __TEMPLATABLE_H__

#include <QObject>
#include <QByteArray>

namespace Logram
{

/**
 * @brief Petit moteur de templates
 * 
 * Les templates permettent de remplacer des clefs par des valeurs dans des
 * chaînes de caractère. Ceci permet par exemple de ne pas coder en dur des
 * valeurs dans les fichiers metadata.xml
 * 
 * Les clefs sont de la forme {{clef}} :
 * 
 * @code
 * QString s = "Bonjour {{prenom}} !";
 * 
 * Templatable tpl(this);
 * 
 * tpl.addKey("prenom", "Logram");
 * qDebug() << tpl.templateString(s);
 * 
 * // Affiche "Bonjour Logram !"
 * @endcode
 */
class Templatable : public QObject
{
    public:
        /**
         * @brief Constructeur
         * @param parent QObject parent
         */
        Templatable(QObject *parent);
        
        /**
         * @brief Destructeur
         */
        virtual ~Templatable();
        
        /**
         * @brief Associe une valeur à une clef
         * 
         * Associe une valeur à une clef. Si cette fonction est appelée
         * plusieurs fois pour une même clef, la dernière valeur est utilisée.
         * 
         * @param key Clef
         * @param value Valeur
         */
        void addKey(const QString &key, const QString &value);
        
        /**
         * @brief Obtient la valeur d'une clef
         * @param key Clef
         * @return Valeur de la clef, ou QString() si non renseignée
         * @sa addKey
         */
        QString getKey(const QString &key) const;
        
        /**
         * @brief Permet de savoir si une clef existe
         * @param key Clef
         * @return True si la clef existe, false sinon
         */
        bool contains(const QString &key) const;
        
        /**
         * @brief Supprime une clef
         * @param key Nom de la clef
         */
        void removeKey(const QString &key);
        
        /**
         * @brief Formatte une chaîne de caratère
         * @param tpl Chaîne contenant le motif
         * @return @p tpl avec les clefs remplacées par les valeurs
         */
        QString templateString(const QString &tpl) const;
        
        /**
         * @overload
         */
        QByteArray templateString(const QByteArray &tpl) const;
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif