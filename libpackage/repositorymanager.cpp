/*
 * repositorymanager.cpp
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

#include "config.h"
#include "repositorymanager.h"
#include "packagesystem.h"
#include "filepackage.h"
#include "packagemetadata.h"

#include <QSettings>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDateTime>
#include <QVector>

#include <QtSql>
#include <QtXml>
#include <QtDebug>

#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>

#ifdef GPGME_FOUND
    #include <gpgme.h>
#endif

using namespace Logram;

struct RepositoryManager::Private
{
    PackageSystem *ps;
    QSettings *set;
    
    // BDD
    QSqlDatabase db;
    QRegExp regex;
    QRegExp slugRegex, slugRegex2;
    bool websiteIntegration;
    
    // Fonctions
    bool registerString(QSqlQuery &query, int package_id, const QString &lang, const QString &cont, int type, int changelog_id = 0);
    bool writeXZ(const QString &fileName, const QByteArray &data);
    QString slugify(const QString &str);
    int sourcePackageId(const QString &name);
};

RepositoryManager::RepositoryManager(PackageSystem *ps) : QObject(ps)
{
    d = new Private;
    d->ps = ps;
    
    d->regex = QRegExp("\\n[ \\t]+");
    d->slugRegex = QRegExp("[^\\w\\s-]");
    d->slugRegex2 = QRegExp("[-\\s]+");
}

RepositoryManager::~RepositoryManager()
{
    delete d;
}
        
bool RepositoryManager::loadConfig(const QString &fileName)
{
    d->set = new QSettings(fileName, QSettings::IniFormat, this);
    
    // Se connecter à la base de donnée
    QSqlDatabase &db = d->db;
    
    // Si l'application cliente s'est déjà connectée, ne pas le refaire (en plus, c'est peristant)
    if (QSqlDatabase::contains("logram_repo_database"))
    {
        db = QSqlDatabase::database("logram_repo_database");
    }
    else
    {
        db = QSqlDatabase::addDatabase("QMYSQL", "logram_repo_database");
        db.setHostName(d->set->value("Database/Hostname", "localhost").toString());
        db.setDatabaseName(d->set->value("Database/Name").toString());
        db.setUserName(d->set->value("Database/User", "root").toString());
        db.setPassword(d->set->value("Database/Password").toString());
        
        if (!db.open())
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenDatabaseError;
            err->info = db.userName() + ':' + db.password() + '@' + db.hostName() + ' ' + db.databaseName();
            
            d->ps->setLastError(err);
            
            return false;
        }
    }
    
    d->websiteIntegration = d->set->value("WebsiteIntegration", false).toBool();
    
    return true;
}

#define TRY_QUERY(q) \
    if (!query.exec(q) || !query.next()) \
    { \
        PackageError *err = new PackageError; \
        err->type = PackageError::QueryError; \
        err->info = query.lastQuery(); \
        \
        d->ps->setLastError(err); \
        \
        return false; \
    }

#define DEPEND(type) fpkg->dependsToString(fpkg->depends(), type)

#define LANGPKGKEY(pkid, langindex) (((pkid) << 8) + (langindex))

static QString e(const QString &str)
{
    QString rs(str);
    
    rs.replace('\\', "\\\\");
    rs.replace('"', "\\\"");
    rs.replace('\'', "\\'");
    rs.replace('%', "\\%");
    
    return rs;
}

QString RepositoryManager::Private::slugify(const QString &str)
{
    // Implémentation en C++ du code Python :
    /* def slugify(value):
     *     """
     *     Normalizes string, converts to lowercase, removes non-alpha characters,
     *     and converts spaces to hyphens.
     *     """
     *     import unicodedata
     *     import re
     *     value = unicodedata.normalize('NFKD', value).encode('ascii', 'ignore')
     *     value = unicode(re.sub('[^\w\s-]', '', value).strip().lower())
     *     return re.sub('[-\s]+', '-', value)
     */
    QString rs;
    rs = str.normalized(QString::NormalizationForm_KD);
    rs = rs.remove(slugRegex);
    rs = rs.trimmed().toLower();
    return rs.replace(slugRegex2, "-");
}

bool RepositoryManager::Private::writeXZ(const QString &fileName, const QByteArray &data)
{
    QProcess xz;
    xz.start("xz", QStringList() << "-c");
        
    if (!xz.waitForStarted())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = "xz -c";
       
        ps->setLastError(err);
       
        return false;
    }
        
    xz.write(data);
    xz.closeWriteChannel();
        
    if (!xz.waitForFinished())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = "xz -c";
            
        ps->setLastError(err);
            
        return false;
    }
        
    QFile fl(fileName);
        
    if (!fl.open(QIODevice::WriteOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fileName;
          
        ps->setLastError(err);
           
        return false;
    }
        
    fl.write(xz.readAll());
    fl.close();
    
    return true;
}

bool RepositoryManager::Private::registerString(QSqlQuery &query, int package_id, const QString &lang, const QString &cont, int type, int changelog_id)
{
    // Récupérer l'id de la chaîne
    QString content = cont;
    QString sql, ctn;
    int str_id;
    
    sql = " SELECT \
            id, \
            content \
            FROM packages_string \
            WHERE package_id=%1 \
            AND language='%2' \
            AND type=%3 \
            AND changelog_id='%4';";
            
    if (!query.exec(sql
                    .arg(package_id)
                    .arg(e(lang))
                    .arg(type)
                    .arg(changelog_id)
                    ))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        ps->setLastError(err);
        
        return false;
    }
    
    if (query.next())
    {
        str_id = query.value(0).toInt();
        ctn = query.value(1).toString();
        
        if (ctn != content)
        {
            sql = " UPDATE packages_string SET \
                    content='%1' \
                    WHERE id=%2;";
           
            if (!query.exec(sql
                            .arg(e(content.replace(regex, "\n")))
                            .arg(str_id)
                            ))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                ps->setLastError(err);
                
                return false;
            }
        }
    }
    else
    {
        sql = " INSERT INTO packages_string (package_id, language, type, content, changelog_id) \
                VALUES(%1, '%2', %3, '%4', %5);";
        
        if (!query.exec(sql
                        .arg(package_id)
                        .arg(e(lang))
                        .arg(type)
                        .arg(e(content.replace(regex, "\n")))
                        .arg(changelog_id)
                        ))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::QueryError;
            err->info = query.lastQuery();
            
            ps->setLastError(err);
            
            return false;
        }
    }
    
    return true;
}

int RepositoryManager::Private::sourcePackageId(const QString &name)
{
    QSqlQuery query(db);
    QString sql;
    
    sql = " SELECT \
            id \
            FROM packages_sourcepackage \
            WHERE name='%1';";
    
    if (!query.exec(sql
            .arg(e(name))))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        ps->setLastError(err);
        
        return -1;
    }
    
    if (query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        // Le paquet source n'existe pas encore, l'ajouter
        sql = " INSERT INTO packages_sourcepackage \
                    (name, topic_id) \
                VALUES ('%1', 0);";
        
        if (!query.exec(sql
                .arg(e(name))))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::QueryError;
            err->info = query.lastQuery();
            
            ps->setLastError(err);
            
            return -1;
        }
        
        int source_id = query.lastInsertId().toInt();
        
        // Créer le sujet pour les commentaires
        if (websiteIntegration)
        {
            sql = " INSERT INTO forum_topic \
                        (parent_id, p_type, author_id, lang, title, subtitle, last_post_id, last_post_page, num_posts, stick, closed, resolved, poll_id) \
                    VALUES (%1, 3, NULL, 'en', '%2', '', NULL, 1, 0, 0, 0, 0, NULL);";
            
            if (!query.exec(sql
                    .arg(source_id)
                    .arg(e(name))))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                ps->setLastError(err);
                
                return -1;
            }
            
            int topic_id = query.lastInsertId().toInt();
            
            // Mettre à jour le topic_id du paquet
            sql = " UPDATE packages_sourcepackage \
                    SET topic_id=%1 \
                    WHERE id=%2;";
            
            if (!query.exec(sql
                    .arg(topic_id)
                    .arg(source_id)))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                ps->setLastError(err);
                
                return -1;
            }
        }
        
        return source_id;
    }
}

bool RepositoryManager::includeSource(const QString &fileName, bool appendHistory)
{
    QSqlQuery query(d->db);
    QString sql;
    
    // Copier le paquet dans le dossier pool
    QString download_url = "pool/%1/%2/%3";
    QString dd;
    QString fname = fileName.section('/', -1, -1);
    QString pkname = fname.section('~', 0, 0);
    QString arch = fname.section('.', -2, -2);
    
    if (arch != "src")
    {
        // This file MUST be a source package
        return false;
    }
    
    if (pkname.startsWith("lib"))
    {
        dd = pkname.left(4);
    }
    else
    {
        dd = pkname.left(1);
    }
    
    download_url = download_url.arg(dd, pkname, fname);
    
    // Copier le paquet, enregistrer les métadonnées
    QDir curDir = QDir::current();
    
    // Fichier du paquet
    if (QFile::exists(download_url))
    {
        QFile::remove(download_url);
    }
    
    if (!curDir.mkpath(download_url.section('/', 0, -2)))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = download_url.section('/', 0, -2);
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (!QFile::copy(fileName, download_url))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = download_url;
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (appendHistory)
    {
        // L'enregistrer dans la base de donnée
        int source_id = d->sourcePackageId(pkname);
        
        if (source_id == -1)
        {
            return false;
        }
        
        // Trouver sa distribution
        struct archive *a;
        struct archive_entry *entry;
        int r;
        
        a = archive_read_new();
        archive_read_support_compression_lzma(a);
        archive_read_support_compression_xz(a);
        archive_read_support_format_all(a);
        
        r = archive_read_open_filename(a, qPrintable(fileName), 10240);
        
        if (r != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = download_url;
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        QByteArray path;
        QDomDocument metadata;
        int size;
        char *buffer;
        
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
        {
            path = QByteArray(archive_entry_pathname(entry));
            
            if (path == "metadata.xml")
            {
                // Lire le fichier
                size = archive_entry_size(entry);
                buffer = new char[size];
                archive_read_data(a, buffer, size);
                
                metadata.setContent(QByteArray(buffer, size));
                
                delete[] buffer;
                break;
            }
        }
        
        r = archive_read_finish(a);
        
        if (r != ARCHIVE_OK)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = download_url;
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        // On a maintenant un document DOM permettant d'avoir toutes les infos voulues sur la source (dépendances, mainteneur, et distribution)
        QDomElement root = metadata.documentElement();
        QDomElement source = root.firstChildElement("source");
        
        QString license = source.attribute("license");
        QString upstreamurl = source.attribute("upstreamurl");
        QString depends, conflicts, suggests;
        
        // Explorer les dépendances
        QDomElement depend = source.firstChildElement("depend");
        
        while (!depend.isNull())
        {
            QString type = depend.attribute("type", "depend");
            
            if (type == "depend")
            {
                if (!depends.isEmpty())
                {
                    depends += "; ";
                }
                depends += depend.attribute("string");
            }
            else if (type == "suggest")
            {
                if (!suggests.isEmpty())
                {
                    suggests += "; ";
                }
                suggests += depend.attribute("string");
            }
            else if (type == "conflict")
            {
                if (!conflicts.isEmpty())
                {
                    conflicts += "; ";
                }
                conflicts += depend.attribute("string");
            }
            
            depend = depend.nextSiblingElement("depend");
        }
        
        // Mainteneur
        QDomElement maint = source.firstChildElement("maintainer");
        QString maintainer = maint.attribute("name") + " <" + maint.attribute("email") + ">";
        
        // Dernière entrée de changelog pour avoir la version, la distribution et l'auteur de la mise à jour
        QDomElement changelog = root.firstChildElement("changelog").firstChildElement("entry");
        QString author = changelog.attribute("author") + " <" + changelog.attribute("email") + ">";
        QString version = fname.section('~', 1, -1).section('.', 0, -3);
        QString distro = changelog.attribute("distribution");
        
        // Prendre l'ID de la distribution
        sql = "SELECT id FROM packages_distribution WHERE name='%1';";
        TRY_QUERY(sql.arg(e(distro)))
        int distro_id = query.value(0).toInt();
        
        // Prendre la liste des architectures connues
        sql = "SELECT id FROM packages_arch WHERE name <> 'all';";
        
        if (!query.exec(sql))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::QueryError;
            err->info = query.lastQuery();
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        QList<int> archsIds;
        
        while (query.next())
        {
            archsIds.append(query.value(0).toInt());
        }
        
        // Insérer un enregistrement par architecture (permet de reconstruire les archis
        // indépendamment les unes des autres
        foreach (int arch_id, archsIds)
        {
            sql = " INSERT INTO packages_sourcelog \
                        (source_id, flags, arch_id, date, author, maintainer, upstream_url, version, distribution_id, license, date_rebuild_asked, depends, suggests, conflicts) \
                    VALUES (%1, %2, %3, NOW(), '%4', '%5', '%6', '%7', %8, '%9', NOW(), '%10', '%11', '%12');";
            
            if (!query.exec(sql
                    .arg(source_id)
                    .arg(SOURCEPACKAGE_FLAG_MANUAL | SOURCEPACKAGE_FLAG_REBUILD)
                    .arg(arch_id)
                    .arg(author)
                    .arg(maintainer)
                    .arg(upstreamurl)
                    .arg(version)
                    .arg(distro_id)
                    .arg(license)
                    .arg(depends)
                    .arg(suggests)
                    .arg(conflicts)))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                d->ps->setLastError(err);
                
                return false;
            }
        }
    }
    
    // On a fini
    return true;
}

struct DirectoryId
{
    QString name;
    int id;
};

struct WikiPage
{
    QString title;
    QString shortdesc;
    QString longdesc;
    QString lang;
};

static QString wikiBody(const WikiPage &wikiPage, FilePackage *fpkg)
{
    QString rs;
    
    rs  = "Here is the wiki page of the package **" + fpkg->name() + "**.\n";
    rs += "\n";
    rs += "These lines are ignored, and you can translate them if you want. The table of contents is also ignored. \
           The only important thing here is the first-level titles. Only their order is important, you can put whatever \
           in them.\n";
    rs += "\n";
    rs += "The title and short description must fit in one single line. Every newline in them will be truncated. The \
           long description can be as long as you want.\n";
    rs += "\n";
    rs += "[TOC]\n";
    rs += "\n";
    rs += "# Title\n";
    rs += "\n";
    rs += wikiPage.title + "\n";
    rs += "\n";
    rs += "# Short description\n";
    rs += "\n";
    rs += wikiPage.shortdesc + "\n";
    rs += "\n";
    rs += "# Long description\n";
    rs += "\n";
    rs += wikiPage.longdesc + "\n";
    
    return rs;
}

static QString libPrefix(const QString &name)
{
    if (name.startsWith("lib"))
    {
        return name.left(4);
    }
    else
    {
        return name.left(1);
    }
}

static QString lpkFileName(const QString &name, const QString &version, const QString &arch)
{
    QString rs = "pool/%1/%2/%3~%4.%5.lpk";
    
    return rs.arg(libPrefix(name), name, name, version, arch);
}

static QString metaFileName(const QString &name, const QString &version)
{
    QString rs = "metadata/%1/%2~%3.metadata.xml.xz";
    
    return rs.arg(libPrefix(name), name, version);
}

static QString iconFileName(const QString &name, const QString &version, const QString &extension)
{
    QString rs = "metadata/%1/%2~%3.icon.%4";
    
    return rs.arg(libPrefix(name), name, version, extension);
}

bool RepositoryManager::includePackage(const QString &fileName)
{
    // Ouvrir le paquet
    FilePackage *fpkg;
    Package *pkg;
    PackageMetaData *md;
    
    if (!d->ps->package(fileName, QString(), pkg))
    {
        return false;
    }
    
    fpkg = static_cast<FilePackage *>(pkg);
    
    if (fpkg == 0)
    {
        return false;
    }
    
    md = fpkg->metadata();
    md->setCurrentPackage(fpkg->name());
    
    QDomElement package = md->currentPackageElement();
    
    // Obtenir l'url du paquet
    QString download_url = lpkFileName(fpkg->name(), fpkg->version(), fpkg->arch());
    QString metadata_url = metaFileName(fpkg->name(), fpkg->version());
    QString icon_url;
    
    // Si le paquet a une icône, l'enregistrer
    QByteArray iconData = md->packageIconData();
    
    if (!iconData.isNull())
    {
        // Enregistrer l'icône dans un fichier, en se servant des attributs de la balise <icon />
        QString extension = md->packageIconOrigFileName().section('.', -1, -1);
        
        if (extension.isEmpty()) extension = "png";
        
        icon_url = iconFileName(fpkg->name(), fpkg->version(), extension);
        
        // Écrire l'icône
        QFile fl(icon_url);
        
        if (fl.open(QIODevice::WriteOnly))
        {
            fl.write(iconData);
            fl.close();
        }
    }
    else
    {
        // Voir si un paquet principal de cette source a une icône. Si c'est le cas, utiliser son icône
        QStringList primaryPackages = md->primaryPackages();
            
        for (int i=0; i<primaryPackages.count(); ++i)
        {
            md->setCurrentPackage(primaryPackages.at(i));
            
            if (md->packageIconType() != PackageMetaData::None)
            {
                QString extension = md->packageIconOrigFileName().section('.', -1, -1);
                icon_url = iconFileName(md->currentPackage(), fpkg->version(), extension);
                break;
            }
        }
        
        if (icon_url.isNull())
        {
            icon_url = "NULL";
        }
        
        md->setCurrentPackage(fpkg->name());
    }
    
    // Mettre à jour la base de donnée
    QString sql;
    QSqlQuery query(d->db);
    
    sql = " SELECT \
            section.name, \
            arch.name, \
            \
            pkg.section_id, \
            pkg.arch_id, \
            pkg.distribution_id, \
            \
            pkg.id, \
            pkg.version \
            \
            FROM packages_package pkg \
            LEFT JOIN packages_section section ON pkg.section_id = section.id \
            LEFT JOIN packages_arch arch ON pkg.arch_id = arch.id \
            LEFT JOIN packages_distribution distro ON pkg.distribution_id = distro.id \
            \
            WHERE pkg.name = '%1' \
            AND distro.name = '%2';";
            
    // Sélectionner le (ou zéro) paquet qui correspond.
    if (!query.exec(sql.arg(e(fpkg->name()), 
                            e(fpkg->distribution()))))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    int distro_id = -1;
    int arch_id = -1;
    int section_id = -1;
    int package_id = -1;
    bool update = false;
    QString oldver;
    QString oldarch;
    
    if (query.next())
    {
        // Le paquet est dans la base de donnée, pré-charger les champs
        update = true;
        distro_id = query.value(4).toInt();
        package_id = query.value(5).toInt();
        oldver = query.value(6).toString();
        oldarch = query.value(1).toString();
        
        if (query.value(0).toString() == fpkg->section())
        {
            // La section n'a pas changé
            section_id = query.value(2).toInt();
        }
        else
        {
            // La section a changé
            sql = "SELECT id FROM packages_section WHERE name='%1';";
            
            TRY_QUERY(sql.arg(e(fpkg->section())))
            
            section_id = query.value(0).toInt();
        }
        
        if (query.value(1).toString() == fpkg->arch())
        {
            // La section n'a pas changé
            arch_id = query.value(3).toInt();
        }
        else
        {
            // La section a changé
            sql = "SELECT id FROM packages_arch WHERE name='%1';";
            
            TRY_QUERY(sql.arg(e(fpkg->arch())))
            
            arch_id = query.value(0).toInt();
        }
    }
    else
    {
        // Trouver les ID de la distribution, architecture et section
        sql = "SELECT id FROM packages_section WHERE name='%1';";
        TRY_QUERY(sql.arg(e(fpkg->section())))
        section_id = query.value(0).toInt();
        
        sql = "SELECT id FROM packages_arch WHERE name='%1';";
        TRY_QUERY(sql.arg(e(fpkg->arch())))
        arch_id = query.value(0).toInt();
        
        sql = "SELECT id FROM packages_distribution WHERE name='%1';";
        TRY_QUERY(sql.arg(e(fpkg->distribution())))
        distro_id = query.value(0).toInt();
    }
    
    // Trouver le paquet source
    int source_id = d->sourcePackageId(fpkg->source());
    
    if (source_id == -1)
    {
        return false;
    }
    
    // Insérer ou mettre à jour le paquet
    if (!update)
    {
        sql = " INSERT INTO \
                packages_package(name, maintainer, section_id, version, arch_id, distribution_id, primarylang, download_size, install_size, date, depends, suggests, conflicts, provides, replaces, source, license, flags, packageHash, metadataHash, download_url, upstream_url, sourcepkg_id, icon) \
                VALUES ('%1', '%2', %3, '%4', %5, %6, '%7', %8, %9, NOW(), '%10', '%11', '%12', '%13', '%14', '%15', '%16', %17, '%18', '%19', '%20', '%21', %22, '%23');";
    }
    else
    {
       sql = QString("  UPDATE packages_package SET \
                name='%1', \
                maintainer='%2', \
                section_id=%3, \
                version='%4', \
                arch_id=%5, \
                distribution_id=%6, \
                primarylang='%7', \
                download_size=%8, \
                install_size=%9, \
                date=NOW(), \
                depends='%10', \
                suggests='%11', \
                conflicts='%12', \
                provides='%13', \
                replaces='%14', \
                source='%15', \
                license='%16', \
                flags=%17, \
                packageHash='%18', \
                metadataHash='%19', \
                download_url='%20', \
                upstream_url='%21', \
                sourcepkg_id=%22, \
                icon='%23' \
                \
                WHERE id=") + QString::number(package_id) + ";";
        
        // Une autre version du paquet est déjà dans le dépôt, la retirer (TODO: C'est ici qu'on differa)
        QFile::remove(lpkFileName(fpkg->name(), oldver, oldarch));
        QFile::remove(metaFileName(fpkg->name(), oldver));
    }
    
    // Lancer la requête
    if (!query.exec(sql
            .arg(e(fpkg->name()), e(fpkg->maintainer()))
            .arg(section_id)
            .arg(e(fpkg->version()))
            .arg(arch_id)
            .arg(distro_id)
            .arg(e(md->primaryLang()))
            .arg(fpkg->downloadSize())
            .arg(fpkg->installSize())
            .arg(e(DEPEND(Depend::DependType)))
            .arg(e(DEPEND(Depend::Suggest)))
            .arg(e(DEPEND(Depend::Conflict)))
            .arg(e(DEPEND(Depend::Provide)))
            .arg(e(DEPEND(Depend::Replace)))
            .arg(e(fpkg->source()))
            .arg(e(fpkg->license()))
            .arg(fpkg->flags())
            .arg(e(QString(fpkg->packageHash().toHex())))
            .arg(e(QString(fpkg->metadataHash().toHex())))
            .arg(e(download_url))
            .arg(e(fpkg->upstreamUrl()))
            .arg(source_id)
            .arg(e(icon_url))
            ))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (!update)
    {
        package_id = query.lastInsertId().toInt();
    }
    
    // Liste des fichiers
    QStringList fileParts;
    
    sql = "DELETE FROM packages_file WHERE package_id=%1;";
          
    if (!query.exec(sql.arg(package_id)))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    QVector<PackageFile *> pkgfiles = fpkg->files();
    QVector<DirectoryId> dirIds;
    
    for (int j=0; j<pkgfiles.count(); ++j)
    {
        PackageFile *file = pkgfiles.at(j);
        QString fileName = file->path();
        int flags = file->flags();
        
        // fileName est par exemple usr/lib/terminfo/x/xterm+pcc2
        // Découper ce nom de fichier suivant '/'
        fileParts = fileName.split('/');
        
        // Explorer ces morceaux
        QString dirPath;
        int dirId = 0;
        for (int i=0; i<fileParts.count(); ++i)
        {
            const QString &part = fileParts.at(i);
            
            if (i < fileParts.count() - 1)
            {
                // Dossier, vérifier s'il existe
                // Voir si on n'a rien en cache
                if (i < dirIds.count() && dirIds.at(i).name == part)
                {
                    dirId = dirIds.at(i).id;
                }
                else
                {
                    // Pas en cache
                    sql = " SELECT id \
                            FROM packages_directory \
                            WHERE name='%1' COLLATE utf8_bin AND directory_id=%2;";
                    
                    if (!query.exec(sql
                            .arg(e(part))
                            .arg(dirId)))
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::QueryError;
                        err->info = query.lastQuery();
                        
                        d->ps->setLastError(err);
                        
                        return false;
                    }
                    
                    if (query.next())
                    {
                        // Le dossier existe déjà, ok
                        dirId = query.value(0).toInt();
                    }
                    else
                    {
                        // Le dossier n'existe pas déjà, le créer
                        sql = " INSERT INTO packages_directory \
                                (directory_id, name, path) \
                                VALUES (%1, '%2', '%3');";
                        
                        if (!query.exec(sql
                                .arg(dirId)
                                .arg(e(part))
                                .arg(e(dirPath))))
                        {
                            PackageError *err = new PackageError;
                            err->type = PackageError::QueryError;
                            err->info = query.lastQuery();
                            
                            d->ps->setLastError(err);
                            
                            return false;
                        }
                        
                        // Récupérer l'ID du dossier
                        dirId = query.lastInsertId().toInt();
                    }
                    
                    // L'insérer dans le cache
                    dirIds.resize(i);
                    
                    DirectoryId did;
                    did.name = part;
                    did.id = dirId;
                    dirIds.append(did);
                }
                
                // Ajouter cet élément au path du dossier
                if (!dirPath.isEmpty())
                {
                    dirPath += '/';
                }
                
                dirPath += part;
            }
            else
            {
                // Fichier, on sait que dirId pointe sur son dossier parent.
                // Il faut juste créer ce fichier.
                // En effet, on les a tous supprimés avant (pour éviter les 
                // problèmes de mise à jour.
                sql = " INSERT INTO packages_file \
                        (package_id, directory_id, name, flags) \
                        VALUES (%1, %2, '%3', %4);";
                
                if (!query.exec(sql
                        .arg(package_id)
                        .arg(dirId)
                        .arg(e(part))
                        .arg(flags)))
                {
                    PackageError *err = new PackageError;
                    err->type = PackageError::QueryError;
                    err->info = query.lastQuery();
                    
                    d->ps->setLastError(err);
                    
                    return false;
                }
            }
        }
    }
    
    // Chaînes
    QDomElement el = package.firstChildElement();
    QList<WikiPage> wikiPages;
    
    while (!el.isNull())
    {
        int type = -1;
        
        if (el.tagName() == "title")
        {
            type = 0;
        }
        else if (el.tagName() == "shortdesc")
        {
            type = 1;
        }
        else if (el.tagName() == "description")
        {
            type = 2;
        }
        
        if (type != -1)
        {
            // Explorer les langues
            QDomElement lang = el.firstChildElement();
            QString l, text;
            
            while (!lang.isNull())
            {
                l = lang.tagName();
                text = lang.text().trimmed().replace(d->regex, "\n");
                
                if (!d->registerString(query, package_id, l, text, type))
                {
                    return false;
                }
                
                // Enregistrer la langue dans la page de wiki
                /*
                    struct WikiPage
                    {
                        QString title;
                        QString shortdesc;
                        QString longdesc;
                        QString lang;
                    };
                */
                bool found = false;
                
                for (int i=0; i<wikiPages.count(); ++i)
                {
                    WikiPage &wikiPage = wikiPages[i];
                    
                    if (wikiPage.lang == l)
                    {
                        // Bonne langue, mettre à jour l'info
                        switch (type)
                        {
                            case 0:
                                wikiPage.title = text;
                                break;
                            case 1:
                                wikiPage.shortdesc = text;
                                break;
                            case 2:
                                wikiPage.longdesc = text;
                                break;
                        }
                        
                        found = true;
                        break;
                    }
                }
                
                if (!found)
                {
                    // On n'a pas encore cette page
                    WikiPage wikiPage;
                    wikiPage.lang = l;
                    
                    switch (type)
                    {
                        case 0:
                            wikiPage.title = text;
                            break;
                        case 1:
                            wikiPage.shortdesc = text;
                            break;
                        case 2:
                            wikiPage.longdesc = text;
                            break;
                    }
                    
                    wikiPages.append(wikiPage);
                }
                
                lang = lang.nextSiblingElement();
            }
        }
        
        el = el.nextSiblingElement();
    }
    
    // Explorer les wikiPages et les enregistrer dans la base de donnée.
    if (d->websiteIntegration)
    {
        QList<int> ids;
        int identifier = -1;
        
        for (int i=0; i<wikiPages.count(); ++i)
        {
            const WikiPage &wikiPage = wikiPages.at(i);
            
            // Voir si une page de wiki dont le slug est {{pkgname|slugify}}-{{distro|slugify}} existe
            // dans la bonne langue
            QString slug = d->slugify(fpkg->name() + "-" + fpkg->distribution());
            
            sql = " SELECT \
                    id, \
                    identifier, \
                    body \
                    FROM wiki_page \
                    WHERE slug='%1' AND lang='%2';";
            
            if (!query.exec(sql
                    .arg(e(slug))
                    .arg(e(wikiPage.lang))))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                d->ps->setLastError(err);
                
                return false;
            }
            
            if (query.next())
            {
                int id = query.value(0).toInt();
                identifier = query.value(1).toInt();
                QString oldBody = query.value(2).toString();
                QString body = wikiBody(wikiPage, fpkg);
                
                if (body != oldBody)
                {
                    // La page existe déjà, la mettre à jour
                    sql = " UPDATE wiki_page \
                            SET body='%1' \
                            WHERE id=%2;";
                    
                    if (!query.exec(sql
                            .arg(e(body))
                            .arg(id)))
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::QueryError;
                        err->info = query.lastQuery();
                        
                        d->ps->setLastError(err);
                        
                        return false;
                    }
                    
                    // Ajouter un enregistrement anonyme dans l'historique
                    sql = " INSERT INTO wiki_logentry \
                                (page_id, comment, body, date, author_ip) \
                            VALUES (%1, 'New package importation', '%2', NOW(), 'LPM import');";
                    
                    if (!query.exec(sql
                            .arg(id)
                            .arg(e(oldBody))))
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::QueryError;
                        err->info = query.lastQuery();
                        
                        d->ps->setLastError(err);
                        
                        return false;
                    }
                }
            }
            else
            {
                // La page n'existe pas encore, la créer
                sql = " INSERT INTO wiki_page \
                        (title, slug, lang, identifier, body, is_protected, is_private) \
                        VALUES ('%1', '%2', '%3', %4, '%5', %6, %7);";
                
                if (!query.exec(sql
                        .arg(e(wikiPage.title))
                        .arg(e(slug))
                        .arg(e(wikiPage.lang))
                        .arg(0)
                        .arg(e(wikiBody(wikiPage, fpkg)))
                        .arg(0)
                        .arg(0)))
                {
                    PackageError *err = new PackageError;
                    err->type = PackageError::QueryError;
                    err->info = query.lastQuery();
                    
                    d->ps->setLastError(err);
                    
                    return false;
                }
                
                int id = query.lastInsertId().toInt();
                ids.append(id);
                
                // Si on n'a pas encore d'identifier, prendre cette id
                if (identifier == -1)
                {
                    identifier = id;
                }
            }
        }
        
        // Mettre à jour toutes les pages listées par ids pour qu'elles aient le même identifiant de groupe
        if (ids.count() != 0)
        {
            QString sids;
            
            for (int i=0; i<ids.count(); ++i)
            {
                if (i != 0)
                {
                    sids += ", ";
                }
                
                sids += QString::number(ids.at(i));
            }
            
            sql = " UPDATE wiki_page \
                    SET identifier=%1 \
                    WHERE id IN (%2);";
            
            if (!query.exec(sql
                    .arg(identifier)
                    .arg(sids)))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                
                d->ps->setLastError(err);
                
                return false;
            }
        }
    }
    
    // Enregistrer le changelog, sans passer par md->changelog() car on a besoin des langues
    QDomElement changelog = md->documentElement().firstChildElement("changelog").firstChildElement("entry");
    QDateTime maxdt, mydt;
    int m_distro_id, changelogType = ChangeLogEntry::LowPriority, changelogID;
    
    // Trouver l'entrée de changelog la plus récente
    sql = " SELECT \
            MAX(date) \
            FROM packages_changelog \
            WHERE package_id=%1;";
            
    TRY_QUERY(sql.arg(package_id))
    
    maxdt = query.value(0).toDateTime();
    
    while (!changelog.isNull())
    {
        // On ne peut qu'ajouter des entrées. Il suffit donc de vérifier si l'entrée
        // est plus récente que maxdt. Si c'est le cas, l'ajouter. Sinon, la passer
        mydt = QDateTime(QDate::fromString(changelog.attribute("date"), "yyyy-MM-dd"),
                         QTime::fromString(changelog.attribute("time"), "hh:mm:ss"));
                         
        if (mydt <= maxdt)
        {
            // Déjà inséré
            changelog = changelog.nextSiblingElement("entry");
            continue;
        }
        
        // Trouver l'ID de la distribution
        sql = "SELECT id FROM packages_distribution WHERE name='%1';";
        TRY_QUERY(sql.arg(e(changelog.attribute("distribution"))))
        m_distro_id = query.value(0).toInt();
        
        // Trouver le type d'entrée
        QString t = changelog.attribute("type", "lowprio");
        
        if (t == "lowprio")
        {
            changelogType = ChangeLogEntry::LowPriority;
        }
        else if (t == "feature")
        {
            changelogType = ChangeLogEntry::Feature;
        }
        else if (t == "bugfix")
        {
            changelogType = ChangeLogEntry::BugFix;
        }
        else if (t == "security")
        {
            changelogType = ChangeLogEntry::Security;
        }
        
        // Créer l'enregistrement
        sql = " INSERT INTO \
                packages_changelog(package_id, version, distribution_id, email, author, type, date) \
                VALUES (%1, '%2', %3, '%4', '%5', %6, '%7');";
                
        if (!query.exec(sql
                .arg(package_id)
                .arg(e(changelog.attribute("version")))
                .arg(m_distro_id)
                .arg(e(changelog.attribute("email")))
                .arg(e(changelog.attribute("author")))
                .arg(changelogType)
                .arg(mydt.toString(Qt::ISODate))
            ))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::QueryError;
            err->info = query.lastQuery();
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        // Gérer les chaînes de caractères
        changelogID = query.lastInsertId().toInt();
        
        el = changelog.firstChildElement();
        
        while (!el.isNull())
        {
            if (!d->registerString(query, package_id, el.tagName(), el.text(), 3, changelogID))
            {
                return false;
            }
            
            el = el.nextSiblingElement();
        }
        
        changelog = changelog.nextSiblingElement();
    }
    
    // Copier le paquet, enregistrer les métadonnées
    QDir curDir = QDir::current();
    
    // Fichier du paquet
    if (QFile::exists(download_url))
    {
        QFile::remove(download_url);
    }
    
    if (!curDir.mkpath(download_url.section('/', 0, -2)))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = download_url.section('/', 0, -2);
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (!QFile::copy(fileName, download_url))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = download_url;
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    // Métadonnées
    if (QFile::exists(metadata_url))
    {
        QFile::remove(metadata_url);
    }
    
    if (!curDir.mkpath(metadata_url.section('/', 0, -2)))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = download_url.section('/', 0, -2);
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (!d->writeXZ(metadata_url, fpkg->metadataContents()))
    {
        return false;
    }
    
    return true;
}

struct LangContent
{
    QByteArray shortdesc;
    QString title;
};

bool RepositoryManager::exp(const QStringList &distros)
{
    QSqlQuery query(d->db);
    QString sql;
    QStringList distributions = distros;
    QStringList archs, langs;
    
    // Si pas de distribs, alors les prendre toutes
    if (distributions.empty())
    {
        distributions = d->set->value("Distributions").toString().split(' ', QString::SkipEmptyParts);
    }
    
    // Sélectionner les architectures
    sql = "SELECT name FROM packages_arch;";
    
    if (!query.exec(sql))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
            
        d->ps->setLastError(err);
            
        return false;
    }
    
    while (query.next())
    {
        archs.append(query.value(0).toString());
    }
    
    // Langues
    langs = d->set->value("Languages", "en").toString().split(' ', QString::SkipEmptyParts);
    
    // QByteArrays nécessaires à l'écriture
    QList<QByteArray> streams;
    
    for (int i=0; i<=langs.count()+3; ++i)
    {
        // deux tours en plus, parce qu'on a aussi la liste des paquets et des sections à écrire
        streams.append(QByteArray());
    }
    
    QByteArray &pkgstream = streams[langs.count()];
    QByteArray &filestream = streams[langs.count()+1];
    QByteArray &sectionstream = streams[langs.count()+2];
    QByteArray &metastream = streams[langs.count()+3];

#ifdef GPGME_FOUND
    // Initialiser GPGME
    bool useGpg = d->set->value("Sign/Enabled", true).toBool();
    gpgme_ctx_t ctx;
    
    if (useGpg)
    {
        QByteArray skey = d->set->value("Sign/Key").toByteArray();
        const char *key_id = skey.constData();
        
        gpgme_key_t gpgme_key;
        
        gpgme_new(&ctx);
        gpgme_set_armor(ctx, 0);
        
        gpgme_keylist_mode_t mode = gpgme_get_keylist_mode(ctx);
        mode |= GPGME_KEYLIST_MODE_LOCAL;
        gpgme_set_keylist_mode(ctx, mode);
    
        if (gpgme_get_key(ctx, key_id, &gpgme_key, 1) != GPG_ERR_NO_ERROR)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::SignError;
            err->info = tr("Impossible de trouver la clef %1").arg(key_id);
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        if (gpgme_signers_add(ctx, gpgme_key) != GPG_ERR_NO_ERROR)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::SignError;
            err->info = tr("Impossible d'ajouter la clef %1 pour signature").arg(key_id);
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        gpgme_key_unref(gpgme_key);
    }
#endif
    
    // Explorer les distributions
    int arch_id, distro_id;
    int eNum = 0;
    int eTot = distributions.count() * archs.count();
    
    int progress = d->ps->startProgress(Progress::Exporting, eTot);
    
    foreach (const QString &distro, distributions)
    {
        sql = "SELECT id FROM packages_distribution WHERE name='%1';";
        TRY_QUERY(sql.arg(e(distro)))
        distro_id = query.value(0).toInt();
        
        // Explorer les architectures
        foreach (const QString &arch, archs)
        {
            // Progression
            if (!d->ps->sendProgress(progress, eNum, distro, arch))
            {
                return false;
            }
            
            eNum++;
            
            sql = "SELECT id FROM packages_arch WHERE name='%1';";
            TRY_QUERY(sql.arg(e(arch)))
            arch_id = query.value(0).toInt();
            
            // Obtenir les traductions
            QHash<int, LangContent> trads;
            
            sql = " SELECT \
                    package_id, \
                    language, \
                    content, \
                    type \
                    \
                    FROM packages_string \
                    \
                    WHERE type = 1 OR type = 0;";
                    
            if (!query.exec(sql))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                    
                d->ps->setLastError(err);
                    
                return false;
            }
            
            while (query.next())
            {
                int lindex = langs.indexOf(query.value(1).toString());
                int ltype = query.value(3).toInt();
                
                if (lindex != -1)
                {
                    if (ltype == 0)
                    {
                        trads[LANGPKGKEY(query.value(0).toInt(), lindex)].title = query.value(2).toString();
                    }
                    else
                    {
                        trads[LANGPKGKEY(query.value(0).toInt(), lindex)].shortdesc = query.value(2).toByteArray();
                    }
                }
            }
            
            // Sauvegarder les sections (nom, description et icône)
            sectionstream = "<?xml version='1.0'?>\n";
            sectionstream += "<sections>\n";
            
            sql = " SELECT \
                    section.name, \
                    section.icon, \
                    section.primarylang, \
                    section.weight, \
                    string.long_name, \
                    string.`desc`, \
                    string.lang \
                    FROM packages_sectionstring string \
                    LEFT JOIN packages_section section ON section.id = string.section_id \
                    ORDER BY section.name DESC;";
            
            if (!query.exec(sql))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                    
                d->ps->setLastError(err);
                    
                return false;
            }
            
            QDomDocument sectionDoc;
            
            // Element root
            QDomElement rootElement = sectionDoc.createElement("sections");
            sectionDoc.appendChild(rootElement);
            
            QDomElement lastSection, sectionDesc, sectionTitle;
            
            while (query.next())
            {
                QString sectname, sectlongname, sectdesc, secticon, sectlang;
                sectname = query.value(0).toString();
                secticon = query.value(1).toString();
                sectlongname = query.value(4).toString();
                sectdesc = query.value(5).toString();
                sectlang = query.value(6).toString();
                
                if (lastSection.attribute("name") != sectname)
                {
                    lastSection = sectionDoc.createElement("section");
                    rootElement.appendChild(lastSection);
                    
                    lastSection.setAttribute("name", sectname);
                    lastSection.setAttribute("primarylang", query.value(2).toString());
                    lastSection.setAttribute("weight", query.value(3).toInt());
                    
                    // Icône de la section
                    QFile fl(d->set->value("SectionsIconBaseDir").toString() + secticon);
                    
                    if (fl.open(QIODevice::ReadOnly))
                    {
                        QDomElement sectionIcon = sectionDoc.createElement("icon");
                        lastSection.appendChild(sectionIcon);
                        
                        QDomCDATASection cdata = sectionDoc.createCDATASection(fl.readAll().toBase64());
                        sectionIcon.appendChild(cdata);
                    }
                    
                    // Description et titre
                    sectionDesc = sectionDoc.createElement("description");
                    sectionTitle = sectionDoc.createElement("title");
                    
                    lastSection.appendChild(sectionDesc);
                    lastSection.appendChild(sectionTitle);
                }
                
                QDomNode titleLang = sectionDoc.createElement(sectlang);
                QDomText titleText = sectionDoc.createTextNode(sectlongname);
                
                titleLang.appendChild(titleText);
                sectionTitle.appendChild(titleLang);
                
                QDomNode descLang = sectionDoc.createElement(sectlang);
                QDomText descText = sectionDoc.createTextNode(sectdesc);
                
                descLang.appendChild(descText);
                sectionDesc.appendChild(descLang);
            }
            
            sectionstream = sectionDoc.toByteArray(0);
            
            // Explorer les fichiers, et les placer dans un fichier du format
            // :usr
            // :bin
            // paquet|flags|paquet_bin
            // ::
            // paquet|flags|fichier_dans_usr_pas_dans_bin
            // ::
            sql = " SELECT \
                    IFNULL(CONCAT(dir.path, '/', dir.name, '/', file.name), file.name), \
                    pkg.name, \
                    file.flags \
                    FROM packages_file file \
                    LEFT JOIN packages_directory dir ON dir.id=file.directory_id \
                    LEFT JOIN packages_package pkg ON pkg.id=file.package_id \
                    WHERE pkg.arch_id=%1 AND pkg.distribution_id=%2 \
                    ORDER BY dir.path ASC, dir.name ASC;";
            
            if (!query.exec(sql
                            .arg(arch_id)
                            .arg(distro_id)))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                    
                d->ps->setLastError(err);
                    
                return false;
            }
            
            QByteArray path, pkgname;
            QList<QByteArray> parts, curParts;
            int level, flags;
            
            while(query.next())
            {
                path = query.value(0).toByteArray();
                pkgname = query.value(1).toByteArray();
                flags = query.value(2).toInt();
                
                if (path[0] == '/')
                {
                    path.remove(0, 1);
                }
                
                parts = path.split('/');
                
                // Trouver le nombre de parts communes entre curParts et parts.
                level = 0;
                for (int i=0; i<qMin(curParts.count()-1, parts.count()-1); ++i)
                {
                    if (parts.at(i) == curParts.at(i))
                    {
                        level++;
                    }
                    else
                    {
                        break;
                    }
                }
                
                // Remonter du bon nombre de dossiers
                for (int i=0; i<(curParts.count()-level-1); ++i)
                {
                    filestream += "::\n";
                }
                
                // Sortir toutes les autres parties
                int level2 = level;
                for (int i=level; i<parts.count(); ++i)
                {
                    if (i == parts.count()-1)
                    {
                        // Fichier
                        filestream += pkgname + '|';
                        
                        if (flags != 0)
                        {
                            filestream += QByteArray::number(flags);
                        }
                        
                        filestream += "||" + parts.at(i) + '\n';
                    }
                    else
                    {
                        filestream += ':' + parts.at(i) + '\n';
                    }
                    level2++;
                }
                
                curParts = parts;
            }
            
            // Sélectionner tous les paquets qu'il y a dedans
            sql = " SELECT \
                    pkg.name, \
                    pkg.version, \
                    pkg.source, \
                    pkg.maintainer, \
                    pkg.license, \
                    pkg.primarylang, \
                    pkg.flags, \
                    pkg.depends, \
                    pkg.provides, \
                    pkg.replaces, \
                    pkg.suggests, \
                    pkg.conflicts, \
                    pkg.download_size, \
                    pkg.install_size, \
                    pkg.packageHash, \
                    pkg.metadataHash, \
                    pkg.upstream_url, \
                    pkg.icon, \
                    pkg.votes, \
                    pkg.total_votes, \
                    \
                    section.name, \
                    pkg.id \
                    \
                    FROM packages_package pkg \
                    LEFT JOIN packages_section section ON section.id = pkg.section_id \
                    \
                    WHERE pkg.distribution_id='%1' \
                    AND pkg.arch_id='%2';";
                    
            if (!query.exec(sql
                            .arg(distro_id)
                            .arg(arch_id)))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                    
                d->ps->setLastError(err);
                    
                return false;
            }
            
            // Explorer les paquets
            bool first = true;
            QByteArray rs, pkname, pkprimlang, trad;
            int i, key, pkid, primlangid;
            
            QDomDocument metaDoc;
            QDomElement packageElement, packageTitleElement;
            rootElement = metaDoc.createElement("packages");
            metaDoc.appendChild(rootElement);
            
            while (query.next())
            {
                pkname = query.value(0).toByteArray();
                pkprimlang = query.value(5).toByteArray();
                pkid = query.value(21).toInt();
                primlangid = -1;
                
                rs = '[' + pkname + "]\n";
                rs += "Name=" + pkname + '\n';
                rs += "Version=" + query.value(1).toByteArray() + '\n';
                rs += "Source=" + query.value(2).toByteArray() + '\n';
                rs += "UpstreamUrl=" + query.value(16).toByteArray() + '\n';
                rs += "Maintainer=" + query.value(3).toByteArray() + '\n';
                rs += "Section=" + query.value(20).toByteArray() + '\n';
                rs += "Distribution=" + distro + '\n';
                rs += "License=" + query.value(4).toByteArray() + '\n';
                rs += "Flags=" + query.value(6).toByteArray() + '\n';
                rs += "Depends=" + query.value(7).toByteArray() + '\n';
                rs += "Provides=" + query.value(8).toByteArray() + '\n';
                rs += "Replaces=" + query.value(9).toByteArray() + '\n';
                rs += "Suggest=" + query.value(10).toByteArray() + '\n';
                rs += "Conflicts=" + query.value(11).toByteArray() + '\n';
                rs += "DownloadSize=" + query.value(12).toByteArray() + '\n';
                rs += "InstallSize=" + query.value(13).toByteArray() + '\n';
                rs += "Arch=" + arch + '\n';
                rs += "PackageHash=" + query.value(14).toByteArray() + '\n';
                rs += "MetadataHash=" + query.value(15).toByteArray() + '\n';
                
                if (!first)
                {
                    pkgstream += '\n';
                }
                else
                {
                    first = false;
                }
                
                pkgstream += rs;
                
                // Métadonnées du paquet extraites dans une liste
                if (query.value(6).toInt() & Package::Primary)
                {
                    packageElement = metaDoc.createElement("package");
                    rootElement.appendChild(packageElement);
                    
                    packageElement.setAttribute("name", QString(pkname));
                    packageElement.setAttribute("primarylang", QString(pkprimlang));
                    packageElement.setAttribute("votes", QString::number(query.value(18).toInt()));
                    packageElement.setAttribute("totalvotes", QString::number(query.value(19).toInt()));
                    
                    packageTitleElement = metaDoc.createElement("title");
                    packageElement.appendChild(packageTitleElement);
                    
                    // Icône
                    QString fname = query.value(17).toString();
                    
                    if (!fname.isEmpty())
                    {
                        QFile fl(fname);
                        
                        if (fl.open(QIODevice::ReadOnly))
                        {
                            QDomElement packageIcon = metaDoc.createElement("icon");
                            packageElement.appendChild(packageIcon);
                            
                            QDomCDATASection cdata = metaDoc.createCDATASection(fl.readAll().toBase64());
                            packageIcon.appendChild(cdata);
                        }
                    }
                }
                
                // Enregistrer les traductions
                for (i=0; i<langs.count(); ++i)
                {
                    key = LANGPKGKEY(pkid, i);
                    
                    if (trads.contains(key))
                    {
                        const LangContent &l = trads.value(key);
                        trad = l.shortdesc;
                        
                        if (query.value(6).toInt() & Package::Primary)
                        {
                            QDomElement langAttr = metaDoc.createElement(langs.at(i));
                            packageTitleElement.appendChild(langAttr);
                            
                            QDomText txt = metaDoc.createTextNode(l.title);
                            langAttr.appendChild(txt);
                        }
                    }
                    else
                    {
                        // Astuce pour avoir du O(pk) au lieu de O(pk*lang)
                        if (primlangid == -1)
                        {
                            primlangid = langs.indexOf(pkprimlang);
                        }
                        
                        trad = trads.value(LANGPKGKEY(pkid, primlangid)).shortdesc;
                    }
                    
                    streams[i] += pkname + ':' + trad + '\n';
                }
            }
            
            metastream = metaDoc.toByteArray(0);
            
            // Écrire les flux
            QString fileName, filePath;
            
            filePath = "dists/" + distro + '/' + arch;
            
            for (int i=0; i<streams.count(); ++i)
            {
                const QByteArray &stream = streams.at(i);
                
                if (i < langs.count())
                {
                    // Traductions
                    fileName = filePath + "/translate." + langs.at(i) + ".xz";
                }
                else if (i == langs.count())
                {
                    // Liste des paquets
                    fileName = filePath + "/packages.xz";
                }
                else if (i == langs.count() + 1)
                {
                    fileName = filePath + "/files.xz";
                }
                else if (i == langs.count() + 2)
                {
                    fileName = filePath + "/sections.xz";
                }
                else
                {
                    fileName = filePath + "/metadata.xz";
                }
                
                // Créer le dossier s'il le fait
                if (!QFile::exists(filePath))
                {
                    QDir::current().mkpath(filePath);
                }
                
                // Supprimer l'ancien fichier si nécessaire
                if (QFile::exists(fileName))
                {
                    QFile::remove(fileName);
                }
                
                // Écrire et compresser
                d->writeXZ(fileName, stream);
                
                // Écrire la signature du flux non-compressé
#ifdef GPGME_FOUND
                if (useGpg)
                {
                    gpgme_data_t in, out;
                    gpgme_sign_result_t result;
                    char *userret;
                    size_t retsize;
                    
                    if (gpgme_data_new_from_mem(&in, stream.constData(), stream.size(), 0) != GPG_ERR_NO_ERROR)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::SignError;
                        err->info = tr("Impossible de créer le tampon mémoire pour la signature.");
                        
                        d->ps->setLastError(err);
                        
                        return false;
                    }
                    
                    if (gpgme_data_new(&out) != GPG_ERR_NO_ERROR)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::SignError;
                        err->info = tr("Impossible de créer le tampon mémoire de sortie.");
                        
                        d->ps->setLastError(err);
                        
                        gpgme_data_release(in);
                        return false;
                    }
                    
                    if (gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_DETACH) != GPG_ERR_NO_ERROR)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::SignError;
                        err->info = tr("Impossible de signer le fichier %1").arg(fileName);
                        
                        d->ps->setLastError(err);
                        
                        gpgme_data_release(in);
                        gpgme_data_release(out);
                        return false;
                    }
                    
                    result = gpgme_op_sign_result(ctx);
                    
                    if (result->invalid_signers)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::SignError;
                        err->info = tr("Mauvais signataires");
                        
                        d->ps->setLastError(err);
                        
                        gpgme_data_release(in);
                        gpgme_data_release(out);
                        return false;
                    }
                    
                    if (!result->signatures)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::SignError;
                        err->info = tr("Pas de signatures dans le résultat");
                        
                        d->ps->setLastError(err);
                        
                        gpgme_data_release(in);
                        gpgme_data_release(out);
                        return false;
                    }
                    
                    userret = gpgme_data_release_and_get_mem(out, &retsize);
                    
                    // Écrire le fichier
                    QFile signFile(fileName + ".sig");
                    
                    if (!signFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::OpenFileError;
                        err->info = signFile.fileName();
                        
                        d->ps->setLastError(err);
                        
                        gpgme_data_release(in);
                        return false;
                    }
                    
                    signFile.write(userret, retsize);
                    
                    gpgme_data_release(in);
                    free(userret);
                }
#endif
                
                // Supprimer le flux
                streams[i].clear();
            }
        }
    }
    
    d->ps->endProgress(progress);
    
#ifdef GPGME_FOUND
    if (useGpg)
    {
        gpgme_release(ctx);
    }
#endif
    
    return true;
}

#include "repositorymanager.moc"
