/*
 * win.h
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

#ifndef __WIN_H__
#define __WIN_H__

#include <QWidget>
#include <QList>

#include <QtXml>

class QVBoxLayout;
class QTreeWidget;
class QPushButton;
class QTreeWidgetItem;

class Win : public QWidget
{
    Q_OBJECT

    public:
        Win(const QString &path);

        void displayPage(const QString &path, int pg = 1);
        void displayPage(const QDomElement &page, int pg = 1);
        void buildTree(const QDomElement &main);
        void buildTree(const QDomElement &el, QTreeWidgetItem *parent);

        QDomElement nodeByPath(const QString &path);
        QDomElement nodeByPath(const QString &path, const QDomElement &parent);
        QTreeWidgetItem *itemByPath(const QString &path, QTreeWidgetItem *parent = 0);

    private slots:
        void prevClicked();
        void nextClicked();
        void nextPageClicked();
        void upClicked();

        void itemClicked(QTreeWidgetItem *item, int column);
        void buttonClicked();

    private:
        QList<QWidget *> widgets;
        QWidget *pageWidget;
        QVBoxLayout *layout;
        QTreeWidget *tree;

        QPushButton *btnQuit, *btnPrev, *btnNext, *btnNextPage, *btnUp;

        QDomDocument doc;
        QDomElement currentPage;
        int currentPg, maxPg;
};

#endif