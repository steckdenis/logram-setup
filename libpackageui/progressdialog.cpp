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
using namespace LogramUi;

struct ProgressDialog::Private
{
    QTimer *closeTimer, *showTimer;
    Logram::Progress *progressToDelete;
    
    bool canceled;
    QPushButton *btnCancel;
    QVBoxLayout *layout;
    
    ProgressList *list;
};

ProgressDialog::ProgressDialog(QWidget *parent) : QDialog(parent)
{
    d = new Private;
    d->canceled = false;
    d->progressToDelete = 0;
    
    // Créer le bouton d'annulation
    d->btnCancel = new QPushButton(this);
    d->btnCancel->setText(tr("&Annuler"));
    d->btnCancel->setIcon(QIcon::fromTheme("dialog-cancel"));
    
    connect(d->btnCancel, SIGNAL(clicked(bool)), this, SLOT(cancelClicked()));
    
    // Initialiser la liste des progressions
    d->list = new ProgressList(this);
    
    // Remplir les layouts
    d->layout = new QVBoxLayout(this);
    setLayout(d->layout);
    
    QHBoxLayout *hl = new QHBoxLayout();
    
    hl->addStretch();
    hl->addWidget(d->btnCancel);
    
    d->layout->addWidget(d->list);
    d->layout->addStretch();
    d->layout->addLayout(hl);
    
    // Timer pour fermer la fenêtre 1 seconde après la dernière progression
    d->closeTimer = new QTimer(this);
    d->showTimer = new QTimer(this);
    
    d->closeTimer->setSingleShot(true);
    d->showTimer->setSingleShot(true);
    
    connect(d->closeTimer, SIGNAL(timeout()), this, SLOT(hideElapsed()));
    connect(d->showTimer, SIGNAL(timeout()), this, SLOT(showElapsed()));
}

ProgressDialog::~ProgressDialog()
{

}

void ProgressDialog::addProgress(Progress *progress)
{
    d->canceled = false;
    
    // S'assurer qu'on ne ferme pas la fenêtre
    d->closeTimer->stop();
    
    if (d->progressToDelete != 0)
    {
        d->list->endProgress(d->progressToDelete);
        d->progressToDelete = 0;
    }
    
    // Bien afficher la fenêtre, 1/2 après le début de la progression, pour que celles qui sont rapide (DL de métadonnées) ne
    // gênent pas
    if (!isVisible() && !d->showTimer->isActive())
    {
        d->showTimer->start(500);
    }
    
    d->list->addProgress(progress);
}

void ProgressDialog::updateProgress(Progress *progress)
{
    QApplication::processEvents();
    
    if (d->canceled) return;
    
    d->list->updateProgress(progress);
}

void ProgressDialog::endProgress(Progress *progress)
{
    // Si on n'a plus de progressions
    if (d->list->count() == 1)
    {
        // On ne retire pas la dernière progression, il faut que l'utilisateur la voie. On attend une seconde
        d->closeTimer->start(1000);
        d->progressToDelete = progress;
        
        return;
    }
    
    d->list->endProgress(progress);
}

bool ProgressDialog::canceled()
{
    return d->canceled;
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    cancelClicked();
}

void ProgressDialog::cancelClicked()
{
    d->canceled = true;
    hide();
    
    // Supprimer les progressions
    d->list->clear();
    
    emit rejected();
}

void ProgressDialog::hideElapsed()
{
    // Supprimer la dernière progression, si elle existe
    d->list->endProgress(d->progressToDelete);
    
    // Fermer
    hide();
    emit accepted();
}

void ProgressDialog::showElapsed()
{
    if (d->closeTimer->isActive())
    {
        // Ne pas afficher une boîte de dialogue pour la fermer ensuite
        return;
    }
    
    show();
}

#include "progressdialog.moc"
