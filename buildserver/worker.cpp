/*
 * worker.cpp
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

#include "worker.h"
#include "smtp.h"

#include <QStringList>
#include <QTextCodec>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <QDirIterator>
#include <QList>
#include <QTime>
#include <QDate>

#include <QtSql>
#include <QtXml>

#include <repositorymanager.h>
#include <packagesystem.h>
#include <package.h>
#include <solver.h>
#include <packagelist.h>
#include <packagesource.h>

#include <iostream>
#include <cerrno>

#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>             // Pour chroot
#include <sys/mount.h>          // Pour mount

using namespace std;
using namespace Logram;

#define QUERY_ERROR(query) \
    log(Error, "Query failed : " + query.lastQuery());

#define SERVER_COMMUNICATION_TOKEN "$[Logram Build Server Communication$$]$*"

Worker::Worker(App *_app, const QSqlQuery &query) : QObject(0)
{
    app = _app;
    repoRoot = app->root();
    state = General;
    
    loadData(query);
}

int Worker::id() const
{
    return log_id;
}

QEventLoop &Worker::eventLoop()
{
    return dl;
}

void Worker::loadData(const QSqlQuery &query)
{
    // Remplir les champs de la classe
    log_id = query.value(0).toInt();
    name = query.value(1).toString();
    old_version = query.value(2).toString();
    distro = query.value(3).toString();
    depends = query.value(4).toString();
    suggests = query.value(5).toString();
    conflicts = query.value(6).toString();
    old_flags = query.value(7).toInt();
    author = query.value(8).toString();
    source_id = query.value(9).toInt();
    maintainer = query.value(10).toString();
    upstream_url = query.value(11).toString();
    distro_id = query.value(12).toInt();
    license = query.value(13).toString();
    arch_id = query.value(14).toInt();
    flags = 0;
    new_log_id = -1;
    
    // Si arch_id vaut zéro, c'est une importation manuelle de source qui a été faite, trouver l'arch
    if (arch_id == 0)
    {
        arch_id = app->archId(app->architecture());
    }
    
    // Calculer le nom du dossier temporaire
    tmpRoot = QDir::currentPath() + "/" + name + "_" + old_version;
    
    // Calculer la nouvelle version
    version = old_version;
    
    if ((old_flags & SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG) != 0)
    {
        QString mainvers = old_version.section('~', 0, -2);
        int logramVer = old_version.section('~', -1, -1).toInt();
        version = mainvers + '~' + QString::number(logramVer + 1);
        
        flags |= SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG;
    }
    
    // Autres modifications de flags
    if ((old_flags & SOURCEPACKAGE_FLAG_CONTINUOUS) != 0)
    {
        flags |= SOURCEPACKAGE_FLAG_CONTINUOUS | SOURCEPACKAGE_FLAG_REBUILD;
    }
}

void Worker::run()
{
    // Insérer le nouvel enregistrement
    setState(General);
    
    log(Operation, "Building of " + name + "~" + version);
    
    if (!appendLog())
    {
        error();
        return;
    }
    
    // État pour pouvoir bien logguer, on a maintenant la nouvelle ID
    truncateLogs();
    setState(Downloading);
    
    // Merger les suggestions dans les dépendances
    if (depends.isEmpty())
    {
        depends = suggests;
    }
    else
    {
        if (!suggests.isEmpty())
        {
            // On a bien des suggestions     
            depends += "; " + suggests;
        }
    }
    
    // Créer et pré-remplir le dossier tmpRoot
    log(Operation, "Populating the working directory (" + tmpRoot + ") with skeleton files");
    
    if (!prepareTemp())
    {
        error();
        return;
    }
    
    // Récupérer le paquet source et le désempaqueter dans tmpRoot
    if (!unpackSource())
    {
        error();
        return;
    }
    
    // Modifications en provenance de la base de donnée (intégration au site web, changelog)
    log(Operation, "Patching metadata.xml with the contents of the wiki, and management of the changelog");
    
    if (!patchMetadata())
    {
        error();
        return;
    }
    
    // Mettre à jour la base de donnée Setup du chroot
    PackageSystem *ps = 0;
    
    log(Operation, "Updating the Setup database");
    
    if (!updateDatabase(ps))
    {
        error();
        return;
    }
    
    // Installer les dépendances à la construction
    log(Operation, "Installing the build dependencies");
    
    if (!installBuildDeps(ps))
    {
        error();
        return;
    }
    
    // Construire le paquet dans un processus en chroot
    log(Operation, "Launching a child process which will build the packages");
    
    if (!buildChroot())
    {
        error();
        return;
    }
    
    // Copier les paquets construits dans le dossier courant
    setState(Packaging);
    log(Operation, "Copying the packages built in the working directory");
    
    if (!copyPackages())
    {
        error();
        return;
    }
    
    // Effacer et démonter tmpRoot
    if (!cleanupTemp())
    {
        error(false);
        return;
    }
    
    // Importer les paquets
    RepositoryManager *mg = 0;
    ps = 0;
    
    setState(Packaging);
    log(Operation, "Including of all the packages built");
    
    if (!importPackages(ps, mg))
    {
        error(false);
        return;
    }
    
    // Exporter les distributions (TODO: Mutex ici !)
    log(Operation, "Exportation of the distribution");
    if (!exportDistros(ps, mg))
    {
        error(false);
        return;
    }
    
    // On a fini
    endPackage();
}

bool Worker::appendLog()
{
    QString sql;
    QSqlQuery query(app->database());
    
    // Gérer le flag continu
    QString rebuild_date = "NULL";
    
    if ((old_flags & SOURCEPACKAGE_FLAG_CONTINUOUS) != 0)
    {
        rebuild_date = "NOW()";
    }
    
    // Insérer le nouvel enregistrement
    sql = " INSERT INTO packages_sourcelog \
                (source_id, flags, arch_id, date, author, maintainer, upstream_url, version, distribution_id, license, date_rebuild_asked, depends, suggests, conflicts) \
            VALUES (%1, %2, %3, NOW(), '%4', '%5', '%6', '%7', %8, '%9', %10, '%11', '%12', '%13');";
    
    if (!query.exec(sql
            .arg(source_id)
            .arg(SOURCEPACKAGE_FLAG_BUILDING)
            .arg(arch_id)
            .arg(author)
            .arg(maintainer)
            .arg(upstream_url)
            .arg(version)
            .arg(distro_id)
            .arg(license)
            .arg(rebuild_date)
            .arg(depends)
            .arg(suggests)
            .arg(conflicts)))
    {
        log(Error, "Impossible to insert the new log entry");
        return false;
    }
    
    // Prendre le nouveau log_id
    new_log_id = query.lastInsertId().toInt();
    
    return true;
}

bool Worker::prepareTemp()
{
    // Utiliser un aufs pour peupler le dossier de construction, plus rapide que cp
    // Également monter /proc, /dev et /sys dans cet arbre
    QDir::root().mkpath(tmpRoot);
    
    // Copier le contenu de QDir::currentPath() + "/skel" dans tmpRoot
    if (!recurseCopy(QDir::currentPath() + "/skel", tmpRoot))
    {
        // log(...) géré par recurseCopy
        return false;
    }
    
    if (mount("/dev", qPrintable(tmpRoot + "/dev"), 0, MS_BIND, 0))
    {
        log(Error, "Unable to bind /dev to " + tmpRoot + "/dev");
        return false;
    }
    
    if (mount("/proc", qPrintable(tmpRoot + "/proc"), 0, MS_BIND, 0))
    {
        log(Error, "Unable to bind /proc to " + tmpRoot + "/proc");
        return false;
    }
    
    if (mount("/sys", qPrintable(tmpRoot + "/sys"), 0, MS_BIND, 0))
    {
        log(Error, "Unable to bind /sys to " + tmpRoot + "/sys");
        return false;
    }
    
    // Générer les fichiers nécessaires au Setup chrooté
    // Sources.list
    QSettings sourcesList(tmpRoot + "/etc/lgrpkg/sources.list", QSettings::IniFormat, 0);
    
    sourcesList.setValue("Language", "en");
    sourcesList.beginGroup("repo");
    sourcesList.setValue("Type", app->sourceType());
    sourcesList.setValue("Mirrors", app->sourceUrl());
    sourcesList.setValue("Distributions", app->enabledDistros(distro));
    sourcesList.setValue("Archs", app->architecture() + " all");
    sourcesList.setValue("Active", true);
    sourcesList.setValue("Description", "Automatic building repository");
    sourcesList.setValue("Sign", false);
    sourcesList.endGroup();
    
    sourcesList.sync();
    
    // weight.qs
    QFile in(":weight.qs");
    QFile out(tmpRoot + "/etc/lgrpkg/scripts/weight.qs");
    
    if (!in.open(QIODevice::ReadOnly))
    {
        log(Error, "Unable to open the resource :weight.qs");
        return false;
    }
    
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        log(Error, "Unable to open " + tmpRoot + "/etc/lgrpkg/scripts/weight.qs for writing");
        return false;
    }
    
    out.write(in.readAll());
    out.close();
    
    // /usr/libexec/scriptapi
    if (!QFile::copy("/usr/libexec/scriptapi", tmpRoot + "/usr/libexec/scriptapi"))
    {
        log(Error, "Unable to copy /usr/libexec/scriptapi to " + tmpRoot + "/usr/libexec/scriptapi");
        return false;
    }
    
    return true;
}

bool Worker::unpackSource()
{
    // Récupérer le paquet
    QString srcFileName = app->root() + "pool/%1/%2/%3~%4.src.lpk";
    QString dd;
    
    if (name.startsWith("lib"))
    {
        dd = name.left(4);
    }
    else
    {
        dd = name.left(1);
    }
    
    srcFileName = srcFileName.arg(dd, name, name, old_version);
    
    log(Operation, "Fetching the source " + srcFileName);
    
    // Extraire le paquet dans le dossier temporaire
    QDir curDir = QDir::current();
    curDir.mkpath(tmpRoot + "/src");
    
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    
    a = archive_read_new();
    ext = archive_write_disk_new();
    entry = archive_entry_new();
    
    archive_read_support_compression_lzma(a);
    archive_read_support_compression_xz(a);
    archive_read_support_format_all(a);
    
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM);
    archive_write_disk_set_standard_lookup(ext);
    
    if (archive_read_open_filename(a, qPrintable(srcFileName), 10240))
    {
        log(Error, "Unable to open the archive " + srcFileName);
        
        archive_entry_free(entry);
        archive_write_finish(ext);
        archive_read_finish(a);
            
        return false;
    }
    
    QByteArray path, filePath, btmpRoot;
    int r;
    
    btmpRoot = tmpRoot.toUtf8();
    
    while (true)
    {
        r = archive_read_next_header2(a, entry);
        
        if (r == ARCHIVE_EOF)
        {
            break;
        }
        
        if (r != ARCHIVE_OK)
        {
            log(Error, "Error when reading the source archive");
            
            archive_entry_free(entry);
            archive_write_finish(ext);
            archive_read_finish(a);
            
            return false;
        }
        
        path = QByteArray(archive_entry_pathname(entry));
        filePath = btmpRoot + "/src/" + path;
        
        archive_entry_set_pathname(entry, filePath.constData());
        
        if (archive_write_header(ext, entry) != ARCHIVE_OK ||
            copyData(a, ext) != ARCHIVE_OK || 
            archive_write_finish_entry(ext) != ARCHIVE_OK)
        {
            log(Error, "Unable to extract the file " + filePath);
            
            archive_entry_free(entry);
            archive_write_finish(ext);
            archive_read_finish(a);
            
            return false;
        }
    }
    
    // Paquet extrait
    archive_entry_free(entry);
    archive_write_finish(ext);
    archive_read_finish(a);
    
    return true;
}

bool Worker::patchMetadata()
{
    QDomDocument metadata;
    QString sql;
    QSqlQuery query(app->database());
    
    QFile fl(tmpRoot + "/src/metadata.xml");
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        log(Error, "Opening of " + tmpRoot + "/src/metadata.xml for reading failed");
        return false;
    }
    
    metadata.setContent(fl.readAll());
    fl.close();
    
    // Explorer les paquets construits par cette source
    QDomElement root = metadata.documentElement();
    QDomElement package = root.firstChildElement("package");
    QString pkgname, pageLang;
    QByteArray pageBody, pkgTitle, pkgShortDesc, pkgLongDesc;
    
    while (!package.isNull())
    {
        pkgname = package.attribute("name");
        
        // Récupérer la première page de wiki qui a comme slug <pkgname>-<distro>.
        sql = " SELECT \
                identifier \
                FROM wiki_page \
                WHERE slug='%1' \
                LIMIT 1;";
        
        if (!query.exec(sql
                .arg(pkgname + '-' + distro)) || !query.next())
        {
            log(Error, "Unable to fetch the wiki pages for the package " + pkgname + " in " + distro);
            return false;
        }
        
        int wiki_identifier = query.value(0).toInt();
        
        // Récupérer toutes les pages avec cet identificateur, ce sont les langues
        sql = " SELECT \
                lang, \
                body \
                FROM wiki_page \
                WHERE identifier=%1;";
        
        if (!query.exec(sql
                .arg(wiki_identifier)))
        {
            log(Error, "Unable to fetch the wiki page with identifier " + QString::number(wiki_identifier));
            return false;
        }
        
        while (query.next())
        {
            // Parser la page. Il suffit pour cela de splitter son contenu suivant \n et d'explorer les lignes, 
            // en compant les lignes commençant par #.
            pageLang = query.value(0).toString();
            pageBody = query.value(1).toByteArray();
            
            QList<QByteArray> parts = pageBody.split('\n');
            int titleCount = 0;
            pkgLongDesc.clear();
            
            for (int i=0; i<parts.count(); ++i)
            {
                QByteArray part = parts.at(i).trimmed();
                
                if (part.count() > 0 && part.at(0) == '#')
                {
                    // Un nouveau titre, on entre dans une partie
                    titleCount++;
                    continue;
                }
                
                switch (titleCount)
                {
                    case 1:
                        if (!part.isEmpty())
                        {
                            pkgTitle = part;
                        }
                        break;
                    case 2:
                        if (!part.isEmpty())
                        {
                            pkgShortDesc = part;
                        }
                        break;
                    case 3:
                        if (!pkgLongDesc.isEmpty())
                        {
                            pkgLongDesc += '\n';
                        }
                        pkgLongDesc += part;
                        break;
                }
            }
            
            // Explorer les enfants de package et les compléter comme il faut
            QDomElement child = package.firstChildElement();
            
            while (!child.isNull())
            {
                QString tagName = child.tagName();
                
                if (tagName == "title")
                {
                    setNodeContent(child, pageLang, pkgTitle);
                }
                else if (tagName == "shortdesc")
                {
                    setNodeContent(child, pageLang, pkgShortDesc);
                }
                else if (tagName == "description")
                {
                    setNodeContent(child, pageLang, pkgLongDesc.trimmed());
                }
                
                child = child.nextSiblingElement();
            }
        }
        
        package = package.nextSiblingElement("package");
    }
    
    // Gérer le changelog en fonction des flags et du type de source :
    //              |    Overwrite Changelog     | Pas Overwrite Changelog |
    // ---------------------------------------------------------------------
    // Paquet devel | «New development snapshot» |      Ne rien faire      |
    // Pas devel    |    «Automatic rebuild»     |      Ne rien faire      |
    
    if ((old_flags & SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG) != 0)
    {
        // Le packageur ne souhaite pas qu'on récupère implement son changelog
        // Il peut souhaiter le contraire s'il upload uniquement un paquet source, 
        // sans binaires, contenant déjà le nouvel enregistrement de changelog.
        
        // Ici, simplement rajouter un enregistrement dans le changelog
        QDomElement entry = metadata.createElement("entry");
        
        // Autres valeurs
        entry.setAttribute("distribution", distro);
        entry.setAttribute("email", author.section('<', 1, 1).section('>', 0, 0));
        entry.setAttribute("author", author.section('<', 0, 0).trimmed());
        entry.setAttribute("type", "feature");
        entry.setAttribute("time", QTime::currentTime().toString(Qt::ISODate));
        entry.setAttribute("date", QDate::currentDate().toString(Qt::ISODate));
        entry.setAttribute("version", version);
        
        // Ajouter le texte en anglais et en français
        bool devel = (root.firstChildElement("source").attribute("devel", "false") == "true");
        
        if (devel)
        {
            setNodeContent(entry, "fr", "Nouvel instantané de développement");
            setNodeContent(entry, "en", "New development snapshot");
        }
        else
        {
            setNodeContent(entry, "fr", "Reconstruction automatique");
            setNodeContent(entry, "en", "Automatic rebuild");
        }
        
        // Ajouter cette entrée au changelog
        QDomElement changelog = root.firstChildElement("changelog");
        changelog.insertBefore(entry, changelog.firstChildElement());
    }
    
    // Écrire le fichier metadata.xml
    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        log(Error, "Opening of " + tmpRoot + "/src/metadata.xml for writing failed");
        return false;
    }
    
    fl.write(metadata.toByteArray(4));
    fl.close();
    
    return true;
}

bool Worker::updateDatabase(PackageSystem* &ps)
{
    ps = new PackageSystem(this);
    
    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
    
    ps->setInstallRoot(tmpRoot);
    ps->setConfRoot(tmpRoot);
    ps->setVarRoot(tmpRoot);
    ps->setRunTriggers(false);
    
    ps->loadConfig();
    
    // Mettre à jour la base de donnée, pas besoin d'init, on fera ça après
    if (!ps->update())
    {
        log(Error, "Updating of the database failed");
        psError(ps);
        
        delete ps;
        return false;
    }
    
    return true;
}

bool Worker::installBuildDeps(PackageSystem *ps)
{
    if (!ps->init())     // On peut maintenant qu'on a les fichiers de la BDD
    {
        log(Error, "Unable to init the PackageSystem");
        psError(ps);
        
        delete ps;
        return false;
    }
    
    QStringList deps = depends.split("; ");
    Solver *solver = ps->newSolver();
    
    solver->addPackage("build-essential", Solver::Install); // Besoin de build-essential
    
    foreach (const QString &dep, deps)
    {
        if (dep.isEmpty()) continue;
        
        solver->addPackage(dep, Solver::Install);
    }
    
    // Éliminer les conflits
    deps = conflicts.split("; ");
    
    foreach (const QString &dep, deps)
    {
        if (dep.isEmpty()) continue;
        
        solver->addPackage(dep, Solver::Remove);
    }
    
    if (!solver->solve())
    {
        log(Error, "Solving of the dependencies failed");
        psError(ps);
        
        delete solver;
        delete ps;
        return false;
    }
    
    // Gérer les résultats
    PackageList *packages;
    int tot = solver->results();
    int index = -1;
    
    for (int i=0; i<tot; ++i)
    {
        packages = solver->result(i);
        
        if (!packages->wrong())
        {
            index = i;
            break;
        }
    }
    
    if (index == -1)
    {
        log(Error, "Unable to handle the dependencies of the package : " + depends + " | conflicts : " + conflicts);
        
        for (int j=0; j<tot; ++j)
        {
            packages = solver->result(j);
            
            log(Error, QString("Tried solution %1 :").arg(j+1));
            
            for (int i=0; i<packages->errors(); ++i)
            {
                PackageList::Error *err = packages->error(i);
                QString s;
                
                switch (err->type)
                {
                    case PackageList::Error::SameNameSameVersionDifferentAction:
                        s = QString("%1~%2 wants to be installed and removed")
                                    .arg(err->package)
                                    .arg(err->version);
                        break;
                        
                    case PackageList::Error::SameNameDifferentVersionSameAction:
                        s = QString("%1 wants to be installed or removed at the versions %2 and %3")
                                    .arg(err->package)
                                    .arg(err->version)
                                    .arg(err->otherVersion);
                        break;
                        
                    case PackageList::Error::NoPackagesMatchingPattern:
                        s = QString("Unable to find a package matching %1 to satisfy %2~%3")
                                    .arg(err->pattern)
                                    .arg(err->package)
                                    .arg(err->version);
                        break;
                        
                    case PackageList::Error::UninstallablePackageInstalled:
                        s = QString("%1~%2 wants to be installed but is tagged non-installable")
                                    .arg(err->package)
                                    .arg(err->version);
                        break;
                        
                    case PackageList::Error::UnremovablePackageRemoved:
                        s = QString("%1~%2 wants to be removed but is tagged non-removable")
                                    .arg(err->package)
                                    .arg(err->version);
                        break;
                        
                    case PackageList::Error::UnupdateablePackageUpdated:
                        s = QString("%1~%2 wants to be updated to %3 but is tagged non-updateable")
                                    .arg(err->package)
                                    .arg(err->version)
                                    .arg(err->otherVersion);
                        break;
                }
                
                log(Error, " >> " + s);
            }
        }
        
        delete solver;
        delete ps;
        return false;
    }
    
    // Installer les paquets
    packages = solver->result(index);
    
    if (!packages->process())
    {
        log(Error, "Error when installing the dependencies");
        psError(ps);
        
        delete solver;
        delete ps;
        return false;
    }
    
    delete solver;
    delete ps;
    
    return true;
}

bool Worker::buildChroot()
{
    QProcess *proc = new QProcess(this);
    
    proc->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(processOut()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
    
    proc->start(app->execName(), QStringList() << "--worker" << tmpRoot, QIODevice::ReadOnly);
    
    if (!proc->waitForStarted())
    {
        log(Error, "Cannot launch the worker process");
        
        delete proc;
        return false;
    }
    
    // Attendre
    int rs = dl.exec();
    
    delete proc;
    
    if (rs != 0)
    {
        log(Error, "An error occured during the package building");
        return false;
    }
    
    return true;
}

bool Worker::copyPackages()
{
    QDirIterator it(tmpRoot + "/src", QStringList() << "*.lpk", QDir::Files);
        
    if (!it.hasNext())
    {
        log(Error, "The source seems not to build any binary package");
        return false;
    }
    
    it.next();
    
    while (true)
    {
        if (QFile::exists(it.fileName()))
        {
            if (!QFile::remove(it.fileName()))
            {
                log(Error, it.fileName() + " already exists and cannot be deleted");
                return false;
            }
        }
        
        if (!QFile::copy(it.filePath(), it.fileName()))
        {
            log(Error, "Unable to copy " + it.filePath() + " to " + it.fileName());
            return false;
        }
        
        // Ajouter à la liste des paquets construits
        builtPackages.append(it.fileName());
        
        if (!it.hasNext())
        {
            break;
        }
        
        it.next();
    }
    
    return true;
}

// TODO: Mutex ici aussi car on modifie quelque-chose de global à l'application (currentDir)
bool Worker::importPackages(PackageSystem* &ps, RepositoryManager* &mg)
{
    QString currentDir = QDir::currentPath();
    QDir::setCurrent(app->root());
    
    // On a besoin d'un PackageSystem
    ps = new PackageSystem(this);
        
    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
        
    ps->loadConfig();
    
    // Et d'un repositoryManager
    mg = new RepositoryManager(ps);
    
    if (!mg->loadConfig("config/repo.conf"))
    {
        log(Error, "Initialization of the RepositoryManager failed");
        psError(ps);
        
        delete mg;
        delete ps;
        return false;
    }
    
    QString farch;
    
    for (int i=0; i<builtPackages.count(); ++i)
    {
        const QString &fname = currentDir + "/" + builtPackages.at(i);
        farch = fname.section('.', -2, -2);
        
        if (farch == "src")
        {
            if (!mg->includeSource(fname, false))
            {
                log(Error, "Unable to include the source " + fname);
                psError(ps);
                
                QDir::setCurrent(currentDir);
                delete mg;
                delete ps;
                return false;
            }
        }
        else
        {
            if (!mg->includePackage(fname))
            {
                log(Error, "Unable to include the package " + fname);
                psError(ps);
                
                QDir::setCurrent(currentDir);
                delete mg;
                delete ps;
                return false;
            }
        }
        
        // Plus besoin du fichier
        QFile::remove(fname);
    }
    
    QDir::setCurrent(currentDir);
    
    return true;
}

bool Worker::exportDistros(PackageSystem *ps, RepositoryManager *mg)
{
    if (!mg->exp(QStringList() << distro))
    {
        log(Error, "Unable to export the distribution " + distro);
        psError(ps);
        
        delete mg;
        delete ps;
        return false;
    }
    
    // Nettoyer
    delete mg;
    delete ps;
    return true;
}

bool Worker::cleanupTemp()
{
    // Démonter proc, dev et sys (errno... : c'est peut-être pas monté)
    if (umount(qPrintable(tmpRoot + "/dev")) && errno != EINVAL)
    {
        log(Error, "Unable to umount " + tmpRoot + "/dev");
        return false;
    }
    
    if (umount(qPrintable(tmpRoot + "/proc")) && errno != EINVAL)
    {
        log(Error, "Unable to umount " + tmpRoot + "/proc");
        return false;
    }
    
    if (umount(qPrintable(tmpRoot + "/sys")) && errno != EINVAL)
    {
        log(Error, "Unable to umount " + tmpRoot + "/sys");
        return false;
    }
    
    // Supprimer récursivement le contenu de tmpRoot
    App::recurseRemove(tmpRoot);
    
    return true;
}

void Worker::error(bool cleanup)
{
    flags |= SOURCEPACKAGE_FLAG_FAILED;
    
    // Nettoyer le répertoire de travail si nécessaire
    if (cleanup && QFile::exists(tmpRoot))
    {
        cleanupTemp();
    }
    
    // Envoyer un mail au mainteneur du paquet
    QString content;
    QString to = maintainer.section('<', 1, 1).section('>', 0, 0);
    Mail *mail = new Mail(this);
    
    if (!mail->connectToHost(app->mailServer(), app->mailPort(), app->mailEncrypted())) goto End;
    
    if (app->mailUseTLS())
    {
        if (!mail->startTls()) goto End;
    }
    
    if (!app->mailUser().isNull())
    {
        if (!mail->login(app->mailUser(), app->mailPassword())) goto End;
    }
    
    content  = "Hello " + maintainer.section('<', 0, 0) + ",\n";
    content += "\n";
    content += "The source package " + name + " at the version " + old_version + " failed to compile !\n";
    content += "\n";
    content += "You can see the logs of your package at these addresses :\n";
    content += "\n";
    content += "  * " + app->mailLogRoot() + logFilePath("download") + " : Downloading\n";
    content += "  * " + app->mailLogRoot() + logFilePath("build") + " : Building\n";
    content += "  * " + app->mailLogRoot() + logFilePath("package") + " : Packaging\n";
    content += "\n";
    content += "Cheers,\n";
    content += "The Logram Build Server.";
    
    if (!mail->sendMail("noreply-buildserver@logram-project.org", to, "Package " + name + "~" + old_version + " failed to build", content)) goto End;
    
End:
    endPackage();
}

bool Worker::recurseCopy(const QString& from, const QString& to)
{
    // Copier les fichiers et dossiers de from dans to
    QDir dfrom(from);
    QFileInfoList entries = dfrom.entryInfoList(QDir::Files | QDir::Dirs | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
    
    for (int i=0; i<entries.count(); ++i)
    {
        const QFileInfo &info = entries.at(i);
        
        if (info.isDir())
        {
            // Créer le sous-dossier
            if (!QDir::root().mkpath(to + "/" + info.fileName()))
            {
                log(Error, "Unable to create " + to + "/" + info.fileName());
                return false;
            }
            
            // Copier ses fichiers
            if (!recurseCopy(from + "/" + info.fileName(), to + "/" + info.fileName()))
            {
                // Log déjà géré
                return false;
            }
        }
        else if (info.isFile())
        {
            if (!QFile::copy(from + "/" + info.fileName(), to + "/" + info.fileName()))
            {
                log(Error, "Unable to copy " + from + "/" + info.fileName() + " to " + to + "/" + info.fileName());
                return false;
            }
        }
        else if (info.isSymLink())
        {
            // Lien symbolique pointant sur rien, Qt ne sait pas le copier
            QFile::link(dfrom.relativeFilePath(info.symLinkTarget()), to + "/" + info.fileName());
        }
    }
    
    return true;
}

void Worker::endPackage()
{
    QSqlQuery query(app->database());
    QString sql;
    
    // Tous les logs de ce paquet et cette distribution doivent avoir des flags sans
    // LATEST, REBUILD et CONTINUOUS, seulement si cette construction n'a pas eu d'erreurs
    if ((flags & SOURCEPACKAGE_FLAG_FAILED) == 0)
    {
        sql = " UPDATE packages_sourcelog \
                SET flags=flags & ~(%1) \
                WHERE source_id=%2 \
                AND distribution_id=%3 \
                AND arch_id=%4;";
        
        if (!query.exec(sql
                .arg(SOURCEPACKAGE_FLAG_LATEST | SOURCEPACKAGE_FLAG_REBUILD | SOURCEPACKAGE_FLAG_CONTINUOUS)
                .arg(source_id)
                .arg(distro_id)
                .arg(arch_id)))
        {
            log(Warning, "Impossible to clear the flags of older Source Log entries");
            flags |= SOURCEPACKAGE_FLAG_WARNINGS;
        }
        
        flags |= SOURCEPACKAGE_FLAG_LATEST;
    }
    
    // Mettre à jour les flags de new_log_id
    sql = " UPDATE packages_sourcelog \
            SET flags=%1 \
            WHERE id=%2;";
    
    if (!query.exec(sql
            .arg(flags | SOURCEPACKAGE_FLAG_LATEST)
            .arg(new_log_id)))
    {
        log(Error, "Impossible to update the new log entry");
        return;
    }
}

/* Copie conforme de copy_data se trouvant dans 
   http://code.google.com/p/libarchive/source/browse/trunk/examples/untar.c */
int Worker::copyData(struct archive *ar, struct archive *aw)
{
    int r;
    const void *buff;
    size_t size;
    off_t offset;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        
        if (r == ARCHIVE_EOF)
        {
            return ARCHIVE_OK;
        }
        
        if (r != ARCHIVE_OK)
        {
            return r;
        }
        
        r = archive_write_data_block(aw, buff, size, offset);
        
        if (r != ARCHIVE_OK) 
        {
            return r;
        }
    }
}

void Worker::setNodeContent(QDomElement &child, const QString &lang, const QByteArray &content)
{
    QDomElement l = child.firstChildElement();
    
    while (!l.isNull())
    {
        if (l.tagName() == lang)
        {
            // Explorer les QDomNode enfants de cet élément et remplacer le contenu du texte
            QDomNode node = l.firstChild();
            
            while (!node.isNull())
            {
                if (node.isText())
                {
                    QDomText txt = node.toText();
                    txt.setData(QString::fromUtf8(content));
                    return;
                }
                
                node = node.nextSibling();
            }
            
            // Ah ? On n'avait pas déjà du texte ? En ajouter
            QDomText txt = child.ownerDocument().createTextNode(QString::fromUtf8(content));
            l.appendChild(txt);
            return;
        }
        
        l = l.nextSiblingElement();
    }
    
    // On n'a pas encore cette langue
    l = child.ownerDocument().createElement(lang);
    QDomText txt = child.ownerDocument().createTextNode(QString::fromUtf8(content));
    l.appendChild(txt);
    child.appendChild(l);
    
    return;
}

QString Worker::logFilePath(const QString &operation)
{
    int part = (log_id >> 10) << 10;
    
    QString fileName = "logs/%1-%2/%3_%4";
    
    fileName = fileName.arg(part).arg(part + 1024).arg(operation).arg(log_id);
    
    return fileName;
}

void Worker::truncateLogs()
{
    QFile::remove(repoRoot + logFilePath("download"));
    QFile::remove(repoRoot + logFilePath("build"));
    QFile::remove(repoRoot + logFilePath("package"));
}

void Worker::processOut()
{
    QProcess *sh = static_cast<QProcess *>(sender());
    
    if (sh == 0)
    {
        return;
    }
    
    // Lire les lignes
    QByteArray line;
    
    while (sh->bytesAvailable() > 0)
    {
        line = sh->readLine().trimmed();
        
        // Le processus enfant peut nous envoyer des changements d'états et des logs, gérer ça
        if (line.startsWith(SERVER_COMMUNICATION_TOKEN))
        {
            line.remove(0, 40);
            char c = line.at(0);
            
            line.remove(0, 1);
            
            switch (c)
            {
                case '1':
                    setState(Building);
                    log(Operation, "Beginning of the build");
                    break;
                case '2':
                    setState(Packaging);
                    log(Operation, "Beginning of the packaging");
                    break;
                case 'E':
                    log(Error, line);
                    break;
                case 'W':
                    log(Warning, line);
                    break;
                case 'O':
                    log(Operation, line);
                    break;
                case 'P':
                    log(Progression, line);
                    break;
            }
        }
        else
        {
            log(ProcessOut, line);
        }
    }
}

void Worker::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    int rs = 0;
    
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        rs = 1;
    }
    
    dl.exit(rs);
}

void Worker::setState(State _state)
{
    // Fermer le fichier
    if (logFile.isOpen())
    {
        logFile.close();
    }
    
    if (_state == General)
    {
        state = _state;
        return;
    }
    
    // Nom du log
    QString logName;
    
    switch (_state)
    {
        case Downloading:
            logName = "download";
            break;
            
        case Building:
            logName = "build";
            break;
            
        case Packaging:
            logName = "package";
            break;
            
        default:
            return;
    }
    
    // Trouver le bon sous-dossier de $Repo/logs dans lequel placer le log
    QString fileName = repoRoot + logFilePath(logName);
    
    QDir::root().mkpath(fileName.section('/', 0, -2));
    logFile.setFileName(fileName);
    
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        log(Error, "Unable to open the log " + fileName);
    }
    
    // Affecter la variable du status
    state = _state;
}

void Worker::log(LogType type, const QString &message)
{
    // Afficher le log si on débogue
    if (app->verbose() || state == General)
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
    
    // Écrire le log dans le fichier de log du paquet en cours de construction
    if (state != General)
    {
        QByteArray header;
        
        switch (type)
        {
            case Message:
                header = "MM: ";
                break;
            case Operation:
                header = "OO: ";
                break;
            case Progression:
                header = "PP: ";
                break;
            case Warning:
                header = "WW: ";
                break;
            case Error:
                header = "EE: ";
                break;
            case ProcessOut:
                header = ">>  ";
                break;
        }
        
        logFile.write(header);
        logFile.write(message.toUtf8());
        logFile.write("\n");
    }
}

void Worker::psError(PackageSystem *ps)
{
    log(Error, App::psErrorString(ps));
}

void Worker::progress(Progress *progress)
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
    else if (progress->type == Progress::Download)
    {
        return;
    }
    
    log(Progression, App::progressString(progress) + progress->info);
}

#include "worker.moc"
