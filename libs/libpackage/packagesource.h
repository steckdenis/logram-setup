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

#ifndef __PACKAGESOURCE_H__
#define __PACKAGESOURCE_H__

#include <QObject>
#include <QVariant>
#include <QProcess>
#include <QStringList>

namespace Logram
{

class PackageSystem;
class FilePackage;
class PackageMetaData;

class PackageSource : public QObject
{
    Q_OBJECT
    
    public:
        PackageSource(PackageSystem *ps);
        ~PackageSource();
        
        enum Option
        {
            SourceDir
        };
        
        bool setMetaData(const QString &fileName);
        void setOption(Option opt, const QVariant &value);
        QVariant option(Option opt, const QVariant &defaultValue);
        
        bool getSource(bool block = true);
        
    private slots:
        void processDataOut();
        void processTerminated(int exitCode, QProcess::ExitStatus exitStatus);
        
    private:
        struct Private;
        Private *d;
};
    
} /* Namespace */

#endif