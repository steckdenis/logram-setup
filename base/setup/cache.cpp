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

#include <logram/package.h>
#include <logram/packagemetadata.h>

#include <QString>

#include <iostream>
using namespace std;

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
        
        Package *pkg = ps->package(id);
        
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
        cout << qPrintable(file);
    }
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
    cout << COLOR(tr("Logiciel graphique  : "), "33") << qPrintable(pkg->isGui() ? tr("Oui") : tr("Non")) << endl;
    cout << COLOR(tr("Section             : "), "33") << qPrintable(pkg->section()) << endl;
    cout << COLOR(tr("Distribution        : "), "33") << qPrintable(pkg->distribution()) << endl;
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
    cout << COLOR(tr("Dépôt d'origine     : "), "33") << qPrintable(pkg->repo()) << endl;
    cout << COLOR(tr("Paquet source       : "), "33") << qPrintable(pkg->source()) << endl;
    cout << COLOR(tr("Licence             : "), "33") << qPrintable(pkg->license()) << endl;
    cout << COLOR(tr("Mainteneur          : "), "33") << qPrintable(pkg->maintainer()) << endl;
    cout << COLOR(tr("Description courte  : "), "33") << qPrintable(pkg->shortDesc()) << endl;
    
    cout << endl << COLOR(tr("Description longue : "), "35") << endl << endl;
    cout << qPrintable(metadata->packageDescription()) << endl;

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
    cout << endl << COLOR(tr("Versions disponibles : "), "35") << endl;
    cout << qPrintable(tr("Légende : * = Disponible, I = installée, R = supprimée")) << endl << endl;

    QList<Package *> vers = pkg->versions();

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
    
    if (changelog)
    {
        cout << endl << COLOR(tr("Historique des versions : "), "35") << endl << endl;
        
        // Afficher le changelog
        QList<ChangeLogEntry *> entries = metadata->changelog();
        
        foreach(ChangeLogEntry *entry, entries)
        {
            cout
            << COLOR(entry->version, "32")
            << " ("
            << COLOR(entry->author + " <" + entry->email + ">", "33")
            << "), "
            << COLOR(entry->date.toString(Qt::DefaultLocaleShortDate), "36") << endl;
            
            cout << endl;
            
            cout << qPrintable(entry->text);
            
            cout << endl << endl;
        }
        
        qDeleteAll(entries);
    }
}