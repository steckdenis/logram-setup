/*
 * packagesystem.cpp
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

#include "libpackage.h"
#include "libpackage_p.h"
#include "package.h"
#include "databasewriter.h"
#include "solver.h"

#include <QSettings>
#include <QStringList>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

#include <QtDebug>

PackageSystem::PackageSystem(QObject *parent) : QObject(parent)
{
    d = new PackageSystemPrivate(this);

    d->nmanager = new QNetworkAccessManager(this);
    connect(d->nmanager, SIGNAL(finished(QNetworkReply *)), this, SLOT(downloadFinished(QNetworkReply *)));

    d->set = new QSettings("/etc/lgrpkg/sources.list", QSettings::IniFormat, this);
    d->ipackages = new QSettings(installRoot() + "/var/cache/lgrpkg/db/installed_packages.list", QSettings::IniFormat, this);

    d->installSuggests = d->set->value("InstallSuggests", true).toBool();
    d->parallelInstalls = d->set->value("ParallelInstalls", 1).toInt();
    d->parallelDownloads = d->set->value("ParallelDownloads", 2).toInt();
    d->installRoot = d->set->value("InstallRoot", "/").toString();
}

void PackageSystem::init()
{
    d->init();
}

struct Enrg
{
    QString url, distroName, arch, sourceName, type;
};

QString PackageSystem::repoType(const QString &repoName)
{
    QString rs;
    
    d->set->beginGroup(repoName);
    rs = d->set->value("Type", "remote").toString();
    d->set->endGroup();

    return rs;
}

QString PackageSystem::repoUrl(const QString &repoName)
{
    QString rs;

    d->set->beginGroup(repoName);
    rs = d->set->value("Url").toString();
    d->set->endGroup();

    return rs;
}

QSettings *PackageSystem::installedPackagesList() const
{
    return d->ipackages;
}

void PackageSystem::update()
{
    // Explorer la liste des mirroirs dans /etc/setup/sources.list, format QSettings
    QString lang = d->set->value("Language", tr("fr", "Langue par défaut pour les paquets")).toString();
    
    DatabaseWriter *db = new DatabaseWriter(this);

    QList<Enrg *> enrgs;

    foreach (const QString &sourceName, d->set->childGroups())
    {
        d->set->beginGroup(sourceName);

        QString type = d->set->value("Type", "remote").toString();
        QString url = d->set->value("Url").toString();
        QStringList distros = d->set->value("Distributions").toString().split(' ', QString::SkipEmptyParts);
        QStringList archs = d->set->value("Archs").toString().split(' ', QString::SkipEmptyParts);

        // Explorer chaque distribution, et chaque architecture dedans
        foreach (const QString &distroName, distros)
        {
            foreach (const QString &arch, archs)
            {
                Enrg *enrg = new Enrg;

                enrg->url = url;
                enrg->distroName = distroName;
                enrg->arch = arch;
                enrg->sourceName = sourceName;
                enrg->type = type;

                enrgs.append(enrg);
            }
        }

        d->set->endGroup();
    }

    // Explorer les enregistrements et les télécharger
    int count = enrgs.count();
    for (int i=0; i<count; ++i)
    {
        Enrg *enrg = enrgs.at(i);

        QString u = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch + "/packages.lzma";
        
        sendProgress(GlobalDownload, i*2, count*2, u);
        db->download(enrg->sourceName, u, enrg->type, false);

        // Traductions
        u = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch + "/translate." + lang + ".lzma";
        
        sendProgress(GlobalDownload, i*2+1, count*2, u);
        db->download(enrg->sourceName, u, enrg->type, true);
    }

    endProgress(GlobalDownload, count*2);

    db->rebuild();

    delete db;
}

QList<Package *> PackageSystem::packagesByName(const QString &regex)
{
    QList<Package *> rs;
    Package *pkg;
    
    QList<int> pkgs = d->packagesByName(regex);

    foreach(int i, pkgs)
    {
        pkg = new Package(i, this, d);

        rs.append(pkg);
    }

    return rs;
}

Package *PackageSystem::package(const QString &name, const QString &version)
{
    Package *pkg;
    int i = d->package(name, version);

    pkg = new Package(i, this, d);

    return pkg;
}

QStringList PackageSystem::filesOfPackage(const QString &packageName)
{
    // Vérifier que le paquet existe
    if (!d->ipackages->childGroups().contains(packageName))
    {
        return QStringList();
    }

    // La liste des fichiers se trouve dans /var/cache/lgrpkg/db/pkgs/<nom>_<version>/files.list
    QString version = d->ipackages->value(packageName + "/Version").toString();
    QString iroot = d->ipackages->value(packageName + "/InstallRoot").toString();
    QString fileName = installRoot() + "/var/cache/lgrpkg/db/pkgs/" + packageName + "_" + version + "/files.list";

    QFile fl(fileName);

    if (!fl.open(QIODevice::ReadOnly))
    {
        raise(OpenFileError, fileName);
    }

    // Lire les lignes et les ajouter au résultat
    QStringList rs;

    while (!fl.atEnd())
    {
        rs.append(fl.readLine().replace("./", iroot.toAscii() + "/"));
    }

    return rs;
}

Solver *PackageSystem::newSolver()
{
    return new Solver(this, d);
}

ManagedDownload *PackageSystem::download(const QString &type, const QString &url, const QString &dest, bool block)
{
    // Ne pas télécharger un fichier en cache
    if (QFile::exists(dest))
    {
        if (!block)
        {
            ManagedDownload *rs = new ManagedDownload;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            
            emit downloadEnded(rs);
            return rs;
        }

        return 0;
    }
    
    if (type == "remote")
    {
        // Lancer le téléchargement
        QNetworkReply *reply = d->nmanager->get(QNetworkRequest(QUrl(url)));
        connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(dlProgress(qint64, qint64)));

        if (!block)
        {
            // Ajouter le ManagedDownload dans la liste
            ManagedDownload *rs = new ManagedDownload;
            rs->reply = reply;
            rs->url = url;
            rs->destination = dest;

            d->managedDls.insert(reply, rs);
            return rs;
        }
        else
        {
            d->dlDest = dest;
            d->loop.exec();
        }
    }
    else if (type == "local")
    {
        if (!QFile::copy(url, dest))
        {
            raise(DownloadError, url);
        }

        // Si on ne bloquait pas, dire que c'est fini quand-même
        if (!block)
        {
            ManagedDownload *rs = new ManagedDownload;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            return rs;
        }
    }

    return 0;
}

void PackageSystem::downloadFinished(QNetworkReply *reply)
{
    // Savoir si on bloquait
    ManagedDownload *md = d->managedDls.value(reply, 0);
    
    // Voir s'il y a eu des erreurs
    if (reply->error() != QNetworkReply::NoError)
    {
        raise(PackageSystem::DownloadError, d->dlDest);

        if (md == 0)
        {
            // On était bloquant, quitter la boucle
            d->loop.exit(1);
        }
    }

    // Téléchargement
    QString dlDest;

    if (md != 0)
    {
        dlDest = md->destination;
    }
    else
    {
        dlDest = d->dlDest;
    }
    
    QFile fl(dlDest);

    if (!fl.open(QIODevice::WriteOnly))
    {
        raise(OpenFileError, fl.fileName());
    }

    fl.write(reply->readAll());

    fl.close();

    if (md == 0)
    {
        // Supprimer la réponse
        delete reply;

        // Quitter la boucle
        d->loop.exit(0);
    }
    else
    {
        // Envoyer le signal comme quoi on a fini
        emit downloadEnded(md);
    }
}

void PackageSystem::dlProgress(qint64 done, qint64 total)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply != 0)
    {
        sendProgress(Download, done, total, reply->url().toString());
    }
}

int PackageSystem::parseVersion(const QString &verStr, QString &name, QString &version)
{
    QStringList parts;
    int rs;
    
    if (verStr.contains(">="))
    {
        parts = verStr.split(">=");
        rs = DEPEND_OP_GREQ;
    }
    else if (verStr.contains("<="))
    {
        parts = verStr.split("<=");
        rs = DEPEND_OP_LOEQ;
    }
    else if (verStr.contains("!="))
    {
        parts = verStr.split("!=");
        rs = DEPEND_OP_NE;
    }
    else if (verStr.contains('<'))
    {
        parts = verStr.split('<');
        rs = DEPEND_OP_LO;
    }
    else if (verStr.contains('>'))
    {
        parts = verStr.split('>');
        rs = DEPEND_OP_GR;
    }
    else if (verStr.contains('='))
    {
        parts = verStr.split('=');
        rs = DEPEND_OP_EQ;
    }
    else
    {
        name = verStr;
        version = QString();
        return DEPEND_OP_NOVERSION;
    }

    name = parts.at(0);
    version = parts.at(1);

    return rs;
}

bool PackageSystem::matchVersion(const QString &v1, const QString &v2, int op)
{
    // Comparer les versions
    int rs = compareVersions(v1, v2);

    // Retourner en fonction de l'opérateur
    switch (op)
    {
        case DEPEND_OP_EQ:
            return (rs == 0);
        case DEPEND_OP_GREQ:
            return ( (rs == 0) || (rs == 1) );
        case DEPEND_OP_GR:
            return (rs == 1);
        case DEPEND_OP_LOEQ:
            return ( (rs == 0) || (rs == -1) );
        case DEPEND_OP_LO:
            return (rs == -1);
        case DEPEND_OP_NE:
            return (rs != 0);
    }

    return true;
}

int PackageSystem::compareVersions(const QString &v1, const QString &v2)
{
    QRegExp regex("[^\\d]");
    QStringList v1s = v1.split(regex, QString::SkipEmptyParts);
    QStringList v2s = v2.split(regex, QString::SkipEmptyParts);

    if (v1 == v2) return 0;

    /* Compare Version */
    /*
     * We compare character by character. For example :
     *      0.1.7-2 => 0 1 7 2
     *      0.1.7   => 0 1 7
     *
     * With "maxlength", we know that we must add 0 like this :
     *      0 1 7 2 => 0 1 7 2
     *      0 1 7   => 0 1 7 0
     *
     * We compare every number with its double. If their are equal
     *      Continue
     * If V1 is more recent
     *      Return true
     * End bloc
     */

    int maxlength = v1s.count() > v2s.count() ? v1s.count() : v2s.count();

    int v1num, v2num;

    for (int i = 0; i < maxlength; i++)
    {
        if (i >= v1s.count())
        {
            v1num = 0;
        }
        else
        {
            v1num = v1s.at(i).toInt();
        }

        if (i >= v2s.count())
        {
            v2num = 0;
        }
        else
        {
            v2num = v2s.at(i).toInt();
        }

        if (v1num > v2num)
        {
            return 1;
        }
        else if (v1num < v2num)
        {
            return -1;
        }
    }

    return 0;       //Égal
}

QString PackageSystem::fileSizeFormat(int size)
{
    if (size < 1024)
    {
        return QString::number(size) + " o";
    }
    else if (size < 1024*1024)
    {
        return QString::number(size/1024) + " Kio";
    }
    else if (size < 1024*1024*1024)
    {
        return QString::number(size/(1024*1024)) + " Mio";
    }
    else
    {
        return QString::number(size/(1024*1024*1024)) + " Gio";
    }
}

/* Options */

bool PackageSystem::installSuggests() const
{
    return d->installSuggests;
}

int PackageSystem::parallelDownloads() const
{
    return d->parallelDownloads;
}

int PackageSystem::parallelInstalls() const
{
    return d->parallelInstalls;
}

QString PackageSystem::installRoot() const
{
    return d->installRoot;
}

void PackageSystem::setInstallSuggests(bool enable)
{
    d->installSuggests = enable;
}

void PackageSystem::setParallelDownloads(int num)
{
    d->parallelDownloads = num;
}

void PackageSystem::setParallelInstalls(int num)
{
    d->parallelInstalls = num;
}

void PackageSystem::setInstallRoot(const QString &root)
{
    d->installRoot = root;
}

/* Signaux */

void PackageSystem::raise(Error err, const QString &info)
{
    emit error(err, info);
}

void PackageSystem::sendProgress(Progress type, int num, int tot, const QString &msg)
{
    emit progress(type, num, tot, msg);
}

void PackageSystem::endProgress(Progress type, int tot)
{
    emit progress(type, tot, tot, QString());
}

void PackageSystem::sendMessage(Package *sndr, const QString &msg)
{
    emit message(sndr, msg);
}