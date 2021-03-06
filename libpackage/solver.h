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

/**
 * @file solver.h
 * @brief Solveur de dépendances
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

/**
 * @brief Solveur de dépendances
 * 
 * La partie la plus importante d'un gestionnaire de paquets est le solveur
 * de dépendances.
 * 
 * LPM en utilise un spécial, utilisant un hypergraphe des paquets, avec
 * une résolution des dépendances en trois étapes :
 * 
 *  - Construction de l'hypergraphe en explorant les dépendances des paquets
 *    en jeux.
 *  - Pesage de l'arbre, avec un script QtScript. Propagation du poids
 *    de noeuds en noeuds, avec les poids minimum et maximum
 *  - Exploration de l'arbre, seule étape avec une API publique
 *    (beginList() et autres)
 * 
 * Le résultat est un solveur extrêmement rapide, exact, personnalisable et
 * facile à utiliser pour l'utilisateur.
 */
class Solver : public QObject
{
    Q_OBJECT
    Q_ENUMS(Action)
    
    public:
        /**
         * @brief Constructeur
         * 
         * Constructeur de Solver. Ne doit pas êtr appelé directement. Utilisez
         * PackageSystem::newSolver().
         * 
         * @param ps PackageSystem utilisé
         * @param psd DatabaseReader permettant de lire la base de donnée LPM
         */
        Solver(PackageSystem *ps, DatabaseReader *psd);
        
        /**
         * @brief Destructeur
         */
        ~Solver();

        /**
         * @brief Action possible pour un paquet
         */
        enum Action
        {
            None = 0,       /*!< @brief Pas d'action */
            Install = 1,    /*!< @brief Installation */
            Remove = 2,     /*!< @brief Suppression */
            Purge = 3,      /*!< @brief Purge */
            Update = 4      /*!< @brief Mise à jour */
        };
        
        struct Node;
        
        /**
         * @brief Erreur dans un noeud
         * 
         * Un paquet peut ne pas pouvoir être retenu dans l'arbre des dépendances pour plusieurs raisons.
         * 
         * Les Node trouvés par le solveur contiennent un champs @b error. S'il est à zéro, tout s'est
         * bien passé. Sinon, une erreur de plusieurs type peut s'être produite.
         */
        struct Error
        {
            /** 
             * @brief Type d'erreur, une seule valeur possible à la fois 
             */
            enum Type
            {
                NoDeps,                             /*!< @brief Dépendance manquante */
                InternalError,                      /*!< @brief Erreur dans PackageSystem. Utilisez PackageSystem::lastError pour voir */
                ChildError,                         /*!< @brief Un enfant du noeud a une erreur, ce qui le rend invalide. @b other est cet enfant */
                SameNameSameVersionDifferentAction, /*!< @brief Deux actions différentes pour un même paquet */
                InstallSamePackageDifferentVersion, /*!< @brief Même action pour deux versions différents d'un paquet */
                UninstallablePackageInstalled,      /*!< @brief Paquet non-installable installé */
                UnremovablePackageRemoved,          /*!< @brief Paquet non-supprimable supprimé */
                UnupdatablePackageUpdated           /*!< @brief Paquet impossible à mettre à jour mis à jour. @b other est le noeud qui ne veut pas être mis à jour, l'ancienne versions (nous sommes la nouvelle version) */
            };
            
            Type type;              /*!< @brief Type d'erreur */
            Node *other;            /*!< @brief Noeud sujet à l'erreur. Peut être zéro si un ChildError porte sur le fait que toutes les variantes d'une dépendance ont échouées. */
            QString pattern;        /*!< @brief Si NoDeps, motif de dépendance introuvable */
        };
        
        /**
         * @brief Noeud dans l'arbre
         * 
         * L'arbre des dépendances contient des noeuds, qui sont en gros des
         * paquets (Package) avec des informations en plus.
         * 
         * La plupart sont internes et ne peuvent être utilisées, mais
         * certaines font partie de l'API publique.
         */
        struct Node
        {
            /**
             * @brief Flags du noeud
             * @internal
             */
            enum Flag
            {
                None = 0,               /*!< @brief Pas de flag */
                Wanted = 1,             /*!< @brief Paquet voulu, par exemple pas encore installé mais qu'on veut installer. Pas interne */
                Weighted = 2,           /*!< @brief Paquet pesé par le script QtScript */
                Proceed = 4,            /*!< @brief Inutilisé */
                MinMaxWeighted = 8,     /*!< @brief Protection de minMaxWeight */
                WeightMin = 16,         /*!< @brief Utilisé par weightChildren */
                MinMaxDone = 32,        /*!< @brief Protection de weightChildren */
                BeingExplored = 64,     /*!< @brief Utilisé par exploreNode */
                Explored = 128,         /*!< @brief Exploration terminée */
            };
            
            Q_DECLARE_FLAGS(Flags, Flag)
            
            Package *package;                   /*!< @brief Paquet, 0 si noeud principal (racine) */
            Flags flags;                        /*!< @brief Flags */
            Error *error;                       /*!< @brief Erreur, 0 si pas d'erreur */
            
            int minWeight;                      /*!< @brief Poids minimum */
            int maxWeight;                      /*!< @brief Poids maximum */
            int weight;                         /*!< @brief Poids brut renvoyé par QtScript */
            int minDlSize;                      /*!< @brief Taille de téléchargement minimale */
            int maxDlSize;                      /*!< @brief Taille de téléchargement maximale */
            int minInstSize;                    /*!< @brief Taille installée minimale */
            int maxInstSize;                    /*!< @brief Taille installée maximale */
            
            Node *weightedBy;                   /*!< @brief Permet de savoir si ce noeud a été pesé par un autre */
            
            /**
             * @brief Enfant d'un noeud
             * 
             * Les noeuds contiennent une liste d'enfants, ayant eux-même une
             * liste de noeuds. C'est ainsi qu'une dépendance (Child) peut
             * avoir plusieurs paquets lui convenant.
             */
            struct Child
            {
                int count;      /*!< @brief Nombre de noeuds enfants */
                int minNode;    /*!< @brief Noeud ayant le poids minimal */
                int maxNode;    /*!< @brief Noeud de poids maximal */
                int chosenNode; /*!< @brief Noeud que l'utilisateur a choisi, -1 si pas encore choix */
                
                /**
                 * Si count = 1, on stocke simplement un pointeur sur l'enfant. Sinon, on stocke un pointeur
                 * sur la liste des enfants.
                 */
                union
                {
                    Node *node;
                    Node **nodes;
                };
            };
            
            int nodeListIndex;                  /*!< @brief Index dans nodeList quand on explore le graphe. */
            
            // Liens
            int childcount;                     /*!< @brief Nombre d'enfants */
            int currentChild;                   /*!< @brief currentChild : enfant courant dans l'exploration du graphe. Permet de reprendre une exploration interrompue par un choix */
            Child *children;                    /*!< @brief Enfants */
        };

        /**
         * @brief Ajoute un paquet
         * 
         * Ajoute un paquet dans la liste des paquets à traiter, en premier
         * niveau.
         * 
         * @param nameStr Nom du paquet, de type name>=version (ou =, <=, !=, etc).
         * @param action Action voulue pour le paquet
         */
        void addPackage(const QString &nameStr, Action action);
        
        /**
         * @brief Résoud les dépendances
         * @return True si tout s'est bien passé, false sinon. errorNode() est placé correctement.
         */
        bool solve();
        
        /**
         * @brief Pèse l'arbre
         * 
         * Appelé normalement juste après solve(), pèse l'arbre contruit par
         * solve().
         * 
         * @return True si tout s'est bien passé, false sinon. errorNode() est placé correctement.
         */
        bool weight();
        
        /**
         * @brief Noeud racine de l'arbre
         * @internal
         */
        Node *root();
        
        /**
         * @brief Commence l'exploration de l'arbre
         * 
         * Une fois solve() et weight() appelés, il faut récupérer les
         * résultats.
         * 
         * Cette fonction commence l'exploration. @p ended est placé
         * à true s'il n'y a pas de choix. list() retourne la liste
         * des paquets devant actuellement être traités.
         * 
         * Si @p ended est placé à false, c'est qu'un choix est nécessaire.
         * Dans ce cas, appelez choices() puis continueList().
         * 
         * @code
         * Solver *solver = foo();
         * 
         * solver->addPackage(...);
         * solver->solve();
         * solver->weight();
         * 
         * bool ended = true;
         * solver->beginList(ended);
         * 
         * while (!ended)
         * {
         *     QVector<Node *> choices = solver->choices();
         *     // Poser la question à l'utilisateur
         *     solver->continueList(index_choix_retenu, ended);
         * }
         * 
         * PackageList *packages = solver->list();
         * @endcode
         * 
         * @param ended Placé à false si un choix se produit
         * @return True si tout s'est bien passé, false sinon
         * @sa continueList
         * @sa upList
         * @sa choices
         * @sa list
         */
        bool beginList(bool &ended);
        
        /**
         * @brief Continue une liste après un choix
         * @param choice Index dans list() du choix retenu
         * @param ended Placé à false si un choix se produit
         * @return True si tout s'est bien passé
         * @sa beginList
         */
        bool continueList(int choice, bool &ended);
        
        /**
         * @brief Liste des choix possible
         * 
         * Si beginList() ou continueList() place @b ended à false, alors
         * cette fonction retournera une liste de noeuds permettant de
         * satisfaire une dépendance. choiceNode() permer d'obtenir des
         * informations sur ce noeud ayant une dépendance permettant un
         * choix.
         * 
         * @return Liste des noeuds satisfaisant une dépendance
         * @sa choiceNode
         */
        QVector<Node *> choices();
        
        /**
         * @brief Remonte dans la liste
         * 
         * continueList() descend dans l'arbre, de choix en choix. Cette
         * fonction permet d'annuler un précédant choix. Elle retourne
         * @b false s'il n'y avait plus de choix à annuler.
         * 
         * @return True si remonter est possible, false sinon
         */
        bool upList();
        
        /**
         * @brief Liste des paquets actuellement en passe d'être installés ou supprimés
         * 
         * Cette fonction est disponible que @b ended soit à true ou false.
         * Si @b ended est à false, cette liste ne peut être procédée sans
         * casser le système, mais permet de voir ce qu'on a déjà dans la
         * liste.
         * 
         * @return Paquets actuellement dans la liste
         */
        PackageList *list();
        
        Node *errorNode();      /*!< @brief Noeud ayant une erreur, si une fonction renvoie @b false */
        Node *choiceNode();     /*!< @brief Noeud ayant une erreur, voir beginList() */
        
        /**
         * @brief Définit si les dépendances des paquets doivent être prises en compte (@b true par défaut) 
         */
        void setUseDeps(bool enable);
        
        /**
         * @brief Définit si les paquets déjà installés/supprimés doivent être utilisés pour simplifier l'arbre (@b true par défaut) 
         */
        void setUseInstalled(bool enable);
        
        /**
         * @brief Définit si les paquets suggérés par ceux que l'utilisateur veut (pas leurs dépendances) doivent être installés
         */
        void setInstallSuggests(bool enable);

    private:
        struct Private;
        Private *d;
};

} /* Namespace */

Q_DECLARE_OPERATORS_FOR_FLAGS(Logram::Solver::Node::Flags)

#endif