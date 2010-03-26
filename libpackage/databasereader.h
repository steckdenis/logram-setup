/*
 * databasereader.h
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
    @file databasereader.h
    @brief Lecteur de la base de donnée binaire
*/

#ifndef __PACKAGESYSTEM_P_H__
#define __PACKAGESYSTEM_P_H__

#include <QString>
#include <QList>
#include <QEventLoop>
#include <QHash>
#include <QRegExp>

#include "databaseformat.h"

class QFile;
class QNetworkAccessManager;
class QSettings;

namespace Logram
{
    
class PackageSystem;
class PackageFile;
class Package;

/**
    @brief Informations sur une mise à jour
*/
struct UpgradeInfo
{
    int installedPackage;   /*!< ID du paquet installé */
    int newPackage;         /*!< ID du paquet plus récent */
};

/**
    @brief Lis les informations de la base de donnée binaire
    
    Cette classe est le coeur de la partie "Gestion de la base de donnée"
    de Setup.
    
    Elle contient un ensemble de fonctions permettant de lire la base de
    donnée binaire utilisée par Setup.
    
    @section technical Aspect technique
    
    Comme expliqué dans databaseformat.h, la base de donnée binaire de
    Setup est découpée en 5 fichiers.
    
    Pour une plus grande performance et facilité de lecture, ces fichiers
    sont mappés en mémoire en utilise QFile::map(). Ainsi, les différents
    pointeurs utilisés partout dans ces fichiers (voir databaseformat.h)
    peuvent être très facilement transformés en pointeurs mémoire.
    
    Par exemple, récupérer la taille à télécharger d'un paquet est d'une
    complexité O(1) :
    
    @code
        // Récupérer le pointeur sur un _Package quand on connait son index
        _Package *DatabaseReader::package(int index)
        {
            // le fichier packages commence par un entier contenant le nombre
            // de paquets géré, vérifier qu'index est bien correct.
            if (index >= *(int *)m_packages)
            {
                return 0;
            }

            // Début de la liste des paquets
            uchar *pkg = m_packages;
            pkg += 4;                   // Sauter l'entier décrit ci-dessus.
            
            // Avancer au bon paquet
            pkg += (index * sizeof(_Package));

            return (_Package *)pkg;
        }
        
        _Package *pkg = package(index);
        
        cout << pkg->isize << endl;
    @endcode
    
    Et c'en est de même pour plein d'autres choses, comme le nom d'un paquet,
    ses dépendances, etc. L'immense majorité des opérations de DatabaseReader
    sont de complexité O(1), ou alors O(n) avec un n très petit (nombre de
    dépendances d'un paquet). Il n'y a qu'un seul O(n) où n est le nombre de
    paquets dans la distribution : explorer les paquets (pour en rechercher un,
    mettre à jour, etc).
    
    @section use Utilisation
    
    DatabaseReader est une classe facile à utiliser. Voici un simple exemple :
    
    @code
        DatabaseReader *dr = new DatabaseReader(ps);
        
        if (!dr->init())
        {
            handle_errors();
            return;
        }
        
        // Nom et version du dernier paquet, ainsi que description
        _Package *pkg = dr->package(dr->packages() - 1);
        
        cout << "Nom         : " << string(false, pkg->name) << endl;
        cout << "Version     : " << string(false, pkg->version) << endl;
        cout << "Description : " << string(true, pkg->desc) << endl;    // true : chaîne traduite
    @endcode
    
    Ce code a une complexité O(1), voir la section Aspect Technique.
    
    @warning Cette classe ne doit être utilisé <strong>que si vous savez
             ce que vous faites</strong> ! Utilisez absolument Package pour
             gérer les paquets, ainsi que les fonctions de faut niveau de
             PackageSystem. D'ailleurs, DatabasePackage n'est pas accessible
             en dehors de la liblpackage du fait que son adresse est stockée
             dans un membre privé de PackageSystem, et qu'en instancier une
             autre n'est pas supporte (lire : si ça plante, tant pis)
*/
class DatabaseReader
{
    public:
        /**
            @brief Construit un PackageSystem
            @param _ps PackageSystem utilisé
        */
        DatabaseReader(PackageSystem *_ps);
        ~DatabaseReader();  /*!< Destructeur */
        
        /**
            @brief Initialise le lecteur
            
            DatabaseReader doit être initialisé autre-part que dans le constructeur.
            
            Cette fonction ouvre les fichiers de la base de donnée et les mappe.
            
            @return true si tout s'est bien passé, false sinon
        */
        bool init();

        /**
            @brief Récupérer un paquet en fonction de son nom et de sa version
            
            Récupère le paquet ayant le bon nom et la bonne version et place son
            index dans @p rs.
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans la distribution
             
            @param name nom du paquet
            @param version version du paquet
            @param rs référence sur un entier qui recevra l'index du paquet
            @return true si tout s'est bien passé, sinon false
        */
        bool package(const QString &name, const QString &version, int &rs);
        
        /**
            @brief Récupère la liste de tous les paquets correspondant à une regex
            
            Place dans @p rs une liste d'entiers dont chaque élément est l'index
            d'un paquet dont le nom correspond à l'expression régulière @p regex.
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans la distribution.
                     
            @param regex Regex
            @param rs Référence sur une liste d'entiers qui recevra le résultat
            @return true si tout s'est bien passé, false sinon
        */
        bool packagesByName(const QRegExp &regex, QList<int> &rs);
        
        /**
            @brief Liste des paquets correspondant à une chaîne de version
            
            Renvoie la liste des paquets qui correspondent à la chaîne
            @p verStr, de la forme "nom", "nom=version", "nom>=version", etc.
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans la distribution.
            
            @param verStr chaîne de version
            @return Liste des paquets qui correspondent
        */
        QList<int> packagesByVString(const QString &verStr);
        
        /**
            @brief Liste des paquets correspondant à un nom, une version et une opération
            
            Plus rapide que la surcharge prendant une QString, il faut ici fournir le nom
            du paquet, sa version, et l'opération à appliquer.
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans la distribution.
                     
            @param name Le nom que doivent avoir les paquets renvoyés
            @param version La version que doivent avoir les paquets renvoyés, comparée avec @p op
            @param op Opération (=, >=, <=, >>, !=, etc)
            @return Liste des paquets qui correspondent
        */
        QList<int> packagesByVString(const QString &name, const QString &version, int op);
        
        /**
            @brief Retourne le fichier correspondant au nom name
            
            Retourne le PackageFile pour le fichier @p name.
            
            @note Cette fonction a une complexité de O(n) où n est le nombre
                  d'éléments dans path, séparés par / .
                  
            @param name nom du fichier à récupérer, sans le premier / .
            @return PackageFile correspondant au nom
        */
        PackageFile *file(const QString &name);
        
        /**
            @brief Crée un PackageFile pour file
            
            Retourne un PackageFile correspondant au fichier @p fl
            
            @note Cette fonction a une complexité de O(n) où n est le nombre
                  de dossiers parents du fichier.
                  
            @param fl _File représentant le fichier en base de donnée
            @param pkg paquet de ce fichier, si voulu (0 si pas utilisé)
            @param bindedpackage true si la suppression du PackageFile doit
                   entraîner la suppression de pkg.
            @return le PackageFile du fichier @p fl
        */
        PackageFile *file(_File *fl, Package *pkg, bool bindedpackage);
        
        /**
            @brief Retourne les fichiers correspondant à l'expression régulière
            
            Retourne la liste des fichiers dont le nom (le chemin d'accès n'est
            pas considéré) correspond à l'expression régulière regex.
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de fichiers dans la base de donnée. Elle peut être très
                     lente si la regex est complexe, ou s'il y a beaucoup de
                     fichiers (forte utilisation de QString::QString(), ainsi
                     que stress du système de fichiers).
                     
            @param regex Expression régulière
            @return Liste des fichiers dont le nom correspond au motif
        */
        QList<PackageFile *> files(const QRegExp &regex);
        
        /**
            @brief Liste des dépendances d'un paquet
            
            Renvoie la liste des dépendances d'un paquet dont l'index est donné.
            
            @note Cette fonction a une complexité de O(n), où n est le nombre
                  de dépendances qu'à ce paquet. Elle est donc très rapide.
                  
            @param pkgIndex index du paquet
            @return Liste des dépendances du paquet
        */
        QList<_Depend *> depends(int pkgIndex);
        
        /**
            @brief Paquets ayant un certain nom et correspondant à une version
            
            Fleuron de DatabaseReader, cette fonction est au coeur de la gestion
            des dépendances.
            
            Elle permet de trouver tous les paquets qui ont le nom @p nameIndex, 
            et qui ont une version correspondant à @p stringIndex et @p op.
            
            Un comportement intéressant de la base de donnée binaire est que les
            <em>provides</em> sont gérés par des enregistrements spéciaux du nom.
            
            @code
                DatabaseReader *dr = readerReadyToBeUsed();
                
                // Pour l'exemple, trouver les dépendances du premier paquet
                QList<_Depend *> deps = dr->depends(0);
                
                // Explorer les dépendances et afficher les paquets qui correspondent
                foreach (_Depend *dep, deps)
                {
                    QList<int> matching = dr->packagesOfString(dep->pkgver, dep->pkgname, dep->op);
                    
                    cout << "Une dépendance de type " << dep->type << " :" << endl;
                    
                    foreach (int index, matching)
                    {
                        _Package *pkg = dr->package(index);
                        
                        cout << dr->string(pkg->name) << " à la version " << dr->string(pkg->version)
                             << "est un paquet qui correspond" << endl;
                    }
                }
            @endcode
            
            Simple, non ? Cette fonction permet d'avoir un solveur de dépendances
            complet en moins de 1000 lignes.
            
            @note Cette fonction a une complexité de O(n) où n est le nombre
                  de paquets ayant le nom @p nameIndex. Elle est donc très rapide.
                  
            @param stringIndex index de la chaîne permettant de vérifier les versions
            @param nameIndex index de la chaîne de nom permettant de trouver les bons noms
            @param op Opération de comparaison
            @return Liste des paquets qui correspondent
        */
        QList<int> packagesOfString(int stringIndex, int nameIndex, int op);
        
        /**
            @brief Liste des paquets dont la version installée est différente de la version du dépôt
            @note C'est normalement une version plus récente dans les dépôts, sauf s'ils sont cassés.
                  Ne pas vérifier cela permet d'éviter un appel à compareVersions, lourd et lent.
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans le dépôt.
            @return Paquets qu'on peut mettre à jour
        */
        QList<UpgradeInfo> upgradePackages();
        
        /**
            @brief Liste des paquets orphelins
            
            Renvoie la liste des paquets qui ont été installés automatiquement en tant que dépendances,
            mais qui ne sont plus nécessaires à aucun paquet demandé par l'utilisateur
            
            @warning Cette fonction a une complexité de O(n) où n est le nombre
                     de paquets dans le dépôt
            @return Paquets orphelins
        */
        QList<int> orphans();
        
        int packages(); /*!< Nombre de paquets disponibles dans la base de donnée */

        /**
            @brief Chaîne ayant un certain index
            
            Retourne la chaîne ayant l'index @p index, dans le bon fichier
            @b strings (ou @b translate).
            
            Si @p translate vaut true, alors le fichier @b translate est
            utilisé. C'est le cas pour la description d'un paquet. Sinon,
            @b strings est utilisé.
            
            @note Cette fonction a une complexité O(1).
            
            @param translate true s'il faut utilisé les chaînes traduites
            @param index index de la chaîne
            @return pointeur sur le premier caractère de la chaîne, terminée par 0.
        */
        const char *string(bool translate, int index);
        
        /**
            @brief Nom d'un fichier dont on a le pointeur
            
            Retourne un pointeur sur la chaîne dans le fichier @b files, 
            généralement le nom du fichier, mais pourrait être autre-chose
            à l'avenir.
            
            @note Cette fonction a une complexité O(1)
            
            @param ptr pointeur sur la chaîne comme on trouve dans une
                       structure _File.
            @return pointeur sur le premier caractère de la chaîne, terminée par 0.
        */
        const char *fileString(int ptr);
        
        /**
            @brief Paquet ayant un certain index
            
            Retourne un pointeur sur le paquet ayant l'index @p index.
            
            @note La valeur de retour se trouve dans le fichier mappé.
                  Toute modification de ce paquet sera enregistrée dans
                  le fichier mappé à sa fermeture. C'est pratique pour
                  modifier les flags, mais il est facile de corrompre la
                  base de donnée.
                  
            @note Cette fonction a une complexité O(1).
            
            @param index index du paquet
            @return pointeur sur la structure _Package du paquet.
        */
        _Package *package(int index);
        
        /**
            @brief Fichier ayant un certain index
            
            Retourne le fichier dont l'index est @p index
            
            @note Cette fonction a une complexité de O(1). Le fichier
                  se trouve dans un fichier mappé en mémoire, toute
                  modification de la structure sera répercutée sur
                  le disque lorsque le constructeur de databasereader
                  sera appelé.
                  
            @param index index du fichier
            @return pointeur sur la structure _File du fichier.
        */    
        _File *file(int index);
        
        _Depend *depend(int32_t ptr);   /*!< Renvoie la dépendance pointée par @p ptr dans le fichier @b depends */
        
    private:
        bool mapFile(const QString &file, QFile **ptr, uchar **map);

    private:
        QFile *f_packages, *f_strings, *f_translate, *f_depends, *f_strpackages, *f_files;
        uchar *m_packages, *m_strings, *m_translate, *m_depends, *m_strpackages, *m_files;

        PackageSystem *ps;
};

} /* Namespace */

#endif