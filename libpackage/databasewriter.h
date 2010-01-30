/*
 * databasewriter.h
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
    @file databasewriter.h
    @brief Écrit la base de donnée binaire
*/

#ifndef __DATABASEWRITER_H__
#define __DATABASEWRITER_H__

#include <QObject>
#include <QEventLoop>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QByteArray>

#include "packagesystem.h"

class QNetworkAccessManager;
class QNetworkReply;
class QIODevice;

namespace Logram {
    
class PackageSystem;

struct _String;
struct _Package;
struct _StrPackage;
struct _Depend;

/**
    @brief Entrée de paquet connue
    
    Permet de relier un nom et une version à une _Package prêt pour l'écriture.
    
    Fait également le lien entre la passe 1 et la passe 2 d'enregistrement (voir code source)
    
    @internal
*/
struct knownEntry
{
    QByteArray version; /*!< Version du paquet */
    _Package *pkg;      /*!< Paquet binaire */
    int index;          /*!< Index de ce paquet binaire */
    bool ignore;        /*!< True si c'est un paquet présent dans les dépôts et installé à la même version */
};

/**
    @brief Enregistre les paquets des dépôts dans la base de donnée binaire
    
    Cette classe est responsable du lourd traitement d'enregistrement des paquets dans
    la base de donnée binaire. Grâce à son travail, Setup est rapide même s'il doit gérer
    plusieurs millions de paquets.
    
    Les tâches accomplies sont titanesques :
    
     - Résolution des dépendances (trouver tous les paquets correspondant à une dépendance
       et les enregistrer dans les _Depend d'un paquet
     - Résolution des provides
     - Empaquetage des données
     
    Le code est long mais pas spécialement complexe.
    
    @internal
*/
class DatabaseWriter : public QObject
{
    Q_OBJECT
    public:
        /**
            @brief Constructeur
            @param _parent PackageSystem parent
        */
        DatabaseWriter(PackageSystem *_parent);

        /**
            @brief Télécharge un élément du dépôt
            @param source Nom du dépôt
            @param url Url du fichier
            @param type Type de dépôt (local, en ligne)
            @param isTranslations true si c'est un fichier translate qui est téléchargé
            @param gpgCheck true s'il faut vérifier avec GPG la signature des fichiers téléchargés
        */
        bool download(const QString &source, const QString &url, Repository::Type type, bool isTranslations, bool gpgCheck);
        
        /**
            @brief Reconstruit la base de donnée binaire
        */
        bool rebuild();

    private:
        PackageSystem *parent;
        
        QStringList cacheFiles;
        QList<bool> checkFiles;

        QList<_Package *> packages;
        
        //QHash<int, int> packagesIndexes;
        QList<_String *> strings;
        QList<_String *> translate;
        QHash<QByteArray, int> stringsIndexes;
        QHash<QByteArray, int> translateIndexes;
        QList<QByteArray> stringsStrings;
        QList<QByteArray> translateStrings;

        int strPtr, transPtr;

        QList<QList<_StrPackage *> > strPackages;
        QList<QList<_Depend *> > depends;
        
        QHash<QByteArray, QList<knownEntry *> > knownPackages; // (nom, [(version, _Package)])
        QList<knownEntry *> knownEntries;

        void handleDl(QIODevice *device);
        int stringIndex(const QByteArray &str, int pkg, bool isTr, bool create = true);
        void setDepends(_Package *pkg, const QByteArray &str, int type);
        void revdep(_Package *pkg, const QByteArray &name, const QByteArray &version, int op, int type);
        
        bool verifySign(const QString &signFileName, const QByteArray &sigtext, bool &rs);
};

} /* Namespace */

#endif