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

    connect(ps, SIGNAL(progress(PackageSystem::Progress, int, int, const QString &)), this, SLOT(progress(PackageSystem::Progress, int, int, const QString &)));
    
    //Parser les arguments
    QStringList args = arguments();

    if (args.count() == 1)
    {
        help();
        return;
    }

    // Explorer les options
    QString opt = args.at(1);
    bool changelog = false;

    while (opt.startsWith('-'))
    {
        if (opt == "-S")
        {
            bool isug = true;

            if (args.at(2) == "off")
            {
                isug = false;
                args.removeAt(1); // normal que ce soit 1 pas 2
            }

            ps->setInstallSuggests(isug);
        }
        else if (opt == "-I")
        {
            ps->setParallelInstalls(args.takeAt(2).toInt());
        }
        else if (opt == "-D")
        {
            ps->setParallelDownloads(args.takeAt(2).toInt());
        }
        else if (opt == "-R")
        {
            QString root = args.takeAt(2);
            
            ps->setInstallRoot(root);
            ps->setConfRoot(root);
            ps->setVarRoot(root);
        }
        else if (opt == "-iR")
        {
            ps->setInstallRoot(args.takeAt(2));
        }
        else if (opt == "-cR")
        {
            ps->setConfRoot(args.takeAt(2));
        }
        else if (opt == "-vR")
        {
            ps->setVarRoot(args.takeAt(2));
        }
        else if (opt == "-C")
        {
            changelog = true;
        }
        else
        {
            help();
            return;
        }

        args.removeAt(1);
        opt = args.at(1);
    }
    
    ps->loadConfig();

    QString cmd = args.at(1);

    // Initialiser le système de paquet si on en a besoin
    if (cmd != "update")
    {
        if (!ps->init())
        {
            error();
            return;
        }
    }

    if (cmd == "help")
    {
        help();
    }
    else if (cmd == "version")
    {
        version();
    }
    else if (cmd == "search")
    {
        if (args.count() != 3)
        {
            help();
            return;
        }
        
        find(args.at(2));
    }
    else if (cmd == "showpkg")
    {
        if (args.count() != 3)
        {
            help();
            return;
        }
        
        showpkg(args.at(2), changelog);
    }
    else if (cmd == "update")
    {
        update();
    }
    else if (cmd == "add")
    {
        if (args.count() < 3)
        {
            help();
            return;
        }
        
        QStringList packages;

        for (int i=2; i<args.count(); ++i)
        {
            packages.append(args.at(i));
        }

        add(packages);
    }
    else if (cmd == "files")
    {
        if (args.count() != 3)
        {
            help();
            return;
        }
        
        showFiles(args.at(2));
    }
    else
    {
        help();
        return;
    }
}

void App::help()
{
    version();

    cout << endl;

    cout << "Usage : setup [options] <action> [arguments]" << endl;
    cout << "    help               Show help" << endl;
    cout << "    version            Show version" << endl;
    cout << "    search <pattern>   Show all the packages matching <pattern>" << endl;
    cout << "    showpkg <name>     Show the informations of the package <name>" << endl;
    cout << "    update             Update the packages' database" << endl;
    cout << "    add <packages>     Add packages (prepend them with \"-\" to remove)" << endl;
    cout << "    files <pkg>        Show the list of the files in the installed package <pkg>" << endl;

    cout << endl;
    cout << "Options :" << endl;
    cout << "    -S [off]           Enable (or disable) the installation of the suggests" << endl;
    cout << "    -I <num>           Number of parallel installs" << endl;
    cout << "    -D <num>           Number of parallel downloads" << endl;
    cout << "    -iR <install root> Root path of the installation (usually /, but it can be anything, for example to build a Logram From Scratch)" << endl;
    cout << "    -cR <conf root>    Root path of the installation files (usually /, the configuration files are read in <conf root>etc/lgrpkg)" << endl;
    cout << "    -vR <var root>     Root path of the temporary files and the database (usually /)" << endl;
    cout << "    -C                 Show changelog in showpkg" << endl;
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

void App::error()
{
    cout << COLOR("ERROR : ", "31");
    
    PackageError *err = ps->lastError();

    switch (err->type)
    {
        case PackageError::OpenFileError:
            cout << "Cannot open file ";
            break;
        case PackageError::MapFileError:
            cout << "Cannot map file ";
            break;
        case PackageError::ProcessError:
            cout << "Error when executing command ";
            break;
        case PackageError::DownloadError:
            cout << "Error when downloading file ";
            break;
        case PackageError::ScriptException:
            cout << "Error in the QtScript ";
            break;
        case PackageError::SignatureError:
            cout << "Invalid GPG signature of file ";
            break;
        case PackageError::SHAError:
            cout << "Bad SHA1 sum, file corrupted ";
            break;
        case PackageError::PackageNotFound:
            cout << "Package not found ";
            break;
        case PackageError::BadDownloadType:
            cout << "Bad download type, please report the bug ";
            break;
    }
    
    cout << COLOR(err->info, "35") << endl;
    
    if (!err->more.isEmpty())
    {
        cout << qPrintable(err->more) << endl;
    }
}

void App::progress(PackageSystem::Progress type, int done, int total, const QString &msg)
{
    if (done == total) return;
    
    // Afficher le message
    cout << COLOR("[" + QString::number(done) + "/" + QString::number(total) + "] ", "33");

    // Type
    switch (type)
    {
        case PackageSystem::Other:
            cout << COLOR(tr("Progression : "), "34");
            break;
            
        case PackageSystem::GlobalDownload:
            cout << COLOR(tr("Téléchargement de "), "34");
            break;
            
        case PackageSystem::Download:
            cout << COLOR(tr("Téléchargement de "), "34");
            break;
            
        case PackageSystem::UpdateDatabase:
            cout << COLOR(tr("Mise à jour de la base de donnée : "), "34");
            break;
            
        case PackageSystem::Install:
            cout << COLOR(tr("Installation de "), "34");
            break;
    }
    
    // Message
    cout << qPrintable(msg);
    cout << endl;

    // NOTE: Trouver une manière de ne pas spammer la sortie quand on télécharge quelque-chose (ça émet vraiment beaucoup de progress()). Plusieurs fichiers peuvent être téléchargés en même temps.
}