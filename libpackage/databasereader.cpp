/*
 * databasereader.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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
#include "package.h"
#include "databasepackage.h"

#include <QFile>
#include <QRegExp>
#include <QDebug>

using namespace Logram;

DatabaseReader::DatabaseReader(PackageSystem *_ps)
{
    ps = _ps;
    _initialized = false;
    
    f_packages = 0;
    f_strings = 0;
    f_translate = 0;
    f_depends = 0;
    f_strpackages = 0;
    f_files = 0;
}

bool DatabaseReader::initialized() const
{
    return _initialized;
}

bool DatabaseReader::init()
{
    // Ouvrir les fichiers
    if (!mapFile("packages", &f_packages, &m_packages)) return false;
    if (!mapFile("strings", &f_strings, &m_strings)) return false;
    if (!mapFile("translate", &f_translate, &m_translate)) return false;
    if (!mapFile("depends", &f_depends, &m_depends)) return false;
    if (!mapFile("strpackages", &f_strpackages, &m_strpackages)) return false;
    if (!mapFile("files", &f_files, &m_files)) return false;
    
    _initialized = true;
    
    return true;
}

DatabaseReader::~DatabaseReader()
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
    if (f_files != 0)
    {
        f_files->close();
        f_files->unmap(m_files);
        delete f_files;
        f_files = 0;
    }
}

bool DatabaseReader::packagesByName(const QRegExp &regex, QVector<int> &rs)
{
    // Explorer le contenu de packages à la recherche d'un paquet dont le nom est bon
    rs = QVector<int>();
    QString pkgname;

    int32_t count = *(int32_t *)m_packages;     // Nombre de paquets

    // Explorer les paquets
    for (int i=0; i<count; ++i)
    {
        pkgname = QString(string(false, package(i)->name));
        
        // Voir si ça correspond à la regex
        if (regex.exactMatch(pkgname))
        {
            // On ajoute le paquet comme résultat
            rs.append(i);
        }
    }

    return true;
}

QVector<int> DatabaseReader::packagesByVString(const QString &name, const QString &version, int op)
{
    QVector<int> rs;
    int32_t count = *(int32_t *)m_packages;
    
    QString pname, pver;
    
    for (int i=0; i<count; ++i)
    {
        _Package *pkg = package(i);
        pname = QString(string(false, pkg->name));

        if (pname != name) continue;

        if (version.isNull())
        {
            rs.append(i);
        }
        else
        {
            pver = QString(string(false, pkg->version));

            // Voir si la version correspond
            if (PackageSystem::matchVersion(pver.toUtf8(), version.toUtf8(), op))
            {
                rs.append(i);
            }
        }
    }

    return rs;
}

QVector<int> DatabaseReader::packagesByVString(const QString &verStr)
{
    // Parser la version
    QByteArray name, version;
    int op;

    op = PackageSystem::parseVersion(verStr.toUtf8(), name, version);

    return packagesByVString(name, version, op);
}

bool DatabaseReader::package(const QString &name, const QString &version, int &rs)
{
    QString pkgname;
    int32_t count = *(int32_t *)m_packages;

    for (int i=0; i<count; ++i)
    {
        _Package *pkg = package(i);
        
        pkgname = QString(string(false, pkg->name));

        if (pkgname == name)
        {
            // Vérifier aussi la version
            if (version.isNull())
            {
                rs = i;
                return true;
            }
            else if (version == QString(string(false, pkg->version)))
            {
                rs = i;
                return true;
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

QList<PackageFile *> DatabaseReader::files(const QRegExp &regex)
{
    QList<PackageFile *> rs;
    
    int count = *(int32_t *)m_files;
    
    // Explorer tous les fichiers
    for (int i=0; i<count; ++i)
    {
        _File *fl = file(i);
        
        if (regex.exactMatch(fileString(fl->name_ptr)))
        {
            rs.append((PackageFile *)(new DatabaseFile(ps, this, fl, new DatabasePackage(fl->package, ps, this), true)));
        }
    }
    
    return rs;
}

QList<PackageFile *> DatabaseReader::files(const QString &name)
{
    QList<PackageFile *> rs;
    QStringList parts = name.split('/', QString::SkipEmptyParts);
    uchar *ptr = m_files;
    int index = 0, curPart = 0;
    
    // Trouver l'index du dossier racine
    ptr += 4;
    index = *(int32_t *)ptr;
    
    // Explorer les fichiers
    _File *fl = file(index);
    
    while (fl)
    {
        QString fileName(fileString(fl->name_ptr));
        
        if (fileName == parts.at(curPart))
        {
            // Nom correct. Si c'est un dossier, entrer dedans
            if (fl->flags & PACKAGE_FILE_DIR && curPart != parts.count() - 1)
            {
                // Dossier et pas dernière partie ==> ok, tout va bien
                curPart++;
                fl = file(fl->first_child);
                continue;
            }
            else if (!(fl->flags & PACKAGE_FILE_DIR) && curPart == parts.count() - 1)
            {
                // Fichier, et dernière partie ==> ok, tout va bien
                rs.append((PackageFile *)(new DatabaseFile(ps, this, fl, new DatabasePackage(0, fl->package, ps, this), true)));
            }
        }
        
        // Passer au suivant
        fl = file(fl->next_file_dir);
    }
    
    return rs;
}

QList<_Depend *> DatabaseReader::depends(int pkgIndex)
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

QVector<int> DatabaseReader::packagesOfString(int stringIndex, int nameIndex, int op)
{
    QVector<int> rs;
    QByteArray cmpVersion;
    
    // Vérifier l'index
    if (op != DEPEND_OP_NOVERSION)
    {
        if (stringIndex >= *(int *)m_strings)
        {
            return rs;
        }   
        
        cmpVersion = QByteArray(string(0, stringIndex));
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
        else if (PackageSystem::matchVersion(
            QByteArray(string(0, ((_StrPackage *)(sptr))->version)), 
            cmpVersion, 
            op))
        {
            rs.append( ((_StrPackage *)(sptr))->package );
        }

        sptr += sizeof(_StrPackage);
    }

    return rs;
}

QVector<int> DatabaseReader::orphans()
{
    int32_t npkgs = *(int *)m_packages;
    QVector<int> rs;
    
    for (int i=0; i<npkgs; ++i)
    {
        _Package *pkg = package(i);
        
        // Si le paquet est installé, n'est pas demandé par l'utilisateur, et a un used = 0, alors le prendre
        if (pkg->flags & PACKAGE_FLAG_INSTALLED && (pkg->flags & PACKAGE_FLAG_WANTED) == 0 && pkg->used == 0)
        {
            rs.append(i);
        }
    }
    
    return rs;
}

QList<UpgradeInfo> DatabaseReader::upgradePackages()
{
    int32_t npkgs = *(int *)m_packages;
    QVector<int> otherVersions;
    QList<UpgradeInfo> rs;
    UpgradeInfo ui;
    
    // Explorer chaque paquet
    for (int i=0; i<npkgs; ++i)
    {
        _Package *pkg = package(i);
        
        // Voir si le paquet est installé
        if (pkg != 0 && pkg->flags & PACKAGE_FLAG_INSTALLED && !(pkg->flags & PACKAGE_FLAG_DONTUPDATE))
        {
            // Trouver les autres versions de ce paquet
            otherVersions = packagesOfString(0, pkg->name, DEPEND_OP_NOVERSION);
            
            for (int j=0; j<otherVersions.count(); ++j)
            {
                // Si l'autre version a une version différente, et la même distribution, alors mettre le paquet à jour
                // Vérifier aussi qu'ils aient le même nom, pour éviter les ennuis avec les provides
                _Package *opkg = package(otherVersions.at(j));
                
                if (opkg != 0 &&
                    pkg->version != opkg->version && 
                    pkg->distribution == opkg->distribution &&
                    pkg->name == opkg->name &&
                    PackageSystem::compareVersions(string(0, pkg->version), string(0, opkg->version)) == -1)
                {
                    ui.installedPackage = i;
                    ui.newPackage = otherVersions.at(j);
                    
                    rs.append(ui);
                }
            }
        }
    }
    
    // Renvoyer la liste des paquets qu'on peut mettre à jour
    return rs;
}

int DatabaseReader::packages()
{
    return *(int *)m_packages;
}

_Depend *DatabaseReader::depend(int32_t ptr)
{
    int numdepsptr = *(int *)m_depends;
    
    uchar *dep = m_depends;
    dep += 4;                   // Sauter count
    dep += numdepsptr * sizeof(_DependPtr);
    dep += ptr;

    return (_Depend *)dep;
}

_Package *DatabaseReader::package(int index)
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

_File *DatabaseReader::file(int index)
{
    if (index >= *(int *)m_files || index < 0)
    {
        return 0;
    }
    
    uchar *fl = m_files;
    fl += 8;        // Nombre de fichiers + Index de la racine
    
    fl += (index * sizeof(_File));
    
    return (_File *)fl;
}

const char *DatabaseReader::fileString(int ptr)
{
    // Chaîne dont on a le pointeur. Elle se trouve
    // simplement à fichiers*sizeof(_File)+8+ptr
    const char *rs = (const char *)m_files;
    
    rs += 8;
    rs += (*(int *)m_files)*sizeof(_File);
    rs += ptr;
    
    return rs;
}

const char *DatabaseReader::string(bool translate, int index)
{
    uchar *map;
    
    if (!translate)
    {
        map = m_strings;
    }
    else
    {
        map = m_translate;
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

bool DatabaseReader::mapFile(const QString &file, QFile **ptr, uchar **map)
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