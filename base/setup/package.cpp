/*
 * package.cpp
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

#include <logram/solver.h>
#include <logram/package.h>

#include <iostream>
using namespace std;

void App::add(const QStringList &packages)
{
    Solver *solver = ps->newSolver();
    
    foreach(const QString &package, packages)
    {
        QString name = package;
        Solver::Action action;

        if (package.startsWith('-'))
        {
            action = Solver::Remove;
            name.remove(0, 1);
        }
        else if (package.startsWith('+'))
        {
            action = Solver::Purge;
            name.remove(0, 1);
        }
        else
        {
            action = Solver::Install;
        }

        solver->addPackage(name, action);
    }

    solver->solve();

    // Afficher et gérer les résultats
    manageResults(solver);
    
    delete solver;
}

void App::manageResults(Solver *solver)
{
    cout << COLOR(tr("Paquets qui seront installés ou supprimés :"), "32") << endl;
    cout << qPrintable(tr("    Légende : "))
         << COLOR(tr("Installer "), "34")
         << COLOR(tr("Supprimer "), "31")
         << COLOR(tr("Mettre à jour "), "33")
         << COLOR(tr("Supprimer totalement "), "35")
         << endl;

    // Boucle pour demander son avis à l'utilisateur
    QList<Package *> packages;
    int index = 0;
    int tot = solver->results();
    int weight;
    int dlSize, instSize;
    char in;
    
    while (true)
    {
        cout << endl;

        dlSize = 0;
        instSize = 0;
        packages = solver->result(index, weight);

        foreach (Package *pkg, packages)
        {
            QString name = pkg->name().leftJustified(15, ' ', true);
            
            cout << "  * ";

            if (pkg->action() == Solver::Install)
            {
                dlSize += pkg->downloadSize();
                instSize += pkg->installSize();
                cout << COLOR(name, "34");
            }
            else if (pkg->action() == Solver::Remove)
            {
                instSize -= pkg->installSize();
                cout << COLOR(name, "31");
            }
            else if (pkg->action() == Solver::Update)
            {
                dlSize += pkg->downloadSize();
                // TODO: Différence entre la version installée et la version qu'on va télécharger
                cout << COLOR(name, "33");
            }
            else if (pkg->action() == Solver::Purge)
            {
                instSize -= pkg->installSize();
                cout << COLOR(name, "35");
            }

            cout << ' '
                 << COLOR(pkg->version().leftJustified(15, ' ', true), "32") << ' '
                 << qPrintable(pkg->shortDesc())
                 << endl;
        }

        cout << endl;

        // Demander si c'est bon
        cout << qPrintable(tr("Solution %1 sur %2, de poids %3. Téléchargement de %4, installation de %5\n"
                              "Accepter (Y), Suivante (n), Précédante (p) ou Annuler (c) ? ")
                                  .arg(QString::number(index+1))
                                  .arg(QString::number(tot))
                                  .arg(weight)
                                  .arg(PackageSystem::fileSizeFormat(dlSize))
                                  .arg(PackageSystem::fileSizeFormat(instSize)));
        cin >> in;

        if (in == 'y')
        {
            break;
        }
        else if (in == 'n')
        {
            index = qMin(index + 1, tot - 1);
        }
        else if (in == 'p')
        {
            index = qMax(index - 1, 0);
        }
        else if (in == 'c')
        {
            return;
        }
        else
        {
            break;
        }
    }

    cout << endl;
    cout << COLOR(tr("Installation des paquets..."), "32") << endl;
    cout << endl;

    // Installer les paquets
    solver->process(index);

    cout << endl;
    cout << COLOR(tr("Paquets installés !"), "32") << endl;
    cout << endl;
}