/*
 * repoma.cpp
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

#include "app.h"

#include <repositorymanager.h>

#include <QtDebug>

using namespace Logram;

void App::include(const QStringList &lpkFileNames)
{
    RepositoryManager *mg = new RepositoryManager(ps);
    
    if (!mg->loadConfig("config/repo.conf"))
    {
        error();
        return;
    }
    
    // Inclure les paquets
    int numPkg = 0;
    int progress = ps->startProgress(Progress::Including, lpkFileNames.count());
    QString arch;
    
    foreach (const QString &fileName, lpkFileNames)
    {
        // C'est nous qui envoyons la progression, car nous savons combien de paquets sont à importer
        if (!ps->sendProgress(progress, numPkg, fileName))
        {
            return;
        }
        
        arch = fileName.section('.', -2, -2);
        
        if (arch == "src")
        {
            if (!mg->includeSource(fileName))
            {
                error();
                return;
            }
        }
        else
        {
            if (!mg->includePackage(fileName))
            {
                error();
                return;
            }
        }
        
        numPkg++;
    }
    
    ps->endProgress(progress);
    
    // Plus besoin
    delete mg;
}

void App::exp(const QStringList &distros)
{
    RepositoryManager *mg = new RepositoryManager(ps);
    
    if (!mg->loadConfig("config/repo.conf"))
    {
        error();
        return;
    }
    
    if (!mg->exp(distros))
    {
        error();
        return;
    }
    
    delete mg;
}