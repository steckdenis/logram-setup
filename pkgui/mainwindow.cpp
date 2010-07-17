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
#include "progressdialog.h"
#include "communicationdialog.h"

#include <QIcon>
#include <QMessageBox>

#include <communication.h>
#include <packagemetadata.h>

using namespace Logram;

MainWindow::MainWindow() : QMainWindow(0)
{
    _error = false;
    _progresses = true;
    ps = 0;
    setupUi(this);
    
    // Progressions
    progressDialog = new ProgressDialog(this);
    progressDialog->setModal(true);
    
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
    btnSearch->setIcon(QIcon::fromTheme("edit-find"));
    btnFlags->setIcon(QIcon::fromTheme("flag"));
    
    txtSearch->setFocus();
    
    // Icônes de la liste des filtres de paquets
    for (int i=0; i<cboFilter->count(); ++i)
    {
        PackageFilter filter = (PackageFilter)i;
        
        switch (filter)
        {
            case NoFilter:
                cboFilter->setItemIcon(i, QIcon::fromTheme("view-filter"));
                break;
            case Installed:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-install.png"));
                break;
            case NotInstalled:
                cboFilter->setItemIcon(i, QIcon(":/images/package.png"));
                break;    
            case Updateable:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-update.png"));
                break;
            case Orphan:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-purge.png"));
                break;
        }
    }
        
    // Ajouter les actions
    treePackages->addAction(actInstallPackage);
    treePackages->addAction(actRemovePackage);
    treePackages->addAction(actPurge);
    treePackages->addAction(actDeselect);
    
    // Initialiser la gestion des paquets
    ps = new PackageSystem(this);
    
    ps->loadConfig();
    connect(ps, SIGNAL(communication(Logram::Package *, Logram::Communication *)), 
            this, SLOT(communication(Logram::Package *, Logram::Communication *)));
    connect(ps, SIGNAL(progress(Logram::Progress *)),
            this, SLOT(progress(Logram::Progress *)));
    
    if (!ps->init())
    {
        psError();
        _error = true;
        return;
    }
    
    // Remplir les sections
    populateSections();
    
    // Remplir la liste des paquets
    displayPackages(NoFilter, QString());
    
    // Connexion des signaux
    connect(actAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(actQuit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(treePackages, SIGNAL(itemActivated(QTreeWidgetItem *, int)), 
            this,         SLOT(itemActivated(QTreeWidgetItem *)));
    connect(lblWebsite, SIGNAL(linkActivated(const QString &)),
            this,         SLOT(websiteActivated(const QString &)));
    connect(lblLicense, SIGNAL(linkActivated(const QString &)),
            this,         SLOT(licenseActivated(const QString &)));
    connect(btnFlags, SIGNAL(clicked(bool)),
            this,       SLOT(showFlags()));
    connect(btnSearch, SIGNAL(clicked(bool)),
            this,        SLOT(searchPackages()));
    connect(txtSearch, SIGNAL(returnPressed()),
            this,        SLOT(searchPackages()));
    connect(cboFilter, SIGNAL(currentIndexChanged(int)),
            this,        SLOT(searchPackages()));
    connect(treeSections, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this,           SLOT(searchPackages()));
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

PackageSystem* MainWindow::packageSystem() const
{
    return ps;
}

void MainWindow::databaseUpdate()
{
    if (!ps->update())
    {
        psError();
        return;
    }
    
    if (!ps->reset())
    {
        psError();
        return;
    }
    
    populateSections();
    searchPackages();
    
    QMessageBox::information(this, tr("Mise à jour de la base de donnée"), tr("La base de donnée a été mise à jour avec succès."));
}

QPixmap MainWindow::pixmapFromData(const QByteArray &data, int width, int height)
{
    QImage img = QImage::fromData(data);
    return QPixmap::fromImage(img).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MainWindow::psError()
{
    PackageError *err = ps->lastError();
    QString s;
    
    if (err == 0)
    {
        s = tr("Pas d'erreur ou erreur inconnue");
    }
    else
    {
        switch (err->type)
        {
            case PackageError::OpenFileError:
                s = tr("Impossible d'ouvrir le fichier ");
                break;

            case PackageError::MapFileError:
                s = tr("Impossible de mapper le fichier ");
                break;

            case PackageError::ProcessError:
                s = tr("Erreur dans la commande ");
                break;

            case PackageError::DownloadError:
                s = tr("Impossible de télécharger ");
                break;

            case PackageError::ScriptException:
                s = tr("Erreur dans le QtScript ");
                break;
                
            case PackageError::SignatureError:
                s = tr("Mauvaise signature GPG du fichier ");
                break;

            case PackageError::SHAError:
                s = tr("Mauvaise somme SHA1, fichier corrompu : ");
                break;

            case PackageError::PackageNotFound:
                s = tr("Paquet inexistant : ");
                break;

            case PackageError::BadDownloadType:
                s = tr("Mauvais type de téléchargement, vérifier sources.list : ");
                break;

            case PackageError::OpenDatabaseError:
                s = tr("Impossible d'ouvrir la base de donnée : ");
                break;

            case PackageError::QueryError:
                s = tr("Erreur dans la requête : ");
                break;

            case PackageError::SignError:
                s = tr("Impossible de vérifier la signature : ");
                break;
                
            case PackageError::InstallError:
                s = tr("Impossible d'installer le paquet ");
                break;

            case PackageError::ProgressCanceled:
                s = tr("Opération annulée : ");
                break;
        }
    }
    
    s += err->info;
    
    if (!err->more.isEmpty())
    {
        s += "<br /><br />";
        s += err->more;
    }
    
    QMessageBox::critical(this, tr("Erreur"), s);
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