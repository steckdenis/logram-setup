/*
 * mainwindow.h
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

#ifndef __BREADCRUMB_H__
#define __BREADCRUMB_H__

#include <QWidget>
#include <QIcon>
#include <QString>

class QAbstractButton;

class Breadcrumb : public QWidget
{
    Q_OBJECT
    
    public:
        Breadcrumb(QWidget *parent);
        ~Breadcrumb();
        
        void addButton(const QString &text);
        void addButton(const QIcon &icon, const QString &text);
        void addButton(QAbstractButton *button);
        
        void insertButton(int index, const QString &text);
        void insertButton(int index, const QIcon &icon, const QString &text);
        void insertButton(int index, QAbstractButton *button);
        
        void removeButton(int index);
        QAbstractButton *button(int index) const;
        int count() const;
        
    private slots:
        void buttonTriggered();
        
    signals:
        void buttonPressed(int index);
        
    private:
        struct Private;
        Private *d;
};

#endif