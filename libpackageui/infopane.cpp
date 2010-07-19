/*
 * infopane.cpp
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

#include "infopane.h"
#include "packagedataproviderinterface.h"
#include "utils.h"
#include "ui_flags.h"
#include "ui_infopane.h"

#include <package.h>
#include <packagesystem.h>
#include <packagemetadata.h>

#include <QDesktopServices>
#include <QUrl>
#include <QIcon>

using namespace Logram;
using namespace LogramUi;

struct InfoPane::Private
{
    PackageDataProviderInterface *data;
    Ui_infoPane *ui;
};

InfoPane::InfoPane(QWidget* parent): QTabWidget(parent)
{
    d = new Private;
    d->data = 0;
    d->ui = new Ui_infoPane;
    d->ui->setupUi(this);
    
    // Icônes
    d->ui->btnFlags->setIcon(QIcon::fromTheme("flag"));
}

InfoPane::~InfoPane()
{
    if (d->data)
    {
        delete d->data;
    }
    
    delete d->ui;
    delete d;
}

void InfoPane::displayData(PackageDataProviderInterface* data)
{
    if (d->data)
    {
        delete d->data;
    }
    
    d->data = data;
    
    // Infos de base
    d->ui->lblTitle->setText("<a href=\"" + data->website() + "\">" + data->title() + "</a>");
    d->ui->lblShortDesc->setText(data->shortDesc());
    d->ui->lblSection->setText(data->section());
    d->ui->lblRepo->setText(data->repository() + "»" + data->distribution());
    d->ui->lblDownload->setText(PackageSystem::fileSizeFormat(data->downloadSize()));
    d->ui->lblInstall->setText(PackageSystem::fileSizeFormat(data->installSize()));
    d->ui->lblLicense->setText("<a href=\"" + data->license() + "\">" + data->license() + "</a>");
    
    // Icône
    QByteArray iconData = (data->iconData());
    
    if (iconData.isNull())
    {
        d->ui->lblIcon->setPixmap(QIcon(":/images/package.png").pixmap(48, 48));
    }
    else
    {
        d->ui->lblIcon->setPixmap(Utils::pixmapFromData(iconData, 48, 48));
    }
    
    // Liste des versions
    QVector<PackageDataProviderInterface *> versions = data->versions();
    d->ui->listVersions->clear();
    
    for (int i=0; i<versions.count(); ++i)
    {
        PackageDataProviderInterface *version = versions.at(i);
        
        QListWidgetItem *mitem = new QListWidgetItem(version->version(), d->ui->listVersions);
        
        if (version->flags() & PACKAGE_FLAG_INSTALLED)
        {
            mitem->setIcon(QIcon::fromTheme("user-online"));
        }
        else
        {
            mitem->setIcon(QIcon::fromTheme("user-offline"));
        }
        
        if (version->version() == data->version())
        {
            QFont fnt = mitem->font();
            fnt.setBold(true);
            mitem->setFont(fnt);
        }
        
        d->ui->listVersions->addItem(mitem);
        
        delete version;     // Plus besoin
    }
    
    // Intégration à KDE
    int kdeintegration = data->flags() & PACKAGE_FLAG_KDEINTEGRATION;
    QPixmap starOn = QIcon::fromTheme("kde").pixmap(22, 22);
    QPixmap starOff = QIcon::fromTheme("kde").pixmap(22, 22, QIcon::Disabled);
    
    d->ui->lblStar1->setPixmap(kdeintegration > 0 ? starOn : starOff);
    d->ui->lblStar2->setPixmap(kdeintegration > 1 ? starOn : starOff);
    d->ui->lblStar3->setPixmap(kdeintegration > 2 ? starOn : starOff);
    
    // Description
    d->ui->txtDescr->setText(data->longDesc());
    
    // Dépendances
    QVector<Depend *> deps = data->depends();
    QHash<int, QTreeWidgetItem *> typeItems;
    
    d->ui->treeDepends->clear();
    
    for (int i=0; i<deps.count(); ++i)
    {
        Depend *dep = deps.at(i);
        QTreeWidgetItem *parent = typeItems.value(dep->type(), 0);
        
        if (parent == 0)
        {
            parent = new QTreeWidgetItem(d->ui->treeDepends);
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
            d->ui->treeDepends->addTopLevelItem(parent);
            typeItems.insert(dep->type(), parent);
        }
        
        // Ajouter un élément au parent
        QTreeWidgetItem *child = new QTreeWidgetItem(parent);
        
        child->setIcon(0, QIcon::fromTheme("file"));
        child->setText(0, PackageSystem::dependString(dep->name(), dep->version(), dep->op()));
        
        parent->addChild(child);
    }
    
    // Historique
    d->ui->listChangelog->clear();
    
    QVector<ChangeLogEntry *> entries = data->changelog();
    
    for (int i=0; i<entries.count(); ++i)
    {
        ChangeLogEntry *entry = entries.at(i);
        
        QListWidgetItem *mitem = new QListWidgetItem(QString(), d->ui->listChangelog);
        d->ui->listChangelog->addItem(mitem);
        
        // Créer le widget
        // ----------------------------------------------------------------------------------
        // | VERSION (par [email](auteur) le <date> dans <distro>)    <---->   type + icône |
        // |                                                                                |
        // |    | texte                                                                     |
        // ----------------------------------------------------------------------------------
        
        QWidget *widget = new QWidget(d->ui->listChangelog);
        widget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
        QVBoxLayout *vlayout = new QVBoxLayout(widget);
        QHBoxLayout *hlayout = new QHBoxLayout();
        QHBoxLayout *hboxlayout2 = new QHBoxLayout();
        
        QLabel *lblText = new QLabel(d->ui->listChangelog);
        lblText->setText(tr("<b>%1</b> (par <a href=\"mailto:%2\">%3</a> le %4 dans %5)")
            .arg(entry->version)
            .arg(entry->email)
            .arg(entry->author)
            .arg(entry->date.toString(Qt::SystemLocaleShortDate))
            .arg(entry->distribution));
        
        connect(lblText, SIGNAL(linkActivated(const QString &)), this, SLOT(websiteActivated(const QString &)));
        
        QLabel *lblIconType = new QLabel(d->ui->listChangelog);
        lblIconType->resize(22, 22);
        lblIconType->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        
        QLabel *lblType = new QLabel(d->ui->listChangelog);
        
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
        
        QLabel *lblDescr = new QLabel(d->ui->listChangelog);
        lblDescr->setText(Utils::markdown(entry->text));
        
        // Peupler les layouts
        hlayout->addWidget(lblText);
        hlayout->addStretch();
        hlayout->addWidget(lblIconType);
        hlayout->addWidget(lblType);
        
        QFrame *line = new QFrame(d->ui->listChangelog);
        line->setFrameShape(QFrame::VLine);
        
        hboxlayout2->addSpacing(30);
        hboxlayout2->addWidget(line);
        hboxlayout2->addWidget(lblDescr);
        
        vlayout->addLayout(hlayout);
        vlayout->addLayout(hboxlayout2);
        
        widget->setLayout(vlayout);
        mitem->setSizeHint(widget->minimumSizeHint());
        d->ui->listChangelog->setItemWidget(mitem, widget);
    }
    
    // Fichiers
    QVector<PackageFile *> files = data->files();
    QVector<QTreeWidgetItem *> parentItems;
    
    QString path;
    QStringList curParts, parts;
    int level;
    
    d->ui->treeFiles->clear();
    
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
                titem = new QTreeWidgetItem(d->ui->treeFiles);
                d->ui->treeFiles->addTopLevelItem(titem);
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

void LogramUi::InfoPane::licenseActivated(const QString& url)
{
    QDesktopServices::openUrl(QUrl("/usr/share/licenses/" + url.toLower() + ".txt"));
}

void LogramUi::InfoPane::websiteActivated(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void LogramUi::InfoPane::showFlags()
{
    QDialog dialog;
    Ui_Flags flags;
    
    flags.setupUi(&dialog);
    
    // Flags
    QString s;
    int mflags = d->data->flags();
    
    switch (mflags & PACKAGE_FLAG_KDEINTEGRATION)
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
    
    flags.chkDontInstall->setChecked(mflags & PACKAGE_FLAG_DONTINSTALL);
    flags.chkDontRemove->setChecked(mflags & PACKAGE_FLAG_DONTREMOVE);
    flags.chkDontUpdate->setChecked(mflags & PACKAGE_FLAG_DONTUPDATE);
    flags.chkEULA->setChecked(mflags & PACKAGE_FLAG_EULA);
    flags.chkGUI->setChecked(mflags & PACKAGE_FLAG_GUI);
    flags.chkInstalled->setChecked(mflags & PACKAGE_FLAG_INSTALLED);
    flags.chkReboot->setChecked(mflags & PACKAGE_FLAG_NEEDSREBOOT);
    flags.chkRemoved->setChecked(mflags & PACKAGE_FLAG_REMOVED);
    flags.chkWanted->setChecked(mflags & PACKAGE_FLAG_WANTED);
    flags.chkPrimary->setChecked(mflags & PACKAGE_FLAG_PRIMARY);
    
    // Afficher la boîte de dialogue
    if (dialog.exec() == QDialog::Accepted)
    {
        // Définir les nouveaux flags
        #define APPLY_FLAG(flag, object) \
            if (object->isChecked()) \
                mflags |= flag; \
            else \
                mflags &= ~flag;
            
        APPLY_FLAG(PACKAGE_FLAG_DONTINSTALL, flags.chkDontInstall)
        APPLY_FLAG(PACKAGE_FLAG_DONTREMOVE, flags.chkDontRemove)
        APPLY_FLAG(PACKAGE_FLAG_DONTUPDATE, flags.chkDontUpdate)
        APPLY_FLAG(PACKAGE_FLAG_WANTED, flags.chkWanted)
        
        d->data->setFlags(mflags);
    }
}

#include "infopane.moc"