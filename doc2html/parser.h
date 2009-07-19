/*
 * parser.h
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

#ifndef __PARSER_H__
#define __PARSER_H__

#include <QtGui>
#include <QtXml>
#include <QtDebug>

class Parser : public QDomDocument
{
    public:
        Parser(const QString &in);
        ~Parser();
        QString parse(bool &success, const QString &prevUrl, const QString &prevTitle, const QString &nextUrl, const QString &nextTitle, const QString &topUrl, const QString &topTitle);
        
    private:
        QString parseElement(const QDomNode &node);
        
        QHash<QString, QString> acronyms;
        int snum;
        QFile *mfile;
};

#endif