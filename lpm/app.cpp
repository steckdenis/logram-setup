/*
 * app.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include <package.h>

#include <QStringList>
#include <QTextCodec>
#include <QTranslator>
#include <QLocale>

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
    
    QTranslator translator;
    
    translator.load("libpackage_" + QLocale::system().name(), "/usr/share/qt4/translations");
    installTranslator(&translator);
    
    // Ouvrir le système de paquets
    ps = new PackageSystem(this);
    colored = true;

    connect(ps, SIGNAL(progress(Logram::Progress *)), 
            this, SLOT(progress(Logram::Progress *)));
    connect(ps, SIGNAL(communication(Logram::Package *, Logram::Communication *)), this, 
                  SLOT(communication(Logram::Package *, Logram::Communication *)));
    
    //Parser les arguments
    QStringList args = arguments();
    
    CHECK_ARGS(< 2)

    // Explorer les options
    QString opt = args.at(1).toLower();
    bool changelog = false;
    bool license = false;
    useDeps = true;
    useInstalled = true;
    depsTree = false;
    confirmMessages = true;
    installSuggests = false;
    verbose = false;

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
            installSuggests = true;
        }
        else if (opt == "-w")
        {
            colored = false;
        }
        else if (opt == "-v")
        {
            verbose = true;
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
        else if (opt == "-p")
        {
            ps->addPluginPath(args.takeAt(2));
        }
        else if (opt == "-c")
        {
            changelog = true;
        }
        else if (opt == "-l")
        {
            license = true;
        }
        else if (opt == "-nd")
        {
            useDeps = false;
        }
        else if (opt == "-ni")
        {
            useInstalled = false;
        }
        else if (opt == "-nc")
        {
            confirmMessages = false;
        }
        else if (opt == "-g")
        {
            depsTree = true;
        }
        else if (opt == "-t")
        {
            ps->setRunTriggers(false);
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
        
        showpkg(args.at(2), changelog, license);
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
    else if (cmd == "autoremove")
    {
        CHECK_ARGS(!= 2)
        
        autoremove();
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
    else if (cmd == "file")
    {
        CHECK_ARGS(!= 3)
        
        infoFile(args.at(2));
    }
    else if (cmd == "tag")
    {
        CHECK_ARGS(!= 5)
        
        if (args.at(2) == "package")
        {
            tagPackage(args.at(3), args.at(4));
        }
        else if (args.at(2) == "file")
        {
            tagFile(args.at(3), args.at(4));
        }
        else
        {
            help();
            return;
        }
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
#ifdef BUILD_TESTS
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
    else if (cmd == "test")
    {
        #define CHECK_RESULT(title, op) \
            cout << "\033[1m["; \
            if (!(op)) \
            { \
                cout << "\033[31m failed "; \
            } \
            else \
            { \
                cout << "\033[32m   ok   "; \
            } \
            cout << "\033[0m] " << title << endl;
            
        /* Test des fonctions helper de PackageSystem */
        // compareVersions
        CHECK_RESULT("0.1b2 > 0c0d9", ps->compareVersions("0.1b2", "0c0d9") == 1)
        CHECK_RESULT("14.7 < 14.7.1", ps->compareVersions("14.7", "14.7.1") == -1)
        CHECK_RESULT("1.1alpha1 = 1-1.1", ps->compareVersions("1.1alpha1", "1-1.1") == 0)
        CHECK_RESULT("1.0machin = 1machin0", ps->compareVersions("1.0machin", "1machin0") == 0)
        CHECK_RESULT("1.0.1~1 > 1.0~2", ps->compareVersions("1.0.1~1", "1.0~2") ==  1);
        CHECK_RESULT("2.3~4 < 2.3.1~1", ps->compareVersions("2.3~4", "2.3.1~1") == -1);
        
        // parseVersion
        int op;
        QByteArray name, version;
        
        op = ps->parseVersion("name", name, version);
        CHECK_RESULT("name : name ~ <null> Depend::NoVersion", op == Depend::NoVersion && name == "name" && version.isNull())
        
        op = ps->parseVersion("name>version", name, version);
        CHECK_RESULT("name>version : name ~ version Depend::Greater", name == "name" && version == "version" && op == Depend::Greater);
        
        op = ps->parseVersion("name>=version", name, version);
        CHECK_RESULT("name>=version : name ~ version Depend::GreaterOrEqual", name == "name" && version == "version" && op == Depend::GreaterOrEqual);
        
        op = ps->parseVersion("name<version", name, version);
        CHECK_RESULT("name<version : name ~ version Depend::Lower", name == "name" && version == "version" && op == Depend::Lower);
        
        op = ps->parseVersion("name<=version", name, version);
        CHECK_RESULT("name<=version : name ~ version Depend::LowerOrEqual", name == "name" && version == "version" && op == Depend::LowerOrEqual);
        
        op = ps->parseVersion("name!=version", name, version);
        CHECK_RESULT("name!=version : name ~ version Depend::NotEqual", name == "name" && version == "version" && op == Depend::NotEqual);
        
        op = ps->parseVersion("name=version", name, version);
        CHECK_RESULT("name=version : name ~ version Depend::Equal", name == "name" && version == "version" && op == Depend::Equal);
        
        // matchVersion
        CHECK_RESULT("1.2.3>=0.1 : yes", ps->matchVersion("1.2.3", "0.1", Depend::GreaterOrEqual) == true)
        CHECK_RESULT("2.1alpha8<=2.0alpha11 : no", ps->matchVersion("2.1alpha8", "2.0alpha11", Depend::LowerOrEqual) == false)
        CHECK_RESULT("1debian2ubuntu1!=1ubuntu2debian1 : no", ps->matchVersion("1debian2ubuntu1", "1ubuntu2debian1", Depend::NotEqual) == false)
        CHECK_RESULT("1.1<1.2 : yes", ps->matchVersion("1.1", "1.2", Depend::Lower) == true);
        CHECK_RESULT("7.9+git20100404~3>=7.9+git20100313 : yes", ps->matchVersion("7.9+git20100404~3", "7.9+git20100313", Depend::GreaterOrEqual) == true);
        CHECK_RESULT("2.8.3~2>=2.8.3 : yes", ps->matchVersion("2.8.3~2", "2.8.3", Depend::GreaterOrEqual) == true)
        
        // fileSizeFormat
        CHECK_RESULT("3 = 3 o", ps->fileSizeFormat(3) == "3 o");
        CHECK_RESULT("1024 = 1.00 Kio", ps->fileSizeFormat(1024) == "1.00 Kio");
        CHECK_RESULT("1280 = 1.25 Kio", ps->fileSizeFormat(1280) == "1.25 Kio");
        
        // dependString
    }
#endif
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

    rs = tr("Utilisation : lpm [options] <action> [arguments]\n"
            "    help               Afficher l'aide\n"
            "    version            Afficher la version\n"
            "    search <pattern>   Afficher tous les paquets dont le nom\n"
            "                       correspond à <pattern>\n"
            "    showpkg <name>     Affiche les informations du paquet <name>\n"
            "    getsource <name>   Télécharge le paquet source de <name>\n"
            "    update             Met à jour la base de donnée des paquets\n"
            "    add <packages>     Ajoute des paquets (préfixés de \"-\" pour les supprimer)\n"
            "    files <pkg>        Affiche la liste des fichiers installés par <pkg>\n"
            "    file <path|regex>  Affiche des informations sur le fichier <path>,\n"
            "                       s'il commence par \"/\". Sinon, <path> est considéré\n"
            "                       comme une expression régulière permettant de trouver\n"
            "                       la liste des fichiers correspondant à ce motif.\n"
            "    upgrade            Mise à jour des paquets. Lancez update avant.\n"
            "    autoremove         Supprimer automatiquement les paquets orphelins.\n"
            "    tag [file|package] Tag les paquets correspondants à <pkg> (p>=v, etc) avec\n"
            "        <pkg|fl> <tag> le tag <tag>. Si <tag> commence par \"-\", alors retirer\n"
            "                       ce tag. Si <pkg> est un fichier (/usr/truc, ou mac*n),\n"
            "                       alors c'est ce seront les fichiers correspondants au\n"
            "                       motif qui seront taggués. <fl> peut être de la forme\n"
            "                       \"pkg:fl\", ce qui permet de ne taguer que les\n"
            "                       fichiers correspondants à <fl> et appartenant à <pkg>\n"
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
            "    -S                 Active l'installation des suggestions.\n"
            "    -V                 Mode verbeux : affiche la sortie des scripts lancés\n"
            "    -nD                Ignorer les dépendances : installer les paquets demandés\n"
            "                       et uniquement eux.\n"
            "    -nI                Ignorer les paquets installés, générer tout l'arbre de\n"
            "                       dépendances.\n"
            "    -nC                Ne pas confirmer les messages des paquets par Entrée.\n"
            "    -G                 Sortie dans stdout la représentation Graphviz de l'arbre\n"
            "                       des dépendances.\n"
            "    -I <num>           Définit le nombre de téléchargements en parallèle.\n"
            "    -D <num>           Définit le nombre d'installations en parallèle.\n"
            "    -iR <install root> Chemin d'installation racine (\"/\" par défaut).\n"
            "                       Sert à installer un «Logram dans le Logram».\n"
            "    -cR <conf root>    Chemin racine de la configuration (\"/\" par défaut).\n"
            "    -vR <var root>     Chemin racine des fichiers temporaires (\"/\" par défaut).\n"
            "    -P path            Ajoute un chemin à inspecter pour trouver les plugins\n"
            "                       de vérification des paquets (lpm binaries).\n"
            "    -C                 Affiche l'historique des modifications d'un paquet\n"
            "                       quand utilisé avec showpkg.\n"
            "    -L                 Affiche la licence d'un paquet quand utilisé avec showpkg.\n"
            "    -T                 Désactive l'exécution des triggers.\n"
            "    -W                 Désactive les couleurs dans la sortie de LPM.\n");
   
    cout << qPrintable(rs);
}

void App::version()
{
    QString rs;
    
    rs = tr("Logram Package Manager " VERSION "\n"
            "\n"
            "Logram is free software; you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation; either version 3 of the License, or\n"
            "(at your option) any later version.\n");
  
    cout << qPrintable(rs);
}

QString App::psError(PackageError *err)
{
    QString rs;

    switch (err->type)
    {
        case PackageError::OpenFileError:
            rs = tr("Impossible d'ouvrir le fichier ");
            break;
            
        case PackageError::MapFileError:
            rs = tr("Impossible de mapper le fichier ");
            break;
            
        case PackageError::ProcessError:
            rs = tr("Erreur dans la commande ");
            break;
            
        case PackageError::DownloadError:
            rs = tr("Impossible de télécharger ");
            break;
            
        case PackageError::ScriptException:
            rs = tr("Erreur dans le QtScript ");
            break;
            
        case PackageError::SignatureError:
            rs = tr("Mauvaise signature GPG du fichier ");
            break;
            
        case PackageError::SHAError:
            rs = tr("Mauvaise somme SHA1, fichier corrompu : ");
            break;
            
        case PackageError::PackageNotFound:
            rs = tr("Paquet inexistant : ");
            break;
            
        case PackageError::BadDownloadType:
            rs = tr("Mauvais type de téléchargement, vérifier sources.list : ");
            break;
            
        case PackageError::OpenDatabaseError:
            rs = tr("Impossible d'ouvrir la base de donnée : ");
            break;
            
        case PackageError::QueryError:
            rs = tr("Erreur dans la requête : ");
            break;
            
        case PackageError::SignError:
            rs = tr("Impossible de vérifier la signature : ");
            break;
            
        case PackageError::InstallError:
            rs = tr("Impossible d'installer le paquet ");
            break;
            
        case PackageError::ProgressCanceled:
            rs = tr("Opération annulée : ");
            break;
    }
    
    return rs;
}

void App::error()
{
    PackageError *err = ps->lastError();
    
    cout << COLOR(tr("ERREUR : "), "31");
    
    if (err == 0)
    {
        cout << COLOR(tr("Pas d'erreur ou erreur inconnue"), "35") << endl;
        return;
    }
    
    cout << COLOR(psError(err), "35") << qPrintable(err->info) << endl;
    
    if (!err->more.isEmpty())
    {
        cout << qPrintable(err->more) << endl;
    }
}

void App::clean()
{
    delete ps;
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
        int percent = (progress->total == 0 ? 0 : progress->current * 100 / progress->total);
        
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
        
        int caracount = (progress->total == 0 ? 0 : progress->current * maxcara / progress->total);
        
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
    else if (progress->type == Progress::ProcessOut && !verbose)
    {
        return;
    }
    
    // Afficher le message
    int usedwidth = 0;
    QString s = "[" + QString::number(progress->current + 1) + "/" + QString::number(progress->total) + "] ";
    
    cout << COLOR(s, "33");
    usedwidth += s.size();

    // Type
    Package *pkg;
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
            // progress->data est un Package, nous pouvons nous en servir pour afficher plein de choses
            pkg = static_cast<Package *>(progress->data);
            
            if (pkg == 0)
            {
                s = tr("Opération sur ");
            }
            else
            {
                switch (pkg->action())
                {
                    case Solver::Install:
                        s = tr("Installation de ");
                        break;
                    case Solver::Remove:
                        s = tr("Suppression de  ");
                        break;
                    case Solver::Purge:
                        s = tr("Purge de        ");
                        break;
                    case Solver::Update:
                        s = tr("Mise à jour de  ");
                        break;
                    default:
                        // N'arrive jamais, juste pour que GCC soit content
                        break;
                }
            }
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
            
        case Progress::Trigger:
            s = tr("Exécution du trigger ");
            break;
            
        default:
            return;
    }
    
    cout << COLOR(s, "34");
    usedwidth += s.size();
    
    // Message
    if (progress->type != Progress::PackageProcess)
    {
        cout << qPrintable(progress->info);
        
        usedwidth += progress->info.size();
        
        if (!progress->more.isNull())
        {
            cout << ", " << qPrintable(progress->more);
            
            usedwidth += progress->more.size() + 2;
        }
    }
    else
    {
        pkg = static_cast<Package *>(progress->data);
        Package *opkg = 0;
        
        if (pkg == 0)
        {
            // Ok, on ne sait rien
            cout << qPrintable(progress->info);
            usedwidth += progress->info.size();
        }
        else
        {
            // Nom et version en couleur, plus info en cas de mise à jour
            cout << COLOR(pkg->name(), "31")
                 << '~'
                 << COLOR(pkg->version(), "32");
                 
            usedwidth += pkg->name().size() + 1 + pkg->version().size();
            
            if (pkg->action() == Solver::Update && (opkg = (Package *)pkg->upgradePackage()) != 0)
            {
                // Rajouter "vers <version>"
                s = tr("vers", "Mise à jour de <paquet>~<version> vers <nouvelle version>");
                
                cout << ' '
                     << qPrintable(s)
                     << ' '
                     << COLOR(opkg->version(), "32");
                     
                usedwidth += s.size() + 2 + opkg->version().size();
            }
        }
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

#include "app.moc"