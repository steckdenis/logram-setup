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
#include "package.h"

#include <QFile>
#include <QRegExp>
#include <QDebug>

PackageSystemPrivate::PackageSystemPrivate(PackageSystem *_ps)
{
    ps = _ps;
    
    f_packages = 0;
    f_strings = 0;
    f_translate = 0;
    f_depends = 0;
    f_strpackages = 0;
}

bool PackageSystemPrivate::init()
{
    // Ouvrir les fichiers
    if (!mapFile("packages", &f_packages, &m_packages)) return false;
    if (!mapFile("strings", &f_strings, &m_strings)) return false;
    if (!mapFile("translate", &f_translate, &m_translate)) return false;
    if (!mapFile("depends", &f_depends, &m_depends)) return false;
    if (!mapFile("strpackages", &f_strpackages, &m_strpackages)) return false;
    
    return true;
}

PackageSystemPrivate::~PackageSystemPrivate()
{
    if (f_packages != 0)
    {
        f_packages->close();
        f_packages->unmap(m_packages);
        delete f_packages;
        f_packages = 0;
    }
    if (f_strings != 0)
    {
        f_strings->close();
        f_strings->unmap(m_strings);
        delete f_strings;
        f_strings = 0;
    }
    if (f_translate != 0)
    {
        f_translate->close();
        f_translate->unmap(m_translate);
        delete f_translate;
        f_translate = 0;
    }
    if (f_depends != 0)
    {
        f_depends->close();
        f_depends->unmap(m_depends);
        delete f_depends;
        f_depends = 0;
    }
    if (f_strpackages != 0)
    {
        f_strpackages->close();
        f_strpackages->unmap(m_strpackages);
        delete f_strpackages;
        f_strpackages = 0;
    }
}

bool PackageSystemPrivate::packagesByName(const QString &regex, QList<int> &rs)
{
    // Explorer le contenu de packages à la recherche d'un paquet dont le nom est bon
    rs = QList<int>();
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

    return true;
}

QList<int> PackageSystemPrivate::packagesByVString(const QString &verStr)
{
    QList<int> rs;
    int32_t count = *(int32_t *)m_packages;

    // Parser la version
    QString name, version, pver, pname;
    int op;

    op = PackageSystem::parseVersion(verStr, name, version);

    for (int i=0; i<count; ++i)
    {
        _Package *pkg = package(i);
        pname = QString(string(0, pkg->name));

        if (pname != name) continue;

        if (version.isNull())
        {
            rs.append(i);
        }
        else
        {
            pver = QString(string(0, pkg->version));

            // Voir si la version correspond
            if (PackageSystem::matchVersion(pver.toUtf8(), version.toUtf8(), op))
            {
                rs.append(i);
            }
        }
    }

    return rs;
}

bool PackageSystemPrivate::package(const QString &name, const QString &version, int &rs)
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
                rs = i;
                return true;
            }
            else if (version == packageVersion(i))
            {
                return true;
                rs = i;
            }
        }
    }

    PackageError *err = new PackageError;
    err->type = PackageError::PackageNotFound;
    
    if (version.isNull())
    {
        err->info = name;
    }
    else
    {
        err->info = name + "~" + version;
    }
    
    ps->setLastError(err);
    return false;
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

QList<int> PackageSystemPrivate::packagesOfString(int stringIndex, int nameIndex, int op)
{
    QList<int> rs;
    QString cmpVersion = QString(string(0, stringIndex));
    
    // Vérifier l'index
    if (stringIndex >= *(int *)m_strings)
    {
        return rs;
    }

    // Trouver la chaîne à l'index spécifié
    uchar *str = m_strings;

    str += 4;                   // Sauter count

    // Index
    str += (nameIndex * sizeof(_String));

    // Trouver le StringPackagePtr
    int32_t spptr = ((_String *)(str))->strpkg;
    uchar *strpkg = m_strpackages;
    int32_t numptrs = *(int32_t *)strpkg;

    strpkg += 4;
    strpkg += spptr * sizeof(_StrPackagePtr);

    // Explorer les StrPackage
    int32_t count = ((_StrPackagePtr *)(strpkg))->count;
    uchar *sptr = m_strpackages;

    sptr += 4;      // Count
    sptr += numptrs * sizeof(_StrPackagePtr);
    sptr += ((_StrPackagePtr *)(strpkg))->ptr;

    for (int i=0; i<count; ++i)
    {
        // Si on n'a pas précisé de version, c'est ok
        if (op == DEPEND_OP_NOVERSION)
        {
            rs.append( ((_StrPackage *)(sptr))->package );
        }
        else if (PackageSystem::matchVersion(QByteArray(string(0, ((_StrPackage *)(sptr))->version)), cmpVersion.toUtf8(), op))
        {
            rs.append( ((_StrPackage *)(sptr))->package );
        }

        sptr += sizeof(_StrPackage);
    }

    return rs;
}

_Depend *PackageSystemPrivate::depend(int32_t ptr)
{
    int numdepsptr = *(int *)m_depends;
    
    uchar *dep = m_depends;
    dep += 4;                   // Sauter count
    dep += numdepsptr * sizeof(_DependPtr);
    dep += ptr;

    return (_Depend *)dep;
}

int PackageSystemPrivate::packageDownloadSize(int index)
{
    _Package *pkg = package(index);
    
    if (pkg == 0) return 0;
    
    return pkg->dsize;
}

int PackageSystemPrivate::packageInstallSize(int index)
{
    _Package *pkg = package(index);
    
    if (pkg == 0) return 0;
    
    return pkg->isize;
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

QString PackageSystemPrivate::packageMaintainer(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->maintainer));
}

QString PackageSystemPrivate::packageShortDesc(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_translate, pkg->short_desc));
}

QString PackageSystemPrivate::packageSource(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->source));
}

QString PackageSystemPrivate::packageRepo(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->repo));
}

QString PackageSystemPrivate::packageSection(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->section));
}

QString PackageSystemPrivate::packageDistribution(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->distribution));
}

QString PackageSystemPrivate::packageLicense(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->license));
}

QString PackageSystemPrivate::packageArch(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->arch));
}

QString PackageSystemPrivate::packagePkgHash(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->pkg_hash));
}

QString PackageSystemPrivate::packageMtdHash(int index)
{
    _Package *pkg = package(index);

    if (pkg == 0) return QString();

    return QString(string(m_strings, pkg->mtd_hash));
}

bool PackageSystemPrivate::packageGui(int index)
{
    _Package *pkg = package(index);
    
    if (pkg == 0) return false;
    
    return pkg->is_gui;
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

bool PackageSystemPrivate::mapFile(const QString &file, QFile **ptr, uchar **map)
{
    *ptr = new QFile(ps->varRoot() + "/var/cache/lgrpkg/db/" + file);
    QFile *f = *ptr;

    if (!f->open(QIODevice::ReadWrite))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = f->fileName();
        
        ps->setLastError(err);
        
        return false;
    }

    *map = f->map(0, f->size());

    if (*map == 0)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::MapFileError;
        err->info = f->fileName();
        
        ps->setLastError(err);
        
        return false;
    }
    
    return true;
}