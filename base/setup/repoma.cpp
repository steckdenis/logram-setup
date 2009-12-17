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

#include <logram/repositorymanager.h>

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
    
    foreach (const QString &fileName, lpkFileNames)
    {
        // C'est nous qui envoyons la progression, car nous savons combien de paquets sont à importer
        ps->sendProgress(PackageSystem::Including, numPkg, lpkFileNames.count(), fileName);
        
        if (!mg->includePackage(fileName))
        {
            error();
            return;
        }
    }
    
    ps->endProgress(PackageSystem::Including, lpkFileNames.count());
    
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