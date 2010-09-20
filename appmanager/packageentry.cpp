/*
 * packageentry.cpp
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

#include "packageentry.h"
#include "ui_entrymoreinfo.h"
#include "mainwindow.h"

#include <databasepackage.h>
#include <packagesystem.h>

#include <QIcon>
#include <QPixmap>
#include <QPixmapCache>
#include <QStyle>
#include <QPainter>
#include <QApplication>
#include <QStyleOption>

using namespace Logram;

Entry::Entry(DatabasePackage* pkg, MainWindow* mainWindow, bool expandable, QWidget* parent): QWidget(parent)
{
    mainpkg = pkg;
    exp = expandable;
    win = mainWindow;
    
    more = 0;
    moreWidget = 0;
    expanded = false;
    containsMouse = false;
    
    setupUi(this);
    
    if (expandable)
    {
        pkgs = pkg->versions();
        
        // Trouver le meilleur index des paquets
        int bestIndex = mainWindow->bestPackageIndex(pkgs, 0);
        
        // Afficher ce paquet
        setCurrentPackage(bestIndex);
    }
    else
    {
        pkgs.append(mainpkg);
        setCurrentPackage(0);
    }
}

Entry::~Entry()
{
    delete mainpkg;
}

DatabasePackage* Entry::currentPackage() const
{
    return curpkg;
}

QVector <DatabasePackage *> Entry::packages() const
{
    return pkgs;
}

void Entry::setCurrentPackage(int index)
{
    curpkg = pkgs.at(index);
    MainWindow::PackageInfo inf = win->packageInfo(curpkg);
    
    // Icône du paquet
    baseIcon = inf.icon;
    
    if (baseIcon.isNull())
    {
        if (!QPixmapCache::find("lgr_pkicon", &baseIcon))
        {
            baseIcon = QIcon(":/images/package.png").pixmap(32, 32);
            
            QPixmapCache::insert("lgr_pkicon", baseIcon);
        }
    }
    
    // Peupler le contrôle
    updateIcon();
    lblTitle->setText("<b>" + inf.title + "</b> " + curpkg->version());
    lblShortDesc->setText(curpkg->shortDesc());
    
    lblDownload->setText(
        PackageSystem::fileSizeFormat(curpkg->downloadSize()) + '/' +
        PackageSystem::fileSizeFormat(curpkg->installSize())
    );
    
    QPixmap starOn, starOff;
    QIcon icon;
    
    if (!QPixmapCache::find("lgr_staron", &starOn))
    {
        icon = QIcon::fromTheme("rating");
        starOn = icon.pixmap(22, 22);
        
        QPixmapCache::insert("lgr_staron", starOn);
    }
    
    if (!QPixmapCache::find("lgr_staroff", &starOff))
    {
        if (icon.isNull())
        {
            icon = QIcon::fromTheme("rating");
        }
        
        starOff = icon.pixmap(22, 22, QIcon::Disabled);
        
        QPixmapCache::insert("lgr_staroff", starOff);
    }
    
    float score = 0.0;
    
    if (inf.total_votes > 0)
    {
        score = inf.votes * 5 / inf.total_votes;
    }
    
    lblStar1->setPixmap(score > 0.5 ? starOn : starOff);
    lblStar2->setPixmap(score > 1.5 ? starOn : starOff);
    lblStar3->setPixmap(score > 2.5 ? starOn : starOff);
    lblStar4->setPixmap(score > 3.5 ? starOn : starOff);
    lblStar5->setPixmap(score > 4.5 ? starOn : starOff);
    
    // Synchroniser l'affichage
    if (isExpanded())
    {
        collapse();
        expand();
    }
}

bool Entry::isExpanded() const
{
    return expanded;
}

void Entry::expand()
{
    if (moreWidget != 0)
    {
        return;
    }
    
    expanded = true;
    
    moreWidget = new QWidget(this);
    more = new Ui_EntryMoreInfos();
    
    more->setupUi(moreWidget);
    
    // Signaux des boutons
    connect(more->btnInstall, SIGNAL(clicked(bool)), this, SLOT(btnInstallClicked(bool)));
    connect(more->btnRemove, SIGNAL(clicked(bool)), this, SLOT(btnRemoveClicked(bool)));
    connect(more->btnUpdate, SIGNAL(clicked(bool)), this, SLOT(btnUpdateClicked(bool)));
    
    // Icônes des boutons
    more->btnInstall->setIcon(QIcon(":/images/pkg-install.png"));
    more->btnRemove->setIcon(QIcon(":/images/pkg-remove.png"));
    more->btnUpdate->setIcon(QIcon(":/images/pkg-update.png"));
    
    // Afficher ou pas les boutons
    more->btnInstall->setVisible((curpkg->flags() & Package::Installed) == 0);
    more->btnRemove->setVisible((curpkg->flags() & Package::Installed) != 0);
    more->btnUpdate->setVisible(false); // TODO
    
    // Statut des boutons
    more->btnInstall->setChecked(curpkg->action() == Solver::Install);
    more->btnRemove->setChecked(curpkg->action() == Solver::Remove || curpkg->action() == Solver::Purge);
    more->btnUpdate->setChecked(curpkg->action() == Solver::Update);
    
    if (exp)
    {
        // Construire la liste des petits liens permettant de sélectionner les versions
        QHBoxLayout *hlayout = new QHBoxLayout(0);
        
        more->verticalLayout->insertLayout(0, hlayout);
        hlayout->addStretch();
        
        for (int i=0; i<pkgs.count(); ++i)
        {
            QLabel *lbl = new QLabel(moreWidget);
            lbl->setText("<em><a href=\"" + QString::number(i) + "\">" + pkgs.at(i)->version() + "</a></em>");
            
            connect(lbl, SIGNAL(linkActivated(QString)), this, SLOT(changeVersion()));
            
            hlayout->addWidget(lbl);
        }
    }
    
    // TODO: Liste des paquets suggérés
    
    // Ajouter le widget au layout
    mainLayout->addWidget(moreWidget);
}

void Entry::collapse()
{
    if (moreWidget == 0)
    {
        return;
    }
    
    expanded = false;
    
    delete moreWidget;
    delete more;
    
    moreWidget = 0;
    more = 0;
}

void Entry::btnInstallClicked(bool checked)
{
    curpkg->setAction(checked ? Solver::Install : Solver::None);
    
    updateIcon();
}

void Entry::btnRemoveClicked(bool checked)
{
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        curpkg->setAction(checked ? Solver::Purge : Solver::None);
    }
    else
    {
        curpkg->setAction(checked ? Solver::Remove : Solver::None);
    }
    
    updateIcon();
}

void Entry::btnUpdateClicked(bool checked)
{
    // TODO
}

void Entry::updateIcon()
{
    // Emblème en fonction de l'état
    QPixmap emblem;
    const char *key = 0, *filename = 0;
    
    if (curpkg->action() == Solver::Install)
    {
        key = "lgr_embleminstall";
        filename = ":/images/pkg-install-emb.png";
    }
    else if (curpkg->action() == Solver::Remove)
    {
        key = "lgr_emblemremove";
        filename = ":/images/pkg-remove-emb.png";
    }
    else if (curpkg->action() == Solver::Purge)
    {
        key = "lgr_emblempurge";
        filename = ":/images/pkg-purge-emb.png";
    }
    else if (curpkg->action() == Solver::Update)
    {
        key = "lgr_emblemremove";
        filename = ":/images/pkg-update-emb.png";
    }
    else if (curpkg->flags() & Package::Installed)
    {
        emblem = QIcon::fromTheme("user-online").pixmap(16, 16);
    }
    
    if (key != 0 && filename != 0)
    {
        if (!QPixmapCache::find(key, &emblem))
        {
            emblem = QPixmap(filename);
            
            QPixmapCache::insert(key, emblem);
        }
    }
    
    QPixmap newIcon(baseIcon);
    
    if (!emblem.isNull())
    {
        QPainter painter(&newIcon);
        
        painter.drawPixmap(newIcon.width() - emblem.width(), newIcon.height() - emblem.height(), emblem);
    }
    
    lblIcon->setPixmap(newIcon);
}

void Entry::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    
    containsMouse = true;
    
    repaint();
}

void Entry::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    
    containsMouse = false;
    
    repaint();
}

void Entry::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    
    emit clicked();
}

void Entry::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter painter(this);
    QStyleOptionViewItemV4 option;
    
    option.initFrom(this);
    option.state = QStyle::State_Active | QStyle::State_Enabled;
    option.checkState = Qt::Unchecked;
    option.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
    
    if (containsMouse)
    {
        option.state |= QStyle::State_MouseOver;
    }
    
    if (expanded)
    {
        option.state |= QStyle::State_Selected;
    }
    
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, &painter, this);
}

#include "packageentry.moc"
