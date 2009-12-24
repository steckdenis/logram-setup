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
using namespace Logram;

#define CHECK_ARGS(cond) \
    if (args.count() cond) \
    { \
        help(); \
        return; \
    }

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    // Initialiser
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    // Ouvrir le système de paquets
    ps = new PackageSystem(this);
    colored = true;

    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
    
    //Parser les arguments
    QStringList args = arguments();
    
    CHECK_ARGS(< 2)

    // Explorer les options
    QString opt = args.at(1).toLower();
    bool changelog = false;

    while (opt.startsWith('-'))
    {
        if (args.count() < 3)
        {
            // Soit un paramètre, ou alors la commande, mais toujours minimum 3
            help();
            return;
        }
        
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
        
        if (args.count() >= 2)
        {
            opt = args.at(1).toLower();
        }
        else
        {
            help();
            return;
        }
    }
    
    ps->loadConfig();
    
    CHECK_ARGS(< 2)

    QString cmd = args.at(1).toLower();

    // Initialiser le système de paquet si on en a besoin
    QStringList noInitCommand;
    
    noInitCommand << "update" << "download" << "build" << "binaries" << "include"
                  << "export";
    
    if (!noInitCommand.contains(cmd))
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
        CHECK_ARGS(!= 3)
        
        find(args.at(2));
    }
    else if (cmd == "showpkg")
    {
        CHECK_ARGS(!= 3)
        
        showpkg(args.at(2), changelog);
    }
    else if (cmd == "update")
    {
        CHECK_ARGS(!= 2)
        
        update();
    }
    else if (cmd == "upgrade")
    {
        CHECK_ARGS(!= 2)
        
        upgrade();
    }
    else if (cmd == "add")
    {
        CHECK_ARGS(< 3)
        
        QStringList packages;

        for (int i=2; i<args.count(); ++i)
        {
            packages.append(args.at(i));
        }

        add(packages);
    }
    else if (cmd == "files")
    {
        CHECK_ARGS(!= 3)
        
        showFiles(args.at(2));
    }
    else if (cmd == "download")
    {
        CHECK_ARGS(!= 3)
        
        sourceDownload(args.at(2));
    }
    else if (cmd == "build")
    {
        CHECK_ARGS(!= 3)
        
        sourceBuild(args.at(2));
    }
    else if (cmd == "binaries")
    {
        CHECK_ARGS(!= 3)
        
        sourceBinaries(args.at(2));
    }
    else if (cmd == "include")
    {
        CHECK_ARGS(< 3)
        
        args.removeAt(0);   // retirer le 0
        args.removeAt(0);   // retirer le 1
        
        include(args);      // Envoyer à partir du 2
    }
    else if (cmd == "export")
    {
        args.removeAt(0);
        args.removeAt(0);
        
        // Pas de vérifications, on peut parfaitement ne rien envoyer, et donc
        // exporter toutes les distributions
        
        exp(args);
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
            "    upgrade            Mise à jour des paquets. Lancez update avant.\n"
            "\n"
            "Commandes pour la gestion des sources :\n"
            "    download <src>     Télécharge la source du paquet dont <src> est le\n"
            "                       metadata.xml.\n"
            "    build <src>        Compile la source spécifiée par le metadata.xml <src>.\n"
            "    binaries <src>     Créer les .lpk binaires du metadata.xml <src>.\n"
            "\n"
            "Commandes pour la gestion des dépôts :\n"
            "    include <pkg>      Inclus le paquet <pkg> dans le dépôt config/repo.conf\n"
            "    export <distros>   Exporte la liste des paquets et créer les fichiers\n"
            "                       packages.lzma et translate.lang.lzma pour les\n"
            "                       distributions spécifiées.\n"
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
            
        case PackageError::OpenDatabaseError:
            cout << qPrintable(tr("Impossible d'ouvrir la base de donnée : "));
            break;
            
        case PackageError::QueryError:
            cout << qPrintable(tr("Erreur dans la requête : "));
            break;
            
        case PackageError::SignError:
            cout << qPrintable(tr("Impossible de vérifier la signature : "));
            break;
            
        case PackageError::InstallError:
            cout << qPrintable(tr("Impossible d'installer le paquet "));
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

void App::progress(Progress *progress)
{
    if (progress->action == Progress::Create)
    {
        return;
    }
    else if (progress->action == Progress::End)
    {
        delete progress;
        return;
    }
    
    // Afficher le message
    cout << COLOR("[" + QString::number(progress->current + 1) + "/" + QString::number(progress->total) + "] ", "33");

    // Type
    switch (progress->type)
    {
        case Progress::Other:
            cout << COLOR(tr("Progression : "), "34");
            break;
            
        case Progress::GlobalDownload:
            cout << COLOR(tr("Téléchargement de "), "34");
            break;
            
        case Progress::Download:
            cout << COLOR(tr("Téléchargement de "), "34");
            break;
            
        case Progress::UpdateDatabase:
            cout << COLOR(tr("Mise à jour de la base de donnée : "), "34");
            break;
            
        case Progress::PackageProcess:
            cout << COLOR(tr("Opération sur "), "34");
            break;
            
        case Progress::ProcessOut:
            cout << COLOR(tr("Sortie du processus : "), "34");
            break;
            
        case Progress::GlobalCompressing:
            cout << COLOR(tr("Création du paquet "), "34");
            break;
            
        case Progress::Compressing:
            cout << COLOR(tr("Compression de "), "34");
            break;
            
        case Progress::Including:
            cout << COLOR(tr("Inclusion de "), "34");
            break;
            
        case Progress::Exporting:
            cout << COLOR(tr("Export de la distribution "), "34");
            break;
    }
    
    // Message
    cout << qPrintable(progress->info);
    
    if (!progress->more.isNull())
    {
        cout << ", " << qPrintable(progress->more);
    }
    
    cout << endl;

    // NOTE: Trouver une manière de ne pas spammer la sortie quand on télécharge quelque-chose (ça émet vraiment beaucoup de progress()). Plusieurs fichiers peuvent être téléchargés en même temps.
}