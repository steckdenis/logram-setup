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
#include <QtDebug>

#include <logram/libpackage.h>

#define VERSION "0.0.1"
#define COLOR(text, color) qPrintable(QString("\033[1m\033[") + color + "m" + text + "\033[0m")

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

        // Construction
        void getSource(const QString &fileName);
        void preparePatch(const QString &fileName);
        void genPatch(const QString &fileName);
        void createSource(const QString &fileName);
        void unpackSource(const QString &fileName);
        void patchSource(const QString &fileName);

    public slots:
        void error(PackageSystem::Error err, const QString &info);
        void message(const QString &msg);

    private:
        PackageSystem *ps;
};

#endif