/*
 * packageinfo.cpp
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

#include <packagemetadata.h>

#include <infopane.h>
#include <packagedataprovider.h>

#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDialog>
#include <QImage>
#include <QtDebug>

using namespace Logram;
using namespace LogramUi;

void MainWindow::itemActivated(QTreeWidgetItem *item)
{
    PackageItem *pitem = static_cast<PackageItem *>(item);
    
    if (pitem == 0) return;
    
    // Remplir les informations sur le paquet
    DatabasePackage *pkg = pitem->package();
    
    // Gérer les actions possibles avec ce paquet
    actionsForPackage(pkg);
    
    PackageDataProvider *provider = new PackageDataProvider(pkg, ps);
    infoPane->displayData(provider);
    
    docInfos->setWindowTitle(QString("%1 %2").arg(pkg->name(), pkg->version()));
}
