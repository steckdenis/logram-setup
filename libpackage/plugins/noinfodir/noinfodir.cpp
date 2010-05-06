/*
 * packagesource.h
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

#include "noinfodir.h"

#include <QtDebug>

NoInfoDir::NoInfoDir() : QObject(0)
{
    return;
}

QString NoInfoDir::name() const
{
    return QString("noinfodir");
}

bool NoInfoDir::byDefault() const
{
    return true;
}

void NoInfoDir::init(PackageMetaData* md, PackageSource* src)
{
    this->src = src;
    
    (void) md;
}

void NoInfoDir::processPackage(const QString& name, QStringList& files, bool isSource)
{
    // Lister les fichiers et ajouter une erreur si /usr/share/info/dir est présent
    if (isSource) return;   // On ne s'occupe pas des sources
    
    for (int i=0; i<files.count(); ++i)
    {
        const QString &file = files.at(i);
        
        if (file == "/usr/share/info/dir")
        {
            // Trouvé, ajouter la remarque
            PackageRemark *remark = new PackageRemark;
            
            remark->severity = PackageRemark::Warning;
            remark->packageName = name;
            remark->message = tr("Le fichier /usr/share/info/dir a été trouvé alors qu'il ne peut être présent. Supprimé.");
            
            src->addRemark(remark);
            
            // Supprimer ce fichier de la liste
            files.removeAt(i);
            break;
        }
    }
}

Q_EXPORT_PLUGIN2(noinfodir, NoInfoDir)

#include "noinfodir.moc"