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

#include <QString>

#include <iostream>
using namespace std;

void App::update()
{
    // Mettre à jour la base de donnée
    ps->update();
}

void App::find(const QString &pattern)
{
    QList<Package *> pkgs = ps->packagesByName("*" + pattern + "*");

    // Afficher joliment les paquets
    for (int i=0; i<pkgs.count(); ++i)
    {
        Package *pkg = pkgs.at(i);
        
        cout
        << COLOR(pkg->name().leftJustified(15, ' ', true), "33") << ' '
        << COLOR(pkg->version().leftJustified(15, ' ', true), "32") << ' '
        << qPrintable(pkg->shortDesc())
        << endl;
    }

    qDeleteAll(pkgs);
}

void App::showFiles(const QString &packageName)
{
    QStringList files = ps->filesOfPackage(packageName);

    foreach(const QString &file, files)
    {
        cout << qPrintable(file);
    }
}

void App::showpkg(const QString &name)
{
    QString n = name.section('=', 0, 0);
    QString v;

    if (name.contains('='))
    {
        v = name.section('=', 1, -1);
    }

    Package *pkg = ps->package(n, v);

    // Vérifier que le paquet est valide
    if (!pkg->isValid())
    {
        cout << COLOR(tr("Le paquet que vous avez entré n'existe pas. Si vous avez précisé une version, ne le faites plus pour voir si un paquet du même nom existe"), "31") << endl;
        return;
    }

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
    cout << COLOR(tr("Titre               : "), "33") << qPrintable(pkg->title()) << endl;
    cout << COLOR(tr("Logiciel graphique  : "), "33") << qPrintable(pkg->isGui() ? tr("Oui") : tr("Non")) << endl;
    cout << COLOR(tr("Section             : "), "33") << qPrintable(pkg->section()) << endl;
    cout << COLOR(tr("Distribution        : "), "33") << qPrintable(pkg->distribution()) << endl;
    cout << COLOR(tr("Status              : "), "33") << qPrintable(status) << endl;
    if (pkg->status() == PACKAGE_STATE_INSTALLED)
    {
        cout << COLOR(tr("Date d'installation : "), "33") << qPrintable(pkg->installedDate().toString(Qt::DefaultLocaleLongDate)) << endl;
        // TODO: Uid vers nom d'utilisateur
        //cout << COLOR(tr("Installé par        : "), "33") << qPrintable(pkg->installedBy()) << endl;
    }
    cout << COLOR(tr("Téléchargement      : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->downloadSize())) << endl;
    cout << COLOR(tr("Taille installée    : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->installSize())) << endl;
    cout << COLOR(tr("Dépôt d'origine     : "), "33") << qPrintable(pkg->repo()) << endl;
    cout << COLOR(tr("Paquet source       : "), "33") << qPrintable(pkg->source()) << endl;
    cout << COLOR(tr("Licence             : "), "33") << qPrintable(pkg->license()) << endl;
    cout << COLOR(tr("Mainteneur          : "), "33") << qPrintable(pkg->maintainer()) << endl;
    cout << COLOR(tr("Description courte  : "), "33") << qPrintable(pkg->shortDesc()) << endl;
    
    cout << endl << COLOR(tr("Description longue : "), "35") << endl << endl;
    cout << qPrintable(pkg->longDesc());

    // Afficher les dépendances
    QList<Depend *> deps = pkg->depends();

    if (deps.count() != 0)
    {
        cout << endl;
        cout << COLOR(tr("Dépendances :"), "35") << endl;
        cout << qPrintable(tr("Légende : "))
            << COLOR(tr("Dépend "), "31")
            << COLOR(tr("Suggère "), "32")
            << COLOR(tr("Conflit "), "33")
            << COLOR(tr("Fourni "), "34")
            << COLOR(tr("Remplace "), "35")
            << COLOR(tr("Est requis par"), "37")
        << endl << endl;

        for (int i=0; i<deps.count(); ++i)
        {
            Depend *dep = deps.at(i);

            cout << "  * ";

            switch (dep->type())
            {
                case DEPEND_TYPE_DEPEND:
                    cout << COLOR(dep->name(), "31");
                    break;
                case DEPEND_TYPE_SUGGEST:
                    cout << COLOR(dep->name(), "32");
                    break;
                case DEPEND_TYPE_CONFLICT:
                    cout << COLOR(dep->name(), "33");
                    break;
                case DEPEND_TYPE_PROVIDE:
                    cout << COLOR(dep->name(), "34");
                    break;
                case DEPEND_TYPE_REPLACE:
                    cout << COLOR(dep->name(), "35");
                    break;
                case DEPEND_TYPE_REVDEP:
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
    cout << endl << COLOR(tr("Versions disponibles : "), "35") << endl << endl;

    QList<Package *> vers = pkg->versions();

    foreach(Package *ver, vers)
    {
        if (ver->status() == PACKAGE_STATE_INSTALLED)
        {
            cout << COLOR("  » ", "34");
        }
        else if (ver->status() == PACKAGE_STATE_REMOVED)
        {
            cout << COLOR("  « ", "31");
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