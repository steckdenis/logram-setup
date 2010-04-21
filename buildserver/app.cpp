/*
 * app.cpp
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

#include "app.h"

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

#include <logram/repositorymanager.h>
#include <logram/packagesystem.h>
#include <logram/package.h>
#include <logram/solver.h>
#include <logram/packagelist.h>
#include <logram/packagesource.h>

#include <iostream>
#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>             // Pour chroot

using namespace std;
using namespace Logram;

#define QUERY_ERROR(query) \
    log(Error, "Query failed : " + query.lastQuery());

#define SERVER_COMMUNICATION_TOKEN "$[Logram Build Server Communication$$]$*"
                                   
static void recurseRemove(const QString &, int);

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    // Initialiser
    error = false;
    debug = false;
    worker = false;
    quitApp = false;
    state = General;
    confFileName = "buildserver.conf";
    
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    // Parser la ligne de commande
    QStringList args = arguments();
    QString arg;
    
    args.takeFirst();   // Pas besoin du nom de l'application
    
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
        else if (arg == "--worker")
        {
            worker = true;
            error = workerProcess();
            
            quitApp = true;
            return;
        }
        else if (arg == "--cleanup")
        {
            recurseRemove("tmp", 0);
            
            quitApp = true;
            return;
        }
    }
    
    // Configuration
    set = new QSettings(confFileName, QSettings::IniFormat, this);
    
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

void App::cleanup()
{
    if (logFile.isOpen())
    {
        logFile.close();
    }
}

bool App::workerProcess()
{
    // Chrooter dans tmp/, puis cd src/, puis construire le paquet
    // Renvoyer true si erreur
    
    // Chroot
    int rs = chroot("tmp");
    
    if (rs != 0)
    {
        log(Error, "Chroot in tmp/ failed");
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
            this, SLOT(progress(Logram::Progress *)));
        
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

void App::buildPackage()
{
    // Lancer la construction d'un paquet
    bool rs = buildWorker();
    
    setState(General, 0);
    
    if (rs)
    {
        // On a construit un paquet, lancer un timer sur ce slot dans une seconde
        log(Message, "Package built, building an other package in 10 second");
        QTimer::singleShot(10*1000, this, SLOT(buildPackage()));
    }
    else
    {
        // On n'a pas trouvé de paquet, attendre le temps qu'il faut
        log(Message, "No package to build, waiting 1 minute");
        QTimer::singleShot(60*1000, this, SLOT(buildPackage()));
    }
}

void App::endPackage(int log_id, int flags)
{
    QSqlQuery query(db);
    QString sql;
    
    // Obtenir des informations sur log_id
    sql = " SELECT \
            distribution_id, \
            source_id, \
            arch_id \
            FROM packages_sourcelog \
            WHERE id=%1;";
    
    if (!query.exec(sql.arg(log_id)) || !query.next())
    {
        log(Error, "Unable to get informations about the log id " + QString::number(log_id));
        return;
    }
    
    int distro_id = query.value(0).toInt();
    int source_id = query.value(1).toInt();
    int arch_id = query.value(2).toInt();
    
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
    
    // Mettre à jour les flags de log_id
    sql = " UPDATE packages_sourcelog \
            SET flags=%1 \
            WHERE id=%2;";
    
    if (!query.exec(sql
            .arg(flags)
            .arg(log_id)))
    {
        log(Error, "Impossible to update the new log entry");
        return;
    }
}

/* Copie conforme de copy_data se trouvant dans 
   http://code.google.com/p/libarchive/source/browse/trunk/examples/untar.c */
static int copy_data(struct archive *ar, struct archive *aw)
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

static void setNodeContent(QDomElement &child, const QString &lang, const QByteArray &content)
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

static void recurseRemove(const QString &path, int level)
{
    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    
    for (int i=0; i<list.count(); ++i)
    {
        const QFileInfo &info = list.at(i);
        
        // Ne pas supprimer certains fichiers
        if (level == 1 && info.isFile() && info.fileName() == "metadata.xml")
        {
            continue;
        }
        
        if (level == 1 && info.isDir() && info.fileName() == "control")
        {
            continue;
        }
        
        if (level == 0 && info.isDir() &&
                (info.fileName() == "dev" ||
                 info.fileName() == "proc" ||
                 info.fileName() == "sys"))
        {
            continue;
        }
        
        if (level == 1 && (info.isFile() || info.isSymLink()) &&
                (info.fileName() == "hosts" ||
                 info.fileName() == "hostname" ||
                 info.fileName() == "resolv.conf" ||
                 info.fileName() == "sh"))
        {
            continue;
        }
        
        // Supprimer
        if (info.isDir())
        {
            recurseRemove(info.absoluteFilePath(), level + 1);
            
            if (level == 0 && info.isDir() && info.fileName() == "tmp")
            {
                // Laisser tmp, vide
                continue;
            }
            
            dir.rmdir(info.fileName());
        }
        else if (info.isFile() || info.isSymLink())
        {
            dir.remove(info.fileName());
        }
    }
}

static QString logFilePath(const QString &operation, int log_id)
{
    int part = (log_id >> 6) << 6;
    
    QString fileName = "logs/%1-%2/%3_%4";
    
    fileName = fileName.arg(part).arg(part + 64).arg(operation).arg(log_id);
    
    return fileName;
}

static void truncateLogs(const QString &root, int log_id)
{
    QFile::remove(root + logFilePath("download", log_id));
    QFile::remove(root + logFilePath("build", log_id));
    QFile::remove(root + logFilePath("package", log_id));
}

bool App::buildWorker()
{
    QString arch = set->value("Arch").toString();
    QSqlQuery query(db);
    QString sql;
    
    log(Operation, "Fetching a package to build...");
    
    // Sélectionner les paquets voulant être reconstruits
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
            WHERE log.arch_id=%1 AND (log.flags & %2 ) != 0\
            ORDER BY log.date_rebuild_asked ASC \
            LIMIT 1;";
    
    if (!query.exec(sql
            .arg(archIds.value(arch))
            .arg(SOURCEPACKAGE_FLAG_REBUILD | SOURCEPACKAGE_FLAG_CONTINUOUS)))
    {
        QUERY_ERROR(query)
        return false;
    }
    
    if (!query.next())
    {
        // Pas de paquet à construire
        return false;
    }
    
    // On a un paquet à construire
    log_id = query.value(0).toInt();
    QString name = query.value(1).toString();
    QString old_version = query.value(2).toString();
    QString distro = query.value(3).toString();
    QString depends = query.value(4).toString();
    QString suggests = query.value(5).toString();
    QString conflicts = query.value(6).toString();
    int log_flags = query.value(7).toInt();
    QString author = query.value(8).toString();
    int source_id = query.value(9).toInt();
    QString maintainer = query.value(10).toString();
    QString upstream_url = query.value(11).toString();
    int distro_id = query.value(12).toInt();
    QString license = query.value(13).toString();
    int arch_id = query.value(14).toInt();
    int new_flags = 0;
    
    // Si arch_id vaut zéro, c'est une importation manuelle de source qui a été faite, trouver l'arch
    if (arch_id == 0)
    {
        arch_id = archIds.value(arch);
    }
    
    // Calculer la nouvelle version
    QString version = old_version;
    
    if ((log_flags & SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG) != 0)
    {
        QString mainvers = old_version.section('~', 0, -2);
        int logramVer = old_version.section('~', -1, -1).toInt();
        version = mainvers + '~' + QString::number(logramVer + 1);
        
        new_flags |= SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG;
    }
    
    // Gérer les always_rebuild
    QString rebuild_date = "NULL";
    
    if ((log_flags & SOURCEPACKAGE_FLAG_CONTINUOUS) != 0)
    {
        new_flags |= SOURCEPACKAGE_FLAG_CONTINUOUS | SOURCEPACKAGE_FLAG_REBUILD;
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
        return true;
    }
    
    // Prendre le nouveau log_id
    log_id = query.lastInsertId().toInt();
    
    // État pour pouvoir bien logguer
    truncateLogs(set->value("Repository/Root").toString(), log_id);
    setState(Downloading, log_id);
    
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
    
    // Récupérer le paquet
    QString srcFileName = set->value("Repository/Root").toString() + "pool/%1/%2/%3~%4.src.lpk";
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
    curDir.mkpath("tmp/src");
    
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
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        archive_entry_free(entry);
        archive_write_finish(ext);
        archive_read_finish(a);
        
        return true;
    }
    
    QByteArray path, filePath;
    int r;
    
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
            endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
            
            archive_entry_free(entry);
            archive_write_finish(ext);
            archive_read_finish(a);
            
            return true;
        }
        
        path = QByteArray(archive_entry_pathname(entry));
        filePath = "tmp/src/" + path;
        
        archive_entry_set_pathname(entry, filePath.constData());
        
        if (archive_write_header(ext, entry) != ARCHIVE_OK ||
            copy_data(a, ext) != ARCHIVE_OK || 
            archive_write_finish_entry(ext) != ARCHIVE_OK)
        {
            log(Error, "Unable to extract the file " + filePath);
            endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
            
            archive_entry_free(entry);
            archive_write_finish(ext);
            archive_read_finish(a);
            
            return true;
        }
    }
    
    // Paquet extrait
    archive_entry_free(entry);
    archive_write_finish(ext);
    archive_read_finish(a);
    
    // Modifications à faire sur le fichier metadata.xml : 
    // o Récupérer les textes des pages de wiki
    // o Gérer le changelog
    log(Operation, "Patching metadata.xml with the contents of the wiki and the changelog");
    
    QDomDocument metadata;
    QFile fl("tmp/src/metadata.xml");
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        log(Error, "Opening of tmp/src/metadata.xml for reading failed");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        return true;
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
            endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
            return true;
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
            endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
            return true;
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
    
    if ((log_flags & SOURCEPACKAGE_FLAG_OVERWRITECHANGELOG) != 0)
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
        log(Error, "Opening of tmp/src/metadata.xml for writing failed");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        return true;
    }
    
    fl.write(metadata.toByteArray(4));
    fl.close();
    
    // Faire ça pour chaque architecture
    QString enabledDistros = set->value("DistroDeps/" + distro, distro).toString();
    
#if 1
    // S'assurer d'être dans le bon état
    setState(Downloading, log_id);
    log(Operation, "Beginning of the build of " + name + " for the architecture " + arch);
    curArch = arch;
    
    // Générer les fichiers nécessaires au Setup chrooté :
    // /etc/lgrpkg/sources.list
    // /etc/lgrpkg/scripts/weight.qs (pris d'une resource)
    // /var/cache/lgrpkg/{db/pkgs,download}
    curDir.mkpath("tmp/etc/lgrpkg/scripts");
    curDir.mkpath("tmp/var/cache/lgrpkg/db/pkgs");
    curDir.mkpath("tmp/var/cache/lgrpkg/download");
    curDir.mkpath("tmp/usr/libexec");
    
    // Sources.list
    QSettings sourcesList("tmp/etc/lgrpkg/sources.list", QSettings::IniFormat, this);
    
    sourcesList.setValue("Language", "en");
    sourcesList.beginGroup("repo");
    sourcesList.setValue("Type", "local");
    sourcesList.setValue("Mirrors", set->value("Repository/Root").toString());
    sourcesList.setValue("Distributions", enabledDistros);
    sourcesList.setValue("Archs", arch + " all");
    sourcesList.setValue("Active", true);
    sourcesList.setValue("Description", "Automatic building repository");
    sourcesList.setValue("Sign", false);
    sourcesList.endGroup();
    
    sourcesList.sync();
    
    // weight.qs
    QFile in(":weight.qs");
    QFile out("tmp/etc/lgrpkg/scripts/weight.qs");
    
    if (!in.open(QIODevice::ReadOnly))
    {
        log(Error, "Unable to open the resource :weight.qs");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        recurseRemove("tmp", 0);
        return true;
    }
    
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        log(Error, "Unable to open tmp/etc/lgrpkg/scripts/weight.qs for writing");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        recurseRemove("tmp", 0);
        return true;
    }
    
    out.write(in.readAll());
    out.close();
    
    // /usr/libexec/scriptapi
    if (!QFile::copy("/usr/libexec/scriptapi", "tmp/usr/libexec/scriptapi"))
    {
        log(Error, "Unable to copy /usr/libexec/scriptapi to tmp/usr/libexec/scriptapi");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Initialiser la gestion des paquets
    log(Operation, "Updating the repository informations");
    PackageSystem *ps = new PackageSystem(this);
    QString rootDir = QDir::currentPath() + "/tmp";
    
    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
    
    ps->setInstallRoot(rootDir);
    ps->setConfRoot(rootDir);
    ps->setVarRoot(rootDir);
    ps->setRunTriggers(false);
    
    ps->loadConfig();
    
    // Mettre à jour la base de donnée, pas besoin d'init, on fera ça après
    if (!ps->update())
    {
        log(Error, "Updating of the database failed");
        psError(ps);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete ps;
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Installer les dépendances de construction de ce paquet
    if (!ps->init())     // On peut maintenant qu'on a les fichiers de la BDD
    {
        log(Error, "Unable to init the PackageSystem");
        psError(ps);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete ps;
        recurseRemove("tmp", 0);
        return true;
    }
    
    log(Operation, "Installation of the build dependencies");
    
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
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete solver;
        delete ps;
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Gérer les résultats
    PackageList *packages;
    int tot = solver->results();
    int index = -1;
    
    for (int i=0; i<tot; ++i)
    {
        packages = solver->result(i);
        
        if (!packages->wrong() && index == -1)
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
        
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete solver;
        delete ps;
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Installer les paquets
    packages = solver->result(index);
    
    if (!packages->process())
    {
        log(Error, "Error when installing the dependencies");
        psError(ps);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete solver;
        delete ps;
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Nettoyer de l'installation
    for (int i=0; i<tot; ++i)
    {
        packages = solver->result(i);
        
        delete packages;
    }
    
    delete solver;
    
    // On a fini la préparation
    delete ps;
    
    // Lancer la construction du paquet
    log(Operation, "Launching of a child process, chrooted in the working directory, to build the package");
    
    QProcess *proc = new QProcess(this);
    
    proc->setProcessChannelMode(QProcess::MergedChannels);
    
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(processOut()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
    
    proc->start(arguments().at(0), QStringList() << "--worker", QIODevice::ReadOnly);
    
    if (!proc->waitForStarted())
    {
        log(Error, "Cannot launch the worker process");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete proc;
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Attendre
    int rs = dl.exec();
    
    delete proc;
    
    if (rs != 0)
    {
        log(Error, "An error occured during the package building");
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        recurseRemove("tmp", 0);
        return true;
    }
    
    // Le paquet est maintenant construit, il y a des fichiers .lpk dans tmp/src
    setState(Packaging, log_id);
    log(Operation, "Copying of the built packages in the working directory");
    
    {
        QDirIterator it("tmp/src", QStringList() << "*.lpk", QDir::Files);
        
        if (!it.hasNext())
        {
            log(Error, "The source seems not to build any binary package");
            endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
            
            recurseRemove("tmp", 0);
            return true;
        }
        
        it.next();
        
        while (true)
        {
            if (QFile::exists(it.fileName()))
            {
                if (!QFile::remove(it.fileName()))
                {
                    log(Error, it.fileName() + " already exists and cannot be deleted");
                    endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
                    
                    recurseRemove("tmp", 0);
                    return true;
                }
            }
            
            if (!QFile::copy(it.filePath(), it.fileName()))
            {
                log(Error, "Unable to copy " + it.filePath() + " to " + it.fileName());
                endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
                
                recurseRemove("tmp", 0);
                return true;
            }
            
            if (!it.hasNext())
            {
                break;
            }
            
            it.next();
        }
    }
    
    // Effacer sélectivement le contenu de tmp
    log(Operation, "Cleaning up the temporary directory");
    recurseRemove("tmp", 0);
#endif
#if 1
    // On a maintenant les fichiers .lpk dans ce dossier. Les importer
    setState(Packaging, log_id);
    log(Operation, "Including of all the packages built");
    
    QString currentDir = QDir::currentPath();
    QDir::setCurrent(set->value("Repository/Root").toString());
    
    // On a besoin d'un PackageSystem
    ps = new PackageSystem(this);
        
    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
        
    ps->loadConfig();
    
    // Et d'un repositoryManager
    RepositoryManager *mg = new RepositoryManager(ps);
    
    if (!mg->loadConfig("config/repo.conf"))
    {
        log(Error, "Initialization of the RepositoryManager failed");
        psError(ps);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete mg;
        delete ps;
        return true;
    }
    
    // Explorer les fichiers .lpk du répertoire courant
    QDirIterator it(currentDir, QStringList() << "*.lpk", QDir::Files);
    
    if (!it.hasNext())
    {
        log(Error, "No .lpk files found in " + currentDir);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete mg;
        delete ps;
        return true;
    }
    
    it.next();
    
    QString farch, fname;
    
    while (true)
    {
        fname = it.filePath();
        farch = fname.section('.', -2, -2);
        
        if (farch == "src")
        {
            if (!mg->includeSource(fname, false))
            {
                log(Error, "Unable to include the source " + fname);
                psError(ps);
                endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
                
                delete mg;
                delete ps;
                return true;
            }
        }
        else
        {
            if (!mg->includePackage(fname))
            {
                log(Error, "Unable to include the package " + fname);
                psError(ps);
                endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
                
                delete mg;
                delete ps;
                return true;
            }
        }
        
        // Plus besoin du fichier
        QFile::remove(fname);
        
        if (!it.hasNext())
        {
            break;
        }
        
        it.next();
    }
    
    // Les paquets sont inclus, plus qu'à exporter la distribution
    log(Operation, "Exportation of the distribution");
    
    if (!mg->exp(QStringList() << distro))
    {
        log(Error, "Unable to export the distribution " + distro);
        psError(ps);
        endPackage(log_id, new_flags | SOURCEPACKAGE_FLAG_FAILED);
        
        delete mg;
        delete ps;
        return true;
    }
    
    // Nettoyer
    delete mg;
    delete ps;
#endif
    endPackage(log_id, new_flags);

    // On a enfin fini !
    setState(General, 0);

    return true;
}

void App::processOut()
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
                    setState(Building, log_id);
                    log(Operation, "Beginning of the build for the architecture " + curArch);
                    break;
                case '2':
                    setState(Packaging, log_id);
                    log(Operation, "Beginning of the packaging for the architecture " + curArch);
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

void App::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    int rs = 0;
    
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        rs = 1;
    }
    
    dl.exit(rs);
}

bool App::failed() const
{
    return error;
}

bool App::mustExit() const
{
    return quitApp;
}

void App::setState(State _state, int log_id)
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
    QString fileName = set->value("Repository/Root").toString() + logFilePath(logName, log_id);
    
    QDir::root().mkpath(fileName.section('/', 0, -2));
    logFile.setFileName(fileName);
    
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        log(Error, "Unable to open the log " + fileName);
    }
    
    // Affecter la variable du status
    state = _state;
}

void App::log(LogType type, const QString &message)
{
    // Afficher le log si on débogue
    if ((debug || state == General) && !worker)
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
    if (state != General && !worker)
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
    else if (worker)
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
    PackageError *err = ps->lastError();
    
    if (err == 0)
    {
        // Interruption sans erreur, ou erreur inconnue
        log(Error, "Unknown error");
        return;
    }
    
    QString s;

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
    
    log(Error, s + err->info);
    
    if (!err->more.isEmpty())
    {
        log(Error, " >> " + err->more);
    }
    
    // Plus besoin de l'erreur
    delete err;
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
            
        case Progress::ProcessOut:
            log(ProcessOut, progress->more);
            return;
            
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
            return;
    }
    
    log(Progression, s + progress->info);
}