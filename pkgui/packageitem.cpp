/*
 * packagelist.cpp
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

#include "mainwindow.h"

#include <packagesystem.h>
#include <databasereader.h>
#include <databasepackage.h>

#include <utils.h>

#include <QTreeWidget>

using namespace Logram;
using namespace LogramUi;

PackageItem::PackageItem(DatabasePackage* pkg, QTreeWidget* parent, PackageItem::Type type, bool expand) : QTreeWidgetItem(parent, QTreeWidgetItem::Type)
{
    _pkg = pkg;
    _type = type;
    
    updateIcon();
    updateText(expand);
}

PackageItem::PackageItem(DatabasePackage* pkg, QTreeWidgetItem* parent, PackageItem::Type type) : QTreeWidgetItem(parent, QTreeWidgetItem::Type)
{
    _pkg = pkg;
    _type = type;
    
    updateIcon();
    updateText(false);
}

PackageItem::~PackageItem()
{
    delete _pkg;
}

QVariant PackageItem::data(int column, int role) const
{
    if (role == Qt::StatusTipRole)
    {
        // Même status tip pour toutes les colonnes
        return _pkg->shortDesc();
    }
    
    return QTreeWidgetItem::data(column, role);
}

void PackageItem::updateIcon()
{
    if (_pkg->action() == Solver::Install)
    {
        setIcon(0, QIcon(":/images/pkg-install.png"));
    }
    else if (_pkg->action() == Solver::Remove || _pkg->action() == Solver::Purge)
    {
        setIcon(0, QIcon(":/images/pkg-remove.png"));
    }
    else if (_type != PackageList)
    {
        setIcon(0, QIcon(":/images/package.png"));
    }
    else if (_pkg->flags() & Package::Installed)
    {
        setIcon(0, QIcon::fromTheme("user-online"));
    }
    else
    {
        setIcon(0, QIcon::fromTheme("user-offline"));
    }
}

void PackageItem::updateText(bool subElements)
{
    if (_type == SmallActionList)
    {
        setText(0, _pkg->name() + "~" + _pkg->version());
    }
    else
    {
        setText(0, _pkg->name());
    }
    
    // Voir si l'élément a des enfants
    if (subElements && _type == PackageList)
    {
        QVector<DatabasePackage *> pkgs = _pkg->versions();
        
        if (pkgs.count() > 1)
        {
            // Ajouter les sous-éléments
            for (int i=0; i<pkgs.count(); ++i)
            {
                DatabasePackage *pkg = pkgs.at(i);
                
                if (pkg->index() == _pkg->index())
                {
                    continue;
                }
                
                PackageItem *item = new PackageItem(pkg, this, PackageList);
                addChild(item);
            }
        }
    }
    
    if (_type == SmallActionList)
    {
        setText(1, Utils::actionNameInf(_pkg->action()));
        setText(2, PackageSystem::fileSizeFormat(_pkg->downloadSize()) + "/" + PackageSystem::fileSizeFormat(_pkg->installSize()));
    }
    else
    {
        setText(1, _pkg->version());
        setText(2, PackageSystem::fileSizeFormat(_pkg->downloadSize()) + "/" + PackageSystem::fileSizeFormat(_pkg->installSize()));
        setText(3, _pkg->repo() + "»" + _pkg->distribution() + "»" + _pkg->section());
        setText(4, _pkg->shortDesc());
    }
}

void PackageItem::setPackage(DatabasePackage *pkg)
{
    _pkg = pkg;
}

DatabasePackage *PackageItem::package()
{
    return _pkg;
}