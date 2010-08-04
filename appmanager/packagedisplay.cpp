/*
 * packagelist.cpp
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

#include "packagedisplay.h"
#include "packageentry.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QPoint>

#include <QtDebug>

#include <databasepackage.h>

using namespace Logram;

PackageDisplay::PackageDisplay(MainWindow* parent): QWidget(parent)
{
    curIndex = -1;
    win = parent;
    pix = QPixmap(":/images/logologram.png");
    
    // Fond comme une zone de liste
    setBackgroundRole(QPalette::Base);
    
    // Initialisation
    l = new QVBoxLayout(this);
    
    l->setContentsMargins(1, 1, 1, 1);
    l->setSpacing(0);
    l->addStretch();
    
    setLayout(l);
}

void PackageDisplay::clear()
{
    for (int i=0; i<entries.count(); ++i)
    {
        delete entries.at(i);
    }
    
    entries.clear();
}

void PackageDisplay::addPackage(Logram::DatabasePackage* pkg, const MainWindow::PackageInfo& inf)
{
    Entry *entry = new Entry(pkg, inf, this);
    entries.append(entry);
    
    connect(entry, SIGNAL(clicked()), this, SLOT(elementClicked()));
    
    l->insertWidget(0, entry);
}

int PackageDisplay::currentIndex() const
{
    return curIndex;
}

int PackageDisplay::count() const
{
    return entries.count();
}

DatabasePackage* PackageDisplay::package(int index)
{
    if (index == -1) return 0;
    
    return entries.at(index)->package();
}

void PackageDisplay::elementClicked()
{
    curIndex = entries.indexOf(static_cast<Entry *>(sender()));
    
    emit currentIndexChanged(curIndex);
}

void PackageDisplay::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    
    // On peut scroller, mais le background est fixé, donc il faut faire quelques calculs
    QWidget *parent = parentWidget();
    QPoint pt = mapFromParent(QPoint(parent->width(), parent->height()));
    
    painter.drawPixmap(pt.x() - pix.width(), pt.y() - pix.height(), pix);
}

#include "packagedisplay.moc"
