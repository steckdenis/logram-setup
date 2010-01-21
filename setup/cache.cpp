/*
 * cache.cpp
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

#include <logram/databasepackage.h>
#include <logram/packagemetadata.h>

#include <QString>

#include <iostream>
using namespace std;

using namespace Logram;

void App::update()
{
    // Mettre à jour la base de donnée
    if (!ps->update())
    {
        error();
    }
}

void App::find(const QString &pattern)
{
    QList<int> pkgs;
    
    if (!ps->packagesByName("*" + pattern + "*", pkgs))
    {
        error();
        return;
    }

    // Afficher joliment les paquets
    for (int i=0; i<pkgs.count(); ++i)
    {
        int id = pkgs.at(i);
        
        DatabasePackage *pkg = ps->package(id);
        
        int sz = 0;
        
        if (pkg->name().length() > 20)
        {
            sz = pkg->name().length() - 20;
        }
        
        sz = 20 - sz;
        
        cout
        << COLOR(pkg->name().leftJustified(20, ' ', false), "33") << ' '
        << COLOR(pkg->version().leftJustified(sz, ' ', false), "32") << ' '
        << qPrintable(pkg->shortDesc())
        << endl;
        
        delete pkg;
    }
}

void App::showFiles(const QString &packageName)
{
    QStringList files;
    
    if (!ps->filesOfPackage(packageName, files))
    {
        error();
        return;
    }

    foreach(const QString &file, files)
    {
        cout << qPrintable(file) << endl;
    }
}

static QStringList pkgFlags(Package *pkg)
{
    QStringList rs;
    QString tmp;
    int flags = pkg->flags();
    
    // Intégration KDE
    int kdeintegration = (flags & PACKAGE_FLAG_KDEINTEGRATION);
    
    tmp = App::tr("Intégration à KDE    : ");
    
    switch (kdeintegration)
    {
        case 0:
            tmp += App::tr("pas intégré");
            break;
        case 1:
            tmp += App::tr("utilisable");
            break;
        case 2:
            tmp += App::tr("bien intégré");
            break;
        case 3:
            tmp += App::tr("parfaitement intégré");
            break;
    }
    
    rs.append(tmp);
    
    #define YESNO(flag, string) \
        rs.append( \
            App::tr(string) \
            + ((flags & flag) != 0 ? syes : sno));
    
    QString syes = App::tr("Oui");
    QString sno = App::tr("Non");
    
    // Graphique
    YESNO(PACKAGE_FLAG_GUI,         "Paquet graphique         : ")
    
    // Ne pas mettre à jour
    YESNO(PACKAGE_FLAG_DONTUPDATE,  "Ne pas mettre à jour     : ")
    
    // Ne pas installer
    YESNO(PACKAGE_FLAG_DONTINSTALL, "Ne pas installer         : ")
    
    // Ne pas supprimer
    YESNO(PACKAGE_FLAG_DONTREMOVE,  "Ne pas supprimer         : ")
    
    // Nécessite une CLUF
    YESNO(PACKAGE_FLAG_HASCLUF,     "License à approuver      : ")
    
    // Nécessite un redémarrage
    YESNO(PACKAGE_FLAG_NEEDSREBOOT, "Nécessite un redémarrage : ")
    
    
    return rs;
}

void App::showpkg(const QString &name, bool changelog)
{
    QString n = name.section('=', 0, 0);
    QString v;

    if (name.contains('='))
    {
        v = name.section('=', 1, -1);
    }

    Package *pkg;
    
    if (!ps->package(n, v, pkg))
    {
        error();
        return;
    }
    
    // Précharger les métadonnées
    PackageMetaData *metadata = pkg->metadata();
    
    if (metadata == 0)
    {
        error();
        return;
    }
    
    metadata->setCurrentPackage(pkg->name());

    // Status du paquet
    QString status;

    if (pkg->status() == PACKAGE_STATE_NOTINSTALLED)
    {
        status = tr("Non-installé");
    }
    else if (pkg->status() == PACKAGE_STATE_INSTALLED)
    {
        status = tr("Installé");
    }
    else
    {
        status = tr("Supprimé");
    }

    // Afficher les informations
    cout << COLOR(tr("Nom                 : "), "33") << COLOR(pkg->name(), "34") << endl;
    cout << COLOR(tr("Version             : "), "33") << qPrintable(pkg->version()) << endl;
    cout << COLOR(tr("Titre               : "), "33") << qPrintable(metadata->packageTitle()) << endl;
    cout << COLOR(tr("Drapeaux            : "), "33");
    
    QStringList flags = pkgFlags(pkg);
    bool fFirst = true;
    
    foreach (const QString &flag, flags)
    {
        if (!fFirst)
        {
            cout << "                      ";
        }
        
        fFirst = false;
        
        cout << "o " << qPrintable(flag) << endl;
    }
    
    cout << COLOR(tr("Section             : "), "33") << qPrintable(pkg->section()) << endl;
    
    Repository repo;
    ps->repository(pkg->repo(), repo);
    
    cout << COLOR(tr("Distribution        : "), "33") << qPrintable(pkg->distribution()) << endl;
    cout << COLOR(tr("Dépôt d'origine     : "), "33") << qPrintable(repo.description) << " (" << qPrintable(pkg->repo()) << ')' << endl;
    cout << COLOR(tr("Status              : "), "33") << qPrintable(status) << endl;
    
    if (pkg->status() == PACKAGE_STATE_INSTALLED)
    {
        cout << COLOR(tr("Date d'installation : "), "33") << qPrintable(pkg->installedDate().toString(Qt::DefaultLocaleLongDate)) << endl;
        // TODO: Uid vers nom d'utilisateur
    }
    else if (pkg->status() == PACKAGE_STATE_REMOVED)
    {
        cout << COLOR(tr("Date de suppression : "), "33") << qPrintable(pkg->installedDate().toString(Qt::DefaultLocaleLongDate)) << endl;
        // TODO: Uid vers nom d'utilisateur
    }
    
    cout << COLOR(tr("Téléchargement      : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->downloadSize())) << endl;
    cout << COLOR(tr("Taille installée    : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->installSize())) << endl;
    cout << COLOR(tr("Paquet source       : "), "33") << qPrintable(pkg->source()) << endl;
    cout << COLOR(tr("Adresse de l'auteur : "), "33") << qPrintable(pkg->upstreamUrl()) << endl;
    cout << COLOR(tr("Licence             : "), "33") << qPrintable(pkg->license()) << endl;
    cout << COLOR(tr("Mainteneur          : "), "33") << qPrintable(pkg->maintainer()) << endl;
    cout << COLOR(tr("Description courte  : "), "33") << qPrintable(pkg->shortDesc()) << endl;
    
    cout << endl << COLOR(tr("Description longue : "), "35") << endl << endl;
    printIndented(metadata->packageDescription().toUtf8(), 4);
    cout << endl;

    // Afficher les dépendances
    QList<Depend *> deps = pkg->depends();

    if (deps.count() != 0)
    {
        cout << endl;
        cout << COLOR(tr("Dépendances :"), "35") << endl;
        cout << qPrintable(tr("Légende : "))
            << COLOR(tr("D: Dépend "), "31")
            << COLOR(tr("S: Suggère "), "32")
            << COLOR(tr("C: Conflit "), "33")
            << COLOR(tr("P: Fourni "), "34")
            << COLOR(tr("R: Remplace "), "35")
            << COLOR(tr("N: Est requis par"), "37")
        << endl << endl;

        for (int i=0; i<deps.count(); ++i)
        {
            Depend *dep = deps.at(i);

            switch (dep->type())
            {
                case DEPEND_TYPE_DEPEND:
                    cout << " D: ";
                    cout << COLOR(dep->name(), "31");
                    break;
                case DEPEND_TYPE_SUGGEST:
                    cout << " S: ";
                    cout << COLOR(dep->name(), "32");
                    break;
                case DEPEND_TYPE_CONFLICT:
                    cout << " C: ";
                    cout << COLOR(dep->name(), "33");
                    break;
                case DEPEND_TYPE_PROVIDE:
                    cout << " P: ";
                    cout << COLOR(dep->name(), "34");
                    break;
                case DEPEND_TYPE_REPLACE:
                    cout << " R: ";
                    cout << COLOR(dep->name(), "35");
                    break;
                case DEPEND_TYPE_REVDEP:
                    cout << " N: ";
                    cout << COLOR(dep->name(), "37");
                    break;
            }

            if (dep->op() != DEPEND_OP_NOVERSION)
            {
                switch (dep->op())
                {
                    case DEPEND_OP_EQ:
                        cout << " (= ";
                        break;
                    case DEPEND_OP_GREQ:
                        cout << " (>= ";
                        break;
                    case DEPEND_OP_GR:
                        cout << " (> ";
                        break;
                    case DEPEND_OP_LOEQ:
                        cout << " (<= ";
                        break;
                    case DEPEND_OP_LO:
                        cout << " (< ";
                        break;
                    case DEPEND_OP_NE:
                        cout << " (!= ";
                        break;
                }

                cout << qPrintable(dep->version()) << ")";
            }

            cout << endl;
        }
    }

    // Versions disponibles
    if (pkg->origin() == Package::Database)
    {
        DatabasePackage *dpkg = (DatabasePackage*)pkg;
        
        cout << endl << COLOR(tr("Versions disponibles : "), "35") << endl;
        cout << qPrintable(tr("Légende : * = Disponible, I = installée, R = supprimée")) << endl << endl;

        QList<Package *> vers = dpkg->versions();

        foreach(Package *ver, vers)
        {
            if (ver->status() == PACKAGE_STATE_INSTALLED)
            {
                cout << COLORC("  I ", "34");
            }
            else if (ver->status() == PACKAGE_STATE_REMOVED)
            {
                cout << COLORC("  R ", "31");
            }
            else
            {
                cout << "  * ";
            }

            cout << COLOR(ver->version().leftJustified(26, ' ', true), "32") << ' '
                << qPrintable(ver->shortDesc()) << endl;
        }

        cout << endl;
    }
    
    if (changelog)
    {
        cout << COLOR(tr("Historique des versions : "), "35") << endl << endl;
        
        // Afficher le changelog
        QList<ChangeLogEntry *> entries = metadata->changelog();
        
        foreach(ChangeLogEntry *entry, entries)
        {
            QString t;
            
            switch (entry->type)
            {
                case ChangeLogEntry::LowPriority:
                    t = tr("Faible priorité : ");
                    break;
                    
                case ChangeLogEntry::Feature:
                    t = tr("Nouvelles fonctionnalités : ");
                    break;
                    
                case ChangeLogEntry::BugFix:
                    t = tr("Correction de bogues : ");
                    break;
                    
                case ChangeLogEntry::Security:
                    t = tr("Mise à jour de sécurité : ");
                    break;
            }
            
            cout
            << "  * "
            << COLOR(t, "37")
            << COLOR(entry->version, "32")
            << ", "
            << qPrintable(entry->date.toString(Qt::DefaultLocaleShortDate))
            << endl
            << "    "
            << qPrintable(tr("Par %1 <%2>").arg(entry->author, entry->email))
            << endl;
            
            cout << endl;
            
            // Ecrire le texte caractères par caractères
            printIndented(entry->text.toUtf8(), 8);
            
            cout << endl << endl;
        }
        
        qDeleteAll(entries);
    }
}