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

#include "parser.h"

#include <iostream>
using namespace std;

bool parsePage(const QDomElement &el, const QString &out, const QString &prevUrl, const QString &prevTitle, const QString &nextUrl, const QString &nextTitle, const QString &topUrl, const QString &topTitle);
QString calcUrl(const QString &xml);
QString pageTitle(const QString &xml);

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    if (app.arguments().count() < 3)
    {
        cout << "Usage : doc2html in.xml out.dir" << endl;
        return 1;
    }
    
    QString in = app.arguments().at(1);
    QString out = app.arguments().at(2);
    
    //Lire le fichier XML d'entrée
    QDomDocument doc;
    doc.setContent(new QFile(in));
    
    QDomElement el = doc.documentElement();
    
    //El est la première page (page d'accueil), la traiter tout à fait normalement
    if (parsePage(el, out, QString(), QString(), QString(), QString(), QString(), QString())) return 1;
    
    return 0;
}

QString calcUrl(const QString &xml)
{
    return xml.section('/', -1, -1).section('.', 0, -2) + ".html";
}

QString pageTitle(const QString &xml)
{
    QFile fl(xml);
    QDomDocument doc;
    doc.setContent(&fl);
    
    return doc.documentElement().attribute("title");
}

bool parsePage(const QDomElement &el, const QString &out, const QString &prevUrl, const QString &prevTitle, const QString &nextUrl, const QString &nextTitle, const QString &topUrl, const QString &topTitle)
{
    QString xmlFileName = el.attribute("src");
    
    //Parser la page
    Parser *parser = new Parser(xmlFileName);
    bool success = true;
    
    //Créer le fichier qui va accueillir la sortie
    QString myUrl = calcUrl(xmlFileName);
    QString myTitle = parser->documentElement().attribute("title");
    
    QFile fout(out + "/" + myUrl);
    
    if (!fout.open(QIODevice::WriteOnly))
    {
        qFatal(qPrintable("Unable to open the output file " + fout.fileName()));
    }
    
    //Parser le fichier d'entrée
    fout.write(parser->parse(success, prevUrl, prevTitle, nextUrl, nextTitle, topUrl, topTitle).toUtf8());
    fout.close();
    
    if (!success) return false;
    
    //Explorer les sous-éléments de l'élément et les parser
    QDomElement elem = el.firstChildElement();
    
    while (!elem.isNull())
    {
        //Obtenir les pages suivantes et précédantes
        QDomElement nextElem = elem.nextSiblingElement();
        QDomElement prevElem = elem.previousSiblingElement();
        
        QString nu, nt, pu, pt;
        
        if (!nextElem.isNull())
        {
            nu = calcUrl(nextElem.attribute("src"));
            nt = pageTitle(nextElem.attribute("src"));
        }
        
        if (!prevElem.isNull())
        {
            pu = calcUrl(prevElem.attribute("src"));
            pt = pageTitle(prevElem.attribute("src"));
        }
        
        bool rs = parsePage(elem, out, pu, pt, nu, nt, myUrl, myTitle);
        
        if (!rs) return false;
        
        elem = elem.nextSiblingElement();
    }
    
    return true;
}