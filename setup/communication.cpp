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

#include <solver.h>
#include <package.h>
#include <communication.h>

#include <iostream>
#include <limits>
using namespace std;

using namespace Logram;

void App::communication(Package *sender, Communication *comm)
{
    char *buffer = new char[4000]; // 96 bytes perdus, mais qui peuvent éviter d'allouer une 2eme page sur un OS nécessitant des infos supplémentaires pour chaque allocation
    
    cout << endl;
    
    if (sender)
    {
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
    }
    else
    {
        if (comm->type() == Communication::Question)
        {
            cout << COLOR(tr("Question générale : "), "33");
        }
        else if (comm->type() == Communication::Message)
        {
            cout << COLOR(tr("Message général : "), "33");
        }
        else
        {
            return;
        }
    }
    
    // Titre de la question
    cout << COLOR(comm->title(), "35") << endl << endl;
    
    // Description de la question
    cout << qPrintable(comm->description()) << endl << endl;
    
    // Information sur le type de question attendue
    if (comm->type() == Communication::Message)
    {
        cout << COLOR(tr("Appuyez sur Entrée pour continuer "), "34");
        
        getString(buffer, 2, 0, true);
    }
    else if (comm->type() == Communication::Question)
    {
        QString defStr, rs;
        bool ok;
        int i;
        double d;
        
        while (1)
        {
            switch (comm->returnType())
            {
                case Communication::String:
                    cout << COLOR(tr("Entrez une chaîne de caractères : "), "34");
                    
                    defStr = comm->defaultString();
                    
                    do
                    {
                        if (!defStr.isEmpty())
                        {
                            getString(buffer, 4000, qPrintable(defStr), true);
                        }
                        else
                        {
                            getString(buffer, 4000, 0, true);
                        }
                        
                        rs = QString(buffer);
                    } while (rs.length() == 0);
                    
                    comm->setValue(rs);
                    
                    break;
                    
                case Communication::Integer:
                    cout << COLOR(tr("Entrez un nombre entier : "), "34");
                    
                    defStr = comm->defaultString();
                    
                    do
                    {
                        if (!defStr.isEmpty())
                        {
                            getString(buffer, 4000, qPrintable(defStr), true);
                        }
                        else
                        {
                            getString(buffer, 4000, 0, true);
                        }
                        
                        rs = QString(buffer);
                        i = rs.toInt(&ok);
                    } while (rs.length() == 0 || !ok);
                    
                    comm->setValue(i);
                    
                    break;
                    
                case Communication::Float:
                    cout << COLOR(tr("Entrez un nombre décimal : "), "34");
                    
                    defStr = comm->defaultString();
                    
                    do
                    {
                        if (!defStr.isEmpty())
                        {
                            getString(buffer, 4000, qPrintable(defStr), true);
                        }
                        else
                        {
                            getString(buffer, 4000, 0, true);
                        }
                        
                        rs = QString(buffer);
                        d = rs.toDouble(&ok);
                    } while (rs.length() == 0 || !ok);
                    
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
                        defStr = QString::number(comm->defaultIndex() + 1);
                        
                        do
                        {
                            cout << COLOR(tr("Entrez le numéro du choix que vous voulez : "), "34");
                            
                            if (defStr != "0")
                            {
                                getString(buffer, 4000, qPrintable(defStr), true);
                            }
                            else
                            {
                                getString(buffer, 4000, 0, true);
                            }
                            
                            rs = QString(buffer);
                            i = rs.toInt(&ok);
                        } while (!ok || i < 1 || i > comm->choicesCount());
                        
                        comm->enableChoice(i - 1, true);
                    }
                    else
                    {
                        cout << COLOR(tr("Entrez les numéros des choix, séparés par des virgules, sans espaces : "), "34");
                        
                        getString(buffer, 4000, 0, true);
                        rs = QString(buffer);
                        
                        QStringList choices = rs.split(',');
                        
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
    
    delete buffer;
}