/*
 * window.h
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

#ifndef __WINDOW_H__
#define __WINDOW_H__

//KDE
#include <kpagedialog.h>

//Qt
#include <QtXml>
#include <QFile>
#include <QList>

class Window : public KPageDialog
{
    Q_OBJECT
    
    public:
        Window(const QString &_specFile);
        ~Window();
        
    private:
        void initInterface();
        void initWidget(QWidget *wg, const QDomElement &el);
        
        QString configValue(const QString &key);
        void setConfigValue(const QString &key, const QString &value);
        
    private slots:
        void resetData();
        void applyData();
        void okData();
        
    private:
        //Général
        QFile *fl;
        QString specFile;
        QDomDocument *doc;
        
        //Onglets
        QList<QWidget *> tabs;
        
        //Widgets
        QList<QWidget *> widgets;
};
#endif