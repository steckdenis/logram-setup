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
    @brief Gère les communications entre un la libpackages et le front-end
*/

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "templatable.h"

namespace Logram
{

/**
    @brief Gère les communications entre la bibliothèque de gestion des paquets et son front-end
    
    Lorsqu'un paquet est installé, mis à jour ou supprimé, il peut avoir besoin
    d'informations. Pour les obtenir, la meilleure manière est de lancer une
    communication.
    
    Du côté du paquet, c'est une simple fonction bash qui s'en charge. Elle 
    envoie une chaîne de caractère spéciale dans la sortie du programme
    d'installation du paquet, et libpackage l'intercepte.
    
    Il s'en sert alors pour créer une Communication, facile à gérer en C++.
    
    @note Vous n'avez normalement pas à instancier cette classe. Vous en recevez une
    copie lorsque le signal PackageList::communication ou PackageSystem::communication
    est émis.
*/
class Communication : public Templatable
{
    public:
        /**
            @brief Constructeur par défaut
            @param parent QObject parent
            @internal
        */
        Communication(QObject *parent);
        
        /**
            @brief Permet de savoir si une erreur s'est produite
            @note PackageSystem::lastError est placé correctement
            @internal
            @return true si la classe est prête à être utilisée
        */
        virtual bool error() const = 0;
        
        /**
            @brief Type de communication
            
            Une communication peut avoir plusieurs types, qui doivent être
            gérés différamment par LPM.
        */
        enum Type
        {
            None,       /*!< @brief Pas de type, communication invalide */
            Question,   /*!< @brief Question, attend une entrée en retour (setValue à appeler) */
            Message     /*!< @brief Message, pas d'entrée nécessaire en retour */
        };
        
        /**
            @brief Origine de la communication
            
            Cette classe est abstraite et ne fait que relayer des communications
            venant soit d'un paquet, soit du la bibliothèque elle-même.
        */
        enum Origin
        {
            Package,    /*!< @brief Communication venant d'un paquet */
            System      /*!< @brief Communication venant du système */
        };
        
        /**
            @brief Retour attendu
            
            LPM supporte différent types de retours pour les communications.
            Ainsi, l'utilisateur peut bénéficier du widget graphique le plus
            approprié, des vérifications peuvent être faites, etc
        */
        enum ReturnType
        {
            Invalid,        /*!< @brief Invalide, communication invalide */
            String,         /*!< @brief Chaîne de caractère, n'oubliez pas d'appeler isEntryValid() */
            Integer,        /*!< @brief Entier */
            Float,          /*!< @brief Nombre flottant */
            SingleChoice,   /*!< @brief Choix d'un élément, utilisez enableChoice() et choicesCount() avec choice() */
            MultiChoice     /*!< @brief Choix de plusieurs éléments avec enableChoice() */
        };
        
        /**
            @brief Choix possible
            
            Une communication de type SingleChoice ou MultiChoice peut attendre
            de l'utilisateur une entrée bien spécifique. Un choix permet de
            savoir quelles sont les propositions qui conviennent.
        */
        struct Choice
        {
            QString title;  /*!< @brief Titre du choix, comme par exemple «Le lapin bleu» */
            QString value;  /*!< @brief Valeur du choix, par exemple «bluebunny» */
            bool selected;  /*!< @brief True si le choix est actuellement sélectionné (c'est le cas du choix par défaut avant une modification de la part du client) */
        };
        
        /**
            @brief Type de la communication
            
            Permet de savoir de quel type est une communication, le plus
            souvent Question ou Message
            
            @return Type de la communication
            @sa Type
        */
        virtual Type type() const = 0;
        
        /**
            @brief Type de retour d'une question
            
            LPM supporte différent types de retours pour les communications.
            Ainsi, l'utilisateur peut bénéficier du widget graphique le plus
            approprié, des vérifications peuvent être faites, etc
            
            @note Cette fonction renverra ReturnType::None si la communication
                  n'est pas une question
            
            @return Type de la valeur de retour
            @sa ReturnType
        */
        virtual ReturnType returnType() const = 0;
        
        /**
            @brief Origine de la communication
            
            Permet de savoir si la communication vient d'un paquet ou du système,
            ou autre, si futures expansions.
            
            @return Type de la communication
        */
        virtual Origin origin() const = 0;
        
        virtual QString title() const = 0;          /*!< @brief Titre de la communication */
        virtual QString description() const = 0;    /*!< @brief Description de la communication */
        
        /**
            @brief Valeur par défaut pour une chaîne
            
            Lorsque la communication est de type Type::Question et qu'elle
            attend un retour de type ReturnType::String, alors cette fonction
            renvoie la valeur par défaut pour le retour.
            
            @return Valeur par défaut pour une réponse de type chaîne de caractère
        */
        virtual QString defaultString() const = 0;
        
        /**
            @brief Valeur par défaut pour une réponse de type entier
            @return Valeur par défaut pour une réponse de type entier
        */
        virtual int defaultInt() const = 0;
        
        /**
            @brief Valeur par défaut pour une réponse de type flottant
            @return Valeur par défaut pour une réponse de type flottant
        */
        virtual double defaultDouble() const = 0;
        
        /**
            @brief Valeur par défaut pour un choix
            
            Renvoie l'index du choix par défaut lorsque la communication est
            une question et que son type de retour est 
            ReturnType::SingleChoice.
            
            @return Index du choix par défaut
            @sa choice
        */
        virtual int defaultIndex() const = 0;
        
        /**
            @brief Nombre de choix possibles
            
            Lorsque la communication est une question avec comme type de 
            retour soit ReturnType::SingleChoice soit
            ReturnType::MultiChoice, permet de connaître le nombre de
            choix disponibles.
            
            @return Nombre de choix possibles
            @sa choice
        */
        virtual int choicesCount() = 0;
        
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
        virtual Choice choice(int i) = 0;
        
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
        virtual void enableChoice(int i, bool enable) = 0;
        
        /** Définis la valeur de retour d'une question chaîne, n'oubliez pas isEntryValid() */
        virtual void setValue(const QString &value) = 0;
        virtual void setValue(int value) = 0;               /*!< @brief Définis la valeur de retour pour un entier */
        virtual void setValue(double value) = 0;            /*!< @brief Définis la valeur de retour pour un flottant */
        
        /**
            @brief Indique si l'entrée est valide
            
            Dans le cas d'une question de type chaîne, entier ou flottant, 
            la communication peut être valide ou non. Suivant le type de
            communication (Paquet ou Système), cette entrée peut être
            validée par un regex dans le paquet ou directement par la
            communication.
            
            @return True si l'entrée est acceptée, false sinon
            @sa entryValidationErrorString
        */
        virtual bool isEntryValid() const = 0;
        
        /**
            @brief Message d'erreur de validation
            
            Au cas où la validation grâce à isEntryValid() rate, cette fonction
            renvoie un message précisant la raison de l'échec de la validation.
            
            @return Message d'erreur de la validation
            @sa isEntryValid
        */
        virtual QString entryValidationErrorString() const = 0;
        
        /**
            @brief Valeur à renvoyer à helperscript
            @internal
            @return Représentation textuelle de la réponse à une question
        */
        virtual QString processData() const = 0;
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif