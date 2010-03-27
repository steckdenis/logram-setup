/*
 * databasepackage.h
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
    @file filepackage.h
    @brief Paquet venant d'un fichier .tlz local
*/

#ifndef __FILEPACKAGE_H__
#define __FILEPACKAGE_H__

#include "solver.h"
#include "package.h"

namespace Logram
{

class PackageSystem;
class DatabaseReader;
    
class FileDepend;

/**
    @brief Paquet venant d'un fichier .tlz local
    
    Cette classe permet d'installer un paquet local directement depuis Setup.
    
    Elle permet également de gérer le paquet en tant qu'archive .tlz, et
    contient les fonctions nécessaires pour lire ses métadonnées, comprendre
    son format, etc.
    
    RepositoryManager se sert de cette classe pour importer les paquets
    
    @note Ses membres sont les mêmes que Package, et ne sont donc pas 
        documentés, à l'exception de certains qui ont des paramètres
        différents
*/
class FilePackage : public Package
{
    Q_OBJECT
    
    public:
        /**
            @brief Constructeur
            @param fileName Nom du fichier .tlz
            @param ps PackageSystem utilisé
            @param psd Lecteur de base de donnée utilisé
            @param _action Action, passé à Package::Package
        */
        FilePackage(const QString &fileName, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        FilePackage(const FilePackage &other);  /*!< Constructeur de copie nécessaire pour la gestion du solveur */
        ~FilePackage();

        bool download();            /*!< Renvoie immédiatement true et émmet immédiatement downloaded() */
        QString tlzFileName();
        bool isValid();
        Package::Origin origin();   /*!< Renvoie Package::File */
        
        QString name();
        QString version();
        QString maintainer();
        QString shortDesc();
        QString source();
        QString upstreamUrl();
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString arch();
        QByteArray packageHash();
        QByteArray metadataHash();
        int flags();
        
        QDateTime installedDate();
        int installedBy();
        int used();
        
        int downloadSize();
        int installSize();

        QList<Depend *> depends();
        
        void registerState(int idate, int iby, int flags);
        
        QByteArray metadataContents();  /*!< Contenu du fichier de métadonnées */
        QList<PackageFile *> files();            /*!< Liste des fichiers du paquet */

    signals:
        void downloaded(bool success);

    private:
        struct Private;
        Private *d;
};

/**
    @brief Dépendance d'un paquet .tlz
    
    Utilise le fichier de métadonnées du paquet pour renvoyer ses dépendances.
    
    Utilisé par RepositoryManager pour importer un paquet
*/
class FileDepend : public Depend
{
    public:
        FileDepend(int8_t type, int8_t op, const QString &name, const QString &version);
        ~FileDepend();

        QString name();
        QString version();
        int8_t type();
        int8_t op();

    private:
        struct Private;
        Private *d;
};

class FileFile : public PackageFile
{
    public:
        FileFile(const QString &path, int flags);
        ~FileFile();
        
        QString path();
        int flags();
        Package *package();
        
        void setFlags(int flags);
        void setPath(const QString &path);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif