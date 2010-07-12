/*
 * installpage.cpp
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

#include "installpage.h"
#include "installwizard.h"
#include "mainwindow.h"

#include <packagesystem.h>
#include <packagelist.h>
#include <communication.h>

using namespace Logram;

InstallPage::InstallPage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    _cancel = false;
    wizard = _wizard;
    win = wizard->mainWindow();
    ps = wizard->packageSystem();

    setupUi(this);
    
    // Liste des progressions
    progressList = new ProgressList(areaProgresses);
    QWidget *pgsWidget = areaProgresses->widget();
    QVBoxLayout *vlayout = new QVBoxLayout(pgsWidget);
    
    pgsWidget->setLayout(vlayout);
    
    vlayout->addWidget(progressList);
    vlayout->addStretch();
    
    // Slots
    connect(wizard, SIGNAL(rejected()), this, SLOT(wizardRejected()));
}

void InstallPage::initializePage()
{
    wizard->button(QWizard::NextButton)->setEnabled(false);
    packageList = wizard->packageList();
        
    // On a besoin des progressions
    win->disableProgressions();
    connect(ps, SIGNAL(progress(Logram::Progress *)), this, SLOT(progress(Logram::Progress *)));
    connect(ps, SIGNAL(communication(Logram::Package*,Logram::Communication*)), 
            this, SLOT(communication(Logram::Package*,Logram::Communication*)));
    
    // Installer les paquets
    if (!packageList->process())
    {
        win->psError();
        
        delete packageList;
        delete wizard->solver();
        wizard->setSolver(0);
        
        wizard->reject();
        return;
    }

    // MainWindow peut à nouveau gérer les progressions
    win->enableProgressions();
    disconnect(this, SLOT(progress(Logram::Progress *)));
    disconnect(this, SLOT(communication(Logram::Package*,Logram::Communication*)));
    
    wizard->button(QWizard::NextButton)->setEnabled(true);
}

void InstallPage::wizardRejected()
{
    _cancel = true;
}

void InstallPage::communication(Package* sender, Communication* comm)
{
    // On ne gère que les communications de type Message, qui seront affichées sur la page Done
    if (comm->type() != Communication::Message)
    {
        return;
    }
    
    PackageMessage msg;
    
    msg.pkg = sender;
    msg.title = comm->title();
    msg.message = comm->description();
    
    wizard->addMessage(msg);
}

void InstallPage::progress(Progress* progress)
{
    QApplication::processEvents();
    
    if (progress->action == Progress::Create)
    {
        if (progress->type == Progress::GlobalDownload)
        {
            pgsDownload->setMaximum(progress->total);
            pgsDownload->setValue(0);
        }
        else if (progress->type == Progress::PackageProcess)
        {
            pgsInstall->setMaximum(progress->total);
            pgsDownload->setValue(0);
        }
        else
        {
            progressList->addProgress(progress);
        }
    }
    else if (progress->action == Progress::Update)
    {
        progress->canceled = _cancel;
        
        if (progress->type == Progress::GlobalDownload)
        {
            pgsDownload->setValue(progress->current + 1);
            lblDownloadMore->setText(tr("Téléchargement de %1").arg(progress->info));
        }
        else if (progress->type == Progress::PackageProcess)
        {
            pgsInstall->setValue(progress->current + 1);
            
            Package *pkg = static_cast<Package *>(progress->data);
            
            if (pkg != 0)
            {
                lblInstallMore->setText(pkg->shortDesc());
                
                switch (pkg->action())
                {
                    case Solver::Install:
                        lblInstall->setText(tr("Installation de %1").arg(progress->info));
                        break;
                    case Solver::Remove:
                        lblInstall->setText(tr("Suppression de %1").arg(progress->info));
                        break;
                    case Solver::Purge:
                        lblInstall->setText(tr("Suppression totale de %1").arg(progress->info));
                        break;
                    case Solver::Update:
                        lblInstall->setText(tr("Mise à jour de %1 vers %2").arg(progress->info, pkg->upgradePackage()->version()));
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            progressList->updateProgress(progress);
        }
    }
    else
    {
        if (progress->type != Progress::PackageProcess && progress->type != Progress::GlobalDownload)
        {
            progressList->endProgress(progress);
        }
    }
}

#include "installpage.moc"
