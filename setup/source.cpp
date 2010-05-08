/*
 * source.cpp
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

#include <packagesource.h>

#include <iostream>

using namespace std;
using namespace Logram;

#define CALL_PACKAGESOURCE(function) \
    PackageSource *src = new PackageSource(ps); \
    \
    if (!src->setMetaData(fileName)) \
    { \
        error(); \
        return; \
    } \
    \
    src->loadKeys(); \
    \
    if (!src->function()) \
    { \
        error(); \
        return; \
    }

void App::sourceDownload(const QString &fileName)
{
    CALL_PACKAGESOURCE(getSource);
    
    delete src;
}

void App::sourceBuild(const QString &fileName)
{
    CALL_PACKAGESOURCE(build);
    
    delete src;
}

void App::sourceBinaries(const QString &fileName)
{
    CALL_PACKAGESOURCE(binaries);
    
    // Explorer les messages du PackageSource
    bool first = true;
    
    int maxlength = 0;
    
    foreach (PackageRemark *remark, src->remarks())
    {
        QString &pkgname = remark->packageName;
        
        if (pkgname.length() > maxlength)
        {
            maxlength = pkgname.length();
        }
    }
    
    foreach (PackageRemark *remark, src->remarks())
    {
        if (first)
        {
            cout << endl;
            cout << COLOR(tr("Remarques sur le paquet :"), "35") << endl;
            cout << endl;
            
            first = false;
        }
        
        cout << "  * ";
        
        if (!remark->packageName.isNull())
        {
            cout << '[' << COLOR(remark->packageName.leftJustified(maxlength), "37") << "] ";
        }
        else
        {
            for (int i=0; i<maxlength + 3; ++i)
            {
                cout << ' ';
            }
        }
        
        switch (remark->severity)
        {
            case PackageRemark::Information:
                cout << COLOR(tr("Information : "), "32");
                break;
                
            case PackageRemark::Warning:
                cout << COLOR(tr("Attention   : "), "33");
                break;
                
            case PackageRemark::Error:
                cout << COLOR(tr("Erreur      : "), "31");
                break;
        }
        
        cout << qPrintable(remark->message) << endl;
    }
    
    delete src;
}