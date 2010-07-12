/*
 * donepage.cpp
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

#include "donepage.h"
#include "installwizard.h"

#include <QIcon>
#include <QGridLayout>
#include <QFrame>

#include <package.h>
#include <packagelist.h>

using namespace Logram;

DonePage::DonePage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    wizard = _wizard;

    setupUi(this);
}

void DonePage::initializePage()
{
    int msgcount = 0;       // Permettra de savoir s'il faut afficher ou pas «L'installation s'est bien déroulée»
    
    // Commencer par les messages des paquets
    QList<PackageMessage> messages = wizard->packageMessages();
    
    foreach (const PackageMessage &message, messages)
    {
        Package *pkg = message.pkg;
        
        if (pkg != 0)
        {
            addMessage(QIcon(":/images/package.png"), QString("<b>%1</b> (%2~%3)").arg(message.title, pkg->name(), pkg->version()), message.message);
        }
        else
        {
            addMessage(QIcon(":/images/icon.svg"), QString("<b>%1</b>").arg(message.title), message.message);
        }
        
        msgcount++;
    }
    
    // S'il faut redémarrer, le dire
    if (wizard->packageList()->needsReboot())
    {
        addMessage(QIcon::fromTheme("system-reboot"), tr("<b>Redémarrage de l'ordinateur</b>"), tr("Un redémarrage de l'ordinateur est nécessaire pour que tous les paquets fonctionnent."));
        
        msgcount++;
    }
    
    // S'il y a des paquets orphelins, le dire aussi
    if (wizard->packageList()->orphans().count() != 0)
    {
        addMessage(QIcon(":/images/pkg-purge.png"), tr("<b>Paquets orphelins</b>"), tr("%n paquet(s) ne sont plus nécessaires au fonctionnement des autres.", "", wizard->packageList()->orphans().count()));
        
        msgcount++;
    }
    
    // Si aucun message n'a encore été affiché, dire que tout s'est bien passé
    if (msgcount == 0)
    {
        addMessage(QIcon::fromTheme("dialog-ok-apply"), tr("<b>Installation des paquets</b>"), tr("L'installation des paquets s'est déroulée avec succès."));
    }
}

void DonePage::addMessage(const QIcon &icon, const QString &title, const QString &message)
{
    QListWidgetItem *item = new QListWidgetItem(listRemarks);
    listRemarks->addItem(item);
    
    // Créer un widget
    QWidget *widget = new QWidget(this);
    QGridLayout *layout = new QGridLayout(widget);
    QLabel *lblIcon = new QLabel(widget);
    QLabel *lblTitle = new QLabel(widget);
    QLabel *lblMessage = new QLabel(widget);
    QFrame *vline = new QFrame(widget);
    
    // Propriétés
    widget->setLayout(layout);
    
    vline->setFrameShape(QFrame::VLine);
    
    lblIcon->setPixmap(icon.pixmap(22, 22));
    lblIcon->resize(22, 22);
    lblIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    lblTitle->setText(title);
    
    lblMessage->setText(message);
    
    // Remplir le layout
    layout->addWidget(lblIcon, 0, 0);
    layout->addWidget(lblTitle, 0, 2);
    
    layout->addWidget(vline, 1, 1);
    layout->addWidget(lblMessage, 1, 2);
    
    // Ajouter l'enregistrement
    item->setSizeHint(widget->minimumSizeHint());
    listRemarks->setItemWidget(item, widget);
}
