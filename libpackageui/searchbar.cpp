/*
 * searchbar.cpp
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

#include "searchbar.h"
#include "filterinterface.h"

#include <QIcon>

using namespace LogramUi;

struct SearchBar::Private
{
    FilterInterface *interface;
};

SearchBar::SearchBar(FilterInterface* interface, QWidget* parent): QWidget(parent)
{
    d = new Private;
    d->interface = interface;
    
    setupUi(this);
    
    // Signaux
    connect(cboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFilter()));
    connect(txtSearch, SIGNAL(returnPressed()), this, SLOT(updateFilter()));
    connect(btnSearch, SIGNAL(clicked(bool)), this, SLOT(updateFilter()));
    
    // Icônes
    btnSearch->setIcon(QIcon::fromTheme("edit-find"));
    
    for (int i=0; i<cboFilter->count(); ++i)
    {
        FilterInterface::StatusFilter filter = (FilterInterface::StatusFilter)i;
        
        switch (filter)
        {
            case FilterInterface::NoFilter:
                cboFilter->setItemIcon(i, QIcon::fromTheme("view-filter"));
                break;
            case FilterInterface::Installed:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-install.png"));
                break;
            case FilterInterface::NotInstalled:
                cboFilter->setItemIcon(i, QIcon(":/images/package.png"));
                break;    
            case FilterInterface::Updateable:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-update.png"));
                break;
            case FilterInterface::Orphan:
                cboFilter->setItemIcon(i, QIcon(":/images/pkg-purge.png"));
                break;
        }
    }
}

SearchBar::~SearchBar()
{
    delete d;
}

void SearchBar::setFocus()
{
    txtSearch->setFocus();
}

void SearchBar::updateFilter()
{
    d->interface->setNamePattern(txtSearch->text());
    d->interface->setStatusFilter((FilterInterface::StatusFilter)cboFilter->currentIndex());
    
    d->interface->updateViews();
}

#include "searchbar.moc"