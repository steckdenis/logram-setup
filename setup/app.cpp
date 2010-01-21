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

#include <QThread>

#include <iostream>
using namespace std;
using namespace Logram;

#include <sys/ioctl.h>
#include <stdlib.h> 
#include <stdio.h> 

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
                  << "export" << "testprogress";
    
    if (!noInitCommand.contains(cmd))
    {
        if (!ps->init())
        {
            error();
            return;
        }
    }
    
    // Obtenir la taille de la console
    struct winsize ws;
    
    if (ioctl(0, TIOCGWINSZ, &ws) != 0)
    {
        width = 80;
    }
    else
    {
        width = ws.ws_col;
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
    else if (cmd == "getsource")
    {
        CHECK_ARGS(!= 3)
        
        getsource(args.at(2));
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
    else if (cmd == "testprogress")
    {
        // Envoyer des progressions
        int p1 = ps->startProgress(Progress::Download, 1024*1024);      // 1Mio
        int p2 = ps->startProgress(Progress::Download, 1536*1024);      // 1,5Mio
        
        int c1 = 0;
        int c2 = 0;
        
        #define CHK(x) if (!x) return;
        
        while (true)
        {
            CHK(ps->sendProgress(p1, c1, "http://archive.logram-project.org/pool/i/initng~0.6.99+git20091223~1.i686.lpk"));
            usleep(50000);
            CHK(ps->sendProgress(p2, c2, "http://archive.logram-project.org/pool/a/amarok~2.2.2~5.i686.lpk"));
            
            c1 += 21480 + (rand() % 2200);    // entre 200 et 220 Kio/s
            c2 += 11240 + (rand() % 2200);    // entre 100 et 120 Kio/s
            
            if (c1 >= 1024*1024)
            {
                c1 = 0;
                ps->endProgress(p1);
                
                p1 = ps->startProgress(Progress::Download, 1024*1024);      // 1Mio
            }
            
            if (c2 >= 1536*1024)
            {
                c2 = 0;
                ps->endProgress(p2);
                
                p2 = ps->startProgress(Progress::Download, 1536*1024);      // 1,5Mio
            }
            
            // Attendre une seconde
            usleep(50000);
        }
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
            "    getsource <name>   Télécharge le paquet source de <name>\n"
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
    
    if (err == 0)
    {
        // Interruption sans erreur, ou erreur inconnue
        cout << qPrintable(tr("Erreur inconnue ou pas d'erreur")) << endl;
        return;
    }

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
            
        case PackageError::ProgressCanceled:
            cout << qPrintable(tr("Opération annulée : "));
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

void App::updatePgs(Progress *p)
{   
    if (progresses.count() == 0)
    {
        return;
    }
    
    // Temps de maintenant
    QTime now = QTime::currentTime();
    
    for (int i=0; i<progresses.count(); ++i)
    {
        Progress *progress = progresses.at(i);
        
        if (progress == 0)
        {
            // Slot vide
            for (int j=0; j<width; j++)
            {
                cout << ' ';
            }
            
            cout << endl;
            
            for (int j=0; j<width; j++)
            {
                cout << ' ';
            }
            
            cout << endl;
            
            continue;
        }
        
        bool isDownload = (progress->type == Progress::Download);
        const QTime &oldTime = tProgresses.at(i);
        
        // Le pourcentage
        int percent = progress->current * 100 / progress->total;
        
        cout << ' ';
        cout << COLOR(QString::number(percent).rightJustified(3, ' ', true) + '%', "33");
        cout << " [";
        
        // Obtenir le nombre de caractères, sachant que ça va
        // de 0 à width-20
        // Ligne : | 100% [=== .. ===] 1023.12 Mio 1023.13 Kib/s |
        // Ligne : | 100% [=== ..                        .. ===] |
        
        int maxcara;
        
        if (isDownload)
        {
            maxcara = width-34;
        }
        else
        {
            maxcara = width-9;
        }
        
        int caracount = progress->current * maxcara / progress->total;
        
        if (caracount > maxcara) caracount = maxcara;
        
        int j;
        
        for (j=0; j<caracount; ++j)
        {
            cout << '=';
        }
        
        for (; j<maxcara; ++j)
        {
            cout << ' ';
        }
        
        cout << "] ";
        
        if (isDownload)
        {
            // Taille téléchargée
            QString dlsize = ps->fileSizeFormat(progress->current);
            
            cout << COLOR(dlsize.rightJustified(11, ' ', true), "34");
            
            // Vitesse de téléchargement
            int timedelta = oldTime.msecsTo(now);
            int bytedelta = progress->current - progress->old;
            
            // Mettre à jour l'heure de dernier passage, si c'est bien nous qui nous sommes mis à jour
            if (p == progress)
            {
                tProgresses[i] = now;
            }
            
            // Nombre de bytes par seconde
            int bps;
            
            if (timedelta == 0)
            {
                bps = 0;
            }
            else
            {
                bps = bytedelta * 1000 / timedelta;
            }
            
            if (bps < 0) bps = 0;
            
            QString sdelta = ps->fileSizeFormat(bps);
            
            cout << COLOR(sdelta.rightJustified(11, ' ', true) + "/s", "32");
        }
            
        cout << endl;
        
        // Deuxième ligne : nom de fichier
        int ln = progress->info.size();
        
        if (ln > width)
        {
            cout << qPrintable(progress->info.left(width));
        }
        else
        {
            int before = (width - ln) / 2;
            
            for (int j=0; j<before; ++j)
            {
                cout << ' ';
            }
            
            cout << qPrintable(progress->info);
            
            before += ln;   // Taille totale déjà écrite
            before = width - before;    // Ce qui reste
            
            for (int j=0; j<before; j++)
            {
                cout << ' ';
            }
        }
        
        cout << endl;
    }
    
    if (p != 0)
    {
        // Quand on nettoie, on envoie 0 dans p
        cout << "\033[" << (progresses.count() * 2) << "A";
    }
    
    cout.flush();
}

void App::progress(Progress *progress)
{
    if (progress->action == Progress::Create)
    {
        // Si le type est un téléchargement, créer une barre de progression
        if ((progress->type == Progress::Download || progress->type == Progress::Compressing) && !progresses.contains(progress))
        {
            // Trouver un emplacement libre
            for (int i=0; i<progresses.count(); ++i)
            {
                if (progresses.at(i) == 0)
                {
                    progresses[i] = progress;
                    tProgresses[i] = QTime::currentTime();
                    
                    updatePgs(progress);
                    return;
                }
            }
            
            // On n'a rien trouvé
            progresses.append(progress);
            tProgresses.append(QTime::currentTime());
            
            updatePgs(progress);
        }
        
        return;
    }
    else if (progress->action == Progress::End)
    {
        // Si un téléchargement, supprimer la barre de progression
        if (progress->type == Progress::Download || progress->type == Progress::Compressing)
        {
            int index = progresses.indexOf(progress);
            
            if (index != -1)
            {
                progresses[index] = 0;
            }
            
            updatePgs(progress);
        }
        
        delete progress;
        return;
    }
    
    if (progress->type == Progress::Download || progress->type == Progress::Compressing)
    {
        updatePgs(progress);
        return;
    }
    
    // Afficher le message
    int usedwidth = 0;
    QString s = "[" + QString::number(progress->current + 1) + "/" + QString::number(progress->total) + "] ";
    
    cout << COLOR(s, "33");
    
    usedwidth += s.size();

    // Type
    switch (progress->type)
    {
        case Progress::Other:
            s = tr("Progression : ");
            break;
            
        case Progress::GlobalDownload:
            s = tr("Téléchargement de ");
            break;
            
        case Progress::UpdateDatabase:
            s = tr("Mise à jour de la base de donnée : ");
            break;
            
        case Progress::PackageProcess:
            s = tr("Opération sur ");
            break;
            
        case Progress::ProcessOut:
            s = tr("Sortie du processus : ");
            break;
            
        case Progress::GlobalCompressing:
            s = tr("Création du paquet ");
            break;
            
        case Progress::Compressing:
            s = tr("Compression de ");
            break;
            
        case Progress::Including:
            s = tr("Inclusion de ");
            break;
            
        case Progress::Exporting:
            s = tr("Export de la distribution ");
            break;
            
        default:
            return;
    }
    
    cout << COLOR(s, "34");
    usedwidth += s.size();
    
    // Message
    cout << qPrintable(progress->info);
    
    usedwidth += progress->info.size();
    
    if (!progress->more.isNull())
    {
        cout << ", " << qPrintable(progress->more);
        
        usedwidth += progress->more.size() + 2;
    }
    
    // Effacer le reste de la largeur de l'écran
    int diff = width - usedwidth;
    
    if (diff < 0) diff = 0;
    
    for (int i=0; i<diff; ++i)
    {
        cout << ' ';
    }
    
    cout << endl;

    // Mettre à jour les barres de progression
    updatePgs(progress);
}