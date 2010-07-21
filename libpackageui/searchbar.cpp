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
#include "ui_searchbar.h"

#include <QIcon>
#include <QMenu>

using namespace LogramUi;

struct SearchBar::Private
{
    FilterInterface *interface;
    Ui_searchBar *ui;
    
    QAction *actFixed;
    QAction *actWildcard;
    QAction *actRegExp;
};

SearchBar::SearchBar(FilterInterface* interface, QWidget* parent): QWidget(parent)
{
    d = new Private;
    d->interface = interface;
    d->ui = new Ui_searchBar;
    
    d->ui->setupUi(this);
    
    // Signaux
    connect(d->ui->cboFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFilter()));
    connect(d->ui->txtSearch, SIGNAL(returnPressed()), this, SLOT(updateFilter()));
    connect(d->ui->btnSearch, SIGNAL(clicked(bool)), this, SLOT(updateFilter()));
    
    // Icônes
    d->ui->btnSearch->setIcon(QIcon::fromTheme("edit-find"));
    
    for (int i=0; i<d->ui->cboFilter->count(); ++i)
    {
        FilterInterface::StatusFilter filter = (FilterInterface::StatusFilter)i;
        
        switch (filter)
        {
            case FilterInterface::NoFilter:
                d->ui->cboFilter->setItemIcon(i, QIcon::fromTheme("view-filter"));
                break;
            case FilterInterface::Installed:
                d->ui->cboFilter->setItemIcon(i, QIcon(":/images/pkg-install.png"));
                break;
            case FilterInterface::NotInstalled:
                d->ui->cboFilter->setItemIcon(i, QIcon(":/images/package.png"));
                break;    
            case FilterInterface::Updateable:
                d->ui->cboFilter->setItemIcon(i, QIcon(":/images/pkg-update.png"));
                break;
            case FilterInterface::Orphan:
                d->ui->cboFilter->setItemIcon(i, QIcon(":/images/pkg-purge.png"));
                break;
        }
    }
    
    // Menu du bouton de recherche
    QMenu *menu = new QMenu(d->ui->btnSearch);
    d->actFixed = new QAction(tr("Nom exact"), menu);
    d->actWildcard = new QAction(tr("Format simple"), menu);
    d->actRegExp = new QAction(tr("Expression régulière"), menu);
    
    d->actFixed->setStatusTip(tr("Tous les paquets dont le nom est exactement ce que vous avez entré."));
    d->actWildcard->setStatusTip(tr("Tous les paquets dont le nom contient ce que vous avez entré (? remplace un caractère, * plusieurs)."));
    d->actRegExp->setStatusTip(tr("Tous les paquets correspondant à une expression régulière (utilisateurs avancés)."));
    
    menu->addAction(d->actFixed);
    menu->addAction(d->actWildcard);
    menu->addAction(d->actRegExp);
    
    d->ui->btnSearch->setMenu(menu);
    
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));
}

SearchBar::~SearchBar()
{
    delete d->ui;
    delete d;
}

void SearchBar::setFocus()
{
    d->ui->txtSearch->setFocus();
}

void SearchBar::updateFilter()
{
    QString pattern = d->ui->txtSearch->text();
    
    if (!pattern.isNull() && d->interface->nameSyntax() == QRegExp::Wildcard && !pattern.contains('*'))
    {
        // Si l'utilisateur n'a entré qu'un nom, rechercher les paquets qui le contiennent
        pattern = "*" + pattern + "*";
    }
    
    d->interface->setNamePattern(pattern);
    d->interface->setStatusFilter((FilterInterface::StatusFilter)d->ui->cboFilter->currentIndex());
    
    d->interface->updateViews();
}

void SearchBar::actionTriggered(QAction* action)
{
    if (action == d->actFixed)
    {
        d->interface->setNameSyntax(QRegExp::FixedString);
    }
    else if (action == d->actWildcard)
    {
        d->interface->setNameSyntax(QRegExp::Wildcard);
    }
    else if (action == d->actRegExp)
    {
        d->interface->setNameSyntax(QRegExp::RegExp2);
    }
    
    // Actualiser la vue
    updateFilter();
}

#include "searchbar.moc"