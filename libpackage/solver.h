/*
 * solver.h
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

#ifndef __SOLVER_H__
#define __SOLVER_H__

#include <QObject>

namespace Logram
{

class Package;
class PackageSystem;
class DatabaseReader;
class PackageList;
struct PackageError;

class Solver : public QObject
{
    Q_OBJECT
    Q_ENUMS(Action)
    
    public:
        Solver(PackageSystem *ps, DatabaseReader *psd);
        ~Solver();

        enum Action
        {
            None = 0,
            Install = 1,
            Remove = 2,
            Purge = 3,
            Update = 4
        };
        
        struct Node;
        
        /**
            Un paquet peut ne pas pouvoir être retenu dans l'arbre des dépendances pour plusieurs raisons.
            
            Les Node trouvés par le solveur contiennent un champs @b error. S'il est à zéro, tout s'est
            bien passé. Sinon, une erreur de plusieurs type peut s'être produite.
        */
        struct Error
        {
            /** Type d'erreur, une seule valeur possible à la fois */
            enum Type
            {
                NoDeps,                             /*!< Dépendance manquante */
                Conflict,                           /*!< Conflit présent */
                InternalError,                      /*!< Erreur dans PackageSystem. Utilisez PackageSystem::lastError pour voir */
                ChildError,                         /*!< Un enfant du noeud a une erreur, ce qui le rend invalide. @b other est cet enfant */
                SameNameSameVersionDifferentAction, /*!< Deux actions différentes pour un même paquet */
                InstallSamePackageDifferentVersion, /*!< Même action pour deux versions différents d'un paquet */
                UninstallablePackageInstalled,      /*!< Paquet non-installable installé */
                UnremovablePackageRemoved,          /*!< Paquet non-supprimable supprimé */
                UnupdatablePackageUpdated           /*!< Paquet impossible à mettre à jour mis à jour. @b other est le noeud qui ne veut pas être mis à jour, l'ancienne versions (nous sommes la nouvelle version) */
            };
            
            Type type;              /*!< Type d'erreur */
            Node *other;            /*!< Noeud sujet à l'erreur. Peut être zéro si un ChildError porte sur le fait que toutes les variantes d'une dépendance ont échouées. */
            QString pattern;        /*!< Si NoDeps, motif de dépendance introuvable */
        };
        
        struct Node
        {
            enum Flag
            {
                None = 0,
                Wanted = 1,
                Weighted = 2,
                Proceed = 4,
                MinMaxWeighted = 8,
                WeightMin = 16,     // Utilisé par weightChildren
                MinMaxDone = 32,
                Explored = 64,      // Utilisé par exploreNode
            };
            
            Q_DECLARE_FLAGS(Flags, Flag)
            
            Package *package;
            Flags flags;
            Error *error;
            
            int minWeight, maxWeight, weight;
            int minDlSize, maxDlSize;
            int minInstSize, maxInstSize;
            
            Node *weightedBy;   // Permet de savoir si ce noeud a été pesé par un autre
            
            struct Child
            {
                int count;
                int minNode, maxNode, chosenNode; // choosenNode : Noeud que l'utilisateur a choisi
                
                // Si count = 1, on stocke simplement un pointeur sur l'enfant. Sinon, on stocke un pointeur
                // sur la liste des enfants.
                union
                {
                    Node *node;
                    Node **nodes;
                };
            };
            
            int nodeListIndex;      // Index dans nodeList quand on explore le graphe.
            
            // Liens
            int childcount, currentChild;   // currentChild : enfant courant dans l'exploration du graphe.
            Child *children;
        };

        void addPackage(const QString &nameStr, Action action);
        bool solve();
        bool weight();
        
        Node *root();
        
        bool beginList(bool &ended);
        bool continueList(int choice, bool &ended);
        QList<Node *> choices();
        bool upList();
        PackageList *list();
        Node *errorNode();
        Node *choiceNode();
        
        /**
            Définit si les dépendances des paquets doivent être prises en compte (@b true par défaut) 
        */
        void setUseDeps(bool enable);
        
        /**
            Définit si les paquets déjà installés/supprimés doivent être utilisés pour simplifier l'arbre (@b true par défaut) 
        */
        void setUseInstalled(bool enable);

    private:
        struct Private;
        Private *d;
};

} /* Namespace */

Q_DECLARE_OPERATORS_FOR_FLAGS(Logram::Solver::Node::Flags)

#endif