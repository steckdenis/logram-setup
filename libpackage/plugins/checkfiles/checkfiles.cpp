/*
 * checkfiles.cpp
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

#include "checkfiles.h"

#include <QDir>
#include <QFileInfo>

#include <QtDebug>

CheckFiles::CheckFiles() : QObject(0)
{
    return;
}

QString CheckFiles::name() const
{
    return QString("checkfiles");
}

bool CheckFiles::byDefault() const
{
    return true;
}

void CheckFiles::init(PackageMetaData* md, PackageSource* src)
{
    this->src = src;
    
    // Remplir la liste builtfiles
    PackageSource::listFiles(src->option(PackageSource::BuildDir, QVariant()).toString(), "/", builtfiles);
    
    (void) md;
}

void CheckFiles::end()
{
    // Tous les fichiers qui restent dans builtfiles sont orphelins
    foreach (const QString &file, builtfiles)
    {
        PackageRemark *remark = new PackageRemark;
            
        remark->severity = PackageRemark::Warning;
        remark->packageName = QString();
        remark->message = tr("Le fichier %1 ne se trouve dans aucun paquet").arg(file);
        
        src->addRemark(remark);
    }
    
    builtfiles.clear();
}

void CheckFiles::processPackage(const QString& name, QStringList& files, bool isSource)
{
    // Lister les fichiers et ajouter une erreur si /usr/share/info/dir est présent
    if (isSource) return;   // On ne s'occupe pas des sources
    
    for (int i=0; i<files.count(); ++i)
    {
        const QString &file = files.at(i);
        
        // Supprimer ce fichier de builtfiles
        for (int j=0; j<builtfiles.count(); ++j)
        {
            const QString &bfile = builtfiles.at(j);
            
            if (bfile == file)
            {
                builtfiles.removeAt(j);
                break;
            }
        }
        
        // Supprimer les fichiers en trop (NOTE: Après la population de builtfiles car les éventuels .removeAt invalident file.
        if (file == "/usr/share/info/dir")
        {
            PackageRemark *remark = new PackageRemark;
            
            remark->severity = PackageRemark::Warning;
            remark->packageName = name;
            remark->message = tr("Le fichier /usr/share/info/dir a été trouvé alors qu'il ne peut être présent. Supprimé.");
            
            src->addRemark(remark);
            
            // Supprimer ce fichier de la liste
            files.removeAt(i);
            i--;
        }
    }
}

Q_EXPORT_PLUGIN2(checkfiles, CheckFiles)

#include "checkfiles.moc"