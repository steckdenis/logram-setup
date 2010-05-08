/*
 * filemanypackages.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include "filemanypackages.h"

#include <QtDebug>

FileManyPackages::FileManyPackages() : QObject(0)
{
    return;
}

QString FileManyPackages::name() const
{
    return QString("filemanypackages");
}

bool FileManyPackages::byDefault() const
{
    return true;
}

void FileManyPackages::init(PackageMetaData* md, PackageSource* src)
{
    this->src = src;
    this->enrgs.clear();
    
    (void) md;
}

void FileManyPackages::end()
{
    return;
}

void FileManyPackages::processPackage(const QString& name, QStringList& files, bool isSource)
{
    // Lister les fichiers et ajouter une erreur si /usr/share/info/dir est présent
    if (isSource) return;   // On ne s'occupe pas des sources
    
    for (int i=0; i<files.count(); ++i)
    {
        const QString &file = files.at(i);
        bool found = false;
        
        // Voir si ce fichier existe déjà.
        for (int j=0; j<enrgs.count(); ++j)
        {
            Enrg &enrg = enrgs[j];
            
            if (enrg.path == file)
            {
                // Ah, ce fichier est déjà dedans.
                PackageRemark *remark = new PackageRemark;
            
                remark->severity = PackageRemark::Warning;
                remark->packageName = name;
                remark->message = tr("Le fichier %1 se trouve également dans %2").arg(file, enrg.package);
                
                src->addRemark(remark);
                
                // Changer le package de cet enregistrement pour que les éventuels futurs messages
                // en rapport avec ce fichier apportent plus d'informations.
                enrg.package = name;
                
                // On a trouvé un fichier, pas la peine de continuer
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // On n'a pas encore le fichier
            Enrg enrg;
            
            enrg.path = file;
            enrg.package = name;
            
            enrgs.append(enrg);
        }
    }
}

Q_EXPORT_PLUGIN2(filemanypackages, FileManyPackages)

#include "filemanypackages.moc"