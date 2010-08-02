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

#include <packagesystem.h>

#include <QMainWindow>
#include <QHash>

#include "ui_appmanager.h"

namespace LogramUi
{
    class CategoryView;
    class FilterInterface;
    class InfoPane;
    class SearchBar;
    class ProgressDialog;
}

namespace Logram
{
    class DatabasePackage;
}

class IntroDisplay;
class MainWindow;
class Breadcrumb;
class PackageList;

class QListWidget;

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT
    
    public:
        MainWindow();
        ~MainWindow();
        bool error() const;
        
        struct PackageInfo
        {
            QPixmap icon;
            QString title;
            int votes, total_votes;
        };
        
        struct RatedPackage
        {
            PackageInfo inf;
            QString repo, distro;
            float score;
        };
        
        QVector<RatedPackage> &ratedPackages();
        
    private slots:
        void progress(Logram::Progress *progress);
        void communication(Logram::Package *sender, Logram::Communication *comm);
        void updateDatabase();
        void delayedInit();
        void readPackages();
        void searchPackages();
        void breadcrumbPressed(int index);
        
    private:
        void setMode(bool packageList);
        bool addPackage(Logram::DatabasePackage *pkg);
        
    protected:
        void closeEvent(QCloseEvent *event);
        
    private:
        Logram::PackageSystem *ps;
        bool _error;
        
        LogramUi::ProgressDialog *progressDialog;
        LogramUi::CategoryView *sections;
        LogramUi::FilterInterface *filterInterface;
        LogramUi::InfoPane *infoPane;
        LogramUi::SearchBar *searchBar;
        
        IntroDisplay *display;
        PackageList *listPackages;
        Breadcrumb *breadcrumb;
        
        QHash<QString, PackageInfo> packages;
        QVector<RatedPackage> highestRated;
        QHash<Logram::DatabasePackage *, int> packageActions;
        
        enum FilterAction
        {
            RemoveSearch = 1,
            RemoveStatus = 2,
            RemoveSection = 4
        };
};

#endif