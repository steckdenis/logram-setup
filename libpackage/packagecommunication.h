/*
 * packagecommunication.h
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
    @file packagecommunication.h
    @brief Gère les communications entre un paquet et Setup
*/

#ifndef __PACKAGECOMMUNICATION_H__
#define __PACKAGECOMMUNICATION_H__

#include "communication.h"

namespace Logram
{
    
class PackageSystem;
class PackageMetaData;

/**
    @brief Gère les communications entre un paquet et Setup
    
    Lorsqu'un paquet est installé, mis à jour ou supprimé, il peut avoir besoin
    d'informations. Pour les obtenir, la meilleure manière est de lancer une
    communication.
    
    Du côté du paquet, c'est une simple fonction bash qui s'en charge. Elle 
    envoie une chaîne de caractère spéciale dans la sortie du programme
    d'installation du paquet, et libpackage l'intercepte.
    
    Il s'en sert alors pour créer une Communication, facile à gérer en C++.
    
    @note Vous n'avez normalement pas à instancier cette classe. Vous en recevez une
    copie lorsque le signal PackageList::communication est émis.
*/
class PackageCommunication : public Communication
{
    public:
        /**
            @brief Constructeur par défaut
            @param ps PackageSystem à utiliser
            @param md Métadonnées du paquet à gérer
            @param name Nom de la communication lancée
            @internal
        */
        PackageCommunication(PackageSystem *ps, PackageMetaData *md, const QString &name);
        ~PackageCommunication();       /*!< Destructeur */
        
        /**
            @brief Permet de savoir si une erreur s'est produite
            
            Renvoie @b false si le constructeur a échoué. Sinon, @b true est
            renvoyé, ce qui veut dire que la classe est prête à être utilisée.
            
            @note PackageSystem::lastError est placé correctement
            @internal
            @return true si la classe est prête à être utilisée
        */
        bool error() const;
        
        Communication::Type type() const;
        Communication::ReturnType returnType() const;
        
        /**
            @brief Origine de la communication
            
            Renvoie Communication::Package
        */
        Communication::Origin origin() const;
        
        QString title() const;
        QString description() const;
        
        QString defaultString() const;
        int defaultInt() const;
        double defaultDouble() const;
        int defaultIndex() const;
        
        
        int choicesCount();
        Communication::Choice choice(int i);
        void enableChoice(int i, bool enable);
        
        void setValue(const QString &value);
        void setValue(int value);
        void setValue(double value);
        
        /**
            @brief Indique si l'entrée est valide
            
            Dans le cas d'une question de type chaîne, entier ou flottant, le
            créateur du paquet peut fournir un ensemble de règles à vérifier :
            
             - @b regex : Pour une chaîne, vérifier qu'elle corresponde bien
                          à l'expression régulière de type Perl (les plus
                          puissantes
             - @b min   : Permet de s'assurer que les nombres entrés sont plus
                          grands qu'une certaine valeur. Pour les chaînes de
                          caractère, c'est la taille minimum
             - @b max   : Même chose que pour min, mais s'assure que les
                          sont plus petits qu'une valeur, et une chaîne moins
                          longue
                          
            Le créateur du paquet fournit également un message d'erreur,
            accessible grâce à entryValidationErrorString().
            
            @return True si l'entrée est acceptée, false sinon
            @sa entryValidationErrorString
        */
        bool isEntryValid() const;
        
        /**
            @brief Message d'erreur de validation
            
            Au cas où la validation grâce à isEntryValid() rate, cette fonction
            renvoie un message créé par l'empaqueteur permettant d'expliquer à
            l'utilisateur ce qu'il a mal fait.
            
            @return Message d'erreur de la validation
            @sa isEntryValid
        */
        QString entryValidationErrorString() const;
        
        QString processData() const;
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif