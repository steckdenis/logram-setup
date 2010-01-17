/*
 * processthread.h
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

#ifndef __PROCESSTHREAD_H__
#define __PROCESSTHREAD_H__

#include <QThread>
#include <QList>
#include <QByteArray>

namespace Logram
{

class Package;
class PackageSystem;
class PackageMetaData;

class ProcessThread : public QThread
{
    public:
        ProcessThread(PackageSystem *ps, Package *pkg);
        ~ProcessThread();
        
        void run();
        bool error() const;
        
    private:
        bool depack(Package *pkg, QList<QByteArray> &files, QByteArray &metadataContents);
        bool install(Package *pkg, QList<QByteArray> &files, PackageMetaData *md, const QString &prescript);
        
        struct Private;
        Private *d;
};

} /* Namespace */

#endif