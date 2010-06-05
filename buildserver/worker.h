/*
 * worker.h
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

#ifndef __WORKER_H__
#define __WORKER_H__

#include <QProcess>
#include <QFile>
#include <QEventLoop>
#include <QSqlQuery>

#include <QDomElement>

#include "app.h"

#include <packagesystem.h>

class App;

namespace Logram {
    class RepositoryManager;
    class Solver;
}

class Worker : public QObject
{
    Q_OBJECT
    
    public:
        Worker(App *_app, const QSqlQuery &query);
        
        int id() const;
        QEventLoop &eventLoop();
        
        void run();
        
        void log(LogType type, const QString &message);
        
    private:
        void setState(State _state);
        
        void psError(Logram::PackageSystem *ps);
        void endPackage();
        int copyData(struct archive *ar, struct archive *aw);
        void setNodeContent(QDomElement &child, const QString &lang, const QByteArray &content);
        QString logFilePath(const QString &operation);
        void truncateLogs();
        
        void loadData(const QSqlQuery &query);
        bool appendLog();
        bool prepareTemp();
        bool unpackSource();
        bool patchMetadata();
        bool updateDatabase(Logram::PackageSystem* &ps);
        bool installBuildDeps(Logram::PackageSystem *ps);
        bool buildChroot();
        bool copyPackages();
        bool importPackages(Logram::PackageSystem* &ps, Logram::RepositoryManager* &mg);
        bool exportDistros(Logram::PackageSystem *ps, Logram::RepositoryManager *mg);
        bool buildDepends();
        
        bool cleanupTemp();
        bool recurseCopy(const QString &from, const QString &to);
        void error(bool cleanup = true);
        
        void solverError(Logram::PackageSystem *ps, Logram::Solver *solver, const QString &defaultString);
        
    private slots:
        void progress(Logram::Progress *progress);
        void processOut();
        void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
        
    private:
        App *app;
        
        int log_id;
        int flags;
        QString name;
        QString version;
        QString old_version;
        QString distro;
        QString depends;
        QString suggests;
        QString conflicts;
        int old_flags;
        QString author;
        int source_id;
        QString maintainer;
        QString upstream_url;
        int distro_id;
        QString license;
        int arch_id;
        int new_log_id;
        
        QString repoRoot;
        QString tmpRoot;
        QEventLoop dl;
        QStringList builtPackages;
        QStringList binaries;
        
        QFile logFile;
        State state;
};

#endif