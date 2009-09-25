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

PackageSystem::PackageSystem(QObject *parent) : QObject(parent)
{
    // Rien à faire
}

void PackageSystem::init()
{
    d = new PackageSystemPrivate(this);
}

struct Enrg
{
    QString url, distroName, arch, sourceName, type;
};

void PackageSystem::update()
{
    // Explorer la liste des mirroirs dans /etc/setup/sources.list, format QSettings
    QSettings set("/etc/setup/sources.list", QSettings::IniFormat);
    QString lang = set.value("Language", tr("fr", "Langue par défaut pour les paquets")).toString();
    
    DatabaseWriter *db = new DatabaseWriter(this);

    QList<Enrg *> enrgs;

    foreach (const QString &sourceName, set.childGroups())
    {
        set.beginGroup(sourceName);

        QString type = set.value("Type", "remote").toString();
        QString url = set.value("Url").toString();
        QStringList distros = set.value("Distributions").toString().split(' ', QString::SkipEmptyParts);
        QStringList archs = set.value("Archs").toString().split(' ', QString::SkipEmptyParts);

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

        set.endGroup();
    }

    // Explorer les enregistrements et les télécharger
    int count = enrgs.count();
    for (int i=0; i<count; ++i)
    {
        Enrg *enrg = enrgs.at(i);

        QString u = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch + "/packages.lzma";
        
        sendProgress((i*2)+1, count*2, tr("Téléchargement de %1...").arg(u));
        db->download(enrg->sourceName, u, enrg->type, false);

        // Traductions
        u = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch + "/translate." + lang + ".lzma";
        
        sendProgress((i*2)+2, count*2, tr("Téléchargement des traductions %1...").arg(u));
        db->download(enrg->sourceName, u, enrg->type, true);
    }

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

Solver *PackageSystem::newSolver()
{
    return new Solver(this, d);
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

void PackageSystem::raise(Error err, const QString &info)
{
    emit error(err, info);
}

void PackageSystem::sendProgress(int num, int tot, const QString &msg)
{
    emit progress(num, tot, msg);
}