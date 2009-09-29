/*
 * libpackage.h
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

#ifndef __LIBPACKAGE_H__
#define __LIBPACKAGE_H__

#include <QObject>
#include <QString>
#include <QList>

class Package;
class Solver;
class PackageSystemPrivate;

class QNetworkReply;

struct ManagedDownload
{
    QNetworkReply *reply;
    QString url;
    QString destination;
};

class PackageSystem : public QObject
{
    Q_OBJECT
    
    public:
        PackageSystem(QObject *parent = 0);
        void init();

        // API publique
        QList<Package *> packagesByName(const QString &regex);
        Package *package(const QString &name, const QString &version = QString());

        void update();
        Solver *newSolver();

        static int compareVersions(const QString &v1, const QString &v2);
        static bool matchVersion(const QString &v1, const QString &v2, int op);
        static int parseVersion(const QString &verStr, QString &name, QString &version);

        ManagedDownload *download(const QString &type, const QString &url, const QString &dest, bool block=true);
        QString repoType(const QString &repoName);
        QString repoUrl(const QString &repoName);

        // Gestion des erreurs
        enum Error
        {
            OpenFileError,
            MapFileError,
            ProcessError,
            DownloadError,
            ScriptException
        };

        enum Progress
        {
            Other,
            GlobalDownload,
            Download,
            UpdateDatabase,
            GlobalInstall
        };

        void raise(Error err, const QString &info);
        void sendProgress(Progress type, int num, int tot, const QString &msg);
        void endProgress(Progress type, int tot);

    signals:
        void error(PackageSystem::Error err, const QString &info);
        void progress(PackageSystem::Progress type, int num, int tot, const QString &msg);
        void downloadEnded(ManagedDownload *reply);

    private slots:
        void downloadFinished(QNetworkReply *reply);
        void dlProgress(qint64 done, qint64 total);

    protected:
        PackageSystemPrivate *d;
};

#endif