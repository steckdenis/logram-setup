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

#include <logram/packagesystem.h>

#define VERSION "0.1-alpha1"

#define COLOR(text, color) (colored ? qPrintable(QString("\033[1m\033[") + color + "m" + text + "\033[0m") : qPrintable(text))

#define COLORC(text, color) (colored ? qPrintable(QString("\033[1m\033[") + color + "m" + text + "\033[0m") : text)

namespace Logram
{
    class Solver;
    class Package;
    class Communication;
}

class App : public QCoreApplication
{
    Q_OBJECT
    
    public:
        App(int &argc, char **argv);

        // Actions
        void help();
        void version();
        void find(const QString &pattern);
        void showpkg(const QString &name, bool changelog);
        void update();
        void upgrade();
        void add(const QStringList &packages);
        void showFiles(const QString &packageName);
        
        void error();
        
        void getString(char *buffer, int max_length, const char *def, bool append_return);
        
        void displayPackages(QList<Logram::Package *> *packages, int &instSize, int &dlSize, bool showType);
        void displayPackages(QList<Logram::DatabasePackage *> *packages, int &instSize, int &dlSize, bool showType);

    public slots:
        void progress(Logram::PackageSystem::Progress type, int done, int total, const QString &msg);
        void communication(Logram::Package *sender, Logram::Communication *comm);

    private:
        Logram::PackageSystem *ps;
        bool colored;

        void manageResults(Logram::Solver *solver);
};

#endif