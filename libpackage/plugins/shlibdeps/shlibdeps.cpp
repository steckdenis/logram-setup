/*
 * shlibdeps.cpp
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

#include "shlibdeps.h"

#include <packagesystem.h>
#include <databasepackage.h>
#include <packagemetadata.h>

#include <gelf.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <QFile>
#include <QDomElement>
#include <QDomDocument>

#include <QtDebug>

ShLibDeps::ShLibDeps() : QObject(0)
{
    error = false;
    
    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        error = true;
    }
}

QString ShLibDeps::name() const
{
    return QString("shlibdeps");
}

bool ShLibDeps::byDefault() const
{
    return false;
}

void ShLibDeps::init(PackageMetaData* md, PackageSource* src)
{
    this->src = src;
    this->md = md;
}

void ShLibDeps::end()
{
    return;
}

static void getSection(QByteArray &ba, GElf_Shdr &shdr, Elf_Scn *scn)
{
    Elf_Data *data = NULL;
    size_t n = 0;
    
    while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != NULL)
    {
        ba += QByteArray((const char *) data->d_buf, data->d_size);
    }
}

void ShLibDeps::processPackage(const QString& name, QStringList& files, bool isSource)
{
    if (isSource || error) return;   // On ne s'occupe pas des sources
        
    QByteArray buildRoot = src->option(PackageSource::BuildDir, QVariant()).toByteArray();
    
    // Lister les fichiers
    Elf *e;
    int fd;
    char *sname;
    Elf_Scn *scn;
    const GElf_Dyn *dyn;
    
    GElf_Shdr shdr;
    size_t shstrndx;
    
    QByteArray dynamic, dynstr;
    QList<QByteArray> needs, runpaths;
    
    runpaths << "/lib" << "/usr/lib";
    
    for (int i=0; i<files.count(); ++i)
    {
        const QString &file = files.at(i);
        
        // Essayer d'ouvrir ce fichier
        fd = open((buildRoot + file).toUtf8().constData(), O_RDONLY, 0);
        
        if (fd < 0) continue;
        
        // Ouvrir ELF
        e = elf_begin(fd, ELF_C_READ, NULL);
        
        if (e == NULL)
        {
            close(fd);
            continue;
        }
        
        if (elf_kind(e) != ELF_K_ELF || elf_getshdrstrndx(e, &shstrndx) != 0)
        {
            elf_end(e);
            close(fd);
            continue;
        }
        
        // Parcourir les sections
        scn = NULL;
        dynamic.clear();
        dynstr.clear();
        
        bool err = false;
        
        while ((scn = elf_nextscn(e, scn)) != NULL)
        {
            if (gelf_getshdr(scn, &shdr) != &shdr)
            {
                err = true;
                break;
            }
            
            if ((sname = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
            {
                err = true;
                break;
            }
            
            if (strcmp(sname, ".dynamic") == 0)
            {
                getSection(dynamic, shdr, scn);
            }
            else if (strcmp(sname, ".dynstr") == 0)
            {
                getSection(dynstr, shdr, scn);
            }
        }
        
        if (dynamic.size() == 0 || dynstr.size() == 0) err = true;
        
        if (err)
        {
            elf_end(e);
            close(fd);
            continue;
        }
        
        // Explorer dynamic, qui contient des enregistrements de type GElf_Dyn
        dyn = (const GElf_Dyn *)dynamic.constData();
        int j = 0;
        QByteArray ba;
        
        while (j * sizeof(dyn) <= dynamic.size())
        {
            if (dyn->d_tag == DT_NEEDED && dyn->d_un.d_ptr < dynstr.size())
            {
                const char *tmp = dynstr.constData();
                tmp += dyn->d_un.d_ptr;
                ba = QByteArray(tmp);
                
                if (!needs.contains(ba))
                {
                    needs.append(ba);
                }
            }
            else if ((dyn->d_tag == DT_RUNPATH || dyn->d_tag == DT_RPATH) && dyn->d_un.d_ptr < dynstr.size())
            {
                const char *tmp = dynstr.constData();
                tmp += dyn->d_un.d_ptr;
                ba = QByteArray(tmp);
                
                if (!runpaths.contains(ba))
                {
                    runpaths.append(ba);
                }
            }
            
            dyn++;
            j++;
        }
        
        // Fermer le tout
        elf_end(e);
        close(fd);
    }
    
    // Voir si on sait initialiser le PackageSystem de src
    PackageSystem *ps = src->packageSystem();
    
    if (!ps->initialized() && !ps->init())
    {
        PackageRemark *remark = new PackageRemark;
            
        remark->severity = PackageRemark::Error;
        remark->packageName = name;
        remark->message = tr("Impossible d'initialiser la gestion des paquets pour trouver les dépendances automatiques");
        
        src->addRemark(remark);
        
        // La raison
        if (ps->lastError())
        {
            remark = new PackageRemark;
                
            remark->severity = PackageRemark::Error;
            remark->packageName = name;
            remark->message = QString::number(ps->lastError()->type) + " " + ps->lastError()->info;
            
            src->addRemark(remark);
        }
        
        return;
    }
    
    // Explorer les needs dans runpaths pour trouver les paquets les contenant, et leurs versions
    // Si un fichier n'est pas trouvé, et ne se trouve dans dans buildDir
    QList<PackageFile *> pfiles;
    
    foreach (const QByteArray &need, needs)
    {
        QString dname, version;
        bool my = false;
        
        foreach (const QByteArray &runpath, runpaths)
        {
            if (QFile::exists(buildRoot + "/" + runpath + "/" + need))
            {
                // Fichier construit par nous-même, ignorer
                my = true;
                break;
            }
            
            // Utiliser la BDD Setup pour trouver quel paquet contient ce fichier, s'il existe
            pfiles.clear();
            
            QString pth = QString::fromUtf8(runpath + "/" + need);
            pth.remove(0, 1);
            
            pfiles = ps->files(pth);
            
            foreach (PackageFile *file, pfiles)
            {
                if (file->package() != 0)
                {
                    Package *pkg = file->package();
                    
                    dname = pkg->name();
                    version = pkg->version();
                    
                    delete file;
                    break;
                }
                
                delete file;
            }
        }
        
        if (my) continue;   // Ignorer les fichiers qu'on fourni nous-même
        
        if (dname.isNull())
        {
            PackageRemark *remark = new PackageRemark;
            
            remark->severity = PackageRemark::Warning;
            remark->packageName = name;
            remark->message = tr("Impossible de trouver le paquet contenant %1").arg(QString::fromUtf8(need));
            
            src->addRemark(remark);
        }
        
        // Remarque
        PackageRemark *remark = new PackageRemark;
            
        remark->severity = PackageRemark::Information;
        remark->packageName = name;
        remark->message = tr("Ajout de %1>=%2 comme dépendance automatique").arg(dname, version);
        
        src->addRemark(remark);
        
        // Ajouter un élément de dépendance au <package> de ce paquet.
        QDomElement depend = md->createElement("depend");
        depend.setAttribute("type", "depend");
        depend.setAttribute("string", dname + ">=" + version);
        
        QDomElement package = md->documentElement().firstChildElement("package");
        
        while (!package.isNull())
        {
            if (package.attribute("name") == name)
            {
                package.insertBefore(depend, package.firstChild());
                break;
            }
            
            package = package.nextSiblingElement("package");
        }
    }
}

Q_EXPORT_PLUGIN2(shlibdeps, ShLibDeps)

#include "shlibdeps.moc"