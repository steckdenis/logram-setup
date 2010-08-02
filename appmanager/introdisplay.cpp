/*
 * display.cpp
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

#include "introdisplay.h"
#include "mainwindow.h"
#include "categorydrawer.h"

#include <QGridLayout>
#include <QLabel>
#include <QListWidget>

#include <QPainter>

IntroDisplay::IntroDisplay(MainWindow* parent) : QWidget(parent)
{
    win = parent;
    lay = 0;
    
    pix = QPixmap(":/images/logologram.png");
    
    setBackgroundRole(QPalette::Base);
}

void IntroDisplay::populate()
{
    // Effacer tous les éléments
    if (lay != 0)
    {
        delete lay;
    }
    
    // Créer le layout
    lay = new QGridLayout(this);
    this->setLayout(lay);
    
    // Ajouter le titre de bienvenue
    CategoryDrawer *cat = new CategoryDrawer(this);
    cat->setIcon(win->windowIcon());
    cat->setText(tr("Bienvenue dans le gestionnaire d'applications Logram !"));
    
    lay->addWidget(cat, 0, 0, 1, 2);
    
    // Ajouter le texte de bienvenue
    QLabel *lblWelcome = new QLabel(this);
    
    lblWelcome->setWordWrap(true);
    lblWelcome->setContentsMargins(30, 0, 0, 0);
    lblWelcome->setAlignment(Qt::AlignTop);
    lblWelcome->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    lblWelcome->setText(tr(
        "<p>Ce programme vous permet d'installer, de supprimer et de mettre à jour des applications "
        "sur votre ordinateur.</p>"
        
        "<p>Pour commencer à choisir des applications, cliquez sur une des catégories à gauche de ce "
        "texte. Ensuite, choisissez les applications voulues dans la liste qui s'affichera. Cliquez "
        "dessus pour les installer ou les supprimer. Une fois terminé, cliquez sur <strong>Appliquer "
        "les changements</strong>.</p>"
    ));
    
    lay->addWidget(lblWelcome, 1, 0, 1, 2);
    
    // Ajouter la titre des meilleurs paquets
    cat = new CategoryDrawer(this);
    cat->setIcon(QIcon(":/images/package.png"));
    cat->setText(tr("Paquets les mieux notés"));
    
    lay->addWidget(cat, 2, 0);
    
    // Ajouter la liste des meilleurs paquets
    QListWidget *listRated = new QListWidget(this);
    
    listRated->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listRated->setIconSize(QSize(32, 32));
    listRated->setTextElideMode(Qt::ElideMiddle);
    listRated->setFrameShape(QFrame::NoFrame);
    
    QPalette pal = listRated->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    listRated->setPalette(pal);
    
    QVector<MainWindow::RatedPackage> ratedPackages = win->ratedPackages();
    
    foreach (const MainWindow::RatedPackage &rated, ratedPackages)
    {
        QListWidgetItem *item = new QListWidgetItem(listRated);
        QIcon ico;
        
        if (rated.inf.icon.isNull())
        {
            ico = QIcon(":/images/package.png");
        }
        else
        {
            ico = QIcon(rated.inf.icon);
        }
        
        item->setIcon(ico);
        item->setText(rated.inf.title);
        
        listRated->addItem(item);
    }
    
    lay->addWidget(listRated, 3, 0);
    
    // Ajouter la liste des derniers paquets
    cat = new CategoryDrawer(this);
    cat->setIcon(QIcon(":/images/repository.png"));
    cat->setText(tr("Derniers paquets"));
    
    lay->addWidget(cat, 2, 1);
    
    // Ajouter la liste des news Logram
    cat = new CategoryDrawer(this);
    cat->setIcon(QIcon(":/images/logologram.svg"));
    cat->setText(tr("Dernières nouvelles de Logram"));
    
    lay->addWidget(cat, 4, 0, 1, 2);
    
    // Ajouter un stretch
    QWidget *stretch = new QWidget(this);
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    lay->addWidget(stretch, lay->rowCount(), 0, 1, 2);
}

void IntroDisplay::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    painter.drawPixmap(width() - pix.width(), height() - pix.height(), pix);
}

#include "introdisplay.moc"
