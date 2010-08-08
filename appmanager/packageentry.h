/*
 * packageentry.h
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

#ifndef __PACKAGEENTRY_H__
#define __PACKAGEENTRY_H__

#include <QWidget>
#include <QPixmap>
#include <QVector>

#include "ui_entry.h"

namespace Logram
{
    class DatabasePackage;
}

class Ui_EntryMoreInfos;
class MainWindow;

class Entry : public QWidget, private Ui_Entry
{
    Q_OBJECT
    
    public:
        Entry(Logram::DatabasePackage *pkg, MainWindow *mainWindow, bool expandable, QWidget *parent);
        ~Entry();
        
        QVector<Logram::DatabasePackage *> packages() const;
        Logram::DatabasePackage *currentPackage() const;
        bool isExpanded() const;
        
    public slots:
        void expand();
        void collapse();
        
    private slots:
        void btnInstallClicked(bool checked);
        void btnRemoveClicked(bool checked);
        void btnUpdateClicked(bool checked);
        
    private:
        void updateIcon();
        void setCurrentPackage(int index);
        
    protected:
        void enterEvent(QEvent *event);
        void leaveEvent(QEvent *event);
        void paintEvent(QPaintEvent *event);
        void mousePressEvent(QMouseEvent *event);
        
    signals:
        void clicked();
        
    private:
        QVector<Logram::DatabasePackage *> pkgs;
        Logram::DatabasePackage *curpkg, *mainpkg;
        MainWindow *win;
        
        Ui_EntryMoreInfos *more;
        QWidget *moreWidget;
        bool containsMouse, expanded, exp;
        QPixmap baseIcon;
};

#endif