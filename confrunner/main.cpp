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

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include "window.h"

int main(int argc, char **argv)
{
    KAboutData aboutData("confrunner", "confrunner",
                         ki18n("Logram Configuration Runner"), "0.2",
                         ki18n("Builds and displays a configuration window based on a project file"),
                         KAboutData::License_GPL,
                         ki18n("Copyright (c) 2009 Denis Steckelmacher"));
    
    KCmdLineArgs::init(argc, argv, &aboutData);
    
    KCmdLineOptions options;
    options.add("+project", ki18n("Project file (XML). \"_LANG_\" is replaced by the ISO code of the current system locale"));
    KCmdLineArgs::addCmdLineOptions(options);
    
    KApplication app;
    
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    
    Window *win = new Window(args->arg(0));
    win->show();
    
    return app.exec();
}