/*
 * main.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

int main(int argc, char **argv)
{
    App app(argc, argv);
    
    if (app.failed())
    {
        return 1;
    }
    
    // Si on était un processus worker dans le chroot, quitter
    if (app.finished())
    {
        return 0;
    }
    
    // Déclencher la construction du premier paquet
    app.buildPackage();
    
    
    // Attendre indéfiniment
    int rs = app.exec();
    app.cleanup();
    
    return rs;
}