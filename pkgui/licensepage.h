/*
 * licensepage.h
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

#ifndef __LICENSEPAGE_H__
#define __LICENSEPAGE_H__

#include <QWizardPage>
#include <QVector>
#include "ui_licensepage.h"

class InstallWizard;
class QCheckBox;

class LicensePage : public QWizardPage, public Ui_licensePage
{
    Q_OBJECT
    
    public:
        LicensePage(InstallWizard *_wizard);
        
        void initializePage();
        bool isComplete();
        bool validatePage();
        
    private slots:
        void boxChecked();

    private:
        InstallWizard *wizard;
        QVector<QCheckBox *> checkBoxes;
        bool nextOk;
};

#endif
