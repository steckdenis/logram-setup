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

#include <QApplication>
#include <QTextCodec>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    QTranslator translator;
    
    translator.load("libpackage_" + QLocale::system().name(), "/usr/share/qt4/translations");
    app.installTranslator(&translator);
    
    MainWindow win;
    
    if (win.error())
    {
        return 1;
    }
    
    win.show();
    
    return app.exec();
}