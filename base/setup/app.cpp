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
    colored = true;

    connect(ps, SIGNAL(progress(PackageSystem::Progress, int, int, const QString &)), this, SLOT(progress(PackageSystem::Progress, int, int, const QString &)));
    
    //Parser les arguments
    QStringList args = arguments();

    if (args.count() == 1)
    {
        help();
        return;
    }

    // Explorer les options
    QString opt = args.at(1).toLower();
    bool changelog = false;

    while (opt.startsWith('-'))
    {
        if (opt == "-s")
        {
            bool isug = true;

            if (args.at(2) == "off")
            {
                isug = false;
                args.removeAt(1); // normal que ce soit 1 pas 2
            }

            ps->setInstallSuggests(isug);
        }
        else if (opt == "-w")
        {
            colored = false;
        }
        else if (opt == "-i")
        {
            ps->setParallelInstalls(args.takeAt(2).toInt());
        }
        else if (opt == "-d")
        {
            ps->setParallelDownloads(args.takeAt(2).toInt());
        }
        else if (opt == "-r")
        {
            QString root = args.takeAt(2);
            
            ps->setInstallRoot(root);
            ps->setConfRoot(root);
            ps->setVarRoot(root);
        }
        else if (opt == "-ir")
        {
            ps->setInstallRoot(args.takeAt(2));
        }
        else if (opt == "-cr")
        {
            ps->setConfRoot(args.takeAt(2));
        }
        else if (opt == "-vr")
        {
            ps->setVarRoot(args.takeAt(2));
        }
        else if (opt == "-c")
        {
            changelog = true;
        }
        else
        {
            help();
            return;
        }

        args.removeAt(1);
        opt = args.at(1).toLower();
    }
    
    ps->loadConfig();

    QString cmd = args.at(1).toLower();

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
    
    QString rs;

    rs = tr("Utilisation : setup [options] <action> [arguments]\n"
            "    help               Afficher l'aide\n"
            "    version            Afficher la version\n"
            "    search <pattern>   Afficher tous les paquets dont le nom\n"
            "                       correspond à <pattern>\n"
            "    showpkg <name>     Affiche les informations du paquet <name>\n"
            "    update             Met à jour la base de donnée des paquets\n"
            "    add <packages>     Ajoute des paquets (préfixés de \"-\" pour les supprimer)\n"
            "    files <pkg>        Affiche la liste des fichiers installés par <pkg>\n"
            "\n"
            "Options (insensible à la casse) :\n"
            "    -S [off]           Active (on) ou pas (off) l'installation des suggestions\n"
            "    -I <num>           Définit le nombre de téléchargements en parallèle\n"
            "    -D <num>           Définit le nombre d'installations en parallèle\n"
            "    -iR <install root> Chemin d'installation racine (\"/\" par défaut).\n"
            "                       Sert à installer un «Logram dans le Logram»\n"
            "    -cR <conf root>    Chemin racine de la configuration (\"/\" par défaut).\n"
            "    -vR <var root>     Chemin racine des fichiers temporaires (\"/\" par défaut)\n"
            "    -C                 Affiche l'historique des modifications d'un paquet\n"
            "                       quand utilisé avec showpkg\n"
            "    -W                 Désactive les couleurs dans la sortie de Setup\n");
   
    cout << qPrintable(rs);
}

void App::version()
{
    QString rs;
    
    rs = tr("Logram Setup " VERSION "\n"
            "\n"
            "Logram is free software; you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation; either version 3 of the License, or\n"
            "(at your option) any later version.\n");
  
    cout << qPrintable(rs);
}

void App::error()
{
    cout << COLOR(tr("ERREUR : "), "31");
    
    PackageError *err = ps->lastError();

    switch (err->type)
    {
        case PackageError::OpenFileError:
            cout << qPrintable(tr("Impossible d'ouvrir le fichier "));
            break;
        case PackageError::MapFileError:
            cout << qPrintable(tr("Impossible de mapper le fichier "));
            break;
        case PackageError::ProcessError:
            cout << qPrintable(tr("Erreur dans la commande "));
            break;
        case PackageError::DownloadError:
            cout << qPrintable(tr("Impossible de télécharger "));
            break;
        case PackageError::ScriptException:
            cout << qPrintable(tr("Erreur dans le QtScript "));
            break;
        case PackageError::SignatureError:
            cout << qPrintable(tr("Mauvaise signature GPG du fichier "));
            break;
        case PackageError::SHAError:
            cout << qPrintable(tr("Mauvaise somme SHA1, fichier corrompu : "));
            break;
        case PackageError::PackageNotFound:
            cout << qPrintable(tr("Paquet inexistant : "));
            break;
        case PackageError::BadDownloadType:
            cout << qPrintable(tr("Mauvais type de téléchargement, vérifier sources.list : "));
            break;
    }
    
    cout << COLOR(err->info, "35") << endl;
    
    if (!err->more.isEmpty())
    {
        cout << qPrintable(err->more) << endl;
    }
    
    // Plus besoin de l'erreur
    delete err;
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