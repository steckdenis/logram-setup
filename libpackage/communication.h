/*
 * communication.h
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
    @file communication.h
    @brief Gère les communications entre un paquet et Setup
*/

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "templatable.h"

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
    copie lorsque le signal Package::communication est émis.
*/
class Communication : public Templatable
{
    public:
        /**
            @brief Constructeur par défaut
            @param ps PackageSystem à utiliser
            @param md Métadonnées du paquet à gérer
            @param name Nom de la communication lancée
            @internal
        */
        Communication(PackageSystem *ps, PackageMetaData *md, const QString &name);
        ~Communication();       /*!< Destructeur */
        
        /**
            @brief Permet de savoir si une erreur s'est produite
            
            Renvoie @b false si le constructeur a échoué. Sinon, @b true est
            renvoyé, ce qui veut dire que la classe est prête à être utilisée.
            
            @note PackageSystem::lastError est placé correctement
            @internal
            @return true si la classe est prête à être utilisée
        */
        bool error() const;
        
        /**
            @brief Type de communication
            
            Une communication peut avoir plusieurs types, qui doivent être
            gérés différamment par Setup.
        */
        enum Type
        {
            None,       /*!< Pas de type, communication invalide */
            Question,   /*!< Question, attend une entrée en retour (setValue à appeler) */
            Message     /*!< Message, pas d'entrée nécessaire en retour */
        };
        
        /**
            @brief Retour attendu
            
            Setup supporte différent types de retours pour les communications.
            Ainsi, l'utilisateur peut bénéficier du widget graphique le plus
            approprié, des vérifications peuvent être faites, etc
        */
        enum ReturnType
        {
            Invalid,        /*!< Invalide, communication invalide */
            String,         /*!< Chaîne de caractère, n'oubliez pas d'appeler isEntryValid() */
            Integer,        /*!< Entier */
            Float,          /*!< Nombre flottant */
            SingleChoice,   /*!< Choix d'un élément, utilisez enableChoice() et choicesCount() avec choice() */
            MultiChoice     /*!< Choix de plusieurs éléments avec enableChoice() */
        };
        
        /**
            @brief Choix possible
            
            Une communication de type SingleChoice ou MultiChoice peut attendre
            de l'utilisateur une entrée bien spécifique. Un choix permet de
            savoir quelles sont les propositions qui conviennent.
        */
        struct Choice
        {
            QString title;  /*!< Titre du choix, comme par exemple «Le lapin bleu» */
            QString value;  /*!< Valeur du choix, par exemple «bluebunny» */
            bool selected;  /*!< True si le choix est actuellement sélectionné (c'est le cas du choix par défaut avant une modification de la part du client) */
        };
        
        /**
            @brief Type de la communication
            
            Permet de savoir de quel type est une communication, le plus
            souvent Question ou Message
            
            @return Type de la communication
            @sa Type
        */
        Type type() const;
        
        /**
            @brief Type de retour d'une question
            
            Setup supporte différent types de retours pour les communications.
            Ainsi, l'utilisateur peut bénéficier du widget graphique le plus
            approprié, des vérifications peuvent être faites, etc
            
            @note Cette fonction renverra ReturnType::None si la communication
                  n'est pas une question
            
            @return Type de la valeur de retour
            @sa ReturnType
        */
        ReturnType returnType() const;
        
        QString title() const;          /*!< Titre de la communication */
        QString description() const;    /*!< Description de la communication */
        
        /**
            @brief Valeur par défaut pour une chaîne
            
            Lorsque la communication est de type Type::Question et qu'elle
            attend un retour de type ReturnType::String, alors cette fonction
            renvoie la valeur par défaut pour le retour.
            
            @return Valeur par défaut pour une réponse de type chaîne de caractère
        */
        QString defaultString() const;
        
        /**
            @brief Valeur par défaut pour une réponse de type entier
            @return Valeur par défaut pour une réponse de type entier
        */
        int defaultInt() const;
        
        /**
            @brief Valeur par défaut pour une réponse de type flottant
            @return Valeur par défaut pour une réponse de type flottant
        */
        double defaultDouble() const;
        
        /**
            @brief Valeur par défaut pour un choix
            
            Renvoie l'index du choix par défaut lorsque la communication est
            une question et que son type de retour est 
            ReturnType::SingleChoice.
            
            @return Index du choix par défaut
            @sa choice
        */
        int defaultIndex() const;
        
        /**
            @brief Nombre de choix possibles
            
            Lorsque la communication est une question avec comme type de 
            retour soit ReturnType::SingleChoice soit
            ReturnType::MultiChoice, permet de connaître le nombre de
            choix disponibles.
            
            @return Nombre de choix possibles
            @sa choice
        */
        int choicesCount();
        
        /**
            @brief Retourne un choix pour la question
            
            Lorsque la communication est une question avec comme type de 
            retour soit ReturnType::SingleChoice soit
            ReturnType::MultiChoice, retourne un objet Choice représentant
            le choix d'index @p i
            
            @code
                Choice c = comm->choice(comm->choicesCount());
                
                cout << '[' << c.value << "] " << c.title << endl;
                
                // Affiche par exemple :
                // [bluebunny] Le lapin bleu
            @endcode
            
            @param i index du choix
            @return Choix d'index @p i
        */
        Choice choice(int i);
        
        /**
            @brief Active ou non un choix
            
            Active ou non le choix d'index @p i.
            
            Dans le cas d'une question de type SingleChoice, l'activation
            d'un choix désactive automatiquement les autres, vous n'avez pas
            à le faire.
            
            Dans le cas d'une question de type MultiChoice, plusieurs choix
            peuvent être activés.
            
            Désactiver tous les choix n'est pas supporté.
            
            @note Pour les développeurs de paquets : ajoutez un choix «Aucun»
                  si votre paquet supporte que l'utilisateur ne réponde pas.
                  Ainsi, il est certain qu'il peut ne pas répondre, c'est plus
                  ergonomique.
                  
            @param i index du choix
            @param enable true s'il faut sélectionner le choix, false pour le désélectionner
        */
        void enableChoice(int i, bool enable);
        
        void setValue(const QString &value);    /*!< Définis la valeur de retour d'une question chaîne, n'oubliez pas isEntryValid() */
        void setValue(int value);               /*!< Définis la valeur de retour pour un entier */
        void setValue(double value);            /*!< Définis la valeur de retour pour un flottant */
        
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
        
        /**
            @brief Valeur à renvoyer à helperscript
            @internal
            @return Représentation textuelle de la réponse à une question
        */
        QString processData() const;
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif