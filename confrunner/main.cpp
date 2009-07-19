/*
 * main.cpp
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

#include <QApplication>

#include "window.h"

#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    if (app.arguments().count() < 2)
    {
        cout << "Usage : confrunner specfile" << endl;
        cout << endl;
        cout << "The file name can contains the token _LANG_ wich will be replaced by the"
        " ISO code of the system locale" << endl;
        return 1;
    }
    
    Window *win = new Window(app.arguments().at(1));
    win->show();
    
    return app.exec();
}