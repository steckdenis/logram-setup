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

#include <QSqlDatabase>

#include <packagesystem.h>

class QSettings;
class Thread;

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
        bool finished() const;
        bool verbose() const;
        bool useWebsite() const;
        void cleanup();
        QSqlDatabase &database();
        int distributionId(const QString &name) const;
        int archId(const QString &name) const;
        
        // Configuration
        QString architecture() const;
        QString root() const;
        QString enabledDistros(const QString &name) const;
        QString sourceType() const;
        QString sourceUrl() const;
        QString execName() const;
        QString mailServer() const;
        int mailPort() const;
        bool mailEncrypted() const;
        bool mailUseTLS() const;
        QString mailUser() const;
        QString mailPassword() const;
        QString mailLogRoot() const;
        
        static void recurseRemove(const QString &path);
        static QString psErrorString(Logram::PackageSystem *ps);
        static QString progressString(Logram::Progress *progress);
        
    public slots:
        void buildPackage();
        void threadFinished();
        void progress(Logram::Progress *progress);
        
    private:
        bool workerProcess(const QString &root);
        
        void psError(Logram::PackageSystem *ps);
        void log(LogType type, const QString &message);

    private:
        Logram::PackageSystem *ps;
        QSqlDatabase db;
        QSettings *set;
        
        bool error;
        
        QHash<QString, int> distroIds;
        QHash<QString, int> archIds;
        
        QList<Thread *> threads;
        
        // Options
        bool debug, quitApp, worker, websiteIntegration;
        int maxThreads;
        QString confFileName, arch, name;
};

#endif