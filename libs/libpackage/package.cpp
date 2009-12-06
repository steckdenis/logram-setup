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

#include "databasepackage.h"

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
    int upgradeIndex;
    PackageSystem *ps;
    DatabaseReader *psd;

    Solver::Action action;

    // Installation
    QProcess *installProcess;
    QString installCommand;
    QString readBuf;
    DatabasePackage *upd;
    
    // Métadonnées
    PackageMetaData *md;
};

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
    d->md = 0;
    d->installProcess = 0;
    d->upd = 0;
}

Package::Package(const Package &other) : QObject(other.d->ps)
{
    d = new Private;
    
    d->upgradeIndex = other.d->upgradeIndex;
    d->ps = other.d->ps;
    d->psd = other.d->psd;
    d->action = other.d->action;
    d->md = 0;
    d->installProcess = 0;
    d->upd = 0;
}

Package::~Package()
{
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
            .arg(tlzFileName())
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
            .arg(upgradePackage()->version())
            .arg(tlzFileName())
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
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      PACKAGE_STATE_INSTALLED);
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
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      PACKAGE_STATE_NOTINSTALLED);
        
        // Également pour l'autre paquet
        other->registerState(QDateTime::currentDateTime().toTime_t(), 
                             QString(getenv("UID")).toInt(),
                             PACKAGE_STATE_INSTALLED);
    }
    else if (action() == Solver::Remove)
    {
        // Enregistrer le paquet comme supprimé
        set->beginGroup(name());
        set->setValue("State", PACKAGE_STATE_REMOVED);
        set->endGroup();
        
        // Également dans la base de donnée binaire, avec l'heure et l'UID de celui qui a supprimé le paquet
        registerState(QDateTime::currentDateTime().toTime_t(), 
                      QString(getenv("UID")).toInt(),
                      PACKAGE_STATE_REMOVED);
    }
    else if (action() == Solver::Purge)
    {
        // Effacer toute trace du paquet
        set->remove(name());
        
        registerState(0, 
                      0,
                      PACKAGE_STATE_NOTINSTALLED);
    }

    // Supprimer le processus
    d->installProcess->deleteLater();
    d->installProcess = 0;
    
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