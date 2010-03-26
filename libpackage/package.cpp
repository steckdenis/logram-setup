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
#include "packagecommunication.h"
#include "processthread.h"

#include "databasepackage.h"

#include <QtDebug>

#include <QCoreApplication>
#include <QProcess>
#include <QSettings>
#include <QFile>

using namespace Logram;

/*************************************
******* Privates *********************
*************************************/

struct Package::Private
{
    int upgradeIndex;
    PackageSystem *ps;
    DatabaseReader *psd;

    Solver::Action action;
    bool wanted;

    // Installation
    ProcessThread *processThread;
    DatabasePackage *upd;
    
    // Métadonnées
    PackageMetaData *md;
};

/*************************************
******* File *************************
*************************************/

struct PackageFile::Private
{
    QString path;
    int flags;
    Package *pkg;
    bool bindedpackage;
};

PackageFile::PackageFile(const QString &path, int flags, Package *pkg, bool bindedpackage)
{
    d = new Private;
    d->path = path;
    d->flags = flags;
    d->pkg = pkg;
    d->bindedpackage = bindedpackage;
}

PackageFile::~PackageFile()
{
    if (d->bindedpackage)
    {
        // Package créé spécialement pour ce PackageFile, il faut le supprimer
        delete d->pkg;
    }
    
    delete d;
}

void PackageFile::setFlags(int flags)
{
    d->flags = flags;
}

void PackageFile::setPath(const QString &path)
{
    d->path = path;
}

QString PackageFile::path() const
{
    return d->path;
}

int PackageFile::flags() const
{
    return d->flags;
}

Package *PackageFile::package() const
{
    return d->pkg;
}

/*************************************
******* Package **********************
*************************************/

Package::Package(PackageSystem *ps, DatabaseReader *psd, Solver::Action _action) : QObject(ps)
{
    d = new Private;
    d->upgradeIndex = -1;
    d->ps = ps;
    d->psd = psd;
    d->action = _action;
    d->wanted = false;
    d->md = 0;
    d->processThread = 0;
    d->upd = 0;
}

Package::Package(const Package &other) : QObject(other.d->ps)
{
    d = new Private;
    
    d->upgradeIndex = other.d->upgradeIndex;
    d->ps = other.d->ps;
    d->psd = other.d->psd;
    d->action = other.d->action;
    d->wanted = other.d->wanted;
    d->md = 0;
    d->processThread = 0;
    d->upd = 0;
}

Package::~Package()
{
    if (d->processThread != 0)
    {
        delete d->processThread;
    }
    
    if (d->md != 0)
    {
        delete d->md;
    }
    
    delete d;
}

PackageMetaData *Package::metadata()
{
    if (d->md == 0)
    {
        // On doit récupérer les métadonnées
        d->md = new PackageMetaData(d->ps);
        
        d->md->bindPackage(this);
        
        if (d->md->error())
        {
            d->md = 0;
        }
    }
    
    return d->md;
}

void Package::setWanted(bool wanted)
{
    d->wanted = wanted;
}

bool Package::wanted() const
{
    return d->wanted;
}

void Package::process()
{
    d->processThread = new ProcessThread(d->ps, this);

    connect(d->processThread, SIGNAL(finished()), this, SLOT(processEnd()));

    d->processThread->start();
}

void Package::processLineOut(QProcess *process, const QByteArray &line)
{
    PackageMetaData *md = static_cast<PackageMetaData *>(sender());
    
    // Gérer les communications
    if (line.startsWith("[[>>|") && line.endsWith("|<<]]"))
    {
        QList<QByteArray> parts = line.split('|');
        
        // Trouver le nom de la communication, en enlevant d'abord le premier [[>>
        parts.removeAt(0);
        QString name = parts.takeAt(0);
        
        // Créer la communication
        Communication *comm = new PackageCommunication(d->ps, md, name);
        
        if (comm->error())
        {
            // Erreur survenue
            process->kill();     // Va émettre processEnded avec une erreur, dans packagemetadata
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
        process->write(comm->processData().toUtf8() + "\n");
    }
}

void Package::processEnd()
{
    int flgs = 0;
    
    if (d->processThread->error())
    {
        PackageError *err = new PackageError;
        err->type = PackageError::InstallError;
        err->info = name() + '~' + version();
        
        d->ps->setLastError(err);
        emit proceeded(false);
        return;
    }
    
    // Enregistrer le paquet dans la liste des paquets installés pour le prochain setup update
    QSettings *set = d->ps->installedPackagesList();
    
    if (action() == Solver::Install)
    {
        flgs = (flags() | PACKAGE_FLAG_INSTALLED | (wanted() ? PACKAGE_FLAG_WANTED : 0))
                      & ~PACKAGE_FLAG_REMOVED;
                      
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
        set->setValue("MetadataHash", metadataHash().constData());
        set->setValue("PackageHash", packageHash().constData());
        set->setValue("Flags", flgs); 

        set->setValue("ShortDesc", QString(shortDesc().toUtf8().toBase64()));
        
        set->setValue("InstalledDate", QDateTime::currentDateTime().toTime_t());
        set->setValue("InstalledRepo", repo());
        set->setValue("InstalledBy", QString(getenv("UID")).toInt());
        set->setValue("InstallRoot", d->ps->installRoot());
        set->endGroup();

        // Enregistrer les informations dans le paquet directement, puisqu'il est dans un fichier mappé
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      flgs);
    }
    else if (action() == Solver::Update)
    {
        Package *other = upgradePackage();
        flgs = (other->flags() | PACKAGE_FLAG_INSTALLED) & ~PACKAGE_FLAG_REMOVED;
        
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
        set->setValue("MetadataHash", other->metadataHash().constData());
        set->setValue("PackageHash", other->packageHash().constData());
        set->setValue("Flags", flgs);

        set->setValue("ShortDesc", QString(other->shortDesc().toUtf8().toBase64()));
        
        set->setValue("InstalledDate", QDateTime::currentDateTime().toTime_t());
        set->setValue("InstalledRepo", other->repo());
        set->setValue("InstalledBy", QString(getenv("UID")).toInt());
        set->setValue("InstallRoot", d->ps->installRoot());
        set->endGroup();

        // Enregistrer l'état du nouveau paquet
        other->registerState(QDateTime::currentDateTime().toTime_t(), 
                             QString(getenv("UID")).toInt(),
                             flgs);
        
        // Également pour l'ancien paquet
        flgs = (flags() | PACKAGE_FLAG_REMOVED) & ~PACKAGE_FLAG_INSTALLED;
        
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      flgs);
    }
    else if (action() == Solver::Remove)
    {
        // Enregistrer le paquet comme supprimé
        flgs = (flags() | PACKAGE_FLAG_REMOVED) & ~PACKAGE_FLAG_INSTALLED;
        
        set->beginGroup(name());
        set->setValue("Flags", flgs);
        set->endGroup();
        
        // Également dans la base de donnée binaire, avec l'heure et l'UID de celui qui a supprimé le paquet
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      flgs);
    }
    else if (action() == Solver::Purge)
    {
        // Effacer toute trace du paquet
        set->remove(name());
        
        registerState(0, 
                      0,
                      flags() & ~(PACKAGE_FLAG_INSTALLED | PACKAGE_FLAG_REMOVED));
    }

    // Supprimer le thread
    d->processThread->deleteLater();
    d->processThread = 0;
    
    // L'installation est finie
    emit proceeded(true);
}

Solver::Action Package::action()
{
    return d->action;
}

void Package::setAction(Solver::Action act)
{
    d->action = act;
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

void Package::setUpgradePackage(int i)
{
    d->upgradeIndex = i;
}

DatabasePackage *Package::upgradePackage()
{
    if (d->upgradeIndex == -1)
    {
        return 0;
    }
    
    if (d->upd == 0)
    {
        d->upd = new DatabasePackage(d->upgradeIndex, d->ps, d->psd, Solver::Install);
    }
    
    return d->upd;
}

/*************************************
******* Depend ***********************
*************************************/

Depend::Depend()
{
    // rien
}