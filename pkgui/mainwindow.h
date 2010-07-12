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

#include <packagesystem.h>
#include <databasepackage.h>

#include "ui_mainwindow.h"

class PackagesModel;
class ProgressDialog;

class PackageItem : public QTreeWidgetItem
{
    public:
        enum Type
        {
            PackageList,        // Liste des paquets, fenêtre principale
            SmallActionList,    // Liste des actions, petite fenêtre à gauche
            LargeActionList     // Liste des actions, quand on applique les changements
        };
        
        PackageItem(Logram::DatabasePackage *pkg, QTreeWidget *parent, Type type);
        PackageItem(Logram::DatabasePackage *pkg, QTreeWidgetItem *parent, Type type);
        virtual ~PackageItem();
        
        void setPackage(Logram::DatabasePackage *pkg);
        Logram::DatabasePackage *package();
        void updateIcon();
        void updateText(bool subElements);
        
    private:
        Logram::DatabasePackage *_pkg;
        Type _type;
};

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT
    
    public:
        MainWindow();
        ~MainWindow();
        
        bool error() const;
        void psError();
        void enableProgressions();
        void disableProgressions();
        Logram::PackageSystem *packageSystem() const;
        
        static Logram::DatabasePackage *duplicatePackage(Logram::PackageSystem *ps, Logram::DatabasePackage *pkg);
        
    public slots:
        void progress(Logram::Progress *progress);
        
    private:
        enum PackageFilter
        {
            NoFilter = 0,
            Installed = 1,
            NotInstalled = 2,
            Updateable = 3,
            Orphan = 4
        };
        
        void displayPackages(PackageFilter filter, const QString &pattern);
        void addPackage(Logram::DatabasePackage* pkg);
        void actionsForPackage(Logram::DatabasePackage *pkg);
        
        void addPackageInList(Logram::DatabasePackage *pkg, QTreeWidget *treeActions);
        void removePackageFromList(Logram::DatabasePackage *pkg, QTreeWidget *treeActions);
        
    private slots:
        void communication(Logram::Package *sender, Logram::Communication *comm);
        
        void itemActivated(QTreeWidgetItem *item);
        void licenseActivated(const QString &url);
        void websiteActivated(const QString &url);
        void showFlags();
        void searchPackages();
        
        void installPackage();
        void removePackage();
        void purgePackage();
        void deselectPackage();
        
        void applyList();
        void cancelList();
        void databaseUpdate();
        
    private:
        Logram::PackageSystem *ps;
        PackagesModel *model;
        bool _error, _progresses;
        QVector<Logram::DatabasePackage *> actionPackages;
        ProgressDialog *progressDialog;
};

#endif