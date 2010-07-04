/*
 * packagesystem.h
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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
#include <QRegExp>

// Architecture de LPM
#ifndef SETUP_ARCH
    #if __SIZEOF_POINTER__ == 4
        #define SETUP_ARCH "i686"
    #else
        #define SETUP_ARCH "x86_64"
    #endif
#endif

class QNetworkReply;
class QSettings;

/**
 * @namespace Logram
 * @brief Espace des noms conteneur des fonctions de LPM
 * 
 * Cet espace des noms contient toutes les classes et toutes les fonctions utilisées par le gestionnaire de paquets Logram.
 * 
 * Il est également utilisé par d'autres bibliothèques de Logram.
 */
namespace Logram
{

class DatabasePackage;
class DatabaseReader;
class Package;
class Solver;
class Communication;
class PackageFile;

/**
 * @brief Structure de gestion des téléchargements
 * 
 * Cette structure permet de gérer les téléchargements asynchrones effectués grâce à
 * PackageSystem::download.
 */
struct ManagedDownload
{
    /**
     * @brief Réponse
     * 
     * Structure QNetworkReply en provenance de Qt, permettant de gérer les erreurs de transfert, des informations
     * comme la progression dans le téléchargement, etc.
     */
    QNetworkReply *reply;
    
    /**
     * @brief Url source
     * 
     * Url du fichier à télécharger. Peut également être le nom d'un fichier local si le téléchargement est de type @b local.
     */
    QString url;
    
    /**
     * @brief Fichier de destination
     * 
     * Le fichier téléchargé est placé là ou le programme appelant l'a décidé. Ce champs contient cette information. C'est
     * toujours un fichier local
     */
    QString destination;
    
    /**
     * @brief Erreur
     * 
     * Est placé à @b true en cas d'erreur. Ce champs est indéterminé tant que le téléchargement n'est pas fini (une erreur le termine).
     */
    bool error;
};

/**
 * @brief Erreur globale
 * @dontinclude app.cpp
 * 
 * Cette classe permet de gérer les erreurs pouvant se produire lors des différents traitements. Les fonctions de l'API
 * de libpackage ne renvoient pas de code d'erreur, mais bien un booléen permettant juste de savoir si tout s'est bien passé.
 * 
 * Un appel à PackageSystem::lastError permet de savoir si une erreur s'est produite, et le cas échant, de la récupérer.
 * 
 * Cette manière de faire permet de stocker des informations supplémentaires dans une erreur, comme un message d'erreur, etc.
 */
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
        InstallError,
        ProgressCanceled
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
        Exporting,
        Trigger
    };
    
    enum Action
    {
        Create,
        Update,
        End
    };
    
    Type type;
    Action action;
    int current, old, total;
    QString info, more;
    void *data;
    bool canceled;
};

struct UpgradeInfo;

class PackageSystem : public QObject
{
    Q_OBJECT
    
    public:
        PackageSystem(QObject *parent = 0);
        ~PackageSystem();
        bool init();
        bool initialized() const;
        void loadConfig();

        // API publique
        bool packagesByName(const QRegExp &regex, QList<int> &rs);
        QList<int> packagesByVString(const QString &name, const QString &version, int op);
        bool package(const QString &name, const QString &version, Package* &rs);
        DatabasePackage *package(int id);
        QList<PackageFile *> files(const QString &name);
        QList<PackageFile *> files(const QRegExp &regex);
        int packages();
        bool update();
        QList<DatabasePackage *> upgradePackages();
        QList<DatabasePackage *> orphans();
        Solver *newSolver();

        // Fonctions statiques
        static int compareVersions(const char *a, const char *b);
        static int compareVersions(const QByteArray &v1, const QByteArray &v2);
        static bool matchVersion(const QByteArray &v1, const QByteArray &v2, int op);
        static int parseVersion(const QByteArray &verStr, QByteArray &name, QByteArray &version);
        static QString fileSizeFormat(int size);
        static QString dependString(const QString &name, const QString &version, int op);

        // API utilisée par des éléments de liblpackages
        bool download(Repository::Type type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs);
        QList<Repository> repositories() const;
        bool repository(const QString &name, Repository &rs) const;
        QSettings *installedPackagesList() const;

        // Options
        bool installSuggests() const;
        int parallelDownloads() const;
        int parallelInstalls() const;
        QString installRoot() const;
        QString confRoot() const;
        QString varRoot() const;
        QStringList pluginPaths() const;
        bool runTriggers() const;
        void setInstallSuggests(bool enable);
        void setParallelDownloads(int num);
        void setParallelInstalls(int num);
        void setInstallRoot(const QString &root);
        void setConfRoot(const QString &root);
        void setVarRoot(const QString &root);
        void setRunTriggers(bool enable);
        void addPluginPath(const QString &path);
        
        // Gestion des erreurs
        void setLastError(PackageError *err); // Thread-safe
        void setLastError(PackageError::Error type, const QString &info, const QString &more = QString());
        PackageError *lastError(); // Thread-safe, peut renvoyer 0

        int startProgress(Progress::Type type, int tot);
        bool sendProgress(int id, int num, const QString &msg, const QString &more = QString(), void *data = 0) __attribute__((warn_unused_result));
        bool processOut(const QString &command, const QString &line) __attribute__((warn_unused_result));
        void endProgress(int id);
        
        void sendCommunication(Package *sender, Communication *comm);
        
        // Usage interne
        QString bestMirror(const Repository &repo);
        void releaseMirror(const QString &mirror);
        DatabaseReader *databaseReader();
        void saveFile(PackageFile *file);
        void syncFiles();

    signals:
        void progress(Logram::Progress *progress);
        void downloadEnded(Logram::ManagedDownload *reply);
        void communication(Logram::Package *sender, Logram::Communication *comm);

    private slots:
        void downloadFinished(QNetworkReply *reply);
        void dlProgress(qint64 done, qint64 total);

    protected:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif