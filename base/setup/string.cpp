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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h> 
#include <unistd.h>

void App::printIndented(const QByteArray &chars, int indent)
{
    char c;
    int pos = 0;
    int maxpos = width - indent;
    
    #define IDENT for (int j=0; j<indent; ++j) putc(' ', stdout);
    
    IDENT
    
    for (int i=0; i<chars.size(); ++i)
    {
        c = chars.at(i);
        
        if (pos >= maxpos || c == '\n')
        {
            pos = 0;
            
            if (c != '\n')
            {
                putc(c, stdout);
            }
            
            putc('\n', stdout);
            IDENT
            
            // On ne commence pas une ligne par un espace, c'est moche
            c = chars.at(i+1);
            
            while ((c == ' ' || c == '\t') && i<chars.size())
            {
                i++;
                c = chars.at(i+1);
            }
        }
        else
        {
            putc(c, stdout);
            pos++;
        }
    }
}

void App::getString(char *buffer, int max_length, const char *def, bool append_return)
{
    int pos = 0, slength = 0;
    int c, cc1, cc2;
    int ln;
    int i;
    struct termios old_sets, new_sets;
    
    // Effacer le buffer
    memset(buffer, 0, max_length);
    
    // Si default n'est pas zéro, l'afficher
    if (def != 0)
    {
        ln = strlen(def);
        
        printf(def);
        pos = ln;
        slength = ln;
        memcpy(buffer, def, ln);
    }
    
    // Configurer le terminal
    tcgetattr(STDIN_FILENO, &old_sets);
    memcpy(&new_sets, &old_sets, sizeof(struct termios));
    
    cfmakeraw(&new_sets);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_sets);
    
    
    // Attendre que l'utilisateur appuie sur ENTER
    while (1)
    {
        c = getc(stdin);
        
        if (c == 13)
        {
            // Return
            break;
        }
        
        if (c == 127)
        {
            if (pos > 0)
            {
                // Backspace
                putc('\b', stdout);
                
                if (pos < slength)
                {
                    printf(buffer + pos);
                    ln = strlen(buffer + pos);
                    
                    putc(' ', stdout);
                    
                    // Revenir en arrière
                    for (i=0; i<=ln; i++)
                    {
                        putc('\b', stdout);
                    }
                    
                    // Également faire le déplacement dans le buffer
                    memcpy(buffer + pos - 1, buffer + pos, ln + 1);
                }
                else
                {
                    printf(" \b");
                    buffer[pos] = 0;
                }
                
                pos--;
                slength--;
            }
            continue;
        }
        
        if (c == 27)
        {
            // Code en plusieurs morceaux
            c = getc(stdin);
            
            if (c == 91)
            {
                // Touche de déplacement
                c = getc(stdin);
                
                if (c == 68 && pos > 0)
                {
                    // <==
                    putc('\b', stdout);
                    pos--;
                }
                else if (c == 67 && pos < slength)
                {
                    // ==>
                    putc(buffer[pos], stdout);
                    pos++;
                }
                else if (c == 72)
                {
                    // HOME
                    for (i = 0; i<pos; i++)
                    {
                        putc('\b', stdout);
                    }
                    
                    pos = 0;
                }
                else if (c == 70)
                {
                    // END
                    printf(buffer + pos);
                    
                    pos = slength;
                }
                else if (c == 51)
                {
                    // DELETE
                    if (pos < slength)
                    {
                        ln = strlen(buffer + pos);
                        
                        printf(buffer + pos + 1);
                        putc(' ', stdout);
                        
                        for (i=0; i<ln; i++)
                        {
                            putc('\b', stdout);
                        }
                        
                        // Adapter le buffer
                        memcpy(buffer + pos, buffer + pos + 1, ln);
                        
                        slength--;
                    }
                    
                    getc(stdin); // Vider le buffer
                }
            }
            else
            {
                // Tout de même vider le buffer
                c = getc(stdin);
            }
            
            continue;
        }
        
        // Saisir le caractère
        if (pos < (max_length - 1) && slength < (max_length - 1))
        {
            putc(c, stdout);
            
            // Si on est dans la chaîne, la décaler
            if (pos < slength)
            {
                ln = strlen(buffer + pos);
                printf(buffer + pos);
                
                // Revenir en arrière
                cc1 = c;
                
                for (i=0; i<=ln; i++)
                {
                    if (i != 0)
                    {
                        putc('\b', stdout);
                    }
                    
                    cc2 = buffer[pos + i];
                    buffer[pos + i] = cc1;
                    cc1 = cc2;
                }
            }
            else
            {
                buffer[pos] = c;
            }
            
            pos++;
            slength++;
        }
    }
    
    // Restaurer la configuration d'avant
    tcsetattr(STDIN_FILENO, TCSANOW, &old_sets);
    
    // Si on le demande, ajouter le retour
    if (append_return)
    {
        putc('\n', stdout);
    }
    
    // S'assurer que le buffer est correct
    if (slength < max_length)
    {
        buffer[slength] = 0;
    }
    else
    {
        buffer[max_length-1] = 0;
    }
}