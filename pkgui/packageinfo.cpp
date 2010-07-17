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
#include "ui_flags.h"

#include <packagemetadata.h>

#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDialog>
#include <QImage>
#include <QtDebug>

using namespace Logram;

void MainWindow::itemActivated(QTreeWidgetItem *item)
{
    PackageItem *pitem = static_cast<PackageItem *>(item);
    
    if (pitem == 0) return;
    
    // Remplir les informations sur le paquet
    DatabasePackage *pkg = pitem->package();
    PackageMetaData *md = pkg->metadata();
    
    if (md == 0)
    {
        psError();
        return;
    }
    
    md->setCurrentPackage(pkg->name());
    
    // Gérer les actions possibles avec ce paquet
    actionsForPackage(pkg);
    
    // Infos de base
    docInfos->setWindowTitle(pkg->name() + "~" + pkg->version());
    lblTitle->setText("<a href=\"" + md->upstreamUrl() + "\">" + md->packageTitle() + "</a>");
    lblShortDesc->setText(pkg->shortDesc());
    lblSection->setText(pkg->section());
    lblRepo->setText(pkg->repo() + "»" + pkg->distribution());
    lblDownload->setText(PackageSystem::fileSizeFormat(pkg->downloadSize()));
    lblInstall->setText(PackageSystem::fileSizeFormat(pkg->installSize()));
    lblLicense->setText("<a href=\"" + pkg->license() + "\">" + pkg->license() + "</a>");
    
    // Icône
    QByteArray iconData = md->packageIconData();
    
    if (iconData.isNull())
    {
        lblIcon->setPixmap(QIcon(":/images/package.png").pixmap(48, 48));
    }
    else
    {
        lblIcon->setPixmap(MainWindow::pixmapFromData(iconData, 48, 48));
    }
    
    // Liste des versions
    QVector<DatabasePackage *> pkgs = pkg->versions();
    listVersions->clear();
    
    for (int i=0; i<pkgs.count(); ++i)
    {
        DatabasePackage *mpkg = pkgs.at(i);
        
        QListWidgetItem *mitem = new QListWidgetItem(mpkg->version(), listVersions);
        
        if (mpkg->flags() & PACKAGE_FLAG_INSTALLED)
        {
            mitem->setIcon(QIcon::fromTheme("user-online"));
        }
        else
        {
            mitem->setIcon(QIcon::fromTheme("user-offline"));
        }
        
        if (pkg->index() == mpkg->index())
        {
            QFont fnt = mitem->font();
            fnt.setBold(true);
            mitem->setFont(fnt);
        }
        
        listVersions->addItem(mitem);
    }
    
    // Intégration à KDE
    int kdeintegration = pkg->flags() & PACKAGE_FLAG_KDEINTEGRATION;
    QPixmap starOn = QIcon::fromTheme("kde").pixmap(22, 22);
    QPixmap starOff = QIcon::fromTheme("kde").pixmap(22, 22, QIcon::Disabled);
    
    lblStar1->setPixmap(kdeintegration > 0 ? starOn : starOff);
    lblStar2->setPixmap(kdeintegration > 1 ? starOn : starOff);
    lblStar3->setPixmap(kdeintegration > 2 ? starOn : starOff);
    
    // Description
    txtDescr->setText(MainWindow::markdown(md->packageDescription()));
    
    // Dépendances
    QVector<Depend *> deps = pkg->depends();
    QHash<int, QTreeWidgetItem *> typeItems;
    
    treeDepends->clear();
    
    for (int i=0; i<deps.count(); ++i)
    {
        Depend *dep = deps.at(i);
        QTreeWidgetItem *parent = typeItems.value(dep->type(), 0);
        
        if (parent == 0)
        {
            parent = new QTreeWidgetItem(treeDepends);
            parent->setIcon(0, QIcon::fromTheme("folder"));
            QString s;
            
            switch (dep->type())
            {
                case DEPEND_TYPE_DEPEND:
                    s = tr("Dépendances");
                    break;
                case DEPEND_TYPE_CONFLICT:
                    s = tr("Conflict");
                    break;
                case DEPEND_TYPE_PROVIDE:
                    s = tr("Fournit");
                    break;
                case DEPEND_TYPE_SUGGEST:
                    s = tr("Suggère");
                    break;
                case DEPEND_TYPE_REVDEP:
                    s = tr("Requis par");
                    break;
                case DEPEND_TYPE_REPLACE:
                    s = tr("Remplace");
                    break;
            }
            
            parent->setText(0, s);
            treeDepends->addTopLevelItem(parent);
            typeItems.insert(dep->type(), parent);
        }
        
        // Ajouter un élément au parent
        QTreeWidgetItem *child = new QTreeWidgetItem(parent);
        
        child->setIcon(0, QIcon::fromTheme("file"));
        child->setText(0, PackageSystem::dependString(dep->name(), dep->version(), dep->op()));
        
        parent->addChild(child);
    }
    
    // Historique
    QVector<ChangeLogEntry *> entries = md->changelog();
    listChangelog->clear();
    
    for (int i=0; i<entries.count(); ++i)
    {
        ChangeLogEntry *entry = entries.at(i);
        
        QListWidgetItem *mitem = new QListWidgetItem(QString(), listChangelog);
        listChangelog->addItem(mitem);
        
        // Créer le widget
        // ----------------------------------------------------------------------------------
        // | VERSION (par [email](auteur) le <date> dans <distro>)    <---->   type + icône |
        // |                                                                                |
        // |    | texte                                                                          |
        // ----------------------------------------------------------------------------------
        
        QWidget *widget = new QWidget(listChangelog);
        widget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
        QVBoxLayout *vlayout = new QVBoxLayout(widget);
        QHBoxLayout *hlayout = new QHBoxLayout();
        QHBoxLayout *hboxlayout2 = new QHBoxLayout();
        
        QLabel *lblText = new QLabel(listChangelog);
        lblText->setText(tr("<b>%1</b> (par <a href=\"mailto:%2\">%3</a> le %4 dans %5)")
            .arg(entry->version)
            .arg(entry->email)
            .arg(entry->author)
            .arg(entry->date.toString(Qt::SystemLocaleShortDate))
            .arg(entry->distribution));
        
        connect(lblText, SIGNAL(linkActivated(const QString &)), this, SLOT(websiteActivated(const QString &)));
        
        QLabel *lblIconType = new QLabel(listChangelog);
        lblIconType->resize(22, 22);
        lblIconType->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        QLabel *lblType = new QLabel(listChangelog);
        
        switch (entry->type)
        {
            case ChangeLogEntry::LowPriority:
                lblType->setText(tr("Faible priorité"));
                lblIconType->setPixmap(QIcon::fromTheme("text-x-generic").pixmap(22, 22));
                break;
            case ChangeLogEntry::Feature:
                lblType->setText(tr("Fonctionnalité"));
                lblIconType->setPixmap(QIcon::fromTheme("applications-other").pixmap(22, 22));
                break;
            case ChangeLogEntry::BugFix:
                lblType->setText(tr("Correction de bug"));
                lblIconType->setPixmap(QIcon::fromTheme("tools-report-bug").pixmap(22, 22));
                break;
            case ChangeLogEntry::Security:
                lblType->setText(tr("Sécurité"));
                lblIconType->setPixmap(QIcon::fromTheme("security-high").pixmap(22, 22));
                break;
        }
        
        QLabel *lblDescr = new QLabel(listChangelog);
        lblDescr->setText(entry->text);
        
        // Peupler les layouts
        hlayout->addWidget(lblText);
        hlayout->addStretch();
        hlayout->addWidget(lblIconType);
        hlayout->addWidget(lblType);
        
        QFrame *line = new QFrame(listChangelog);
        line->setFrameShape(QFrame::VLine);
        
        hboxlayout2->addSpacing(30);
        hboxlayout2->addWidget(line);
        hboxlayout2->addWidget(lblDescr);
        
        vlayout->addLayout(hlayout);
        vlayout->addLayout(hboxlayout2);
        
        widget->setLayout(vlayout);
        mitem->setSizeHint(widget->minimumSizeHint());
        listChangelog->setItemWidget(mitem, widget);
    }
    
    // Fichiers
    QVector<PackageFile *> files = pkg->files();
    QVector<QTreeWidgetItem *> parentItems;
    
    QString path;
    QStringList curParts, parts;
    int level;
    
    treeFiles->clear();
    
    for (int i=0; i<files.count(); ++i)
    {
        PackageFile *file = files.at(i);
        path = file->path();
        
        parts = path.split('/');
        
        // Trouver le nombre de parts communes entre curParts et parts.
        level = 0;
        for (int i=0; i<qMin(curParts.count()-1, parts.count()-1); ++i)
        {
            if (parts.at(i) == curParts.at(i))
            {
                level++;
            }
            else
            {
                break;
            }
        }
        
        // Sortir toutes les autres parties, avec indentation
        int level2 = level;
        for (int i=level; i<parts.count(); ++i)
        {
            // Créer un élément qui a comme parent parentItems[i-1], ou le root si i = 0
            QTreeWidgetItem *titem;
            
            if (i == 0)
            {
                titem = new QTreeWidgetItem(treeFiles);
                treeFiles->addTopLevelItem(titem);
            }
            else
            {
                QTreeWidgetItem *parent = parentItems.at(i - 1);
                titem = new QTreeWidgetItem(parent);
                parent->addChild(titem);
            }
            
            if (i != parts.count() - 1)
            {
                // Dossier
                titem->setIcon(0, QIcon::fromTheme("folder"));
                titem->setText(1, tr("Dossier"));
                
                // Ajouter titem dans la liste des items parents
                if (parentItems.size() < i + 1)
                {
                    parentItems.append(titem);
                }
                else
                {
                    parentItems[i] = titem;
                }
            }
            else
            {
                // Fichier
                QString s;
                
                #define OUT_FLAG(flag, str) \
                    if (file->flags() & flag) \
                    { \
                        if (!s.isEmpty()) \
                            s += ", "; \
                        s += str; \
                    }
                
                OUT_FLAG(PACKAGE_FILE_INSTALLED, tr("Installé"))
                OUT_FLAG(PACKAGE_FILE_DONTREMOVE, tr("Ne pas supprimer"))
                OUT_FLAG(PACKAGE_FILE_DONTPURGE, tr("Ne pas purger"))
                OUT_FLAG(PACKAGE_FILE_BACKUP, tr("Sauvegarder"))
                OUT_FLAG(PACKAGE_FILE_CHECKBACKUP, tr("Sauvegarder si modifié"))
                OUT_FLAG(PACKAGE_FILE_OVERWRITE, tr("Écraser"))
                OUT_FLAG(PACKAGE_FILE_VIRTUAL, tr("Virtuel"))
                
                titem->setText(1, s);
                titem->setIcon(0, QIcon::fromTheme("file"));
            }
            
            titem->setText(0, parts.at(i));
            
            level2++;
        }
        
        curParts = parts;
    }
}

void MainWindow::websiteActivated(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::licenseActivated(const QString &url)
{
    QDesktopServices::openUrl(QUrl("/usr/share/licenses/" + url.toLower() + ".txt"));
}

void MainWindow::showFlags()
{
    QDialog dialog;
    Ui_Flags flags;
    
    flags.setupUi(&dialog);
    
    // Peupler la boîte de dialogue
    if (treePackages->selectedItems().count() == 0) return;
    
    PackageItem *item = static_cast<PackageItem *>(treePackages->selectedItems().at(0));
    
    if (item == 0) return;
    
    DatabasePackage *pkg = item->package();
    QString s;
    
    switch (pkg->flags() & PACKAGE_FLAG_KDEINTEGRATION)
    {
        case 0:
            s = tr("Pas intégré");
        case 1:
            s = tr("Utilisable");
        case 2:
            s = tr("Bien intégré");
        case 3:
            s = tr("Parfaitement intégré");
    }
    
    flags.lblKDEIntegration->setText(s);
    
    flags.chkDontInstall->setChecked(pkg->flags() & PACKAGE_FLAG_DONTINSTALL);
    flags.chkDontRemove->setChecked(pkg->flags() & PACKAGE_FLAG_DONTREMOVE);
    flags.chkDontUpdate->setChecked(pkg->flags() & PACKAGE_FLAG_DONTUPDATE);
    flags.chkEULA->setChecked(pkg->flags() & PACKAGE_FLAG_EULA);
    flags.chkGUI->setChecked(pkg->flags() & PACKAGE_FLAG_GUI);
    flags.chkInstalled->setChecked(pkg->flags() & PACKAGE_FLAG_INSTALLED);
    flags.chkReboot->setChecked(pkg->flags() & PACKAGE_FLAG_NEEDSREBOOT);
    flags.chkRemoved->setChecked(pkg->flags() & PACKAGE_FLAG_REMOVED);
    flags.chkWanted->setChecked(pkg->flags() & PACKAGE_FLAG_WANTED);
    flags.chkPrimary->setChecked(pkg->flags() & PACKAGE_FLAG_PRIMARY);
    
    // Afficher la boîte de dialogue
    if (dialog.exec() == QDialog::Accepted)
    {
        // Définir les nouveaux flags
        #define APPLY_FLAG(flag, object) \
            if (object->isChecked()) \
                mflags |= flag; \
            else \
                mflags &= ~flag;
            
        int mflags = pkg->flags();
            
        APPLY_FLAG(PACKAGE_FLAG_DONTINSTALL, flags.chkDontInstall)
        APPLY_FLAG(PACKAGE_FLAG_DONTREMOVE, flags.chkDontRemove)
        APPLY_FLAG(PACKAGE_FLAG_DONTUPDATE, flags.chkDontUpdate)
        APPLY_FLAG(PACKAGE_FLAG_WANTED, flags.chkWanted)
        
        pkg->setFlags(mflags);
    }
}






