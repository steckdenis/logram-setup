/*
 * communication.cpp
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
#include <logram/communication.h>

#include <iostream>
#include <limits>
using namespace std;

void App::communication(Package *sender, Communication *comm)
{
    cout << endl;
    
    if (comm->type() == Communication::Question)
    {
        cout << COLOR(tr("Question de "), "33");
    }
    else if (comm->type() == Communication::Message)
    {
        cout << COLOR(tr("Message de "), "33");
    }
    else
    {
        return;
    }
    
    cout << qPrintable(sender->name()) << " : ";
    
    // Titre de la question
    cout << COLOR(comm->title(), "35") << endl << endl;
    
    // Description de la question
    cout << qPrintable(comm->description()) << endl << endl;
    
    // Information sur le type de question attendue
    if (comm->type() == Communication::Message)
    {
        cout << COLOR(tr("Appuyez sur Entrée pour continuer "), "34");
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    else if (comm->type() == Communication::Question)
    {
        string s;
        int i;
        double d;
        
        while (1)
        {
            switch (comm->returnType())
            {
                case Communication::String:
                    cout << COLOR(tr("Entrez une chaîne de caractères : "), "34");
                    while (s.size() == 0)
                    {
                        getline(cin, s);
                    }
                    comm->setValue(QString::fromStdString(s));
                    s.clear();
                    break;
                    
                case Communication::Integer:
                    cout << COLOR(tr("Entrez un nombre entier : "), "34");
                    cin >> i; cin.get();
                    comm->setValue(i);
                    break;
                    
                case Communication::Float:
                    cout << COLOR(tr("Entrez un nombre décimal : "), "34");
                    cin >> d; cin.get();
                    comm->setValue(d);
                    break;
                    
                case Communication::SingleChoice:
                case Communication::MultiChoice:
                    // Afficher les choix possibles
                    cout << COLOR(tr("Voici les choix possibles : "), "34") << endl << endl;
                    
                    for (int j=0; j<comm->choicesCount(); ++j)
                    {
                        Communication::Choice choice = comm->choice(j);
                        
                        cout << " " << (j + 1) << ". ";
                        
                        if (choice.selected)
                        {
                            cout << "»";
                        }
                        else
                        {
                            cout << " ";
                        }
                        
                        cout << qPrintable(choice.title) << endl;
                    }
                    
                    cout << endl;
                    
                    if (comm->returnType() == Communication::SingleChoice)
                    {
                        do
                        {
                            cout << COLOR(tr("Entrez le numéro du choix que vous voulez : "), "34");
                            cin >> i; cin.get();
                        } while (i < 1 || i > comm->choicesCount());
                        
                        comm->enableChoice(i - 1, true);
                    }
                    else
                    {
                        cout << COLOR(tr("Entrez les numéros des choix, séparés par des virgules, sans espaces : "), "34");
                        cin >> s; cin.get();
                        
                        QStringList choices = QString::fromStdString(s).split(',');
                        
                        foreach (const QString &c, choices)
                        {
                            i = c.toInt();
                            
                            if (i >= 1 && i <= comm->choicesCount())
                            {
                                comm->enableChoice(i - 1, true);
                            }
                        }
                    }
                    
                default:
                    break;
            }
            // TODO: SingleChoice et MultiChoice
            
            // Vérifier l'entrée
            if (comm->isEntryValid())
            {
                break;
            }
            else
            {
                cout << COLOR(tr("Entrée invalide : "), "31");
                
                if (comm->entryValidationErrorString().isNull())
                {
                    cout << qPrintable(tr("Aucune information donnée")) << endl;
                }
                else
                {
                    cout << qPrintable(comm->entryValidationErrorString()) << endl;
                }
            }
        }
    }
}