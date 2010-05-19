/*
 * packagelist.cpp
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

#include "packagelist.h"
#include "packagesystem.h"
#include "package.h"
#include "databasepackage.h"
#include "databasereader.h"
#include "packagemetadata.h"
#include "templatable.h"

#include <QtScript>
#include <QList>
#include <QEventLoop>
#include <QFile>
#include <QSettings>

using namespace Logram;

struct PackageList::Private
{
    PackageSystem *ps;
    bool needsReboot;
    int numLicenses;
    
    int downloadProgress, processProgress;
    
    int parallelInstalls, parallelDownloads;
    
    // Pour l'installation
    QEventLoop loop;
    int ipackages, dpackages, pipackages;
    bool idone;
    Package *installingPackage;
    QList<Package *> list;
    QList<Package *> downloadedPackages;
    QList<int> orphans;
    QList<QString> triggers;
};

Q_DECLARE_METATYPE(Package *)

PackageList::PackageList(PackageSystem *ps) : QObject(ps), QList<Package *>()
{
    d = new Private;
    
    d->needsReboot = false;
    d->numLicenses = 0;
    d->ps = ps;
    
    d->parallelInstalls = ps->parallelInstalls();
    d->parallelDownloads = ps->parallelDownloads();
}

PackageList::~PackageList()
{   
    for (int i=0; i<count(); ++i)
    {
        if (at(i)->origin() != Package::File)
        {
            delete at(i);
        }
    }
    
    delete d;
}

bool PackageList::needsReboot() const
{
    return d->needsReboot;
}

int PackageList::numLicenses() const
{
    return d->numLicenses;
}

QList<int> PackageList::orphans() const
{
    return d->orphans;
}
        
void PackageList::addPackage(Package *pkg)
{
    append(pkg);
    
    // Ce paquet nécessite-t-il un redémarrage ?
    if (pkg->flags() & PACKAGE_FLAG_NEEDSREBOOT)
    {
        d->needsReboot = true;
    }
    
    // Ce paquet nécessite-t-il l'approbation d'une license ?
    if (pkg->flags() & PACKAGE_FLAG_EULA && pkg->action() == Solver::Install)
    {
        d->numLicenses++;
    }
}

Package *PackageList::installingPackage() const
{
    return d->installingPackage;
}
        
bool PackageList::process()
{
    // Installer les paquets d'une liste, donc explorer ses paquets
    d->ipackages = 0;
    d->pipackages = 0;

    int maxdownloads = qMin(d->parallelDownloads, count());
    d->dpackages = maxdownloads;
    
    d->downloadProgress = d->ps->startProgress(Progress::GlobalDownload, count());
    d->processProgress = d->ps->startProgress(Progress::PackageProcess, count());
    
    for(int i=0; i<maxdownloads; ++i)
    {
        Package *pkg = at(i);

        connect(pkg, SIGNAL(proceeded(bool)), this, SLOT(packageProceeded(bool)));
        connect(pkg, SIGNAL(downloaded(bool)), this, SLOT(packageDownloaded(bool)), Qt::QueuedConnection);
        connect(pkg, SIGNAL(communication(Logram::Package *, Logram::Communication *)), d->ps,
                     SIGNAL(communication(Logram::Package *, Logram::Communication *)));

        // Progression
        if (!d->ps->sendProgress(d->downloadProgress, i, pkg->name()))
        {
            return false;
        }

        // Téléchargement
        if (!pkg->download())
        {
            return false;
        }
    }

    // Attendre
    int rs = d->loop.exec();
    
    if (rs != 0) return false;
    
    DatabaseReader *dr = d->ps->databaseReader();
    QList<int> pkgs;
    QSettings *set = d->ps->installedPackagesList();
    
    for (int i=0; i<count(); ++i)
    {
        Package *pkg = at(i);
        
        // Enregistrer les triggers du paquet
        PackageMetaData *md = pkg->metadata();
        
        if (md == 0)
        {
            return false;
        }
        
        md->setCurrentPackage(pkg->name());
        
        foreach (const QString &trigger, md->triggers())
        {
            if (!d->triggers.contains(trigger))
            {
                d->triggers.append(trigger);
            }
        }
    
        // Enregistrer le taux d'utilisation de chaque paquet
        if (pkg->origin() == Package::Database)
        {
            // Paquet en base de donnée, explorer ses dépendances et incrémenter leur compteur
            DatabasePackage *dpkg = static_cast<DatabasePackage *>(pkg);
            
            QList<_Depend *> deps = dr->depends(dpkg->index());
            
            foreach (_Depend *dep, deps)
            {
                if (dep->type != DEPEND_TYPE_DEPEND)
                {
                    // On n'incrémente que les dépendances
                    continue;
                }
                
                // Paquets qui correspondent
                pkgs = dr->packagesOfString(dep->pkgver, dep->pkgname, dep->op);
                
                foreach (int p, pkgs)
                {
                    _Package *pp = dr->package(p);
                    
                    if (pp->flags & PACKAGE_FLAG_INSTALLED)
                    {
                        if (pkg->action() == Solver::Install)
                        {
                            // On installe un paquet, ses dépendances recoivent un jeton
                            pp->used++;
                            
                            // Enregistrer également l'information dans le fichier installed_packages
                            set->beginGroup(dr->string(false, pp->name));
                            set->setValue("Used", set->value("Used").toInt() + 1);
                            set->endGroup();
                        }
                        else if (pkg->action() == Solver::Remove || pkg->action() == Solver::Purge)
                        {
                            // On supprime un paquet, ses dépendances perdent un jeton
                            pp->used--;
                            
                            set->beginGroup(dr->string(false, pp->name));
                            set->setValue("Used", set->value("Used").toInt() - 1);
                            set->endGroup();
                            
                            // Si le paquet n'est plus utilisé par personne, le déclarer comme orphelin
                            if (pp->used == 0 && (pp->flags & PACKAGE_FLAG_WANTED) == 0)
                            {
                                d->orphans.append(p);
                            }
                        }
                    }
                }
            }
        }
        // TODO : Même chose pour les paquets fichiers, sauf qu'on n'a pas un traitement O(1) dans la BDD pour
        //        leurs dépendances
    }

    // Nettoyage
    disconnect(this, SLOT(packageProceeded(bool)));
    disconnect(this, SLOT(packageDownloaded(bool)));
    
    d->downloadedPackages.clear();
    
    d->ps->endProgress(d->downloadProgress);
    d->ps->endProgress(d->processProgress);
    
    // Lancer les triggers
    if (d->ps->runTriggers())
    {
        int trigProgress = d->ps->startProgress(Progress::Trigger, d->triggers.count());
        Templatable *tpl = new Templatable(this);
        
        // Remplir la template
        tpl->addKey("instroot", d->ps->installRoot());
        tpl->addKey("varroot", d->ps->varRoot());
        tpl->addKey("confroot", d->ps->confRoot());
        
        for (int i=0; i<d->triggers.count(); ++i)
        {
            const QString &trigger = d->triggers.at(i);
            
            // Progression
            if (!d->ps->sendProgress(trigProgress, i, tpl->templateString(trigger), QString()))
            {
                d->ps->endProgress(trigProgress);
                delete tpl;
                return false;
            }
            
            QProcess *proc = new QProcess;
            
            connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)),
                    this,   SLOT(triggerFinished(int, QProcess::ExitStatus)));
                    
            connect(proc, SIGNAL(readyReadStandardOutput()),
                    this,   SLOT(triggerOut()));
                    
            proc->setProcessChannelMode(QProcess::MergedChannels);
            
            // Exécuter le processus
            proc->setProperty("lgr_trigger", tpl->templateString(trigger));  // Qt <3
            QString exec = d->ps->installRoot() + "/usr/share/triggers/" + tpl->templateString(trigger);
            proc->start(exec, QIODevice::ReadOnly);
            
            rs = d->loop.exec();
            
            if (rs != 0) return false;
        }
        
        delete tpl;
        d->ps->endProgress(trigProgress);
    }
    
    return true;
}

void PackageList::triggerFinished(int exitCode, QProcess::ExitStatus exitStatus)
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
        err->info = sh->property("lgr_trigger").toString();
        
        d->ps->setLastError(err);
        
        rs = 1;
    }
    
    // Quitter la boucle
    d->loop.exit(rs);
}

void PackageList::triggerOut()
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
        
        // Envoyer la progression
        if (!d->ps->processOut(sh->property("lgr_trigger").toString(), line))
        {
            // Kill the process
            sh->terminate();
        }
    }
}

void PackageList::packageProceeded(bool success)
{
    if (!success)
    {
        // Un paquet a planté, arrêter
        d->loop.exit(1);
        return;
    }
    
    // Voir si on a un paquet suivant
    if (d->downloadedPackages.count() != 0)
    {
        Package *next = d->downloadedPackages.takeAt(0);

        // Progression
        if (!d->ps->sendProgress(d->processProgress, d->ipackages, next->name() + "~" + next->version(), QString(), next))
        {
            d->loop.exit(1);
            return;
        }

        // Installation
        d->ipackages++;
        d->installingPackage = next;

        next->process();
    }
    else if (d->ipackages < count())
    {
        // On n'a pas tout installé, mais les téléchargements ne suivent pas, arrêter cette branche et dire à
        // packageDownloaded qu'on l'attend
        d->pipackages--;
        return;
    }
    else if (d->ipackages < (count() + (d->pipackages - 1)))
    {
        // Laisser le dernier se finir
        d->ipackages++;
    }
    else
    {
        // On a tout installé et fini
        d->loop.exit(0);
    }
}

void PackageList::packageDownloaded(bool success)
{
    Package *pkg = qobject_cast<Package *>(sender());

    if (!success || !pkg)
    {
        // Un paquet a planté, arrêter
        d->loop.exit(1);
        return;
    }

    // Installer le paquet si le nombre de paquets installés en parallèle n'est pas dépassé
    if (d->pipackages < d->parallelInstalls)
    {
        // Progression
        if (!d->ps->sendProgress(d->processProgress, d->ipackages, pkg->name() + "~" + pkg->version(), QString(), pkg))
        {
            d->loop.exit(1);
            return;
        }
        
        // Installer
        d->ipackages++;
        d->pipackages++;
        d->installingPackage = pkg;
        pkg->process();
    }
    else
    {
        // Placer ce paquet en attente d'installation
        d->downloadedPackages.append(pkg);
    }

    // Si disponible, télécharger un autre paquet
    if (d->dpackages < count())
    {
        Package *next = at(d->dpackages);

        // Connexion de signaux
        connect(next, SIGNAL(proceeded(bool)), this, SLOT(packageProceeded(bool)));
        connect(next, SIGNAL(downloaded(bool)), this, SLOT(packageDownloaded(bool)), Qt::QueuedConnection);
        connect(next, SIGNAL(communication(Logram::Package *, Logram::Communication *)), this, 
                      SLOT(communication(Logram::Package *, Logram::Communication *)));
        
        // Progression
        if (!d->ps->sendProgress(d->downloadProgress, d->dpackages, next->name()))
        {
            d->loop.exit(1);
            return;
        }

        // Téléchargement
        d->dpackages++;
        
        if (!next->download())
        {
            d->loop.exit(1);
            return;
        }
    }
}

void PackageList::communication(Logram::Package *pkg, Logram::Communication *comm)
{
    d->ps->sendCommunication(pkg, comm);
}

#include "packagelist.moc"