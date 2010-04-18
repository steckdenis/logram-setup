/*
 * app.h
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#ifndef __APP_H__
#define __APP_H__

#include <QCoreApplication>
#include <QStringList>
#include <QHash>
#include <QFile>
#include <QEventLoop>
#include <QProcess>

#include <QSqlDatabase>

#include <QtDebug>

#include <logram/packagesystem.h>

class QSettings;

enum LogType
{
    Message,
    Operation,
    Progression,
    Warning,
    Error,
    ProcessOut
};

enum State
{
    General,
    Downloading,
    Building,
    Packaging
};

class App : public QCoreApplication
{
    Q_OBJECT
    
    public:
        App(int &argc, char **argv);
        bool failed() const;
        bool mustExit() const;
        
        bool buildWorker();
        void cleanup();
        bool workerProcess();
        
        void setState(State _state, int log_id);
        void log(LogType type, const QString &message);

    public slots:
        void progress(Logram::Progress *progress);
        void buildPackage();
        
        void processOut();
        void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    private:
        Logram::PackageSystem *ps;
        QSqlDatabase db;
        QSettings *set;
        QFile logFile;
        
        bool error;
        State state;
        QEventLoop dl;
        int log_id;
        QString curArch;
        
        QHash<QString, int> distroIds;
        
        // Options
        bool debug, quitApp, worker;
        QString confFileName;
        
        // Fonctions
        void psError(Logram::PackageSystem *ps);
        void endPackage(int log_id, int flags);
};

#endif