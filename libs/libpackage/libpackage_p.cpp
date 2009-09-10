/*
 * libpackage_p.cpp
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

#include <QFile>
#include <QRegExp>

PackageSystemPrivate::PackageSystemPrivate(PackageSystem *_ps)
{
    ps = _ps;
    
    // Ouvrir les fichiers
    mapFile("packages", &f_packages, &m_packages);
    mapFile("strings", &f_strings, &m_strings);
    mapFile("translate", &f_translate, &m_translate);
    mapFile("depends", &f_depends, &m_depends);
    mapFile("strpackages", &f_strpackages, &m_strpackages);
}

QList<int> PackageSystemPrivate::packagesByName(const QString &regex)
{
    // Explorer le contenu de packages à la recherche d'un paquet dont le nom est bon
    QList<int> rs;
    QString pkgname;
    
    QRegExp exp(regex, Qt::CaseSensitive, QRegExp::Wildcard);
    int32_t count = *(int32_t *)m_packages;     // Nombre de paquets

    // Explorer les paquets
    for (int i=0; i<count; ++i)
    {
        pkgname = packageName(i);

        // Voir si ça correspond à la regex
        if (exp.exactMatch(pkgname))
        {
            // On ajoute le paquet comme résultat
            rs.append(i);
        }
    }

    return rs;
}

int PackageSystemPrivate::package(const QString &name, const QString &version)
{
    QString pkgname;
    int32_t count = *(int32_t *)m_packages;

    for (int i=0; i<count; ++i)
    {
        pkgname = packageName(i);

        if (pkgname == name)
        {
            // Vérifier aussi la version
            if (version.isNull())
            {
                return i;
            }
            else if (version == packageVersion(i))
            {
                return i;
            }
        }
    }

    return -1;
}

QList<_Depend *> PackageSystemPrivate::depends(int pkgIndex)
{
    _Package *pkg = package(pkgIndex);
    QList<_Depend *> rs;

    if (pkg == 0)
    {
        return rs;
    }
    else if (pkg->deps == -1)
    {
        // NOTE: 0xFFFFFFFF dans le fichier
        return rs;
    }

    // Obtenir le pointeur sur la première dépendance
    uchar *depptr = m_depends;
    depptr += 4;                                // Sauter count
    depptr += pkg->deps * sizeof(_DependPtr);   // Bon index

    // Explorer les dépendances
    int numdeps = ((_DependPtr *)depptr)->count;

    _Depend *dep = depend(((_DependPtr *)depptr)->ptr);

    for (int i=0; i<numdeps; ++i)
    {
        rs.append(dep);

        dep++;          // Les dépendances sont en boucle, l'addition a la granularité de la taille du pointeur
    }

    return rs;
}

_Depend *PackageSystemPrivate::depend(int ptr)
{
    int numdepsptr = *(int *)m_depends;
    
    uchar *dep = m_depends;
    dep += 4;                   // Sauter count
    dep += numdepsptr * sizeof(_DependPtr);
    dep += ptr;

    return (_Depend *)dep;
}

QString PackageSystemPrivate::packageName(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->name));
}

QString PackageSystemPrivate::packageVersion(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->version));
}

QString PackageSystemPrivate::packageLongDesc(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_translate, pkg->long_desc));
}

QString PackageSystemPrivate::packageShortDesc(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_translate, pkg->short_desc));
}

_Package *PackageSystemPrivate::package(int index)
{
    // Trouver l'adresse du paquet
    if (index >= *(int *)m_packages)
    {
        return 0;
    }

    // Début de la liste des paquets
    uchar *pkg = m_packages;
    pkg += 4;
    
    // Paquet au bon index
    pkg += (index * sizeof(_Package));

    return (_Package *)pkg;
}

const char *PackageSystemPrivate::string(uchar *map, int index)
{
    // Si map == 0, on prend m_strings (c'est qu'on a été appelé d'ailleurs)
    if (map == 0)
    {
        map = m_strings;
    }
    
    // Vérifier l'index
    if (index >= *(int *)map)
    {
        return 0;
    }

    // Trouver la chaîne à l'index spécifié
    uchar *str = map;
    int count = *(int *)map;    // count
    
    str += 4;                   // Sauter count

    // Index
    str += (index * sizeof(_String));

    // En fonction du pointeur, trouver l'adresse de la chaîne
    const char *ptr = (const char *)map;
    ptr += ((_String *)(str))->ptr;                    // Ajouter l'adresse du pointeur
    ptr += 4;                           // Sauter le count
    ptr += (count * sizeof(_String));   // Sauter la table des chaînes

    return ptr;
}

void PackageSystemPrivate::mapFile(const QString &file, QFile **ptr, uchar **map)
{
    *ptr = new QFile("/var/cache/lgrpkg/" + file);
    QFile *f = *ptr;

    if (!f->open(QIODevice::ReadWrite))
    {
        ps->raise(PackageSystem::OpenFileError, f->fileName());
        return;
    }

    *map = f->map(0, f->size());

    if (*map == 0)
    {
        ps->raise(PackageSystem::MapFileError, f->fileName());
        return;
    }
}