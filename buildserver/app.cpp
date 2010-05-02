/*
 * app.cpp
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

#include "config.h"
#include "app.h"
#include "thread.h"

#include <QTextCodec>
#include <QSettings>

#include <solver.h>
#include <package.h>
#include <packagesource.h>
#include <repositorymanager.h>

#include <QtSql>

#include <iostream>

using namespace std;
using namespace Logram;

#define QUERY_ERROR(query) \
    log(Error, "Query failed : " + query.lastQuery());

#define SERVER_COMMUNICATION_TOKEN "$[Logram Build Server Communication$$]$*"

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    // Initialiser
    error = false;
    debug = false;
    worker = false;
    quitApp = false;
    confFileName = QDir::currentPath() + "/buildserver.conf";
    
    if (!QFile::exists(confFileName))
    {
        confFileName = "/etc/buildserver.conf";
    }
    
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    // Parser la ligne de commande
    QStringList args = arguments();
    QString arg;
    
    name = args.takeFirst();   // Retenir le nom de l'appli
    
    while (args.count())
    {
        arg = args.takeFirst();
        
        if (arg == "-d")
        {
            debug = true;
        }
        else if (arg == "--config" && args.count())
        {
            confFileName = args.takeFirst();
        }
        else if (arg == "--worker" && args.count())
        {
            worker = true;
            error = workerProcess(args.takeFirst());
            
            quitApp = true;
            return;
        }
        else
        {
            cout << "Logram Build Server v" << VERSION << endl;
            cout << endl;
            cout << "Logram is free software; you can redistribute it and/or modify" << endl
                 << "it under the terms of the GNU General Public License as published by" << endl
                 << "the Free Software Foundation; either version 3 of the License, or" << endl
                 << "(at your option) any later version." << endl;
            cout << endl;
            cout << "Usage : buildserver [-d] [--config <configfile>] [--worker <dir> (internal)]" << endl;
            cout << endl;
            cout << "Options :" << endl;
            cout << "    -d" << endl;
            cout << "        Enable the verbose (debug) mode. All output is put in the logs of the packages and in stdout, with nice colors" << endl;
            cout << "    --config <configfile>" << endl;
            cout << "        Use <configfile> as config file instead of " << qPrintable(confFileName) << endl;
            cout << "    --worker <dir>" << endl;
            cout << "        Internal command. Chroot in <dir> and launche the equivalent of setup download && setup build && setup binaries on the file <dir>/src/metadata.xml" << endl;
            
            quitApp = true;
            return;
        }
    }
    
    // Configuration
    set = new QSettings(confFileName, QSettings::IniFormat, this);
    
    maxThreads = set->value("MaxThreads", 1).toInt();
    arch = set->value("Arch", SETUP_ARCH).toString();
    
    // Connexion à la base de donnée
    db = QSqlDatabase::addDatabase("QMYSQL", "logram_repo_database");
    db.setHostName(set->value("Database/Hostname", "localhost").toString());
    db.setDatabaseName(set->value("Database/Name").toString());
    db.setUserName(set->value("Database/User", "root").toString());
    db.setPassword(set->value("Database/Password").toString());
    
    if (!db.open())
    {
        error = true;
        log(Error, "Unable to connect to the database, check the settings in " + confFileName);
        return;
    }
    
    log(Message, "Connected to the database");
    
    // Récupérer l'association entre le nom d'une distribution et son ID
    QString sql;
    QSqlQuery query(db);
    
    log(Operation, "Fetching the IDs of all the distributions");
    
    sql = " SELECT \
            id, \
            name \
            FROM packages_distribution;";
    
    if (!query.exec(sql))
    {
        QUERY_ERROR(query)
        return;
    }
    
    while (query.next())
    {
        distroIds.insert(query.value(1).toString(), query.value(0).toInt());
    }
    
    // Association entre le nom et l'ID d'une architecture
    log(Operation, "Fetching the IDs of all the architectures");
    
    sql = " SELECT \
            id, \
            name \
            FROM packages_arch;";
    
    if (!query.exec(sql))
    {
        QUERY_ERROR(query);
        return;
    }
    
    while (query.next())
    {
        archIds.insert(query.value(1).toString(), query.value(0).toInt());
    }
}

void App::buildPackage()
{
    QString sql;
    QSqlQuery query(db);
    
    // Trouver le nombre de threads à lancer
    int numThreads = maxThreads - threads.count();
    
    if (numThreads == 0)
    {
        // On en a déjà assez
        return;
    }
    
    log(Operation, "Fetching at most " + QString::number(numThreads) + " packages to build");
    
    // Liste des workers déjà en cours
    QString badIds;
    
    for (int i=0; i<threads.count(); ++i)
    {
        Thread *thread = threads.at(i);
        
        if (!badIds.isNull())
        {
            badIds += ", ";
        }
        else
        {
            // Liste non vide, ajouter une condition
            badIds = "AND NOT log.id IN (";
        }
        
        badIds += QString::number(thread->id());
    }
    
    if (!badIds.isNull())
    {
        badIds += ')';
    }
    
    // Sélectionner les informations sur les paquets à construire
    sql = " SELECT \
            log.id, \
            source.name, \
            log.version, \
            distro.name, \
            log.depends, \
            log.suggests, \
            log.conflicts, \
            log.flags, \
            log.author, \
            log.source_id, \
            log.maintainer, \
            log.upstream_url, \
            log.distribution_id, \
            log.license, \
            log.arch_id \
            FROM packages_sourcelog log \
            LEFT JOIN packages_sourcepackage source ON source.id = log.source_id \
            LEFT JOIN packages_distribution distro ON distro.id = log.distribution_id \
            LEFT JOIN packages_arch arch ON arch.id = log.arch_id \
            WHERE log.arch_id=%1 AND (log.flags & %2 ) != 0 %3\
            ORDER BY log.date_rebuild_asked ASC \
            LIMIT %4;";
    
    if (!query.exec(sql
            .arg(archIds.value(arch))
            .arg(SOURCEPACKAGE_FLAG_REBUILD | SOURCEPACKAGE_FLAG_CONTINUOUS)
            .arg(badIds)
            .arg(numThreads)))
    {
        QUERY_ERROR(query)
        
        // Réessayer plus tard
        QTimer::singleShot(60*1000, this, SLOT(buildPackage()));
        return;
    }
    
    if (!query.next())
    {
        // Pas de paquet à construire
        log(Message, "No package to build, waiting 1 minute");
        
        QTimer::singleShot(60*1000, this, SLOT(buildPackage()));
        return;
    }
    
    log(Message, "Found " + QString::number(query.size()) + " packages to build");
    
    do
    {
        // Lancer un thread
        Thread *thread = new Thread(this, query);
        
        threads.append(thread);
        connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
        
        thread->start();
    } while (query.next());
}

void App::cleanup()
{
    // TODO: Supprimer tous les Threads
}

bool App::failed() const
{
    return error;
}

bool App::finished() const
{
    return quitApp;
}

bool App::verbose() const
{
    return debug;
}

QSqlDatabase &App::database()
{
    return db;
}

int App::distributionId(const QString &name) const
{
    return distroIds.value(name);
}

int App::archId(const QString &name) const
{
    return archIds.value(name);
}

QString App::architecture() const
{
    return arch;
}

QString App::root() const
{
    return set->value("Repository/Root").toString();
}

bool App::useWebsite() const
{
    return websiteIntegration;
}

QString App::enabledDistros(const QString &name) const
{
    return set->value("DistroDeps/" + name).toString();
}

QString App::sourceType() const
{
    return set->value("Repository/SourceType", "local").toString();
}

QString App::sourceUrl() const
{
    return set->value("Repository/SourceMirrors", root()).toString();
}

QString App::execName() const
{
    return name;
}

QString App::mailServer() const
{
    return set->value("Mail/Server").toString();
}

int App::mailPort() const
{
    return set->value("Mail/Port", 25).toInt();
}

bool App::mailEncrypted() const
{
    return set->value("Mail/Encrypted", false).toBool();
}

bool App::mailUseTLS() const
{
    return set->value("Mail/UseTLS", false).toBool();
}

QString App::mailPassword() const
{
    return set->value("Mail/Password").toString();
}

QString App::mailUser() const
{
    return set->value("Mail/User").toString();
}

QString App::mailLogRoot() const
{
    return set->value("Mail/LogRoot").toString();
}

void App::threadFinished()
{
    // Un thread a fini, le libérer
    Thread *thread = static_cast<Thread *>(sender());
    
    if (thread)
    {
        threads.removeOne(thread);
        delete thread;
    }
    
    // Lancer les suivants
    buildPackage();
}

bool App::workerProcess(const QString &root)
{
    // Renvoyer true si erreur
    
    // Chroot
    int rs = chroot(root.toUtf8().constData());
    
    if (rs != 0)
    {
        log(Error, "Chroot in " + root + " failed");
        return true;
    }
    
    // Aller dans /src (on est en chroot maintenant ;-) )
    if (!QDir::setCurrent("/src"))
    {
        log(Error, "Unable to change directory to src/");
        return true;
    }
    
    // On a normalement ici un fichier metadata.xml. Simplement lancer
    // la suite setup download && setup build && setup binaries
    
    // Il nous faut un PackageSystem (pour les progressions et la sortie processus)
    PackageSystem *ps = new PackageSystem(this);
        
    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)), Qt::DirectConnection);
        
    ps->loadConfig();
    
    // Opération téléchargement
    log(Operation, "Launching the download script");
    
    PackageSource *src = new PackageSource(ps);
    
    if (!src->setMetaData(QString("metadata.xml")))
    {
        log(Error, "Unable to load the metadata.xml file");
        psError(ps);
        delete src;
        delete ps;
        return true;
    }
    
    src->loadKeys();
    
    if (!src->getSource())
    {
        log(Error, "Download script failed");
        psError(ps);
        delete src;
        delete ps;
        return true;
    }
    
    // Opération construction
    cout << SERVER_COMMUNICATION_TOKEN << '1' << endl;
    log(Operation, "Launching the build script of the package");
    
    if (!src->build())
    {
        log(Error, "Build script failed");
        psError(ps);
        delete src;
        delete ps;
        return true;
    }
    
    // Opération empaquetage
    cout << SERVER_COMMUNICATION_TOKEN << '2' << endl;
    log(Operation, "Packaging of the package");
    
    if (!src->binaries())
    {
        log(Error, "Impossible to make the binaries");
        psError(ps);
        delete src;
        delete ps;
        return true;
    }
    
    // On a fini !
    delete src;
    delete ps;
    
    return false;
}

void App::log(LogType type, const QString &message)
{
    if (!worker)
    {
        switch (type)
        {
            case Message:
                cout << "\033[1mMM: \033[0m";
                break;
            case Operation:
                cout << "\033[1m\033[32mOO: \033[0m";
                break;
            case Progression:
                cout << "\033[1m\033[34mPP: \033[0m";
                break;
            case Warning:
                cout << "\033[1m\033[33mWW: \033[0m";
                break;
            case Error:
                cout << "\033[1m\033[31mEE: \033[0m";
                break;
            case ProcessOut:
                cout << "\033[1m\033[37m>>  \033[0m";
                break;
        }
        
        cout << qPrintable(message) << endl;
    }
    else
    {
        if (type == ProcessOut)
        {
            cout << qPrintable(message) << endl;
            return;
        }
        
        cout << SERVER_COMMUNICATION_TOKEN;
        
        switch (type)
        {
            case Operation:
                cout << 'O';
                break;
            case Progression:
                cout << 'P';
                break;
            case Warning:
                cout << 'W';
                break;
            case Error:
                cout << 'E';
                break;
            default:
                break;
        }
        
        cout << qPrintable(message) << endl;
    }
}

void App::psError(PackageSystem *ps)
{
    log(Error, psErrorString(ps));
}

void App::progress(Progress *progress)
{
    if (progress->action == Progress::End)
    {
        delete progress;
        return;
    }
    else if (progress->action == Progress::Create)
    {
        return;
    }
    
    if (progress->type == Progress::ProcessOut)
    {
        log(ProcessOut, progress->more);
        return;
    }
    
    log(Progression, progressString(progress) + progress->info);
}

QString App::psErrorString(PackageSystem *ps)
{
    PackageError *err = ps->lastError();
    QString s;
    
    if (err == 0)
    {
        // Interruption sans erreur, ou erreur inconnue
        s = "Unknown error";
        return s;
    }

    switch (err->type)
    {
        case PackageError::OpenFileError:
            s = "Unable to open the file ";
            break;
            
        case PackageError::MapFileError:
            s = "Unable to map the file ";
            break;
            
        case PackageError::ProcessError:
            s = "Error in the command : ";
            break;
            
        case PackageError::DownloadError:
            s = "Impossible to download ";
            break;
            
        case PackageError::ScriptException:
            s = "Error in the QtScript : ";
            break;
            
        case PackageError::SignatureError:
            s = "Bad GPG sign of the file ";
            break;
            
        case PackageError::SHAError:
            s = "Bad SHA1 sum of the file ";
            break;
            
        case PackageError::PackageNotFound:
            s = "Package not found : ";
            break;
            
        case PackageError::BadDownloadType:
            s = "Bad download type : ";
            break;
            
        case PackageError::OpenDatabaseError:
            s = "Impossible to open the database : ";
            break;
            
        case PackageError::QueryError:
            s = "Error in the query : ";
            break;
            
        case PackageError::SignError:
            s = "Unable to verify the signature : ";
            break;
            
        case PackageError::InstallError:
            s = "Impossible to install the package : ";
            break;
            
        case PackageError::ProgressCanceled:
            s = "Operation canceled : ";
            break;
    }
    
    if (!err->more.isEmpty())
    {
        s += "\n" + err->more;
    }
    
    // Plus besoin de l'erreur
    delete err;
    
    // Fini
    return s;
}

QString App::progressString(Progress *progress)
{   
    QString s;
    Package *pkg;
    
    switch (progress->type)
    {
        case Progress::Other:
            break;
            
        case Progress::GlobalDownload:
            s = "Downloading ";
            break;
            
        case Progress::UpdateDatabase:
            s = "Updating the database : ";
            break;
            
        case Progress::PackageProcess:
            // progress->data est un Package, nous pouvons nous en servir pour afficher plein de choses
            pkg = static_cast<Package *>(progress->data);
            
            if (pkg == 0)
            {
                s = "Operation on the package ";
            }
            else
            {
                switch (pkg->action())
                {
                    case Solver::Install:
                        s = "Installation of ";
                        break;
                    case Solver::Remove:
                        s = "Removing of     ";
                        break;
                    case Solver::Purge:
                        s = "Purge of        ";
                        break;
                    case Solver::Update:
                        s = "Updating of     ";
                        break;
                    default:
                        // N'arrive jamais, juste pour que GCC soit content
                        break;
                }
            }
            break;
            
        case Progress::GlobalCompressing:
            s = "Creation of the package ";
            break;
            
        case Progress::Compressing:
            s = "Compression of the file ";
            break;
            
        case Progress::Including:
            s = "Including of ";
            break;
            
        case Progress::Exporting:
            s = "Exportation of ";
            break;
            
        case Progress::Trigger:
            s = "Triggering ";
            break;
            
        default:
            return s;
    }
    
    return s;
}

void App::recurseRemove(const QString &path)
{
    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    
    for (int i=0; i<list.count(); ++i)
    {
        const QFileInfo &info = list.at(i);
        
        // Supprimer
        if (info.isDir())
        {
            recurseRemove(info.absoluteFilePath());
            
            dir.rmdir(info.fileName());
        }
        else if (info.isFile() || info.isSymLink())
        {
            dir.remove(info.fileName());
        }
    }
}

#include "app.moc"
