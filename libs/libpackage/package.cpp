/*
 * package.cpp
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

#include "package.h"
#include "packagesystem.h"
#include "databasereader.h"
#include "packagemetadata.h"
#include "communication.h"

#include <QtDebug>

#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QFile>
#include <QCryptographicHash>

using namespace Logram;

/*************************************
******* Privates *********************
*************************************/

struct Package::Private
{
    int index, upgradeIndex;
    PackageSystem *ps;
    DatabaseReader *psd;

    bool depok;
    QList<Depend *> deps;

    Solver::Action action;

    // Téléchargement
    QString waitingDest;

    // Installation
    QProcess *installProcess;
    QString installCommand;
    QString readBuf;
    Package *upd;
    
    // Métadonnées
    PackageMetaData *md;
};

struct Depend::Private
{
    _Depend *dep;
    DatabaseReader *psd;

    QList<Depend *> subdeps;
};

/*************************************
******* Package **********************
*************************************/

Package::Package(int index, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action) : QObject(ps)
{
    d = new Private;
    d->index = index;
    d->upgradeIndex = -1;
    d->ps = ps;
    d->psd = psd;
    d->depok = false;
    d->action = _action;
    d->md = 0;
    d->installProcess = 0;
    d->upd = 0;

    connect(ps, SIGNAL(downloadEnded(Logram::ManagedDownload *)), this, SLOT(downloadEnded(Logram::ManagedDownload *)));
}

Package::~Package()
{
    qDeleteAll(d->deps);
    
    if (d->installProcess != 0)
    {
        delete d->installProcess;
    }
    
    delete d;
}

PackageMetaData *Package::metadata()
{
    if (d->md == 0)
    {
        // On doit récupérer les métadonnées
        d->md = new PackageMetaData(this, d->ps);
        
        if (d->md->error())
        {
            d->md = 0;
        }
    }
    
    return d->md;
}

void Package::process()
{
    d->installProcess = new QProcess();
    
    d->installProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(d->installProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processEnd(int, QProcess::ExitStatus)));
    connect(d->installProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOut()));

    // En fonction de l'action qui nous est demandée, faire ce qui convient
    if (action() == Solver::Install)
    {
        d->installCommand = QString("/usr/bin/helperscript install \"%1\" \"%2\" \"%3\" \"%4\"")
            .arg(name())
            .arg(version())
            .arg(d->waitingDest)
            .arg(d->ps->installRoot());
    }
    else if (action() == Solver::Remove)
    {
        d->installCommand = QString("/usr/bin/helperscript remove \"%1\" \"%2\" \"%3\" false")
            .arg(name())
            .arg(version())
            .arg(d->ps->installRoot());
    }
    else if (action() == Solver::Purge)
    {
        d->installCommand = QString("/usr/bin/helperscript remove \"%1\" \"%2\" \"%3\" true")
            .arg(name())
            .arg(version())
            .arg(d->ps->installRoot());
    }
    else if (action() == Solver::Update)
    {
        d->installCommand = QString("/usr/bin/helperscript update \"%1\" \"%2\" \"%3\" \"%4\" \"%5\"")
            .arg(name())
            .arg(newerVersion())
            .arg(d->waitingDest)
            .arg(d->ps->installRoot())
            .arg(version());
    }
        
    d->installProcess->start(d->installCommand);
}

void Package::processOut()
{
    QString buf = d->installProcess->readAll();
    d->readBuf += buf;
    QString out = buf.trimmed();

    // Voir si une communication n'est pas arrivée
    if (out.startsWith("[[>>|") && out.endsWith("|<<]]"))
    {
        QStringList parts = out.split('|');
        
        // Trouver le nom de la communication, en enlevant d'abord le premier [[>>
        parts.removeAt(0);
        QString name = parts.takeAt(0);
        
        // Créer la communication
        Communication *comm = new Communication(d->ps, this, name);
        
        if (comm->error())
        {
            // Erreur survenue
            emit proceeded(false);
            return;
        }
        
        // Explorer les paramètres
        while (parts.count() != 1)
        {
            QString key = parts.takeAt(0);
            QString value = parts.takeAt(0);
            
            comm->addKey(key, value);
        }
        
        // Envoyer la demande de communication
        emit communication(this, comm);
        
        // Retourner le résultat au processus
        d->installProcess->write(comm->processData().toUtf8() + "\n");
    }
}

void Package::processEnd(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = d->installCommand;
        err->more = d->readBuf;
        
        d->readBuf.clear();
        
        d->ps->setLastError(err);
        emit proceeded(false);
        return;
    }
    
    d->readBuf.clear();
    
    // Enregistrer le paquet dans la liste des paquets installés pour le prochain setup update
    QSettings *set = d->ps->installedPackagesList();
    _Package *pkg = d->psd->package(d->index);
    
    if (action() == Solver::Install)
    {
        set->beginGroup(name());
        set->setValue("Name", name());
        set->setValue("Version", version());
        set->setValue("Source", source());
        set->setValue("Maintainer", maintainer());
        set->setValue("Section", section());
        set->setValue("Distribution", distribution());
        set->setValue("License", license());
        set->setValue("Depends", dependsToString(depends(), DEPEND_TYPE_DEPEND));
        set->setValue("Provides", dependsToString(depends(), DEPEND_TYPE_PROVIDE));
        set->setValue("Suggest", dependsToString(depends(), DEPEND_TYPE_SUGGEST));
        set->setValue("Replaces", dependsToString(depends(), DEPEND_TYPE_REPLACE));
        set->setValue("Conflicts", dependsToString(depends(), DEPEND_TYPE_CONFLICT));
        set->setValue("DownloadSize", downloadSize());
        set->setValue("InstallSize", installSize());
        set->setValue("MetadataHash", metadataHash());
        set->setValue("PackageHash", packageHash());

        set->setValue("ShortDesc", QString(shortDesc().toUtf8().toBase64()));
        
        set->setValue("InstalledDate", QDateTime::currentDateTime().toTime_t());
        set->setValue("InstalledRepo", repo());
        set->setValue("InstalledBy", QString(getenv("UID")).toInt());
        set->setValue("State", PACKAGE_STATE_INSTALLED);
        set->setValue("InstallRoot", d->ps->installRoot());
        set->endGroup();

        // Enregistrer les informations dans le paquet directement, puisqu'il est dans un fichier mappé
        pkg->idate = QDateTime::currentDateTime().toTime_t();
        pkg->iby = QString(getenv("UID")).toInt();
        pkg->state = PACKAGE_STATE_INSTALLED;
    }
    else if (action() == Solver::Update)
    {
        Package *other = upgradePackage();
        
        set->beginGroup(name());
        set->setValue("Name", name());
        set->setValue("Version", other->version());
        set->setValue("Source", other->source());
        set->setValue("Maintainer", other->maintainer());
        set->setValue("Section", other->section());
        set->setValue("Distribution", other->distribution());
        set->setValue("License", other->license());
        set->setValue("Depends", dependsToString(other->depends(), DEPEND_TYPE_DEPEND));
        set->setValue("Provides", dependsToString(other->depends(), DEPEND_TYPE_PROVIDE));
        set->setValue("Suggest", dependsToString(other->depends(), DEPEND_TYPE_SUGGEST));
        set->setValue("Replaces", dependsToString(other->depends(), DEPEND_TYPE_REPLACE));
        set->setValue("Conflicts", dependsToString(other->depends(), DEPEND_TYPE_CONFLICT));
        set->setValue("DownloadSize", other->downloadSize());
        set->setValue("InstallSize", other->installSize());
        set->setValue("MetadataHash", other->metadataHash());
        set->setValue("PackageHash", other->packageHash());

        set->setValue("ShortDesc", QString(other->shortDesc().toUtf8().toBase64()));
        
        set->setValue("InstalledDate", QDateTime::currentDateTime().toTime_t());
        set->setValue("InstalledRepo", other->repo());
        set->setValue("InstalledBy", QString(getenv("UID")).toInt());
        set->setValue("State", PACKAGE_STATE_INSTALLED);
        set->setValue("InstallRoot", d->ps->installRoot());
        set->endGroup();

        // Enregistrer les informations dans le paquet directement, puisqu'il est dans un fichier mappé
        pkg->idate = QDateTime::currentDateTime().toTime_t();
        pkg->iby = QString(getenv("UID")).toInt();
        pkg->state = PACKAGE_STATE_NOTINSTALLED;
        
        // Également pour l'autre paquet (le C++ autorise l'accès aux membres privés d'autres instances)
        _Package *opkg = d->psd->package(other->d->index);
        opkg->idate = pkg->idate;
        opkg->iby = pkg->iby;
        opkg->state = PACKAGE_STATE_INSTALLED;
    }
    else if (action() == Solver::Remove)
    {
        // Enregistrer le paquet comme supprimé
        set->beginGroup(name());
        set->setValue("State", PACKAGE_STATE_REMOVED);
        set->endGroup();
        
        // Également dans la base de donnée binaire, avec l'heure et l'UID de celui qui a supprimé le paquet
        pkg->state = PACKAGE_STATE_REMOVED;
        pkg->idate = QDateTime::currentDateTime().toTime_t();
        pkg->iby = QString(getenv("UID")).toInt();
    }
    else if (action() == Solver::Purge)
    {
        // Effacer toute trace du paquet
        set->remove(name());
        
        pkg->state = PACKAGE_STATE_NOTINSTALLED;
        pkg->idate = 0;
        pkg->iby = 0;
    }

    // Supprimer le processus
    d->installProcess->deleteLater();
    d->installProcess = 0;
    
    // L'installation est finie
    emit proceeded(true);
}

bool Package::download()
{
    QString fname, r, type, u;
    
    if (action() == Solver::Remove || action() == Solver::Purge)
    {
        // Pas besoin de télécharger
        emit downloaded(true);
        return true;
    }
    else if (action() == Solver::Install)
    {
        // Télécharger le paquet
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + url().section('/', -1, -1);
        r = repo();
        type = d->ps->repoType(r);
        u = d->ps->repoUrl(r) + "/" + url();
    }
    else /* Update */
    {
        Package *other = upgradePackage();
        
        fname = d->ps->varRoot() + "/var/cache/lgrpkg/download/" + other->url().section('/', -1, -1);
        r = other->repo();
        type = d->ps->repoType(r);
        u = d->ps->repoUrl(r) + "/" + other->url();
    }
    
    d->waitingDest = fname;
        
    ManagedDownload *md;

    return d->ps->download(type, u, fname, false, md); // Non-bloquant
        
}

void Package::downloadEnded(ManagedDownload *md)
{
    if (md != 0)
    {
        if (md->destination == d->waitingDest)
        {
            if (md->error)
            {
                delete md;
                
                emit downloaded(false);
                return;
            }
            
            // Vérifier le sha1sum du paquet
            QFile fl(d->waitingDest);
            
            if (!fl.open(QIODevice::ReadOnly))
            {
                PackageError *err = new PackageError;
                err->type = PackageError::OpenFileError;
                err->info = d->waitingDest;
                
                d->ps->setLastError(err);
            
                delete md;
                emit downloaded(false);
                return;
            }
            
            QByteArray contents = fl.readAll();
            fl.close();
            
            QString sha1sum = QCryptographicHash::hash(contents, QCryptographicHash::Sha1).toHex();
            
            // Comparer les hashs
            QString myHash;
            
            if (action() == Solver::Update)
            {
                myHash = upgradePackage()->packageHash();
            }
            else
            {
                myHash = packageHash();
            }
            
            if (sha1sum != myHash)
            {
                PackageError *err = new PackageError;
                err->type = PackageError::SHAError;
                err->info = name();
                err->more = md->url;
                
                d->ps->setLastError(err);
                
                delete md;
                emit downloaded(false);
                return;
            }
            
            // On a téléchargé le paquet !
            delete md;
            emit downloaded(true);
        }
    }
}

Solver::Action Package::action()
{
    return d->action;
}

bool Package::isValid()
{
    return d->index != -1;
}

QList<Package *> Package::versions()
{
    QList<int> pkgs = d->psd->packagesOfString(0, d->psd->package(d->index)->name, DEPEND_OP_NOVERSION);
    QList<Package *> rs;

    foreach(int pkg, pkgs)
    {
        Package *pk = new Package(pkg, d->ps, d->psd);
        rs.append(pk);
    }

    return rs;
}

QList<Depend *> Package::depends()
{
    if (d->depok)
    {
        // On a les dépendances en cache
        return d->deps;
    }

    // Trouver les dépendances
    QList<_Depend *> mdeps = d->psd->depends(d->index);
    QList<Depend *> rs;
    
    // Créer les dépendances pour tout ça
    Depend *dep;
    
    for (int i=0; i<mdeps.count(); ++i)
    {
        dep = new Depend(mdeps.at(i), d->psd);
        rs.append(dep);
    }

    // Mettre en cache
    d->deps = rs;
    d->depok = true;

    return rs;
}

QString Package::dependsToString(const QList<Depend *> &deps, int type)
{
    QString rs;
    bool first = true;
    
    for (int i=0; i<deps.count(); ++i)
    {
        Depend *dep = deps.at(i);

        if (dep->type() != type)
        {
            continue;
        }

        if (!first)
        {
            rs += "; ";
        }
        
        first = false;
        
        rs += PackageSystem::dependString(dep->name(), dep->version(), dep->op());
    }

    return rs;
}

QString Package::name()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->name));
}

QString Package::version()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->version));
}

QString Package::newerVersion()
{
    if (d->upgradeIndex == -1)
    {
        return QString();
    }
    
    _Package *pkg = d->psd->package(d->upgradeIndex);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->version));
}

QString Package::maintainer()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->maintainer));
}

QString Package::shortDesc()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(true, pkg->short_desc));
}

QString Package::source()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->source));
}

QString Package::repo()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->repo));
}

QString Package::section()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->section));
}

QString Package::distribution()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->distribution));
}

QString Package::license()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->license));
}

QString Package::arch()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->arch));
}

QString Package::packageHash()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->pkg_hash));
}

QString Package::metadataHash()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return QString();
    
    return QString(d->psd->string(false, pkg->mtd_hash));
}

QString Package::url(UrlType type)
{
    // Construire la partie premières_lettres/nom
    QString n = name();
    QString d;
    
    if (n.startsWith("lib"))
    {
        d = n.left(4);
    }
    else
    {
        d = n.left(1);
    }
    
    // Compléter en fonction du type demandé
    if (type == Binary)
    {
        return "pool/" + d + "/" + n + "/" + n + "~" + version() + "." + arch() + ".tlz";
    }
    else
    {
        return "metadata/" + d + "/" + n + "/" + n + "~" + version() + ".metadata.xml.lzma";
    }
}

bool Package::isGui()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->is_gui;
}

int Package::downloadSize()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->dsize;
}

int Package::installSize()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->isize;
}


QDateTime Package::installedDate()
{
    QDateTime rs;
    _Package *pkg = d->psd->package(d->index);

    rs.setTime_t(pkg->idate);
    
    return rs;
}

int Package::installedBy()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->iby;
}

int Package::status()
{
    _Package *pkg = d->psd->package(d->index);
    
    if (pkg == 0) return false;
    
    return pkg->state;
}

void Package::setUpgradePackage(int i)
{
    d->upgradeIndex = i;
}

Package *Package::upgradePackage()
{
    if (d->upgradeIndex == -1)
    {
        return 0;
    }
    
    if (d->upd == 0)
    {
        d->upd = new Package(d->upgradeIndex, d->ps, d->psd, Solver::Install);
    }
    
    return d->upd;
}

/*************************************
******* Depend ***********************
*************************************/

Depend::Depend(_Depend *dep, DatabaseReader *psd)
{
    d = new Private;
    d->dep = dep;
    d->psd = psd;
}

Depend::~Depend()
{
    qDeleteAll(d->subdeps);
    delete d;
}

QString Depend::name()
{
    return d->psd->string(false, d->dep->pkgname);
}

QString Depend::version()
{
    return d->psd->string(false, d->dep->pkgver);
}

int8_t Depend::type()
{
    return d->dep->type;
}

int8_t Depend::op()
{
    return d->dep->op;
}