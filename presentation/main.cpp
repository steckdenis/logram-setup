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

//Own
#include "myclass.h"

int main(int argc, char **argv)
{
    KAboutData about("presentation", "presentation", ki18n("Presentation"), "0.1",
                      ki18n("Logram's presentation viewer"),
                      KAboutData::License_GPL,
                      ki18n("Copyright (c) 2009 Denis Steckelmacher"));
    
    KCmdLineArgs::init(argc, argv, &about);
    
    KCmdLineOptions options;
    
    options.add("+url", ki18n("Url of the presentation to open"));
    
    KCmdLineArgs::addCmdLineOptions(options);
 
    KApplication app(true);
    
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    
    QWidget *win = new QWidget(0);
    win->setWindowTitle("Logram");
    win->setFixedSize(800, 600);
    win->move((QApplication::desktop()->screen()->width() / 2) - 400, (QApplication::desktop()->screen()->height() / 2) - 300);
    
    QVBoxLayout *layout = new QVBoxLayout(win);
    layout->setContentsMargins(0, 0, 0, 0);
    win->setLayout(layout);
    
    KUrl url = args->url(0);
    KHTMLPart *w = new KHTMLPart(win);
    w->openUrl(url);
    w->view()->resize(800, 600);
    w->show();
    w->view()->show();
    layout->addWidget(w->view());
    
    MyClass *mc = new MyClass(0, w);
    
    win->show();
    
    app.exec();
}

MyClass::MyClass(QObject *parent, KHTMLPart *_w) : QObject(parent)
{
    w = _w;
    
    connect(w->browserExtension(),
            SIGNAL(openUrlRequestDelayed( const KUrl &,const KParts::OpenUrlArguments &,const KParts::BrowserArguments &)),
            this,
            SLOT(openUrlRequestDelayed( const KUrl &,const KParts::OpenUrlArguments &,const KParts::BrowserArguments &)));
}

void MyClass::openUrlRequestDelayed( const KUrl &url,
                              const KParts::OpenUrlArguments& arguments,
                              const KParts::BrowserArguments &browserArguments )
{
    w->openUrl(url);
}