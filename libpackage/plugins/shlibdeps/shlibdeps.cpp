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
#include <QRegExp>

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

static void getSection32(QByteArray &ba, Elf32_Shdr *shdr, Elf_Scn *scn)
{
    Elf_Data *data = NULL;
    size_t n = 0;
    
    while (n < shdr->sh_size && (data = elf_getdata(scn, data)) != NULL)
    {
        ba += QByteArray((const char *) data->d_buf, data->d_size);
    }
}

static void getSection64(QByteArray &ba, Elf64_Shdr *shdr, Elf_Scn *scn)
{
    Elf_Data *data = NULL;
    size_t n = 0;
    
    while (n < shdr->sh_size && (data = elf_getdata(scn, data)) != NULL)
    {
        ba += QByteArray((const char *) data->d_buf, data->d_size);
    }
}

struct Pkg
{
    QString name;
    QString version;
    bool local;
};

void ShLibDeps::processPackage(const QString& name, QStringList& files, bool isSource)
{
    if (isSource || error) return;   // On ne s'occupe pas des sources
        
    QByteArray buildRoot = src->option(PackageSource::BuildDir, QVariant()).toByteArray();
    
    // Lister les fichiers
    Elf *e;
    int fd;
    char *sname;
    Elf_Scn *scn;
    const Elf32_Dyn *dyn32;
    const Elf64_Dyn *dyn64;
    
    Elf32_Shdr *shdr32;
    Elf64_Shdr *shdr64;
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
        
        int eclass =  gelf_getclass(e);
        
        if (eclass == ELFCLASSNONE)
        {
            elf_end(e);
            close(fd);
            continue;
        }
        
        bool is32 = (eclass == ELFCLASS32);
        
        // Parcourir les sections
        scn = NULL;
        dynamic.clear();
        dynstr.clear();
        
        bool err = false;
        
        while ((scn = elf_nextscn(e, scn)) != NULL)
        {
            if (is32)
            {
                shdr32 = elf32_getshdr(scn);
                
                if (shdr32 == NULL)
                {
                    err = true;
                    break;
                }
            }
            else
            {
                shdr64 = elf64_getshdr(scn);
                
                if (shdr64 == NULL)
                {
                    err = true;
                    break;
                }
            }
            
            if ((sname = elf_strptr(e, shstrndx, (is32 ? shdr32->sh_name : shdr64->sh_name))) == NULL)
            {   
                err = true;
                break;
            }
            
            if (strcmp(sname, ".dynamic") == 0)
            {
                if (is32) getSection32(dynamic, shdr32, scn);
                else getSection64(dynamic, shdr64, scn);
            }
            else if (strcmp(sname, ".dynstr") == 0)
            {
                if (is32) getSection32(dynstr, shdr32, scn);
                else getSection64(dynstr, shdr64, scn);
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
        dyn32 = (const Elf32_Dyn *)dynamic.constData();
        dyn64 = (const Elf64_Dyn *)dyn32;
        
        int j = 0;
        QByteArray ba;
        
        while (j * (is32 ? sizeof(Elf32_Dyn) : sizeof(Elf64_Dyn)) <= dynamic.size())
        {
            int tag = (is32 ? dyn32->d_tag : dyn64->d_tag);
            int dptr = (is32 ? dyn32->d_un.d_ptr : dyn64->d_un.d_ptr);
            
            if (tag == DT_NEEDED && dptr < dynstr.size())
            {
                const char *tmp = dynstr.constData();
                tmp += dptr;
                ba = QByteArray(tmp);
                
                if (!needs.contains(ba))
                {
                    needs.append(ba);
                }
            }
            else if ((tag == DT_RUNPATH || tag == DT_RPATH) && dptr < dynstr.size())
            {
                const char *tmp = dynstr.constData();
                tmp += dptr;
                ba = QByteArray(tmp);
                
                if (!runpaths.contains(ba))
                {
                    runpaths.append(ba);
                }
            }
            
            dyn32++;
            dyn64++;
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
    QList<Pkg> pkgs;
    Pkg pkg;
    
    foreach (const QByteArray &need, needs)
    {
        QString dname, version;
        bool my = false;
        
        foreach (const QByteArray &runpath, runpaths)
        {
            QString pth = QString::fromUtf8(runpath + "/" + need);
            
            if (QFile::exists(buildRoot + pth))
            {
                // Fichier construit par nous-même : chercher le binaire qui le contient
                QDomElement package = md->documentElement().firstChildElement("package");
                
                while (!package.isNull())
                {
                    QString arch = package.attribute("arch");
                    
                    // Le paquet doit être de la bonne architecture
                    if (arch == "all" || arch == "any" || arch == SETUP_ARCH)
                    {
                        // Explorer les éléments files de ce paquet et voir si notre fichier correspond
                        QDomElement files = package.firstChildElement("files");
                        
                        while (!files.isNull())
                        {
                            QRegExp regex(files.attribute("pattern", "*"), Qt::CaseSensitive, QRegExp::Wildcard);
                            
                            if (regex.exactMatch(pth))
                            {
                                // On a trouvé le binaire qui contient la bibliothèque
                                pkg.name = package.attribute("name");
                                pkg.version = "{{version}}";
                                pkg.local = true;
                                
                                pkgs.append(pkg);
                                my = true;
                                break;
                            }
                            
                            files = files.nextSiblingElement("files");
                        }
                        
                        if (my) break;
                    }
                    package = package.nextSiblingElement("package");
                }
                
                if (my) break;
            }
            
            // Utiliser la BDD Setup pour trouver quel paquet contient ce fichier, s'il existe
            pfiles.clear();
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
        
        if (my) continue;   // Déjà traité
        
        if (dname.isNull())
        {
            PackageRemark *remark = new PackageRemark;
            
            remark->severity = PackageRemark::Warning;
            remark->packageName = name;
            remark->message = tr("Impossible de trouver le paquet contenant %1").arg(QString::fromUtf8(need));
            
            src->addRemark(remark);
        }
        
        // Ajouter le paquet à la liste
        pkg.name = dname;
        pkg.version = version;
        pkg.local = false;
        
        pkgs.append(pkg);
    }
    
    // Ajouter les enregistrements XML
    foreach (const Pkg &p, pkgs)
    {   
        // Opération : >= normalement, mais = si c'est une dépendance de la même source
        QString op = (p.local ? "=" : ">=");
        
        // Remarque
        PackageRemark *remark = new PackageRemark;
            
        remark->severity = PackageRemark::Information;
        remark->packageName = name;
        remark->message = tr("Ajout de %1%2%3 comme dépendance automatique").arg(p.name, op, p.version);
        
        src->addRemark(remark);
        
        // Ajouter un élément de dépendance au <package> de ce paquet.
        QDomElement depend = md->createElement("depend");
        depend.setAttribute("type", "depend");
        depend.setAttribute("string", p.name + op + p.version);
        
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