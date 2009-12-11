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
class PackageMetaData;
class Communication;
class DatabaseReader;

class Depend;
class DatabasePackage;

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
    Q_PROPERTY(Origin origin READ origin)
    
    Q_PROPERTY(bool gui READ isGui)
    
    Q_PROPERTY(int downloadSize READ downloadSize)
    Q_PROPERTY(int installSize READ installSize)
    
    public:
        Package(PackageSystem *ps, DatabaseReader *psd, Solver::Action _action = Solver::None);
        Package(const Package &other);
        ~Package();
        
        enum Origin
        {
            Database,
            File
        };

        // Interface
        virtual bool download() = 0;
        virtual QString tlzFileName() = 0;
        virtual bool isValid() = 0;
        virtual Origin origin() = 0;
        
        virtual QString name() = 0;
        virtual QString version() = 0;
        virtual QString maintainer() = 0;
        virtual QString shortDesc() = 0;
        virtual QString source() = 0;
        virtual QString repo() = 0;
        virtual QString section() = 0;
        virtual QString distribution() = 0;
        virtual QString license() = 0;
        virtual QString arch() = 0;
        virtual QByteArray metadataHash() = 0;
        virtual QByteArray packageHash() = 0;
        virtual bool isGui() = 0;
        virtual int status() = 0;
        
        virtual int downloadSize() = 0;
        virtual int installSize() = 0;
        
        virtual QList<Depend *> depends() = 0;
        virtual QDateTime installedDate() = 0;
        virtual int installedBy() = 0;
        
        virtual void registerState(int idate, int iby, int state) = 0;
        
        // Commun à tous les types de paquets
        void process();
        Solver::Action action();
        void setAction(Solver::Action act);
        PackageMetaData *metadata();

        // Utilitaire
        static QString dependsToString(const QList<Depend *> &deps, int type);
        
        // Mise à jour
        DatabasePackage *upgradePackage();
        void setUpgradePackage(int i);

    signals:
        void proceeded(bool success);
        void downloaded(bool success);
        
        void communication(Logram::Package *sender, Logram::Communication *comm);

    private slots:
        void processOut();
        void processEnd(int exitCode, QProcess::ExitStatus exitStatus);

    private:
        struct Private;
        Private *d;
};

class Depend
{
    public:
        Depend();

        virtual QString name() = 0;
        virtual QString version() = 0;
        virtual int8_t type() = 0;
        virtual int8_t op() = 0;
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