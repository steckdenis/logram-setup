/*
 * branchepage.cpp
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

#include "branchepage.h"
#include "installwizard.h"
#include "actionpage.h"
#include "utils.h"

#include <packagesystem.h>
#include <solver.h>
#include <packagelist.h>
#include <package.h>
#include <databasepackage.h>

#include <QGridLayout>
#include <QtDebug>

using namespace Logram;
using namespace LogramUi;

BranchePage::BranchePage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    wizard = _wizard;
    nextOk = false;
    level = 0;
    packageList = 0;
    ps = wizard->packageSystem();

    setupUi(this);
    
    btnUp->setIcon(QIcon::fromTheme("go-up"));
    btnUseChoice->setIcon(QIcon::fromTheme("go-down"));
    
    connect(btnUp, SIGNAL(clicked(bool)), this, SLOT(choiceUp()));
    connect(btnUseChoice, SIGNAL(clicked(bool)), this, SLOT(choiceSelect()));
}

bool BranchePage::isComplete()
{
    return nextOk;
}

int BranchePage::nextId() const
{
    if (packageList && packageList->numLicenses() != 0)
    {
        return InstallWizard::Licenses;
    }
    else
    {
        return InstallWizard::Install;
    }
}

void BranchePage::initializePage()
{
    solver = ps->newSolver();
    wizard->setSolver(solver);
        
    solver->setUseDeps(true);
    solver->setUseInstalled(true);
    
    // On ne peut pas passer à l'étape suivante tant que tout n'est pas prêt
    nextOk = false;
    
    // Suggestions
    ActionPage *actPage = static_cast<ActionPage *>(wizard->page(InstallWizard::Actions));
    
    solver->setInstallSuggests(actPage->chkInstallSuggests->isEnabled());
    
    // Liste des paquets
    QVector<Package *> packages = wizard->packages();
    
    for (int i=0; i<packages.count(); ++i)
    {
        Package *pkg = packages.at(i);
        
        solver->addPackage(pkg->name() + "=" + pkg->version(), pkg->action());
    }
    
    // Résoudre les dépendances et peser
    if (!solver->solve() || !solver->weight())
    {
        wizard->solverError();
        
        delete solver;
        
        wizard->reject();
        return;
    }
    
    // Voir s'il y a des choix
    bool ended = true;
    
    if (!solver->beginList(ended))
    {
        wizard->solverError();
        
        delete solver;
        
        wizard->reject();
        return;
    }
    
    // Afficher les choix et les paquets dans la liste
    updateChoiceList();
    
    // Si ended est à true, on peut continuer
    if (ended)
    {
        btnUp->setEnabled(false);
        btnUseChoice->setEnabled(false);
        nextOk = true;
        
        emit completeChanged();
    }
}

void BranchePage::choiceSelect()
{
    bool ended = true;
    
    QListWidgetItem *item = listPackages->currentItem();
    if (item == 0) return;
    
    int index = item->data(Qt::UserRole).toInt();
    if (index < 1) return;
    
    if (!solver->continueList(index - 1, ended))
    {
        wizard->solverError();
        return;
    }
    
    level++;
    btnUseChoice->setEnabled(true);
    
    updateChoiceList();
    
    if (ended)
    {
        btnUp->setEnabled(false);
        btnUseChoice->setEnabled(false);
        nextOk = true;
        
        emit completeChanged();
    }
}

void BranchePage::choiceUp()
{
    solver->upList();
    
    updateChoiceList();
    
    btnUp->setEnabled(true);
    btnUseChoice->setEnabled(true);
    nextOk = false;
    
    level--;
    
    if (level == 0)
    {
        btnUseChoice->setEnabled(false);
    }
    
    emit completeChanged();
}

void BranchePage::updateChoiceList()
{
    // Commencer par effacer la liste
    listPackages->clear();
    
    // Ajouter les paquets déjà connus du Solver
    if (packageList != 0)
    {
        delete packageList;
    }
    
    packageList = solver->list();
    wizard->setPackageList(packageList);
    
    QVector<Solver::Node *> choices = solver->choices();
    int instSize = 0, dlSize = 0;
    
    for (int i=0; i<packageList->count(); ++i)
    {
        Package *pkg = packageList->at(i);
        QListWidgetItem *item = new QListWidgetItem(listPackages);
        listPackages->addItem(item);
        
        int pdlsize = 0;
        int pinstsize = 0;
        
        if (pkg->action() == Solver::Install && pkg->origin() != Package::File)
        {
            pdlsize = pkg->downloadSize();
            pinstsize = pkg->installSize();
        }
        else if (pkg->action() == Solver::Remove || pkg->action() == Solver::Purge)
        {
            pinstsize = -pkg->installSize();
        }
        else if (pkg->action() == Solver::Update)
        {
            DatabasePackage *other = pkg->upgradePackage();
            
            if (other != 0)
            {
                pdlsize += other->downloadSize();
                pinstsize = other->installSize() - pkg->installSize();
            }
        }
        
        QString sInstSize, sDlSize;
        
        sInstSize = QString(pinstsize < 0 ? "<span style=\"color:#ff0000;\">" : "") + PackageSystem::fileSizeFormat(pinstsize) +
                    QString(pinstsize < 0 ? "</span>" : "");
                    
        sDlSize = PackageSystem::fileSizeFormat(pdlsize);
        
        instSize += pinstsize;
        dlSize += pdlsize;
        
        // Créer un widget spécial pour ce paquet
        QWidget *widget = Utils::choiceWidget(this,
                                       false, 
                                       sDlSize,
                                       sInstSize, 
                                       pkg->shortDesc(),
                                       pkg->name(),
                                       pkg->version(),
                                       (pkg->action() == Solver::Update ? pkg->upgradePackage()->version() : QString()),
                                       pkg->action(), 
                                       pkg->flags());
        
        item->setSizeHint(widget->minimumSizeHint());
        item->setFlags(Qt::ItemIsEnabled);              // Activé, mais pas sélectionnable
        listPackages->setItemWidget(item, widget);
    }
    
    // Résumé
    QString summary;
    
    if (instSize < 0)
    {
        summary = tr("Téléchargement de %1, suppression de %2, %3 licence(s) à accepter");
    }
    else
    {
        summary = tr("Téléchargement de %1, installation de %2, %3 licence(s) à accepter");
    }
    
    lblSummary->setText(summary.arg(PackageSystem::fileSizeFormat(dlSize),
                                    PackageSystem::fileSizeFormat(qAbs(instSize)))
                               .arg(packageList->numLicenses()));
    
    // Ajouter les choix
    QListWidgetItem *minItem = 0;
    int minWeight = 0;
    
    for (int i=0; i<choices.count(); ++i)
    {
        Solver::Node *node = choices.at(i);
        QListWidgetItem *item = new QListWidgetItem(listPackages);
        listPackages->addItem(item);
        
        // Créer un widget spécial pour ce choix
        QWidget *widget = Utils::choiceWidget(this,
                                       i == 0,
                                       tr("Entre %1 et %2").arg(PackageSystem::fileSizeFormat(node->minDlSize),
                                                                PackageSystem::fileSizeFormat(node->maxDlSize)),
                                       tr("Entre %1 et %2").arg(
                                           QString((node->minInstSize < 0 ? "<span style=\"color:#ff0000;\">" : "")) +
                                           PackageSystem::fileSizeFormat(node->minInstSize) +
                                           QString((node->minInstSize < 0 ? "</span>" : "")),
                                                                //
                                           QString((node->maxInstSize < 0 ? "<span style=\"color:#ff0000;\">" : "")) +
                                           PackageSystem::fileSizeFormat(node->maxInstSize) +
                                           QString((node->maxInstSize < 0 ? "</span>" : ""))),
                                       (node->flags & Solver::Node::Wanted ? node->package->shortDesc() : tr("(n'ajoute pas de dépendance)")),
                                       node->package->name(),
                                       node->package->version(),
                                       (node->package->action() == Solver::Update ? node->package->upgradePackage()->version() : QString()),
                                       node->package->action(),
                                       0);
        
        item->setSizeHint(widget->minimumSizeHint());
        item->setData(Qt::UserRole, i + 1);             // i + 1 : pas de 0, qui permet de détecter un mauvais choix
        listPackages->setItemWidget(item, widget);
        
        if (minItem == 0 || node->minWeight < minWeight)
        {
            minWeight = node->minWeight;
            minItem = item;
        }
    }
    
    if (minItem != 0)
    {
        listPackages->setCurrentItem(minItem);
    }
}

#include "branchepage.moc"
