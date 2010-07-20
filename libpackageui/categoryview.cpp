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

#include <QHash>
#include <QTreeWidget>

#include <QtXml>
#include <boost/iterator/iterator_concepts.hpp>

using namespace Logram;
using namespace LogramUi;

struct CategoryView::Private
{
    FilterInterface *interface;
    PackageSystem *ps;
    
    QTreeWidget *sections, *distros;
    
    QTreeWidgetItem *noSectionFilterItem;
    QTreeWidgetItem *noDistroFilterItem;
    QHash<QString, QTreeWidgetItem *> sectionItems;
};

LogramUi::CategoryView::CategoryView(Logram::PackageSystem* ps, FilterInterface* interface, QWidget* parent): QToolBox(parent)
{
    d = new Private;
    d->interface = interface;
    d->ps = ps;
    
    // Ajouter les treeWidgets
    d->sections = new QTreeWidget(this);
    d->sections->setIconSize(QSize(32, 32));
    d->sections->setHeaderHidden(true);
    
    d->distros = new QTreeWidget(this);
    d->distros->setIconSize(QSize(32, 32));
    d->distros->setHeaderHidden(true);
    
    connect(d->sections, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateFilter()));
    connect(d->distros,  SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updateFilter()));
    
    // Insérer les pages
    addItem(d->sections, QIcon::fromTheme("applications-other"), tr("Sections"));
    addItem(d->distros, QIcon(":/images/repository.png"), tr("Distributions"));
    
    // Peupler la vue
    reload();
}

LogramUi::CategoryView::~CategoryView()
{
    delete d;
}

void CategoryView::updateFilter()
{
    QTreeWidgetItem *sectionItem = d->sections->currentItem();
    QTreeWidgetItem *distroItem = d->distros->currentItem();
    
    if (sectionItem && sectionItem != d->noSectionFilterItem)
    {
        d->interface->setSection(sectionItem->data(0, Qt::UserRole).toString());
    }
    else
    {
        d->interface->setSection(QString());
    }
    
    if (distroItem && distroItem != d->noDistroFilterItem)
    {
        d->interface->setRepository(distroItem->data(0, Qt::UserRole).toString());
        d->interface->setDistribution(distroItem->data(0, Qt::UserRole + 1).toString());
    }
    else
    {
        d->interface->setRepository(QString());
        d->interface->setDistribution(QString());
    }
    
    d->interface->updateViews();
}

void LogramUi::CategoryView::reload()
{
    d->sections->clear();
    d->distros->clear();
    d->sectionItems.clear();
    
    QDir dir(d->ps->varRoot() + "/var/cache/lgrpkg/db");
    QStringList files = dir.entryList(QDir::Files);
    
    d->noSectionFilterItem = new QTreeWidgetItem(d->sections);
    d->noSectionFilterItem->setIcon(0, QIcon::fromTheme("edit-select-all"));
    d->noSectionFilterItem->setText(0, tr("Tout afficher"));
    
    d->noDistroFilterItem = new QTreeWidgetItem(d->distros);
    d->noDistroFilterItem->setIcon(0, QIcon::fromTheme("edit-select-all"));
    d->noDistroFilterItem->setText(0, tr("Tout afficher"));
    
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
        
        // Ajouter la distribution
        QTreeWidgetItem *item = d->distros->invisibleRootItem();
        QTreeWidgetItem *distroParent = 0;
        
        for (int i=0; i<item->childCount(); ++i)
        {
            if (item->child(i)->data(0, Qt::UserRole).toString() == repo)
            {
                distroParent = item->child(i);
                break;
            }
        }
        
        if (distroParent == 0)
        {
            // Pas encore de données pour ce dépôt
            distroParent = new QTreeWidgetItem(d->distros);
            
            Repository r;
            
            if (d->ps->repository(repo, r))
            {
                distroParent->setText(0, r.description);
            }
            else
            {
                distroParent->setText(0, tr("Dépôt %1").arg(repo));
            }
            
            distroParent->setData(0, Qt::UserRole, repo);
            distroParent->setIcon(0, QIcon(":/images/repository.png"));
        }
        
        QTreeWidgetItem *distroItem = new QTreeWidgetItem(distroParent);
        
        distroItem->setData(0, Qt::UserRole, repo);
        distroItem->setData(0, Qt::UserRole + 1, distro);
        distroItem->setIcon(0, QIcon(":/images/repository.png"));
        distroItem->setText(0, tr("Distribution %1").arg(distro));
        
        // Ajouter les sections
        QDomDocument doc;
        doc.setContent(fl.readAll());
        QDomElement section = doc.documentElement().firstChildElement();
        
        while (!section.isNull())
        {
            QString name = section.attribute("name");
            QTreeWidgetItem *item = 0;
            
            if (name.isEmpty())
            {
                section = section.nextSiblingElement();
                continue;
            }
            
            // Si on a déjà cette section, la sauter si elle est de poids plus faible
            if (d->sectionItems.contains(name))
            {
                int weight = section.attribute("weight").toInt();
                int otherWeight = d->sectionItems.value(name)->data(0, Qt::UserRole + 1).toInt();
                
                if (weight > otherWeight)
                {
                    // On mettra à jour cette section
                    item = d->sectionItems.value(name);
                }
                else
                {
                    // Sauter la section
                    section = section.nextSiblingElement();
                    continue;
                }
            }
            
            if (item == 0)
            {
                // Créer un nouvel élément
                QStringList parts = name.split('/');
                QString lastParentName;
                QTreeWidgetItem *lastParent = 0;
                
                for (int i=0; i<parts.count(); ++i)
                {
                    if (!lastParentName.isNull())
                    {
                        lastParentName += '/';
                    }
                    
                    lastParentName += parts.at(i);
                    
                    // Voir si cet élément existe
                    if (d->sectionItems.contains(lastParentName))
                    {
                        lastParent = d->sectionItems.value(lastParentName);
                    }
                    else
                    {
                        // Il faut créer l'élément
                        if (lastParent == 0)
                        {
                            lastParent = new QTreeWidgetItem(d->sections);
                        }
                        else
                        {
                            lastParent = new QTreeWidgetItem(lastParent);
                        }
                        
                        if (i < parts.count() - 1)
                        {
                            // Nouveau parent, en créer un avec un poids de -1 pour qu'il soit remplacé
                            // plus tard dans la lecture du XML
                            lastParent->setData(0, Qt::UserRole, lastParentName);
                            lastParent->setData(0, Qt::UserRole + 1, -1);
                            lastParent->setText(0, tr("Section %1").arg(lastParentName));
                            lastParent->setIcon(0, QIcon(":/images/repository.png"));
                            
                            d->sectionItems.insert(lastParentName, lastParent);
                        }
                    }
                }
                
                item = lastParent;
            }
            
            item->setData(0, Qt::UserRole, section.attribute("name"));
            item->setData(0, Qt::UserRole + 1, section.attribute("weight", 0));
            item->setText(0, PackageMetaData::stringOfKey(section.firstChildElement("title"), section.attribute("primarylang")));
            item->setToolTip(0, PackageMetaData::stringOfKey(section.firstChildElement("description"), section.attribute("primarylang")));;
            
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
            
            d->sectionItems.insert(name, item);
            
            section = section.nextSiblingElement();
        }
    }
    
    d->sections->expandAll();
    d->distros->expandAll();
}

#include "categoryview.moc"