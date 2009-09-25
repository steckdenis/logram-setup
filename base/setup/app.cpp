/*
 * app.cpp
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

#include <logram/package.h>

#include <QStringList>
#include <QTextCodec>

#include <iostream>
using namespace std;

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    // Initialiser
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    // Ouvrir le système de paquets
    ps = new PackageSystem(this);

    connect(ps, SIGNAL(progress(int, int, const QString &)), this, SLOT(progress(int, int, const QString &)));
    connect(ps, SIGNAL(error(PackageSystem::Error, const QString &)), this, SLOT(error(PackageSystem::Error, const QString &)));
    
    //Parser les arguments
    QStringList args = arguments();

    if (args.count() == 1)
    {
        help();
        return;
    }

    QString cmd = args.at(1);

    // Initialiser le système de paquet si on en a besoin
    if (cmd != "update")
    {
        ps->init();
    }

    if (cmd == "--help")
    {
        help();
    }
    else if (cmd == "--version")
    {
        version();
    }
    else if (cmd == "search")
    {
        find(args.at(2));
    }
    else if (cmd == "showpkg")
    {
        showpkg(args.at(2));
    }
    else if (cmd == "update")
    {
        update();
    }
    else if (cmd == "add")
    {
        QStringList packages;

        for (int i=2; i<args.count(); ++i)
        {
            packages.append(args.at(i));
        }

        add(packages);
    }
}

void App::help()
{
    version();

    cout << endl;

    cout << "Usage : setup <action> [arguments]" << endl;
    cout << "    --help             Show help" << endl;
    cout << "    --version          Show version" << endl;
    cout << "    search <pattern>   Show all the packages matching <pattern>" << endl;
    cout << "    showpkg <name>     Show the informations of the package <name>" << endl;
    cout << "    update             Update the packages' database" << endl;
    cout << "    add <packages>     Add packages (prepend them with \"-\" to remove)" << endl;
}

void App::version()
{
    cout << "Logram Setup " VERSION << endl;
    cout << endl;
    cout << "Logram is free software; you can redistribute it and/or modify" << endl;
    cout << "it under the terms of the GNU General Public License as published by" << endl;
    cout << "the Free Software Foundation; either version 3 of the License, or" << endl;
    cout << "(at your option) any later version." << endl;
}

void App::error(PackageSystem::Error err, const QString &info)
{
    cout << COLOR("ERROR : ", "31");

    switch (err)
    {
        case PackageSystem::OpenFileError:
            cout << "Cannot open file ";
            break;
        case PackageSystem::MapFileError:
            cout << "Cannot map file ";
            break;
        case PackageSystem::ProcessError:
            cout << "Error when executing command ";
            break;
        case PackageSystem::DownloadError:
            cout << "Error when downloading file ";
            break;
    }
    
    cout << COLOR(info, "35") << endl;
}

void App::progress(int done, int total, const QString &msg)
{
    // Si on affiche de nouveau le même message, revenir à la ligne au dessus
    if (lastMsg == msg)
    {
        cout << "\033M";
    }

    // Afficher le message
    cout << COLOR("[" + QString::number(done) + "/" + QString::number(total) + "] ", "33");
    cout << qPrintable(msg);
    cout << endl;
}