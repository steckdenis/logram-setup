/*
 * progresslist.cpp
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

#include "progresslist.h"

#include <package.h>
#include <packagesystem.h>

#include <QHash>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QFrame>
#include <QLabel>
#include <QTime>

using namespace Logram;
using namespace LogramUi;

struct LogramUi::ProgressList::Private
{
    struct ProgressData
    {
        QWidget *widget;
        QProgressBar *pgs;
        QFrame *frm;
        QLabel *lblInfo, *lblMore;
        QString s;
        QTime lastTime;
        int lastValue;
    };
  
    QHash<Logram::Progress *, ProgressData *> progresses;
    QVBoxLayout *layout;
    int count;
};

ProgressList::ProgressList(QWidget *parent) : QWidget(parent)
{
    d = new Private;
    d->count = 0;
    d->layout = new QVBoxLayout(this);
    setLayout(d->layout);
}

ProgressList::~ProgressList()
{
    delete d;
}

void ProgressList::addProgress(Logram::Progress* progress)
{
    // Créer les différent éléments
    Private::ProgressData *data = new Private::ProgressData;
    
    data->lastValue = 0;
    
    data->widget = new QWidget(this);
    data->pgs = new QProgressBar(data->widget);
    data->frm = new QFrame(data->widget);
    data->lblInfo = new QLabel(data->widget);
    data->lblMore = new QLabel(data->widget);
    
    // Placer les éléments dans le layout
    QVBoxLayout *layout = new QVBoxLayout(data->widget);
    data->widget->setLayout(layout);
    
    layout->addWidget(data->lblInfo);
    layout->addWidget(data->pgs);
    layout->addWidget(data->lblMore);
    layout->addWidget(data->frm);
    
    // Définir les propriétés
    data->frm->setFrameShape(QFrame::HLine);
    data->lblMore->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    data->lblMore->setWordWrap(true);
    
    // Chaîne du type
    Package *pkg;
    switch (progress->type)
    {
        case Progress::Other:
            data->s = tr("Progression : ");
            break;
            
        case Progress::GlobalDownload:
            data->s = tr("Téléchargement de ");
            break;
            
        case Progress::Download:
            data->s = tr("Téléchargement de ");
            break;
            
        case Progress::UpdateDatabase:
            data->s = tr("Mise à jour de la base de donnée : ");
            break;
            
        case Progress::Trigger:
            data->s = tr("Exécution du trigger ");
            break;
            
        default:
            qDebug("Progress::Type unhandled by ProgressList : %i", progress->type);
            break;
    }
    
    data->s = "<b>" + data->s + "</b>";
    
    // Insérer le widget dans le layout principal
    d->layout->addWidget(data->widget);
    d->progresses.insert(progress, data);
    
    d->count++;
}

void ProgressList::updateProgress(Logram::Progress* progress)
{
    Private::ProgressData *data = d->progresses.value(progress);
    
    if (data == 0) return;
    
    // Mettre à jour les valeurs
    data->lblInfo->setText(data->s + progress->info);
    data->pgs->setMaximum(progress->total);
    data->pgs->setValue(progress->current + 1);
    
    if (progress->more.isNull())
    {
        data->lblMore->hide();
    }
    else
    {
        data->lblMore->show();
        data->lblMore->setText(progress->more);
    }
    
    // Si la progression est un téléchargement, mettre à jour quelques infos
    if (progress->type == Progress::Download)
    {
        QTime curTime = QTime::currentTime();
        
        if (data->lastTime.isNull() || data->lastTime.msecsTo(curTime) > 500)
        {
            // Vitesse de DL toutes les demi secondes
            int lastSpeed = (progress->current - data->lastValue) * 1000 / data->lastTime.msecsTo(curTime);
            
            // Mise à jour de la progressbar
            data->pgs->setFormat(QString("%p% (%1 / %2) %3/s").arg(
                PackageSystem::fileSizeFormat(progress->current), 
                PackageSystem::fileSizeFormat(progress->total),
                PackageSystem::fileSizeFormat(lastSpeed)));
            
            data->lastTime = curTime;
            data->lastValue = progress->current;
        }
    }
}

void ProgressList::endProgress(Logram::Progress* progress)
{
    Private::ProgressData *data = d->progresses.value(progress);
    
    if (data == 0) return;
    
    d->progresses.remove(progress);
    delete data->widget;            // Supprime tout le reste aussi
    delete data;
    
    d->count--;
}

int ProgressList::count() const
{
    return d->count;
}

void ProgressList::clear()
{
    QList<Progress *> pgs = d->progresses.keys();          // Liste séparée car endProgress modifie progresses.keys()
    
    foreach (Progress *progress, pgs)
    {
        endProgress(progress);
    }
}

#include "progresslist.moc"
