/*
 * packagemetadata.cpp
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

#include "packagemetadata.h"
#include "libpackage.h"
#include "package.h"

#include <QProcess>
#include <QFile>
#include <QLocale>
#include <QRegExp>
#include <QCryptographicHash>

#include <QtXml>
#include <QtDebug>

struct PackageMetaData::Private
{
    PackageSystem *ps;
    Package *pkg;
    
    QDomElement currentPackage;
    
    bool error;
};

PackageMetaData::PackageMetaData(Package *pkg, PackageSystem *ps) : QDomDocument(), QObject(pkg)
{
    d = new Private;
    d->error = false;
    d->ps = ps;
    d->pkg = pkg;
    
    QString fname = ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + pkg->name() + "_" + pkg->version() + "/metadata.xml";
    
    // Si le paquet n'est pas téléchargé, télécharger les métadonnées
    if (!QFile::exists(fname))
    {
        // Télécharger les métadonnées
        QString repo = pkg->repo();
        QString type = ps->repoType(repo);
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + pkg->name() + "~" + pkg->version() + ".metadata.xml.lzma";
        
        ManagedDownload *md = new ManagedDownload;
        
        if (!ps->download(type, ps->repoUrl(repo) + "/" + pkg->url(Package::Metadata), fname, true, md))
        {
            d->error = true;
            return;
        }
        
        delete md;
        
        // Décompresser les métadonnées
        QString cmd = "unlzma " + fname;
        fname.remove(".lzma");
        
        if (QFile::exists(fname))
        {
            QFile::remove(fname);
        }
        
        if (QProcess::execute(cmd) != 0)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::ProcessError;
            err->info = cmd;
            
            d->ps->setLastError(err);
            
            d->error = true;
            return;
        }
    }
    
    // Charger le document
    QFile fl(fname);
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fname;
        
        d->ps->setLastError(err);
        
        d->error = true;
        return;
    }
    
    QByteArray contents = fl.readAll();
    fl.close();
    
    // Vérifier le hash
    QByteArray sha1sum = QCryptographicHash::hash(contents, QCryptographicHash::Sha1).toHex();
    
    if (sha1sum != pkg->metadataHash())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::SHAError;
        err->info = pkg->name();
        err->more = fname;
        
        d->ps->setLastError(err);
        
        d->error = true;
        return;
    }
    
    setContent(contents);
}

PackageMetaData::~PackageMetaData()
{
    delete d;
}

bool PackageMetaData::error() const
{
    return d->error;
}

QString PackageMetaData::primaryLang() const
{
    return documentElement().attribute("primarylang");
}

QString PackageMetaData::stringOfKey(const QDomElement &element) const
{
    // element contient des sous-éléments du type <fr>, <en>, <nl>, <de>, <es>, etc. Prendre celui qui correspond à la langue de l'utilisateur, ou au primarylang du paquet
    QString userLang = QLocale::system().name().section("_", 0, 0);
    
    QDomElement child = element.firstChildElement();
    QDomElement retained;
    
    while (!child.isNull())
    {
        if (child.tagName() == userLang)
        {
            retained = child;
            break;
        }
        else if (child.tagName() == primaryLang())
        {
            retained = child;
        }
        child = child.nextSiblingElement();
    }
    
    return retained.text().trimmed().replace(QRegExp("\\n[ \\t]+"), "\n");
}

QString PackageMetaData::packageDescription() const
{
    return stringOfKey(d->currentPackage.firstChildElement("description"));
}

QString PackageMetaData::packageTitle() const
{
    return stringOfKey(d->currentPackage.firstChildElement("title"));
}

QString PackageMetaData::currentPackage() const
{
    return d->currentPackage.attribute("name");
}
       
void PackageMetaData::setCurrentPackage(const QString &name)
{
    // Explorer les paquets à la recherche de celui qu'on cherche
    QDomElement package = documentElement().firstChildElement("package");
    
    while (!package.isNull())
    {
        if (package.attribute("name") == name)
        {
            d->currentPackage = package;
            return;
        }
        
        package = package.nextSiblingElement("package");
    }
}

QList<ChangeLogEntry *> PackageMetaData::changelog() const
{
    QDomElement entry = documentElement().firstChildElement("changelog").firstChildElement("entry");
    
    QList<ChangeLogEntry *> rs;
    
    while (!entry.isNull())
    {
        ChangeLogEntry *e = new ChangeLogEntry;
        
        e->version = entry.attribute("version");
        e->author = entry.attribute("author");
        e->email = entry.attribute("email");
        e->distribution = entry.attribute("distribution");
        e->text = stringOfKey(entry);
        
        // Trouver la date
        e->date = QDateTime(QDate::fromString(entry.attribute("date"), "yyyy-MM-dd"),
                            QTime::fromString(entry.attribute("time"), "hh:mm:ss"));
        
        // Ajouter à la liste
        rs.append(e);
        
        entry = entry.nextSiblingElement("entry");
    }
    
    return rs;
}