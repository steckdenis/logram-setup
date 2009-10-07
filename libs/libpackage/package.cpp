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
#include "libpackage.h"
#include "libpackage_p.h"

#include <QtDebug>

#include <QCoreApplication>
#include <QProcess>
#include <QSettings>

/*************************************
******* Privates *********************
*************************************/

struct Package::Private
{
    int index;
    PackageSystem *ps;
    PackageSystemPrivate *psd;

    bool depok;
    QList<Depend *> deps;

    Solver::Action action;

    // Téléchargement
    QString waitingDest;

    // Installation
    QProcess *installProcess;
    QString installCommand;
};

struct Depend::Private
{
    _Depend *dep;
    PackageSystemPrivate *psd;

    QList<Depend *> subdeps;
};

/*************************************
******* Package **********************
*************************************/

Package::Package(int index, PackageSystem *ps, PackageSystemPrivate *psd, Solver::Action _action) : QObject(ps)
{
    d = new Private;
    d->index = index;
    d->ps = ps;
    d->psd = psd;
    d->depok = false;
    d->action = _action;

    connect(ps, SIGNAL(downloadEnded(ManagedDownload *)), this, SLOT(downloadEnded(ManagedDownload *)));
}

Package::~Package()
{
    qDeleteAll(d->deps);
    delete d;
}

void Package::install()
{
    d->installProcess = new QProcess();

    connect(d->installProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processEnd(int, QProcess::ExitStatus)));
    connect(d->installProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOut()));

    d->installCommand = QString("/usr/bin/helperscript install %1 %2 %3 %4")
        .arg(name())
        .arg(version())
        .arg(d->waitingDest)
        .arg(d->ps->installRoot());
        
    d->installProcess->start(d->installCommand);
}

void Package::processOut()
{
    QString out = d->installProcess->readAll().trimmed();

    // Parser la sortie de type TYPE#paramètres#...
    QStringList parts = out.split('#');
    QString command = parts.takeAt(0);

    if (command == "MESSAGE")
    {
        d->ps->sendMessage(this, parts.join("#"));
    }
    else
    {
        // Message en texte brut
        d->ps->sendMessage(this, out);
    }
}

void Package::processEnd(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 && exitStatus != QProcess::NormalExit)
    {
        d->ps->raise(PackageSystem::ProcessError, d->installCommand);
    }
    
    // Enregistrer le paquet dans la liste des paquets installés pour le prochain setup update
    QSettings *set = d->ps->installedPackagesList();
    
    set->beginGroup(name());
    set->setValue("Url", url());
    set->setValue("Name", name());
    set->setValue("Version", version());
    set->setValue("Source", source());
    set->setValue("Maintainer", maintainer());
    set->setValue("Section", section());
    set->setValue("Distribution", distribution());
    set->setValue("Licence", license());
    set->setValue("Depends", dependsToString(depends(), DEPEND_TYPE_DEPEND));
    set->setValue("Provides", dependsToString(depends(), DEPEND_TYPE_PROVIDE));
    set->setValue("Suggest", dependsToString(depends(), DEPEND_TYPE_SUGGEST));
    set->setValue("Replaces", dependsToString(depends(), DEPEND_TYPE_REPLACE));
    set->setValue("Conflicts", dependsToString(depends(), DEPEND_TYPE_CONFLICT));
    set->setValue("DownloadSize", downloadSize());
    set->setValue("InstallSize", installSize());

    set->setValue("Title", QString(title().toUtf8().toBase64()));
    set->setValue("ShortDesc", QString(shortDesc().toUtf8().toBase64()));
    set->setValue("LongDesc", QString(longDesc().toUtf8().toBase64()));
    
    set->setValue("InstalledDate", QDateTime::currentDateTime().toTime_t());
    set->setValue("InstalledRepo", repo());
    set->setValue("InstalledBy", QString(getenv("UID")).toInt());
    set->setValue("State", PACKAGE_STATE_INSTALLED);
    set->endGroup();

    // Enregistrer les informations dans le paquet directement, puisqu'il est dans un fichier mappé

    // L'installation est finie, le dire, même si on a eu une erreur (pas rester coincé dans le QEventLoop)
    emit installed();
}

void Package::download()
{
    // Télécharger le paquet
    QString fname = "/var/cache/lgrpkg/download/" + url().section('/', -1, -1);
    QString r = repo();
    QString type = d->ps->repoType(r);
    QString u = d->ps->repoUrl(r) + "/" + url();

    d->waitingDest = fname;

    d->ps->download(type, u, fname, false); // Non-bloquant
    
    //emit downloaded();
}

void Package::downloadEnded(ManagedDownload *md)
{
    if (md != 0)
    {
        if (md->destination == d->waitingDest)
        {
            // On a téléchargé le paquet !
            emit downloaded();
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
    
    for (int i=0; i<deps.count(); ++i)
    {
        Depend *dep = deps.at(i);

        if (dep->type() != type)
        {
            continue;
        }

        if (i != 0)
        {
            rs += "; ";
        }

        rs += dep->name();

        if (dep->op() != DEPEND_OP_NOVERSION)
        {
            switch (dep->op())
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

            rs += dep->version() + ")";
        }
    }

    return rs;
}

QString Package::name()
{
    return d->psd->packageName(d->index);
}

QString Package::version()
{
    return d->psd->packageVersion(d->index);
}

QString Package::maintainer()
{
    return d->psd->packageMaintainer(d->index);
}

QString Package::shortDesc()
{
    return d->psd->packageShortDesc(d->index);
}

QString Package::longDesc()
{
    return d->psd->packageLongDesc(d->index);
}

QString Package::title()
{
    return d->psd->packageTitle(d->index);
}

QString Package::source()
{
    return d->psd->packageSource(d->index);
}

QString Package::repo()
{
    return d->psd->packageRepo(d->index);
}

QString Package::section()
{
    return d->psd->packageSection(d->index);
}

QString Package::distribution()
{
    return d->psd->packageDistribution(d->index);
}

QString Package::license()
{
    return d->psd->packageLicense(d->index);
}

QString Package::url()
{
    return d->psd->packageUrl(d->index);
}

int Package::downloadSize()
{
    return d->psd->packageDownloadSize(d->index);
}

int Package::installSize()
{
    return d->psd->packageInstallSize(d->index);
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
    return d->psd->package(d->index)->iby;
}

int Package::status()
{
    return d->psd->package(d->index)->state;
}

/*************************************
******* Depend ***********************
*************************************/

Depend::Depend(_Depend *dep, PackageSystemPrivate *psd)
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
    return d->psd->string(0, d->dep->pkgname);
}

QString Depend::version()
{
    return d->psd->string(0, d->dep->pkgver);
}

int8_t Depend::type()
{
    return d->dep->type;
}

int8_t Depend::op()
{
    return d->dep->op;
}