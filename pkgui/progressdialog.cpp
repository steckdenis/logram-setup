/*
 * progressdialog.cpp
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

#include "progressdialog.h"
#include "progresslist.h"

#include <package.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QIcon>
#include <QApplication>

using namespace Logram;

ProgressDialog::ProgressDialog(QWidget *parent) : QDialog(parent)
{
    _canceled = false;
    progressToDelete = 0;
    
    // Créer le bouton d'annulation
    btnCancel = new QPushButton(this);
    btnCancel->setText(tr("&Annuler"));
    btnCancel->setIcon(QIcon::fromTheme("dialog-cancel"));
    
    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(cancelClicked()));
    
    // Initialiser la liste des progressions
    list = new ProgressList(this);
    
    // Remplir les layouts
    _layout = new QVBoxLayout(this);
    setLayout(_layout);
    
    QHBoxLayout *hl = new QHBoxLayout();
    
    hl->addStretch();
    hl->addWidget(btnCancel);
    
    _layout->addWidget(list);
    _layout->addStretch();
    _layout->addLayout(hl);
    
    // Timer pour fermer la fenêtre 1 seconde après la dernière progression
    closeTimer = new QTimer(this);
    showTimer = new QTimer(this);
    
    closeTimer->setSingleShot(true);
    showTimer->setSingleShot(true);
    
    connect(closeTimer, SIGNAL(timeout()), this, SLOT(hideElapsed()));
    connect(showTimer, SIGNAL(timeout()), this, SLOT(showElapsed()));
}

ProgressDialog::~ProgressDialog()
{

}

void ProgressDialog::addProgress(Progress *progress)
{
    _canceled = false;
    
    // S'assurer qu'on ne ferme pas la fenêtre
    closeTimer->stop();
    
    if (progressToDelete != 0)
    {
        list->endProgress(progressToDelete);
        progressToDelete = 0;
    }
    
    // Bien afficher la fenêtre, 1/2 après le début de la progression, pour que celles qui sont rapide (DL de métadonnées) ne
    // gênent pas
    if (!isVisible() && !showTimer->isActive())
    {
        showTimer->start(500);
    }
    
    list->addProgress(progress);
}

void ProgressDialog::updateProgress(Progress *progress)
{
    QApplication::processEvents();
    
    if (_canceled) return;
    
    list->updateProgress(progress);
}

void ProgressDialog::endProgress(Progress *progress)
{
    // Si on n'a plus de progressions
    if (list->count() == 1)
    {
        // On ne retire pas la dernière progression, il faut que l'utilisateur la voie. On attend une seconde
        closeTimer->start(1000);
        progressToDelete = progress;
        
        return;
    }
    
    list->endProgress(progress);
}

bool ProgressDialog::canceled()
{
    return _canceled;
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    cancelClicked();
}

void ProgressDialog::cancelClicked()
{
    _canceled = true;
    hide();
    
    // Supprimer les progressions
    list->clear();
    
    emit rejected();
}

void ProgressDialog::hideElapsed()
{
    // Supprimer la dernière progression, si elle existe
    list->endProgress(progressToDelete);
    
    // Fermer
    hide();
    emit accepted();
}

void ProgressDialog::showElapsed()
{
    if (closeTimer->isActive())
    {
        // Ne pas afficher une boîte de dialogue pour la fermer ensuite
        return;
    }
    
    show();
}

#include "progressdialog.moc"
