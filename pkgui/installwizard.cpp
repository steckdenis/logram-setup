/*
 * installwizard.cpp
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

#include "installwizard.h"
#include "mainwindow.h"
#include "progresslist.h"

#include "actionpage.h"
#include "branchepage.h"
#include "licensepage.h"
#include "installpage.h"
#include "donepage.h"

#include <packagesystem.h>
#include <databasepackage.h>
#include <solver.h>
#include <packagelist.h>

#include <QTreeWidget>
#include <QIcon>
#include <QPixmap>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>

#include <QtDebug>

using namespace Logram;

InstallWizard::InstallWizard(MainWindow* parent): QWizard(parent)
{
    win = parent;
    _packageList = 0;
    _solver = 0;
    ps = parent->packageSystem();
    
    resize(800, 600);
    setWindowTitle(tr("Installer et supprimer des paquets"));
    setPixmap(QWizard::LogoPixmap, QPixmap(":/images/package.png"));
    
    // Ajouter des icônes aux boutons
    button(BackButton)->setIcon(QIcon::fromTheme("go-previous"));
    button(NextButton)->setIcon(QIcon::fromTheme("go-next"));
    button(CancelButton)->setIcon(QIcon::fromTheme("dialog-cancel"));
    button(CommitButton)->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    button(FinishButton)->setIcon(QIcon::fromTheme("dialog-ok"));
    
    setButtonText(BackButton, tr("« &Précédant"));
    setButtonText(NextButton, tr("&Suivant »"));
    
    // Ajouter les pages
    setPage(Actions, new ActionPage(this));
    setPage(Branches, new BranchePage(this));
    setPage(Licenses, new LicensePage(this));
    setPage(Install, new InstallPage(this));
    setPage(Done, new DonePage(this));
    
    // Signaux
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(pageChanged(int)));
    
    //btnUp->setIcon(QIcon::fromTheme("go-up"));
    //btnUseChoice->setIcon(QIcon::fromTheme("go-next"));
    
    /*
    
    // Liste des progressions
    progressList = new ProgressList(areaProgresses);
    QWidget *pgsWidget = areaProgresses->widget();
    QVBoxLayout *vlayout = new QVBoxLayout(pgsWidget);
    
    pgsWidget->setLayout(vlayout);
    
    vlayout->addWidget(progressList);
    vlayout->addStretch();
    */
}

MainWindow *InstallWizard::mainWindow() const
{
    return win;
}

PackageList *InstallWizard::packageList() const
{
    return _packageList;
}

Solver *InstallWizard::solver() const
{
    return _solver;
}

PackageSystem *InstallWizard::packageSystem() const
{
    return ps;
}

void InstallWizard::setPackageList(PackageList *_packageList)
{
    this->_packageList = _packageList;
}

void InstallWizard::setSolver(Solver *_solver)
{
    this->_solver = _solver;
}

void InstallWizard::addMessage(const PackageMessage &message)
{
    messages.append(message);
}

QList<PackageMessage> InstallWizard::packageMessages() const
{
    return messages;
}

void InstallWizard::pageChanged(int id)
{
    // On ne peut jamais revenir en arrière
    button(BackButton)->setEnabled(false);
}

// NOTE: Synchronize with lpm/package.cpp:solverError
void InstallWizard::solverError()
{
    // Trouver le noeud du solveur qui a une erreur
    Solver::Node *errorNode = _solver->errorNode();
    
    if (errorNode == 0)
    {
        // Ce n'est pas un noeud qui a provoqué l'erreur, voir si ce n'est pas PackageSystem
        PackageError *err = ps->lastError();
        
        if (err != 0)
        {
            win->psError();
            return;
        }
        else
        {
            // Erreur par défaut
            QMessageBox::critical(this, tr("Erreur lors de la résolution des dépendances"), tr("Impossible de résoudre les dépendances, mais le problème n'est pas spécifié par la gestion des paquets de Logram. Veuillez rapporter le bug à l'équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Le message suivant pourrait les aider : «errorNode = 0»"));
            return;
        }
    }
    
    // On a un noeud qui a fait des erreurs. Descendre dans son arborescence s'il est en childError
    Solver::Error *err = errorNode->error;
    
    while (err->type == Solver::Error::ChildError && err->other != 0)
    {
        if (err->other->error == 0)
        {
            QMessageBox::critical(this, tr("Erreur lors de la résolution des dépendances"), tr("Impossible de résoudre les dépendances, la structure interne de la gestion des paquets semble cassée. Veuillez rapporter le bug à l'équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Si vous avez les connaissances nécessaires, essayez de lancer «lpm add -G <les paquets que vous installiez, ou préfixés de \"-\" si vous les supprimiez>» dans une console, et postez la sortie complète de cette commande dans le rapport de bug"));
            return;
        }
        
        errorNode = err->other;
        err = errorNode->error;
    }
    
    // err est l'erreur la plus profonde, trouver son nom
    QString pkgname, othername;
    
    if (errorNode->package)
    {
        pkgname = errorNode->package->name() + "~" + errorNode->package->version();
    }
    
    if (err->other && err->other->package)
    {
        othername = err->other->package->name() + "~" + err->other->package->version();
    }
    
    // En fonction du type
    PackageError *perr;
    QString s;
    
    switch (err->type)
    {
        case Solver::Error::NoDeps:
            s = tr("Impossible de trouver la dépendance correspondant à %1").arg(err->pattern);
       
        case Solver::Error::InternalError:
            perr = ps->lastError();
        
            if (perr != 0)
            {
                win->psError();
                return;
            }
            else
            {
                // Erreur par défaut
                QMessageBox::critical(this, tr("Erreur lors de la résolution des dépendances"), tr("Impossible de résoudre les dépendances, mais le problème n'est pas spécifié par la gestion des paquets de Logram. Veuillez rapporter le bug à l'équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Le message suivant pourrait les aider : «InternalError, ps->lastError() = 0»"));
                return;
            }
            
        case Solver::Error::ChildError:
            if (!pkgname.isNull())
            {
                s = tr("Aucun des choix nécessaires à l'installation de %1 n'est possible").arg(pkgname);
                // TODO: Explorer les enfants d'errorNode pour lister les erreurs.
            }
            else
            {
                s = tr("Aucun des choix permettant d'effectuer l'opération que vous demandez n'est possible");
            }
            
        case Solver::Error::SameNameSameVersionDifferentAction:
            s = tr("Le paquet %1 devrait être installé et supprimé en même temps").arg(pkgname);
            
        case Solver::Error::InstallSamePackageDifferentVersion:
            s = tr("Les paquets %1 et %2 devraient être installés en même temps").arg(pkgname, othername);
            
        case Solver::Error::UninstallablePackageInstalled:
            s = tr("Le paquet %1 ne peut être installé mais devrait être installé").arg(pkgname);
            
        case Solver::Error::UnremovablePackageRemoved:
            s = tr("Le paquet %1 ne peut être supprimé mais devrait être supprimé").arg(pkgname);
            
        case Solver::Error::UnupdatablePackageUpdated:
            s = tr("Le paquet %1 ne peut être mis à jour mais devrait être mis à jour").arg(pkgname);
    }
    
    QMessageBox::critical(this, tr("Erreur lors de la résolution des dépendances"), tr("Impossible de résoudre les dépendances : %1. Si cette erreur est survenue juste après que vous ayiez choisi un choix dans la liste, essayez de cliquer sur le bouton «Remonter»").arg(s));
}

#include "installwizard.moc"
