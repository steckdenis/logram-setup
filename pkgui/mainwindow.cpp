/*
 * mainwindow.cpp
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

#include "mainwindow.h"

#include <QIcon>
#include <QMessageBox>
#include <QSettings>

#include <communication.h>
#include <packagemetadata.h>

#include <progressdialog.h>
#include <communicationdialog.h>
#include <infopane.h>
#include <searchbar.h>
#include <categoryview.h>
#include <filterinterface.h>
#include <utils.h>

#include <string>
#include <sstream>

using namespace Logram;
using namespace LogramUi;

MainWindow::MainWindow() : QMainWindow(0)
{
    _error = false;
    _progresses = true;
    ps = 0;
    setupUi(this);
    
    // Changer comment les docks se placent.
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    
    treePackages->sortByColumn(0, Qt::AscendingOrder);
    
    // Initialiser la gestion des paquets
    ps = new PackageSystem(this);
    
    ps->loadConfig();
    connect(ps, SIGNAL(communication(Logram::Package *, Logram::Communication *)), 
            this, SLOT(communication(Logram::Package *, Logram::Communication *)));
    connect(ps, SIGNAL(progress(Logram::Progress *)),
            this, SLOT(progress(Logram::Progress *)));
    
    if (!ps->init())
    {
        Utils::packageSystemError(ps);
        _error = true;
        return;
    }
    
    // Progressions
    progressDialog = new ProgressDialog(this);
    progressDialog->setModal(true);
    
    // Ajouter les contrôles LogramUi à la fenêtre
    filterInterface = new FilterInterface(this);
    connect(filterInterface, SIGNAL(dataChanged()), this, SLOT(searchPackages()));
    
    searchBar = new SearchBar(filterInterface, this);
    mainLayout->insertWidget(0, searchBar);
    
    infoPane = new InfoPane(this);
    docInfos->setWidget(infoPane);
    
    sections = new CategoryView(ps, filterInterface, this);
    docSections->setWidget(sections);
    
    // Définir les icônes
    setWindowIcon(QIcon(":/images/icon.svg"));
    
    actInstallPackage->setIcon(QIcon(":/images/pkg-install.png"));
    actRemovePackage->setIcon(QIcon(":/images/pkg-remove.png"));
    actPurge->setIcon(QIcon(":/images/pkg-purge.png"));
    actDeselect->setIcon(QIcon(":/images/package.png"));
    actApplyChanges->setIcon(QIcon(":/images/list-apply.png"));
    actCancelChanges->setIcon(QIcon(":/images/list-cancel.png"));
    actUpdate->setIcon(QIcon(":/images/repository.png"));
    actQuit->setIcon(QIcon::fromTheme("application-exit"));
    
    actAboutPkgui->setIcon(QIcon(":/images/icon.svg"));
    
    btnListApply->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    btnListClean->setIcon(QIcon::fromTheme("dialog-cancel"));
    
    searchBar->setFocus();
    treePackages->setMouseTracking(true);
        
    // Ajouter les actions
    treePackages->addAction(actInstallPackage);
    treePackages->addAction(actRemovePackage);
    treePackages->addAction(actPurge);
    treePackages->addAction(actDeselect);
    
    // Remplir la liste des paquets
    searchPackages();
    
    // Connexion des signaux
    connect(actAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(actQuit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(treePackages, SIGNAL(itemActivated(QTreeWidgetItem *, int)), 
            this,         SLOT(itemActivated(QTreeWidgetItem *)));
    connect(actInstallPackage, SIGNAL(triggered(bool)),
            this,                SLOT(installPackage()));
    connect(actRemovePackage, SIGNAL(triggered(bool)),
            this,               SLOT(removePackage()));
    connect(actPurge, SIGNAL(triggered(bool)),
            this,       SLOT(purgePackage()));
    connect(actDeselect, SIGNAL(triggered(bool)),
            this,          SLOT(deselectPackage()));
    connect(actApplyChanges, SIGNAL(triggered(bool)),
            this,              SLOT(applyList()));
    connect(actCancelChanges, SIGNAL(triggered(bool)),
            this,               SLOT(cancelList()));
    connect(btnListApply, SIGNAL(clicked(bool)),
            this,           SLOT(applyList()));
    connect(btnListClean, SIGNAL(clicked(bool)),
            this,           SLOT(cancelList()));
    connect(actUpdate, SIGNAL(triggered(bool)),
            this,        SLOT(databaseUpdate()));
    
    // Restaurer l'état de la fenêtre
    QSettings settings("Logram", "Pkgui");
    
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
#if 0
    // Envoyer des progressions
    int p1 = ps->startProgress(Progress::Download, 1024*1024);      // 1Mio
    int p2 = ps->startProgress(Progress::Download, 1536*1024);      // 1,5Mio
    
    int c1 = 0;
    int c2 = 0;
    
    show();
    
    #define CHK(x) if (!x) return;
    
    while (true)
    {
        QApplication::processEvents();
        
        CHK(ps->sendProgress(p1, c1, "http://archive.logram-project.org/pool/i/initng~0.6.99+git20091223~1.i686.lpk"));
        usleep(50000);
        CHK(ps->sendProgress(p2, c2, "http://archive.logram-project.org/pool/a/amarok~2.2.2~5.i686.lpk"));
        
        c1 += 21480 + (rand() % 2200);    // entre 200 et 220 Kio/s
        c2 += 11240 + (rand() % 2200);    // entre 100 et 120 Kio/s
        
        if (c1 >= 1024*1024)
        {
            c1 = 0;
            ps->endProgress(p1);
            
            p1 = ps->startProgress(Progress::Download, 1024*1024);      // 1Mio
        }
        
        if (c2 >= 1536*1024)
        {
            c2 = 0;
            ps->endProgress(p2);
            
            p2 = ps->startProgress(Progress::Download, 1536*1024);      // 1,5Mio
        }
        
        // Attendre une seconde
        usleep(50000);
    }
#endif
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings("Logram", "Pkgui");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    // Nécessaire pour deleter les Package avant le PackageSystem
    delete treeActions;
    delete treePackages;
    
    delete ps;
    delete progressDialog;
}

bool MainWindow::error() const
{
    return _error;
}

void MainWindow::disableProgressions()
{
    _progresses = false;
}

void MainWindow::enableProgressions()
{
    _progresses = true;
}

void MainWindow::databaseUpdate()
{
    if (!ps->update(PackageSystem::Minimal | PackageSystem::Sections))
    {
        Utils::packageSystemError(ps);
        return;
    }
    
    if (!ps->reset())
    {
        Utils::packageSystemError(ps);
        return;
    }
    
    sections->reload();
    searchPackages();
    
    QMessageBox::information(this, tr("Mise à jour de la base de donnée"), tr("La base de donnée a été mise à jour avec succès."));
}

void MainWindow::communication(Logram::Package *sender, Logram::Communication *comm)
{
    if (comm->type() != Communication::Question)
    {
        // Les messages sont gérés par InstallWizard
        return;
    }
    
    CommunicationDialog dialog(sender, comm, this);
    
    dialog.exec();
}

void MainWindow::progress(Progress *progress)
{
    if (!_progresses) return;
    
    if (progress->action == Progress::Create)
    {
        progressDialog->addProgress(progress);
    }
    else if (progress->action == Progress::Update)
    {
        progressDialog->updateProgress(progress);
        
        if (progressDialog->canceled())
        {
            progress->canceled = true;
        }
    }
    else
    {
        progressDialog->endProgress(progress);
    }
}

#include "mainwindow.moc"
