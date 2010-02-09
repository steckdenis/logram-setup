/*
 * repopwdcommunication.h
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
    @file repopwdcommunication.h
    @brief Permet à l'utilisateur de saisir le mot de passe de la clef GPG d'un dépôt
*/

#ifndef __REPOCOMMUNICATION_H__
#define __REPOCOMMUNICATION_H__

#include "communication.h"

namespace Logram
{

/**
    @brief Permet à l'utilisateur de saisir le mot de passe de la clef GPG d'un dépôt
    
    Lors de l'exportation d'un dépôt, l'utilisateur doit signer certains fichiers.
    
    Il y a alors deux possibilités :
    
     - L'utilisateur utilie un agent GPG (Kleopatra, etc). Auquel cas, cet agent
       affiche une boîte de dialogue graphique dans laquelle il suffit d'entrer
       le mot de passe.
     - L'utilisateur n'utilise pas d'agent, auquel cas une communication est
       émise pour lui demander d'entrer ce mot de passe.
       
    Cette communication sert à couvrir le deuxième cas.
    
    @note Vous n'avez normalement pas à instancier cette classe. Vous en recevez une
    copie lorsque le signal PackageSystem::communication est émis.
*/
class RepoCommunication : public Communication
{
    public:
        /**
            @brief Constructeur par défaut
            @param parent QObject parent
            @internal
        */
        RepoCommunication(QObject *parent);
        ~RepoCommunication();       /*!< Destructeur */
        
        /**
            @brief Permet de savoir si une erreur s'est produite
            @return false
        */
        bool error() const;
        
        Communication::Type type() const;
        Communication::ReturnType returnType() const;
        
        /**
            @brief Origine de la communication
            
            Renvoie Communication::System
            
            @return Communication::System
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
            @note Renverra true même si le mot de passe est mauvais. Auqel cas,
                  la communication sera simplement réémise.
            @return true
        */
        bool isEntryValid() const;
        QString entryValidationErrorString() const;
        
        QString processData() const;
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif