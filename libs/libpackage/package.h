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

class PackageSystem;
class PackageSystemPrivate;
struct ManagedDownload;

class _Depend;

class Depend;

class Package : public QObject
{
    Q_OBJECT
    
    public:
        Package(int index, PackageSystem *ps, PackageSystemPrivate *psd, Solver::Action _action = Solver::None);
        ~Package();

        void install();
        void download();
        bool isValid();
        Solver::Action action();

        QString name();
        QString version();
        QString shortDesc();
        QString longDesc();
        QString title();
        QString source();
        QString repo();
        QString section();
        QString distribution();
        QString license();
        QString url();
        QString installedVersion();
        QDateTime installedDate();
        QString installedRepo();
        bool isInstalled();
        int downloadSize();
        int installSize();

        QList<Depend *> depends();

    signals:
        void installed();
        void downloaded();

    private slots:
        void downloadEnded(ManagedDownload *md);
        void processOut();
        void processEnd(int exitCode, QProcess::ExitStatus exitStatus);

    private:
        struct Private;
        Private *d;
};

class Depend
{
    public:
        Depend(_Depend *dep, PackageSystemPrivate *psd);
        ~Depend();

        QString name();
        QString version();
        int8_t type();
        int8_t op();

    private:
        struct Private;
        Private *d;
};

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

#endif