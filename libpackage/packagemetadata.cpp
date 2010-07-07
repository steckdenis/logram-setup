/*
 * packagemetadata.cpp
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

#include "packagemetadata.h"
#include "packagesystem.h"
#include "package.h"
#include "databasepackage.h"
#include "filepackage.h"
#include "templatable.h"
#include "communication.h"

#include <QProcess>
#include <QEventLoop>
#include <QFile>
#include <QLocale>
#include <QRegExp>
#include <QCryptographicHash>
#include <QTemporaryFile>

#include <QtXml>
#include <QtDebug>

using namespace Logram;

struct PackageMetaData::Private
{
    PackageSystem *ps;
    Templatable *tpl;
    Package *pkg;
    
    QDomElement currentPackage;
    
    bool error;
    
    // Processus
    QEventLoop loop;
    QString commandLine;
    QByteArray buffer;
};

PackageMetaData::PackageMetaData(PackageSystem *ps)
    : QObject(ps), QDomDocument()
{
    d = new Private;
    d->error = true;    // Encore invalide
    d->ps = ps;
    d->tpl = 0;
    d->pkg = 0;
}

PackageMetaData::PackageMetaData(PackageSystem *ps, QObject *parent)
    : QObject(parent), QDomDocument()
{
    d = new Private;
    d->error = true;    // Encore invalide
    d->ps = ps;
    d->tpl = 0;
    d->pkg = 0;
}

void PackageMetaData::setTemplatable(Templatable *tpl)
{
    d->tpl = tpl;
}

void PackageMetaData::setPackage(Package *pkg)
{
    d->pkg = pkg;
}

void PackageMetaData::loadFile(const QString &fileName, const QByteArray &sha1hash, bool decompress)
{    
    // Décompresser les métadonnées
    QString fname = fileName;
    
    if (decompress)
    {
        QString cmd = "unxz";
        QStringList args;
        
        args << fname;
        
        fname.remove(".xz");
        
        if (QFile::exists(fname))
        {
            QFile::remove(fname);
        }
        
        if (QProcess::execute(cmd, args) != 0)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::ProcessError;
            err->info = cmd + " \"" + fname + '"';
            
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
    
    QByteArray data = fl.readAll();
    fl.close();
    
    // Vérifier le hash
    if (!sha1hash.isNull())
    {
        QByteArray fileSha1 = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
        
        if (fileSha1 != sha1hash)
        {
            PackageError *err = new PackageError;
            err->type = PackageError::SHAError;
            err->info = fname;
            
            d->ps->setLastError(err);
            
            d->error = true;
            return;
        }
    }
    
    loadData(data);
}

void PackageMetaData::loadData(const QByteArray &data)
{
    setContent(data);
    
    d->error = false;
}

void PackageMetaData::bindPackage(Package *pkg)
{
    QString fname = d->ps->varRoot() + "/var/cache/lgrpkg/db/pkgs/" + pkg->name() + "~" + pkg->version() + ".xml";
    
    if (pkg->origin() == Package::File)
    {
        FilePackage *fpkg = (FilePackage *)pkg;
        
        loadData(fpkg->metadataContents());
        return;
    }
    else if ((pkg->flags() & PACKAGE_FLAG_INSTALLED) && QFile::exists(fname))
    {
        // Déjà téléchargé
        loadFile(fname, QByteArray(), false);
    }
    else
    {
        // Télécharger les métadonnées
        QString repo = pkg->repo();
        
        Repository r;
        d->ps->repository(repo, r);
        
        Repository::Type type = r.type;
        ManagedDownload *md = new ManagedDownload;
        DatabasePackage *dpkg = (DatabasePackage *)pkg;
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + pkg->name() + "~" + pkg->version() + ".metadata.xml.xz";
        
        QString url = r.mirrors.at(0) + "/" + dpkg->url(DatabasePackage::Metadata);
        
        if (!d->ps->download(type, url, fname, true, md))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::DownloadError;
            err->info = url;
            
            d->ps->setLastError(err);
            
            d->error = true;
            return;
        }
        
        delete md;
        
        loadFile(fname, pkg->metadataHash(), true);
    }
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

QString PackageMetaData::upstreamUrl() const
{
    return documentElement().firstChildElement("source").attribute("upstreamurl");
}

QStringList PackageMetaData::triggers() const
{
    QStringList rs;
    
    // Explorer les tags <trigger> du paquet courant
    QDomElement trigger = d->currentPackage.firstChildElement("trigger");
    
    while (!trigger.isNull())
    {
        rs.append(trigger.text());
        
        trigger = trigger.nextSiblingElement("trigger");
    }
    
    return rs;
}

QString PackageMetaData::stringOfKey(const QDomElement &element) const
{
    return stringOfKey(element, primaryLang());
}

QString PackageMetaData::stringOfKey(const QDomElement &element, const QString &primaryLang)
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
        else if (child.tagName() == primaryLang)
        {
            retained = child;
        }
        else if (retained.isNull())
        {
            // On doit au moins en avoir un, même si ce n'est ni le primaryLang, ni la locale
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

QString PackageMetaData::packageEula() const
{
    return stringOfKey(d->currentPackage.firstChildElement("eula"));
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

QDomElement PackageMetaData::currentPackageElement() const
{
    return d->currentPackage;
}

QVector<ChangeLogEntry *> PackageMetaData::changelog() const
{
    QDomElement entry = documentElement().firstChildElement("changelog").firstChildElement("entry");
    
    QVector<ChangeLogEntry *> rs;
    
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
                            
        // Type de mise à jour
        QString t = entry.attribute("type", "lowprio");
        
        if (t == "lowprio")
        {
            e->type = ChangeLogEntry::LowPriority;
        }
        else if (t == "feature")
        {
            e->type = ChangeLogEntry::Feature;
        }
        else if (t == "bugfix")
        {
            e->type = ChangeLogEntry::BugFix;
        }
        else if (t == "security")
        {
            e->type = ChangeLogEntry::Security;
        }
        
        // Ajouter à la liste
        rs.append(e);
        
        entry = entry.nextSiblingElement("entry");
    }
    
    return rs;
}

QVector<SourceDepend *> PackageMetaData::sourceDepends() const
{
    QVector<SourceDepend *> rs;
    SourceDepend *dep;
    QString deptype;
    
    QDomElement depend = documentElement().firstChildElement("source").firstChildElement("depend");
    
    while (!depend.isNull())
    {
        dep = new SourceDepend;
        deptype = depend.attribute("type", "depend");
        
        dep->string = depend.attribute("string");
        
        if (deptype == "depend")
        {
            dep->type = DEPEND_TYPE_DEPEND;
        }
        else if (deptype == "suggest")
        {
            dep->type = DEPEND_TYPE_SUGGEST;
        }
        else if (deptype == "conflict")
        {
            dep->type = DEPEND_TYPE_CONFLICT;
        }
        else if (deptype == "provide")
        {
            dep->type = DEPEND_TYPE_PROVIDE;
        }
        else if (deptype == "replace")
        {
            dep->type = DEPEND_TYPE_REPLACE;
        }
        
        rs.append(dep);
        
        depend = depend.nextSiblingElement("depend");
    }
    
    return rs;
}

QByteArray PackageMetaData::scriptOut() const
{
    return d->buffer;
}

QByteArray PackageMetaData::script(const QString &key, const QString &type)
{
    QDomElement package = documentElement().firstChildElement();
    
    while (!package.isNull())
    {
        if ((package.tagName() == "source" && key == "source") ||
            (package.tagName() == "package" && package.attribute("name") == key))
        {
            QDomElement script = package.firstChildElement("script");
            QByteArray rs, h, f;
            QString header, footer;
            bool found = false;
            
            // Trouver le script lui-même
            while (!script.isNull())
            {
                if (script.attribute("type") == type)
                {
                    // Savoir s'il veut des en-têtes ou autre
                    header = script.attribute("header", "header");
                    footer = script.attribute("footer", "footer");
                    
                    rs = script.text().toUtf8();
                    found = true;
                    break;
                }
                
                script = script.nextSiblingElement("script");
            }
            
            if (!found) break;
            
            // Trouver son en-tête ou pied si nécessaire
            script = package.firstChildElement("script");
            
            while (!script.isNull())
            {
                if (script.attribute("type") == header)
                {
                    h = script.text().toUtf8();
                }
                else if (script.attribute("type") == footer)
                {
                    f = script.text().toUtf8();
                }
                
                script = script.nextSiblingElement("script");
            }
            
            // Renvoyer le résultat
            return h + '\n' + rs + '\n' + f;
        }
        package = package.nextSiblingElement();
    }
    
    return QByteArray();
}

bool PackageMetaData::runScript(const QByteArray &script, const QStringList &args)
{
    if (script.isNull())
    {
        // On ne doit rien lancer
        return true;
    }
    
    // Charger le script
    QFile scriptapi("/usr/bin/lgrpkg/scriptapi");
    
    if (!scriptapi.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = scriptapi.fileName();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    QByteArray header = scriptapi.readAll();
    header += script;
    scriptapi.close();
    
    // Écrire le fichier de script complet dans un QTemporaryFile
    QTemporaryFile tfl;
    
    if (!tfl.open())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = tfl.fileName();
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    if (d->tpl != 0)
    {
        tfl.write(d->tpl->templateString(header));
    }
    else
    {
        tfl.write(header);
    }
    
    tfl.write("\nexit 0\n");
    tfl.close();
    
    // Lancer bash, en lui envoyant comme entrée le script ainsi que scriptapi
    QProcess *sh = new QProcess(this);
    
    QStringList arguments;
    arguments << tfl.fileName();
    arguments << args;
    
    connect(sh, SIGNAL(readyReadStandardOutput()), 
            this, SLOT(processDataOut()));
    connect(sh, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(processTerminated(int, QProcess::ExitStatus)));
    
    d->buffer.clear();
    
    d->commandLine = "/bin/sh " + arguments.join(" ");
    sh->setProcessChannelMode(QProcess::MergedChannels);
    sh->start("/bin/sh", arguments, QIODevice::ReadWrite);
    
    if (!sh->waitForStarted())
    {
        delete sh;
        
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = d->commandLine;
        
        d->ps->setLastError(err);
        
        return false;
    }
    
    // attendre
    int rs = d->loop.exec();
        
    return (rs == 0);
}

bool PackageMetaData::runScript(const QString &key, const QString &type, const QString &currentDir, const QStringList &args)
{
    QString cDir = QDir::currentPath();
    
    QByteArray s = script(key, type);
    
    if (s.isNull())
    {
        return true;
    }
    
    QDir::setCurrent(currentDir);
    
    if (!runScript(s, args))
    {
        QDir::setCurrent(cDir);
        return false;
    }
     
    QDir::setCurrent(cDir);
    
    return true;
}

void PackageMetaData::processDataOut()
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
        
        d->buffer = line;
        
        // Envoyer la progression
        if (!d->ps->processOut(d->commandLine, line))
        {
            // Kill the process
            sh->terminate();
        }
        
        // Émettre le bon signal
        emit processLineOut(sh, line);
    }
}

void PackageMetaData::processTerminated(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Plus besoin du processus
    QProcess *sh = static_cast<QProcess *>(sender());
    
    if (sh != 0)
    {
        sh->deleteLater();
    }
    
    // Savoir si tout est ok
    int rs = 0;
    
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = d->commandLine;
        err->more = d->buffer;
        
        d->ps->setLastError(err);
        
        rs = 1;
    }
    
    // Quitter la boucle
    d->loop.exit(rs);
}

#include "packagemetadata.moc"