/*
 * categoryview.cpp
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

#include "categoryview.h"
#include "filterinterface.h"
#include "utils.h"

#include <packagesystem.h>
#include <packagemetadata.h>

#include <QtXml>

using namespace Logram;
using namespace LogramUi;

struct CategoryView::Private
{
    FilterInterface *interface;
    PackageSystem *ps;
    
    QTreeWidgetItem *noSectionFilterItem;
};

LogramUi::CategoryView::CategoryView(Logram::PackageSystem* ps, FilterInterface* interface, QWidget* parent): QTreeWidget(parent)
{
    d = new Private;
    d->interface = interface;
    d->ps = ps;
    
    // Propriétés
    setIconSize(QSize(32, 32));
    setHeaderHidden(true);
    
    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateFilter()));
    
    // Peupler la vue
    reset();
}

LogramUi::CategoryView::~CategoryView()
{
    delete d;
}

void CategoryView::updateFilter()
{
    QTreeWidgetItem *item = currentItem();
    
    if (item == 0) return;
    
    if (item == d->noSectionFilterItem)
    {
        d->interface->setRepository(QString());
        d->interface->setDistribution(QString());
        d->interface->setSection(QString());
    }
    else
    {
        d->interface->setRepository(item->data(0, Qt::UserRole).toString());
        d->interface->setDistribution(item->data(0, Qt::UserRole + 1).toString());
        d->interface->setSection(item->data(0, Qt::UserRole + 1).toString());
    }
    
    d->interface->updateViews();
}

void LogramUi::CategoryView::reset()
{
    clear();
    
    QDir dir(d->ps->varRoot() + "/var/cache/lgrpkg/db");
    QStringList files = dir.entryList(QDir::Files);
    
    d->noSectionFilterItem = new QTreeWidgetItem(this);
    d->noSectionFilterItem->setIcon(0, QIcon::fromTheme("edit-select-all"));
    d->noSectionFilterItem->setText(0, tr("Tout afficher"));
    
    foreach (const QString &file, files)
    {
        if (!file.endsWith(".sections")) continue;
        
        QString filename = dir.absoluteFilePath(file);
        QStringList parts = file.split('_');
        QString repo = parts.at(0);
        QString distro = parts.at(1).section('.', 0, 0);
        
        // Ouvrir le fichier
        QFile fl(filename);
        
        if (!fl.open(QIODevice::ReadOnly))
        {
            continue;
        }
        
        // Ajouter un enregistrement dans la liste
        QTreeWidgetItem *parentItem = new QTreeWidgetItem(this);
        
        parentItem->setData(0, Qt::UserRole, repo);
        parentItem->setData(0, Qt::UserRole + 1, distro);
        parentItem->setIcon(0, QIcon(":/images/repository.png"));
        parentItem->setText(0, tr("Distribution %1 du dépôt %2").arg(distro, repo));
        
        // Ajouter les éléments lus depuis le fichier XML
        QDomDocument doc;
        doc.setContent(fl.readAll());
        QDomElement section = doc.documentElement().firstChildElement();
        
        while (!section.isNull())
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
            
            item->setData(0, Qt::UserRole, repo);
            item->setData(0, Qt::UserRole + 1, distro);
            item->setData(0, Qt::UserRole + 2, section.attribute("name"));
            item->setText(0, PackageMetaData::stringOfKey(section.firstChildElement("title"), section.attribute("primarylang")));
            item->setToolTip(0, PackageMetaData::stringOfKey(section.firstChildElement("description"), section.attribute("primarylang")));;
            
            parentItem->addChild(item);
            
            // Trouver l'icône
            QDomElement iconElement = section.firstChildElement("icon");
            QByteArray iconData;
            
            if (!iconElement.isNull())
            {
                // Trouver le noeud CDATA
                QDomNode node = iconElement.firstChild();
                QDomCDATASection section;
                
                while (!node.isNull())
                {
                    if (node.isCDATASection())
                    {
                        section = node.toCDATASection();
                        break;
                    }
                    
                    node = node.nextSibling();
                }
                
                if (!section.isNull())
                {
                    iconData = QByteArray::fromBase64(section.data().toAscii());
                }
            }
            
            if (iconData.isNull())
            {
                item->setIcon(0, QIcon(":/images/repository.png"));
            }
            else
            {
                item->setIcon(0, QIcon(Utils::pixmapFromData(iconData, 32, 32)));
            }
            
            section = section.nextSiblingElement();
        }
    }
    
    expandAll();
}

#include "categoryview.moc"