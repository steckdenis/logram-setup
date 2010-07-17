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

#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>

#include <QtXml>

#include <packagemetadata.h>

using namespace Logram;

SectionItem::SectionItem(const QString &section, QTreeWidgetItem *parent) : QTreeWidgetItem(parent)
{
    _section = section;
}

QString SectionItem::section() const
{
    return _section;
}

void MainWindow::populateSections()
{
    // Lire les fichiers XML de /var/cache/lgrpkg/db/<repo>_<distro>.sections
    QDir dir(ps->varRoot() + "/var/cache/lgrpkg/db");
    QStringList files = dir.entryList(QDir::Files);
    
    treeSections->clear();
    
    noSectionFilterItem = new QTreeWidgetItem(treeSections);
    noSectionFilterItem->setIcon(0, QIcon::fromTheme("edit-select-all"));
    noSectionFilterItem->setText(0, tr("Tout afficher"));
    
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
        QTreeWidgetItem *parentItem = new QTreeWidgetItem(treeSections);
        
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
            SectionItem *item = new SectionItem(section.attribute("name"), parentItem);
            
            item->setData(0, Qt::UserRole, repo);
            item->setData(0, Qt::UserRole + 1, distro);
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
                item->setIcon(0, QIcon(pixmapFromData(iconData, 32, 32)));
            }
            
            section = section.nextSiblingElement();
        }
    }
    
    treeSections->expandAll();
}