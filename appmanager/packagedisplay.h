/*
 * packagelist.h
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

#ifndef __PACKAGEDISPLAY_H__
#define __PACKAGEDISPLAY_H__

#include <QWidget>
#include <QPixmap>

#include "mainwindow.h"

namespace Logram
{
    class DatabasePackage;
}

class QVBoxLayout;
class Entry;

class PackageDisplay : public QWidget
{
    Q_OBJECT
    
    public:
        PackageDisplay(MainWindow *parent);
        
        void clear();
        void addPackage(Logram::DatabasePackage *pkg, bool expandable);
        
        int currentIndex() const;
        int count() const;
        Logram::DatabasePackage *currentPackage(int index);
        
    protected:
        void paintEvent(QPaintEvent *event);
        
    private slots:
        void elementClicked();
        
    signals:
        void currentIndexChanged(int index, int previous);
        
    private:
        MainWindow *win;
        QPixmap pix;
        QVBoxLayout *l;
        
        QVector<Entry *> entries;
        int curIndex;
};

#endif