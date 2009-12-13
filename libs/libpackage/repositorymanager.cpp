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

#include <QtSql>
#include <QtXml>
#include <QtDebug>

using namespace Logram;

struct RepositoryManager::Private
{
    PackageSystem *ps;
    QSettings *set;
    
    // BDD
    QSqlDatabase db;
    QRegExp regex;
    
    // Fonctions
    bool registerString(QSqlQuery &query, int package_id, const QString &lang, const QString &cont, int type);
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

bool RepositoryManager::Private::registerString(QSqlQuery &query, int package_id, const QString &lang, const QString &cont, int type)
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
            AND type=%3;";
            
    if (!query.exec(sql
                    .arg(package_id)
                    .arg(lang)
                    .arg(type)
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
                            .arg(content.replace(regex, "\n"))
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
        sql = " INSERT INTO packages_string (package_id, language, type, content) \
                VALUES(%1, '%2', %3, '%4');";
        
        if (!query.exec(sql
                        .arg(package_id)
                        .arg(lang)
                        .arg(type)
                        .arg(content.replace(regex, "\n"))
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
    if (!query.exec(sql.arg(fpkg->name(), fpkg->distribution(), fpkg->arch())))
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
            
            TRY_QUERY(sql.arg(fpkg->section()))
            
            section_id = query.value(0).toInt();
        }
    }
    else
    {
        // Trouver les ID de la distribution, architecture et section
        sql = "SELECT id FROM packages_section WHERE name='%1';";
        TRY_QUERY(sql.arg(fpkg->section()))
        section_id = query.value(0).toInt();
        
        sql = "SELECT id FROM packages_arch WHERE name='%1';";
        TRY_QUERY(sql.arg(fpkg->arch()))
        arch_id = query.value(0).toInt();
        
        sql = "SELECT id FROM packages_distribution WHERE name='%1';";
        TRY_QUERY(sql.arg(fpkg->distribution()))
        distro_id = query.value(0).toInt();
    }
    
    // Insérer ou mettre à jour le paquet
    if (!update)
    {
        sql = " INSERT INTO \
                packages_package(name, maintainer, section_id, version, arch_id, distribution_id, primarylang, download_size, install_size, date, depends, suggests, conflicts, provides, replaces, source, license, flags, packageHash, metadataHash, download_url) \
                VALUES ('%1', '%2', %3, '%4', %5, %6, '%7', %8, %9, NOW(), '%10', '%11', '%12', '%13', '%14', '%15', '%16', %17, '%18', '%19', '%20');";
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
                download_url='%20' \
                \
                WHERE id=") + QString::number(package_id) + ";";
    }
    
    // Lancer la requête
    if (!query.exec(sql
            .arg(fpkg->name(), fpkg->maintainer())
            .arg(section_id)
            .arg(fpkg->version())
            .arg(arch_id)
            .arg(distro_id)
            .arg(md->primaryLang())
            .arg(fpkg->downloadSize())
            .arg(fpkg->installSize())
            .arg(DEPEND(DEPEND_TYPE_DEPEND))
            .arg(DEPEND(DEPEND_TYPE_SUGGEST))
            .arg(DEPEND(DEPEND_TYPE_CONFLICT))
            .arg(DEPEND(DEPEND_TYPE_PROVIDE))
            .arg(DEPEND(DEPEND_TYPE_REPLACE))
            .arg(fpkg->source())
            .arg(fpkg->license())
            .arg(int(fpkg->isGui()))
            .arg(QString(fpkg->packageHash()))
            .arg(QString(fpkg->metadataHash()))
            .arg(download_url)
            )) // TODO: Récupérer l'id
    {
        PackageError *err = new PackageError;
        err->type = PackageError::QueryError;
        err->info = query.lastQuery();
        
        d->ps->setLastError(err);
        
        return false;
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
    
    // Copier le paquet, enregistrer les métadonnées
    QDir curDir = QDir::current();
    
    // Fichier du paquet
    if (!QFile::exists(download_url))
    {
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
    }
    
    // Métadonnées
    if (!QFile::exists(metadata_url))
    {
        if (!curDir.mkpath(metadata_url.section('/', 0, -2)))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = download_url.section('/', 0, -2);
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        QProcess xz;
        xz.start("xz", QStringList() << "-c");
        
        if (!xz.waitForStarted())
        {
            PackageError *err = new PackageError;
            err->type = PackageError::ProcessError;
            err->info = "xz -c";
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        xz.write(fpkg->metadataContents());
        xz.closeWriteChannel();
        
        if (!xz.waitForFinished())
        {
            PackageError *err = new PackageError;
            err->type = PackageError::ProcessError;
            err->info = "xz -c";
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        QFile fl(metadata_url);
        
        if (!fl.open(QIODevice::WriteOnly))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::OpenFileError;
            err->info = metadata_url;
            
            d->ps->setLastError(err);
            
            return false;
        }
        
        fl.write(xz.readAll());
        fl.close();
    }
    
    return true;
}