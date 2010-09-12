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

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QPixmap>

#include <packagesystem.h>
#include <databasepackage.h>

#include "ui_mainwindow.h"

namespace LogramUi
{
    class ProgressDialog;
    class InfoPane;
    class SearchBar;
    class CategoryView;
    class FilterInterface;
}

class PackageItem : public QTreeWidgetItem
{
    public:
        enum Type
        {
            PackageList,        // Liste des paquets, fenêtre principale
            SmallActionList,    // Liste des actions, petite fenêtre à gauche
        };
        
        PackageItem(Logram::DatabasePackage *pkg, QTreeWidget *parent, Type type, bool expand);
        PackageItem(Logram::DatabasePackage *pkg, QTreeWidgetItem *parent, Type type);
        virtual ~PackageItem();
        
        QVariant data(int column, int role) const;
        
        void setPackage(Logram::DatabasePackage *pkg);
        Logram::DatabasePackage *package();
        void updateIcon();
        void updateText(bool subElements);
        
    private:
        Logram::DatabasePackage *_pkg;
        Type _type;
};

class SectionItem : public QTreeWidgetItem
{
    public:
        SectionItem(const QString &section, QTreeWidgetItem *parent);
        
        QString section() const;
        
    private:
        QString _section;
};

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT
    
    public:
        MainWindow();
        ~MainWindow();
        
        bool error() const;
        void enableProgressions();
        void disableProgressions();
        
        static Logram::DatabasePackage *duplicatePackage(Logram::PackageSystem *ps, Logram::DatabasePackage *pkg);
        
    private:
        bool addPackage(Logram::DatabasePackage* pkg, bool expand);
        void actionsForPackage(Logram::DatabasePackage *pkg);
        
        void addPackageInList(Logram::DatabasePackage *pkg, QTreeWidget *treeActions);
        void removePackageFromList(Logram::DatabasePackage *pkg, QTreeWidget *treeActions);
        
    private slots:
        void progress(Logram::Progress *progress);
        void communication(Logram::Package *sender, Logram::Communication *comm);
        
        void itemActivated(QTreeWidgetItem *item);
        void searchPackages();
        
        void installPackage();
        void removePackage();
        void purgePackage();
        void deselectPackage();
        
        void applyList();
        void cancelList();
        void databaseUpdate();
        
        void aboutPkgui();
        
    protected:
        void closeEvent(QCloseEvent *event);
        
    private:
        Logram::PackageSystem *ps;
        bool _error, _progresses;
        QVector<Logram::DatabasePackage *> actionPackages;
        
        LogramUi::ProgressDialog *progressDialog;
        LogramUi::SearchBar *searchBar;
        LogramUi::InfoPane *infoPane;
        LogramUi::CategoryView *sections;
        LogramUi::FilterInterface *filterInterface;
};

#endif