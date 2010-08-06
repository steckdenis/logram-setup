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

Entry::Entry(DatabasePackage* pkg, const MainWindow::PackageInfo& inf, QWidget *parent): QWidget(parent)
{
    _pkg = pkg;
    containsMouse = false;
    
    setupUi(this);
    
    // Peupler le contrôle
    lblIcon->setPixmap(packageIcon(inf));
    lblTitle->setText("<b>" + inf.title + "</b> " + pkg->version());
    lblShortDesc->setText(pkg->shortDesc());
    
    lblDownload->setText(
        PackageSystem::fileSizeFormat(pkg->downloadSize()) + '/' +
        PackageSystem::fileSizeFormat(pkg->installSize())
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
}

Entry::~Entry()
{

}

DatabasePackage* Entry::package() const
{
    return _pkg;
}

QPixmap Entry::packageIcon(const MainWindow::PackageInfo& inf)
{
    QPixmap rs(inf.icon);
    
    if (rs.isNull())
    {
        if (!QPixmapCache::find("lgr_pkicon", &rs))
        {
            rs = QIcon(":/images/package.png").pixmap(32, 32);
            
            QPixmapCache::insert("lgr_pkicon", rs);
        }
    }
    
    // Emblème en fonction de l'état
    QPixmap emblem;
    const char *key = 0, *filename = 0;
    
    if (_pkg->action() == Solver::Install)
    {
        key = "lgr_embleminstall";
        filename = ":/images/pkg-install-emb.png";
    }
    else if (_pkg->action() == Solver::Remove)
    {
        key = "lgr_emblemremove";
        filename = ":/images/pkg-remove-emb.png";
    }
    else if (_pkg->action() == Solver::Purge)
    {
        key = "lgr_emblempurge";
        filename = ":/images/pkg-purge-emb.png";
    }
    else if (_pkg->action() == Solver::Update)
    {
        key = "lgr_emblemremove";
        filename = ":/images/pkg-update-emb.png";
    }
    else if (_pkg->flags() & PACKAGE_FLAG_INSTALLED)
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
    
    if (!emblem.isNull())
    {
        QPainter painter(&rs);
        
        painter.drawPixmap(rs.width() - emblem.width(), rs.height() - emblem.height(), emblem);
    }
    
    return rs;
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
    
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, &painter, this);
}

#include "packageentry.moc"
