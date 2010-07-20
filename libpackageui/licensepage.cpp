/*
 * licensepage.cpp
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

#include "licensepage.h"
#include "installwizard.h"
#include "utils.h"

#include <packagelist.h>
#include <packagemetadata.h>
#include <package.h>

#include <QCheckBox>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace Logram;
using namespace LogramUi;

LicensePage::LicensePage(InstallWizard *_wizard) : QWizardPage(_wizard)
{
    wizard = _wizard;

    setupUi(this);
}

bool LicensePage::isComplete()
{
    // On définit le bouton manuellement
    return false;
}

void LicensePage::initializePage()
{
    tbLicences->clear();
    
    PackageList *list = wizard->packageList();
    
    // Explorer les paquets et ajouter des onglets
    for (int i=0; i<list->count(); ++i)
    {
        Package *pkg = list->at(i);
        
        if ((pkg->flags() & PACKAGE_FLAG_EULA) == 0)
        {
            continue;
        }
        
        QWidget *page = new QWidget(tbLicences);
        QVBoxLayout *layout = new QVBoxLayout(page);
        QTextEdit *edit = new QTextEdit(page);
        QCheckBox *box = new QCheckBox(page);
        
        page->setLayout(layout);
        
        // Propriétés
        box->setText(tr("Accepter la licence"));
        box->setChecked(false);
        
        edit->setReadOnly(true);
        
        connect(box, SIGNAL(stateChanged(int)), this, SLOT(boxChecked()));
        checkBoxes.append(box);
        
        // Ajouter les éléments aux widgets
        layout->addWidget(edit);
        layout->addWidget(box, Qt::AlignRight);
        
        // Ajouter la page
        tbLicences->addTab(page, pkg->name());
        
        // Prendre la licence du paquet
        PackageMetaData *md = pkg->metadata();
        
        if (md == 0)
        {
            Utils::packageSystemError(wizard->packageSystem());
            wizard->reject();
            
            delete list;
            return;
        }
        
        md->setCurrentPackage(pkg->name());
        
        edit->setText(Utils::markdown(md->packageEula()));
    }
    
    boxChecked();
}

void LicensePage::boxChecked()
{
    // Explorer les cases à cocher et vérifier qu'elles sont toutes bonnes
    bool nextOk = true;
    
    for (int i=0; i<checkBoxes.count(); ++i)
    {
        if (checkBoxes.at(i)->isChecked() == false)
        {
            nextOk = false;
            break;
        }
    }
    
    wizard->button(QWizard::NextButton)->setEnabled(nextOk);
}

bool LicensePage::validatePage()
{
    boxChecked();
    
    return wizard->button(QWizard::NextButton)->isEnabled();
}

#include "licensepage.moc"
