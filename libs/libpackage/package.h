/*
 * package.h
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

#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include <QObject>
#include <QProcess>
#include <QDateTime>

#include "solver.h"

#include <stdint.h>

namespace Logram
{

class PackageSystem;
class DatabaseReader;
class PackageMetaData;
class Communication;
struct ManagedDownload;

class _Depend;

class Depend;

class Package : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(Logram::Solver::Action action READ action)
    
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QString maintainer READ maintainer)
    Q_PROPERTY(QString shortDesc READ shortDesc)
    Q_PROPERTY(QString source READ source)
    Q_PROPERTY(QString repo READ repo)
    Q_PROPERTY(QString section READ section)
    Q_PROPERTY(QString distribution READ distribution)
    Q_PROPERTY(QString license READ license)
    Q_PROPERTY(QString arch READ arch)
    Q_PROPERTY(QString url READ url)
    
    Q_PROPERTY(bool gui READ isGui)
    
    Q_PROPERTY(int downloadSize READ downloadSize)
    Q_PROPERTY(int installSize READ installSize)
    
    public:
        Package(int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        ~Package();

        void process();
        bool download();
        bool isValid();
        Solver::Action action();

        enum UrlType
        {
            Binary,
            Metadata
        };
        
        QString name();
        QString version();
        QString newerVersion();
        QString maintainer();
        QString shortDesc();
        QString source();
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString arch();
        QString url(UrlType type = Package::Binary);
        QString packageHash();
        QString metadataHash();
        bool isGui();
        
        QDateTime installedDate();
        int installedBy();
        int status();
        
        int downloadSize();
        int installSize();
        
        PackageMetaData *metadata();

        Package *upgradePackage();
        QList<Package *> versions();
        QList<Depend *> depends();
        QString dependsToString(const QList<Depend *> &deps, int type);
        
        // Interne, à usage de libpackage
        void setUpgradePackage(int i);

    signals:
        void proceeded(bool success);
        void downloaded(bool success);
        
        void communication(Logram::Package *sender, Logram::Communication *comm);

    private slots:
        void downloadEnded(Logram::ManagedDownload *md);
        void processOut();
        void processEnd(int exitCode, QProcess::ExitStatus exitStatus);

    private:
        struct Private;
        Private *d;
};

class Depend
{
    public:
        Depend(_Depend *dep, DatabaseReader *psd);
        ~Depend();

        QString name();
        QString version();
        int8_t type();
        int8_t op();

    private:
        struct Private;
        Private *d;
};

} /* Namespace */

// Constantes

#define DEPEND_TYPE_INVALID  0
#define DEPEND_TYPE_DEPEND   1
#define DEPEND_TYPE_SUGGEST  2
#define DEPEND_TYPE_CONFLICT 3
#define DEPEND_TYPE_PROVIDE  4
#define DEPEND_TYPE_REPLACE  5
#define DEPEND_TYPE_REVDEP   6   // Dans ce cas, name = index du paquet dans packages, version = 0

#define DEPEND_OP_NOVERSION  0   // Pas de version spécifiée
#define DEPEND_OP_EQ         1   // =
#define DEPEND_OP_GREQ       2   // >=
#define DEPEND_OP_GR         3   // >
#define DEPEND_OP_LOEQ       4   // <=
#define DEPEND_OP_LO         5   // <
#define DEPEND_OP_NE         6   // !=

#define PACKAGE_STATE_NOTINSTALLED  0   // Non-installé
#define PACKAGE_STATE_INSTALLED     1   // Installé
#define PACKAGE_STATE_REMOVED       2   // Supprimé

#endif