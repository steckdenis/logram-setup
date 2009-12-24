/*
 * packagesystem.h
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
#include <QStringList>

class QNetworkReply;
class QSettings;

namespace Logram
{

class DatabasePackage;
class Package;
class Solver;
class PackageSystemPrivate;

struct ManagedDownload
{
    QNetworkReply *reply;
    QString url;
    QString destination;
    bool error;
};

struct PackageError
{
    enum Error
    {
        OpenFileError,
        MapFileError,
        ProcessError,
        DownloadError,
        ScriptException,
        SignatureError,
        SHAError,
        PackageNotFound,
        BadDownloadType,
        OpenDatabaseError,
        QueryError,
        SignError,
        InstallError
    };
    
    Error type;
    QString info;    // Informations en une ligne (le nom du fichier qu'on peut pas ouvrir, etc)
    QString more;    // Facultatif : informations supplémentaires (sortie du script qui a planté)
};

struct Repository
{
    enum Type
    {
        Unknown,
        Remote,
        Local
    };
    
    Type type;
    
    QString name;
    QString description;
    
    QStringList mirrors;
    QStringList distributions;
    QStringList architectures;
    
    bool active;
    bool gpgcheck;
};

struct Progress
{
    enum Type
    {
        Other,
        GlobalDownload,
        Download,
        UpdateDatabase,
        PackageProcess,
        ProcessOut,
        GlobalCompressing,
        Compressing,
        Including,
        Exporting
    };
    
    enum Action
    {
        Create,
        Update,
        End
    };
    
    Type type;
    Action action;
    int current, total;
    QString info, more;
};

struct UpgradeInfo;

class PackageSystem : public QObject
{
    Q_OBJECT
    
    public:
        PackageSystem(QObject *parent = 0);
        ~PackageSystem();
        bool init();
        void loadConfig();

        // API publique
        bool packagesByName(const QString &regex, QList<int> &rs);
        bool package(const QString &name, const QString &version, Package* &rs);
        DatabasePackage *package(int id);
        bool update();
        QList<DatabasePackage *> upgradePackages();
        Solver *newSolver();

        // Fonctions statiques
        static int compareVersions(const char *a, const char *b);
        static int compareVersions(const QByteArray &v1, const QByteArray &v2);
        static bool matchVersion(const QByteArray &v1, const QByteArray &v2, int op);
        static int parseVersion(const QString &verStr, QString &name, QString &version);
        static QString fileSizeFormat(int size);
        static QString dependString(const QString &name, const QString &version, int op);

        // API utilisée par des éléments de liblpackages
        bool download(Repository::Type type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs);
        QList<Repository> repositories() const;
        bool repository(const QString &name, Repository &rs) const;
        QSettings *installedPackagesList() const;
        bool filesOfPackage(const QString &packageName, QStringList &rs);

        // Options
        bool installSuggests() const;
        int parallelDownloads() const;
        int parallelInstalls() const;
        QString installRoot() const;
        QString confRoot() const;
        QString varRoot() const;
        void setInstallSuggests(bool enable);
        void setParallelDownloads(int num);
        void setParallelInstalls(int num);
        void setInstallRoot(const QString &root);
        void setConfRoot(const QString &root);
        void setVarRoot(const QString &root);
        
        // Gestion des erreurs
        void setLastError(PackageError *err); // Thread-safe
        PackageError *lastError(); // Thread-safe

        int startProgress(Progress::Type type, int tot);
        void sendProgress(int id, int num, const QString &msg, const QString &more = QString());
        void processOut(const QString &command, const QString &line);
        void endProgress(int id);

    signals:
        void progress(Logram::Progress *progress);
        void downloadEnded(Logram::ManagedDownload *reply);

    private slots:
        void downloadFinished(QNetworkReply *reply);
        void dlProgress(qint64 done, qint64 total);

    protected:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif