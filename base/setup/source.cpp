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

#include <logram/packagesource.h>

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
    if (!src->function(true)) \
    { \
        error(); \
        return; \
    } \
    \
    delete src;

void App::sourceDownload(const QString &fileName)
{
    CALL_PACKAGESOURCE(getSource);
}

void App::sourceBuild(const QString &fileName)
{
    CALL_PACKAGESOURCE(build);
}

void App::sourceBinaries(const QString &fileName)
{
    CALL_PACKAGESOURCE(binaries);
}