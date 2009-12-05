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

#include "packagesystem.h"
#include "databasereader.h"
#include "databasewriter.h"
#include "databasepackage.h"
#include "solver.h"

#include <QSettings>
#include <QStringList>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

#include <cctype>

#include <QtDebug>

using namespace Logram;

struct Logram::PackageSystem::Private
{
    DatabaseReader *dr;
    
    QEventLoop loop;
    QNetworkAccessManager *nmanager;
    QString dlDest;
    QHash<QNetworkReply *, ManagedDownload *> managedDls;
    QSettings *set, *ipackages;
    
    PackageError *lastError;

    // Options
    bool installSuggests;
    int parallelInstalls, parallelDownloads;
    QString installRoot, confRoot, varRoot;
    int setParams;
};

Logram::PackageSystem::PackageSystem(QObject *parent) : QObject(parent)
{
    d = new Private;
    
    d->dr = new DatabaseReader(this);
    d->nmanager = new QNetworkAccessManager(this);
    d->setParams = 0;
    d->lastError = 0;
    
    connect(d->nmanager, SIGNAL(finished(QNetworkReply *)), this, SLOT(downloadFinished(QNetworkReply *)));
}

Logram::PackageSystem::~PackageSystem()
{
    delete d->dr;
    delete d;
}

void Logram::PackageSystem::loadConfig()
{
    d->set = new QSettings(confRoot() + "/etc/lgrpkg/sources.list", QSettings::IniFormat, this);
    
    if ((d->setParams & PACKAGESYSTEM_OPT_INSTALLSUGGESTS) == 0)
    {
        d->installSuggests = d->set->value("InstallSuggests", true).toBool();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_PARALLELINSTALLS) == 0)
    {
        d->parallelInstalls = d->set->value("ParallelInstalls", 1).toInt();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_PARALLELDOWNLOADS) == 0)
    {
        d->parallelDownloads = d->set->value("ParallelDownloads", 2).toInt();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_INSTALLROOT) == 0)
    {
        d->installRoot = d->set->value("InstallRoot", "/").toString();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_CONFROOT) == 0)
    {
        d->confRoot = d->set->value("ConfigRoot", "/").toString();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_VARROOT) == 0)
    {
        d->varRoot = d->set->value("VarRoot", "/").toString();
    }
    
    d->ipackages = new QSettings(varRoot() + "/var/cache/lgrpkg/db/installed_packages.list", QSettings::IniFormat, this);
}

bool Logram::PackageSystem::init()
{
    return d->dr->init();
}

struct Enrg
{
    QString url, distroName, arch, sourceName, type;
};

QString Logram::PackageSystem::repoType(const QString &repoName)
{
    QString rs;
    
    d->set->beginGroup(repoName);
    rs = d->set->value("Type", "remote").toString();
    d->set->endGroup();

    return rs;
}

QString Logram::PackageSystem::repoUrl(const QString &repoName)
{
    QString rs;

    d->set->beginGroup(repoName);
    rs = d->set->value("Url").toString();
    d->set->endGroup();

    return rs;
}

QSettings *Logram::PackageSystem::installedPackagesList() const
{
    return d->ipackages;
}

bool Logram::PackageSystem::update()
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
        
        if (!db->download(enrg->sourceName, u, enrg->type, false))
        {
            return false;
        }

        // Traductions
        u = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch + "/translate." + lang + ".lzma";
        
        sendProgress(GlobalDownload, i*2+1, count*2, u);
        
        if (!db->download(enrg->sourceName, u, enrg->type, true))
        {
            return false;
        }
        
        delete enrg;
    }

    endProgress(GlobalDownload, count*2);

    if (!db->rebuild())
    {
        return false;
    }

    delete db;
    return true;
}

QList<Logram::DatabasePackage *> Logram::PackageSystem::upgradePackages()
{
    QList<UpgradeInfo> upds = d->dr->upgradePackages();
    QList<DatabasePackage *> rs;
    
    // Créer les paquets à mettre à jour
    foreach(const UpgradeInfo &inf, upds)
    {
        DatabasePackage *pkg = new DatabasePackage(inf.installedPackage, this, d->dr, Solver::Update);
        
        pkg->setUpgradePackage(inf.newPackage);
        
        rs.append(pkg);
    }
    
    return rs;
}

bool Logram::PackageSystem::packagesByName(const QString &regex, QList<int> &rs)
{
    return d->dr->packagesByName(regex, rs);
}

bool Logram::PackageSystem::package(const QString &name, const QString &version, DatabasePackage* &rs)
{
    int i;
    
    if (!d->dr->package(name, version, i))
    {
        return false;
    }
    
    rs = new DatabasePackage(i, this, d->dr);

    return true;
}

DatabasePackage *Logram::PackageSystem::package(int id)
{
    return new DatabasePackage(id, this, d->dr);
}

bool Logram::PackageSystem::filesOfPackage(const QString &packageName, QStringList &rs)
{
    rs = QStringList();
    
    // Vérifier que le paquet existe
    if (!d->ipackages->childGroups().contains(packageName))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::PackageNotFound;
        err->info = packageName;
        
        setLastError(err);
        
        return false;
    }

    // La liste des fichiers se trouve dans /var/cache/lgrpkg/db/pkgs/<nom>_<version>/files.list
    QString version = d->ipackages->value(packageName + "/Version").toString();
    QString iroot = d->ipackages->value(packageName + "/InstallRoot").toString();
    QString fileName = varRoot() + "/var/cache/lgrpkg/db/pkgs/" + packageName + "_" + version + "/files.list";

    QFile fl(fileName);

    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fileName;
        
        setLastError(err);
        
        return false;
    }

    // Lire les lignes et les ajouter au résultat
    while (!fl.atEnd())
    {
        rs.append(fl.readLine().replace("./", iroot.toAscii() + "/"));
    }
    
    return true;
}

Solver *Logram::PackageSystem::newSolver()
{
    return new Solver(this, d->dr);
}

bool Logram::PackageSystem::download(const QString &type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs)
{
    // Ne pas télécharger un fichier en cache
    if (QFile::exists(dest))
    {
        if (!block)
        {
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            
            emit downloadEnded(rs);
        }

        return true;
    }
    
    if (type == "remote")
    {
        // Lancer le téléchargement
        QNetworkReply *reply = d->nmanager->get(QNetworkRequest(QUrl(url)));
        connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(dlProgress(qint64, qint64)));

        if (!block)
        {
            // Ajouter le ManagedDownload dans la liste
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = reply;
            rs->url = url;
            rs->destination = dest;

            d->managedDls.insert(reply, rs);
            return true;
        }
        else
        {
            d->dlDest = dest;
            
            int rs = d->loop.exec();
            
            delete reply;
            
            return (rs == 0);
        }
    }
    else if (type == "local")
    {
        if (!QFile::copy(url, dest))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::DownloadError;
            err->info = url;
            
            setLastError(err);
            
            return false;
        }

        // Si on ne bloquait pas, dire que c'est fini quand-même
        if (!block)
        {
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            
            emit downloadEnded(rs);
            return true;
        }
        
        return true;
    }
    
    PackageError *err = new PackageError;
    err->type = PackageError::BadDownloadType;
    err->info = type;
    
    setLastError(err);
    return false;
}

void Logram::PackageSystem::downloadFinished(QNetworkReply *reply)
{
    // Savoir si on bloquait
    ManagedDownload *md = d->managedDls.value(reply, 0);
    
    // Voir s'il y a eu des erreurs
    if (reply->error() != QNetworkReply::NoError)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::DownloadError;
        err->info = d->dlDest;
        
        setLastError(err);
        
        if (md == 0)
        {
            d->loop.exit(1);
            return;
        }
        else
        {
            md->error = true;
            emit downloadEnded(md);
            return;
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
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        setLastError(err);
        
        if (md == 0)
        {
            d->loop.exit(1);
            return;
        }
        else
        {
            md->error = true;
            return;
        }
    }

    fl.write(reply->readAll());

    fl.close();

    if (md == 0)
    {
        // Quitter la boucle
        d->loop.exit(0);
    }
    else
    {
        // Envoyer le signal comme quoi on a fini
        emit downloadEnded(md);
    }
}

void Logram::PackageSystem::dlProgress(qint64 done, qint64 total)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply != 0)
    {
        sendProgress(Download, done, total, reply->url().toString());
    }
}

/* Erreurs */

void Logram::PackageSystem::setLastError(PackageError *err)
{
    if (d->lastError != 0)
    {
        delete d->lastError;
    }
    
    d->lastError = err;
}

PackageError *Logram::PackageSystem::lastError()
{
    return d->lastError;
}

/* Utilitaires */

int Logram::PackageSystem::parseVersion(const QString &verStr, QString &name, QString &version)
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

bool Logram::PackageSystem::matchVersion(const QByteArray &v1, const QByteArray &v2, int op)
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

int Logram::PackageSystem::compareVersions(const QByteArray &v1, const QByteArray &v2)
{
    return compareVersions(v1.constData(), v2.constData());
}

int Logram::PackageSystem::compareVersions(const char *a, const char *b)
{
    if (strcmp(a, b) == 0) return 0;
    
    // Explorer chaque caractère
    int numa, numb;
    
    while (*a && *b)
    {
        //Sauter tout ce qui n'est pas nombre
        while (*a && !isdigit(*a)) a++;
        while (*b && !isdigit(*b)) b++;
        
        // On a donc maintenant des digits sous la dent, les transformer en entiers
        numa = 0;
        numb = 0;
        
        if (!*b && !*a)
        {
            // Les deux chaînes sont arrivées au bout, elles sont les mêmes
            return 0;
        }
        
        if (*a)
        {
            while (*a && isdigit(*a))
            {
                numa *= 10;
                numa += (*a - '0');
                a++;
            }
        }
        else
        {
            // Il manque un champ, l'ajouter
            numa = 0;
        }
        
        if (*b)
        {
            while (*b && isdigit(*b))
            {
                numb *= 10;
                numb += (*b - '0');
                b++;
            }
        }
        else
        {
            numb = 0;
        }
        
        // Comparer les nombres
        if (numa > numb)
        {
            return 1;
        }
        else if (numa < numb)
        {
            return -1;
        }
        
        // Deux nombres égaux, passer à la suite des champs
    }
    
    return 0;   // Les mêmes
}

QString Logram::PackageSystem::fileSizeFormat(int size)
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

QString Logram::PackageSystem::dependString(const QString &name, const QString &version, int op)
{
    QString rs(name);

    if (op != DEPEND_OP_NOVERSION)
    {
        switch (op)
        {
            case DEPEND_OP_EQ:
                rs += " (= ";
                break;
            case DEPEND_OP_GREQ:
                rs += " (>= ";
                break;
            case DEPEND_OP_GR:
                rs += " (> ";
                break;
            case DEPEND_OP_LOEQ:
                rs += " (<= ";
                break;
            case DEPEND_OP_LO:
                rs += " (< ";
                break;
            case DEPEND_OP_NE:
                rs += " (!= ";
                break;
        }

        rs += version + ")";
    }
    
    return rs;
}

/* Options */

bool Logram::PackageSystem::installSuggests() const
{
    return d->installSuggests;
}

int Logram::PackageSystem::parallelDownloads() const
{
    return d->parallelDownloads;
}

int Logram::PackageSystem::parallelInstalls() const
{
    return d->parallelInstalls;
}

QString Logram::PackageSystem::installRoot() const
{
    return d->installRoot;
}

QString Logram::PackageSystem::confRoot() const
{
    return d->confRoot;
}

QString Logram::PackageSystem::varRoot() const
{
    return d->varRoot;
}

void Logram::PackageSystem::setConfRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_CONFROOT;
    
    d->confRoot = root;
}

void Logram::PackageSystem::setVarRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_VARROOT;
    
    d->varRoot = root;
}

void Logram::PackageSystem::setInstallSuggests(bool enable)
{
    d->setParams |= PACKAGESYSTEM_OPT_INSTALLSUGGESTS;
    
    d->installSuggests = enable;
}

void Logram::PackageSystem::setParallelDownloads(int num)
{
    d->setParams |= PACKAGESYSTEM_OPT_PARALLELDOWNLOADS;
    
    d->parallelDownloads = num;
}

void Logram::PackageSystem::setParallelInstalls(int num)
{
    d->setParams |= PACKAGESYSTEM_OPT_PARALLELINSTALLS;
    
    d->parallelInstalls = num;
}

void Logram::PackageSystem::setInstallRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_INSTALLROOT;
    
    d->installRoot = root;
}

/* Signaux */
void Logram::PackageSystem::sendProgress(Progress type, int num, int tot, const QString &msg)
{
    emit progress(type, num, tot, msg);
}

void Logram::PackageSystem::endProgress(Progress type, int tot)
{
    emit progress(type, tot, tot, QString());
}