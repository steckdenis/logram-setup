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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>

#include <communication.h>
#include <packagemetadata.h>

#include <progressdialog.h>
#include <communicationdialog.h>
#include <packagedataprovider.h>
#include <infopane.h>
#include <utils.h>
#include <installwizard.h>

#include <QtDebug>

using namespace Logram;
using namespace LogramUi;

MainWindow::MainWindow() : QWidget(0)
{
    _error = false;
    ps = 0;
    
    resize(600, 300);
    
    // Vérifier qu'on a les bons paramètres
    QString filename;
    
    if (qApp->arguments().count() != 2)
    {
        filename = QFileDialog::getOpenFileName(this, tr("Installer un paquet"), QString(), tr("Paquets Logram (*.lpk)"));
    }
    else
    {
        filename = qApp->arguments().at(1);
    }
    
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
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *btnLayout = new QHBoxLayout(0);
    btnOk = new QPushButton(this);
    btnQuit = new QPushButton(this);
    
    btnOk->setText(tr("&Installer"));
    btnOk->setIcon(QIcon(":/images/pkg-install.png"));
    
    btnQuit->setText(tr("&Quitter"));
    btnQuit->setIcon(QIcon::fromTheme("application-exit"));
    
    infoPane = new InfoPane(this);
    
    setLayout(mainLayout);
    
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnQuit);
    
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(infoPane);
    mainLayout->addLayout(btnLayout);
    
    // Définir les icônes
    setWindowIcon(QIcon(":/images/package.png"));
    setWindowTitle(tr("Installer un paquet hors des dépôts"));
    
    // Connexion des signaux
    connect(btnOk, SIGNAL(clicked(bool)),
            this,    SLOT(btnInstallClicked()));
    connect(btnQuit, SIGNAL(clicked(bool)),
            this,      SLOT(close()));
    
    // Peupler la page d'infos
    displayPackage(filename);
}

MainWindow::~MainWindow()
{
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

void MainWindow::displayPackage(const QString& filename)
{
    Package *pkg;
    
    if (!ps->package(filename, QString(), pkg))
    {
        Utils::packageSystemError(ps);
        _error = true;
        return;
    }
    
    Q_ASSERT(pkg->origin() == Package::File);
    
    fpkg = static_cast<FilePackage *>(pkg);
    
    PackageDataProvider *provider = new PackageDataProvider(fpkg, ps);
    infoPane->displayData(provider);
}

void MainWindow::btnInstallClicked()
{
    InstallWizard wizard(ps, this);
    
    wizard.addPackage(fpkg);
    
    disableProgressions();
    wizard.exec();
    enableProgressions();
    
    if (wizard.result() == QDialog::Accepted)
    {
        btnOk->setEnabled(false);
        btnQuit->setFocus();
    }
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
