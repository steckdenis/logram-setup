/*
 * app.h
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

#ifndef __APP_H__
#define __APP_H__

#include <QCoreApplication>
#include <QStringList>
#include <QtDebug>

#include <logram/libpackage.h>

#define VERSION "0.0.1"
#define COLOR(text, color) qPrintable(QString("\033[1m\033[") + color + "m" + text + "\033[0m")

class Solver;
class Package;

class App : public QCoreApplication
{
    Q_OBJECT
    
    public:
        App(int &argc, char **argv);

        // Actions
        void help();
        void version();
        void find(const QString &pattern);
        void showpkg(const QString &name);
        void update();
        void add(const QStringList &packages);
        void showFiles(const QString &packageName);

    public slots:
        void error(PackageSystem::Error err, const QString &info);
        void progress(PackageSystem::Progress type, int done, int total, const QString &msg);

        void message(Package *sndr, const QString &msg);

    private:
        PackageSystem *ps;

        void manageResults(Solver *solver);
};

#endif