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

#include <QWidget>

#include <packagesystem.h>
#include <filepackage.h>

namespace LogramUi
{
    class ProgressDialog;
    class InfoPane;
}

class QPushButton;

class MainWindow : public QWidget
{
    Q_OBJECT
    
    public:
        MainWindow();
        ~MainWindow();
        
        bool error() const;
        void enableProgressions();
        void disableProgressions();
        
    private slots:
        void progress(Logram::Progress *progress);
        void communication(Logram::Package *sender, Logram::Communication *comm);
        
        void btnInstallClicked();
        
        void displayPackage(const QString &filename);
        
    private:
        Logram::PackageSystem *ps;
        Logram::FilePackage *fpkg;
        bool _error, _progresses;
        
        LogramUi::ProgressDialog *progressDialog;
        LogramUi::InfoPane *infoPane;
        
        QPushButton *btnOk;
        QPushButton *btnQuit;
};

#endif