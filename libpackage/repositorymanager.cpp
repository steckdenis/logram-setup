/*
 * repositorymanager.cpp
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

#include <QtSql>
#include <QtXml>
#include <QtDebug>

#include <gpgme.h>
#include <unistd.h>

using namespace Logram;

struct RepositoryManager::Private
{
    PackageSystem *ps;
    QSettings *set;
    
    // BDD
    QSqlDatabase db;
    QRegExp regex;
    
    // Fonctions
    bool registerString(QSqlQuery &query, int package_id, const QString &lang, const QString &cont, int type, int changelog_id = 0);
    bool writeXZ(const QString &fileName, const QByteArray &data);
};

RepositoryManager::RepositoryManager(PackageSystem *ps) : QObject(ps)
{
    d = new Private;
    d->ps = ps;
    
    d->regex = QRegExp("\\n[ \\t]+");
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
    
    db = QSqlDatabase::addDatabase("QMYSQL");
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

static gpgme_error_t passphrase_cb(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
    // TODO: Pas très sécurisé tout ça !
    RepositoryManager::Private *d = (RepositoryManager::Private *)hook;
    
    QString pass = d->set->value("Sign/Passphrase").toString();
    
    write(fd, pass.toUtf8().constData(), pass.length());
    write(fd, "\n", 1);
    
    return 0;
    
    (void) uid_hint;
    (void) passphrase_info;
    (void) prev_was_bad;
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

bool RepositoryManager::includeSource(const QString &fileName)
{
    // Simply copy the file in the pool directory
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
    
    return true;
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
    
    // Obtenir l'url du paquet
    QString download_url = "pool/%1/%2/%3";
    QString metadata_url = "metadata/%1/%2~%3.metadata.xml.xz";
    QString dd;
    QString fname = fileName.section('/', -1, -1);
    
    if (fpkg->name().startsWith("lib"))
    {
        dd = fpkg->name().left(4);
    }
    else
    {
        dd = fpkg->name().left(1);
    }
    
    download_url = download_url.arg(dd, fpkg->name(), fname);
    metadata_url = metadata_url.arg(dd, fpkg->name(), fpkg->version());
    
    // Mettre à jour la base de donnée
    QString sql;
    QSqlQuery query;
    
    sql = " SELECT \
            section.name, \
            \
            pkg.section_id, \
            pkg.arch_id, \
            pkg.distribution_id, \
            \
            pkg.id \
            \
            FROM packages_package pkg \
            LEFT JOIN packages_section section ON pkg.section_id = section.id \
            LEFT JOIN packages_arch arch ON pkg.arch_id = arch.id \
            LEFT JOIN packages_distribution distro ON pkg.distribution_id = distro.id \
            \
            WHERE pkg.name = '%1' \
            AND distro.name = '%2' \
            AND arch.name = '%3';";
            
    // Sélectionner le (ou zéro) paquet qui correspond.
    if (!query.exec(sql.arg(e(fpkg->name()), 
                            e(fpkg->distribution()), 
                            e(fpkg->arch()))))
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
    
    if (query.next())
    {
        // Le paquet est dans la base de donnée, pré-charger les champs
        update = true;
        arch_id = query.value(2).toInt();
        distro_id = query.value(3).toInt();
        package_id = query.value(4).toInt();
        
        if (query.value(0).toString() == fpkg->section())
        {
            // La section n'a pas changé
            section_id = query.value(1).toInt();
        }
        else
        {
            // La section a changé
            sql = "SELECT id FROM packages_section WHERE name='%1';";
            
            TRY_QUERY(sql.arg(e(fpkg->section())))
            
            section_id = query.value(0).toInt();
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
    
    // Insérer ou mettre à jour le paquet
    if (!update)
    {
        sql = " INSERT INTO \
                packages_package(name, maintainer, section_id, version, arch_id, distribution_id, primarylang, download_size, install_size, date, depends, suggests, conflicts, provides, replaces, source, license, flags, packageHash, metadataHash, download_url, upstream_url) \
                VALUES ('%1', '%2', %3, '%4', %5, %6, '%7', %8, %9, NOW(), '%10', '%11', '%12', '%13', '%14', '%15', '%16', %17, '%18', '%19', '%20', '%21');";
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
                upstream_url='%21' \
                \
                WHERE id=") + QString::number(package_id) + ";";
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
            .arg(e(DEPEND(DEPEND_TYPE_DEPEND)))
            .arg(e(DEPEND(DEPEND_TYPE_SUGGEST)))
            .arg(e(DEPEND(DEPEND_TYPE_CONFLICT)))
            .arg(e(DEPEND(DEPEND_TYPE_PROVIDE)))
            .arg(e(DEPEND(DEPEND_TYPE_REPLACE)))
            .arg(e(fpkg->source()))
            .arg(e(fpkg->license()))
            .arg(fpkg->flags())
            .arg(e(QString(fpkg->packageHash())))
            .arg(e(QString(fpkg->metadataHash())))
            .arg(e(download_url))
            .arg(e(fpkg->upstreamUrl()))
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
    
    // Chaînes
    QDomElement package = md->currentPackageElement();
    QDomElement el = package.firstChildElement();
    
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
            
            while (!lang.isNull())
            {
                if (!d->registerString(query, package_id, lang.tagName(), lang.text(), type))
                {
                    return false;
                }
                
                lang = lang.nextSiblingElement();
            }
        }
        
        el = el.nextSiblingElement();
    }
    
    // Enregistrer le changelog, sans passer par md->changelog() car on a besoin des langues
    QDomElement changelog = md->documentElement().firstChildElement("changelog").firstChildElement("entry");
    QDateTime maxdt, mydt;
    int m_distro_id, changelogType, changelogID;
    
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

bool RepositoryManager::exp(const QStringList &distros)
{
    QSqlQuery query;
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
    
    for (int i=0; i<=langs.count(); ++i)
    {
        // un tour en plus, parce qu'on a aussi la liste des paquets à écrire
        streams.append(QByteArray());
    }
    
    QByteArray &pkgstream = streams[langs.count()];
    
    // Initialiser GPGME
    QByteArray skey = d->set->value("Sign/Key").toByteArray();
    const char *key_id = skey.constData();
    
    gpgme_ctx_t ctx;
    gpgme_key_t gpgme_key;
    
    gpgme_new(&ctx);
    gpgme_set_armor(ctx, 0);
    
    gpgme_keylist_mode_t mode = gpgme_get_keylist_mode(ctx);
    mode |= GPGME_KEYLIST_MODE_LOCAL;
    gpgme_set_keylist_mode(ctx, mode);
    
    gpgme_set_passphrase_cb(ctx, passphrase_cb, d);
    
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
            QHash<int, QString> trads;
            
            sql = " SELECT \
                    package_id, \
                    language, \
                    content \
                    \
                    FROM packages_string \
                    \
                    WHERE type = 1;";
                    
            if (!query.exec(sql))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::QueryError;
                err->info = query.lastQuery();
                    
                d->ps->setLastError(err);
                    
                return false;
            }
            
            int lindex;
            
            while (query.next())
            {
                lindex = langs.indexOf(query.value(1).toString());
                
                if (lindex != -1)
                {
                    trads.insert(
                        LANGPKGKEY(query.value(0).toInt(), lindex),
                        query.value(2).toString()
                    );
                }
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
            QString rs, pkname, pkprimlang, trad;
            int i, key, pkid, primlangid;
            
            while (query.next())
            {
                pkname = query.value(0).toString();
                pkprimlang = query.value(5).toString();
                pkid = query.value(18).toInt();
                primlangid = -1;
                
                rs = '[' + pkname + "]\n";
                rs += "Name=" + pkname + '\n';
                rs += "Version=" + query.value(1).toString() + '\n';
                rs += "Source=" + query.value(2).toString() + '\n';
                rs += "UpstreamUrl=" + query.value(16).toString() + '\n';
                rs += "Maintainer=" + query.value(3).toString() + '\n';
                rs += "Section=" + query.value(17).toString() + '\n';
                rs += "Distribution=" + distro + '\n';
                rs += "License=" + query.value(4).toString() + '\n';
                rs += "PrimaryLang=" + pkprimlang + '\n';
                rs += "Flags=" + query.value(6).toString() + '\n';
                rs += "Depends=" + query.value(7).toString() + '\n';
                rs += "Provides=" + query.value(8).toString() + '\n';
                rs += "Replaces=" + query.value(9).toString() + '\n';
                rs += "Suggest=" + query.value(10).toString() + '\n';
                rs += "Conflicts=" + query.value(11).toString() + '\n';
                rs += "DownloadSize=" + query.value(12).toString() + '\n';
                rs += "InstallSize=" + query.value(13).toString() + '\n';
                rs += "Arch=" + arch + '\n';
                rs += "PackageHash=" + query.value(14).toString() + '\n';
                rs += "MetadataHash=" + query.value(15).toString() + '\n';
                
                if (!first)
                {
                    pkgstream += '\n';
                }
                else
                {
                    first = false;
                }
                
                pkgstream += rs;
                
                // Enregistrer les traductions
                for (i=0; i<langs.count(); ++i)
                {
                    key = LANGPKGKEY(pkid, i);
                    
                    if (trads.contains(key))
                    {
                        trad = trads.value(key);
                    }
                    else
                    {
                        // Astuce pour avoir du O(pk) au lieu de O(pk*lang)
                        if (primlangid == -1)
                        {
                            primlangid = langs.indexOf(pkprimlang);
                        }
                        
                        trad = trads.value(LANGPKGKEY(pkid, primlangid));
                    }
                    
                    streams[i] += pkname + ':' + trad + '\n';
                }
            }
            
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
                else
                {
                    // Liste des paquets
                    fileName = filePath + "/packages.xz";
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
                
                // Supprimer le flux
                streams[i].clear();
            }
        }
    }
    
    d->ps->endProgress(progress);
    
    gpgme_release(ctx);
    
    return true;
}