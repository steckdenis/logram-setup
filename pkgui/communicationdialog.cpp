/*
 * communicationdialog.cpp
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

#include "communicationdialog.h"
#include "mainwindow.h"

#include <QIcon>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

#include <communication.h>
#include <package.h>
#include <packagemetadata.h>

using namespace Logram;

CommunicationDialog::CommunicationDialog(Package *_pkg, Communication* _comm, QWidget* parent): QDialog(parent)
{
    comm = _comm;
    pkg = _pkg;
    
    setupUi(this);
    
    // Propriétés
    if (_pkg != 0)
    {
        setWindowTitle(QString("%1 - %2~%3").arg(_comm->title(), _pkg->name(), _pkg->version()));
        
        PackageMetaData *md = _pkg->metadata();
        QByteArray iconData;
        
        if (md != 0)
        {
            md->setCurrentPackage(_pkg->name());
        }
        
        if (md == 0 || (iconData = md->packageIconData()).isNull())
        {
            lblIcon->setPixmap(QIcon(":/images/package.png").pixmap(32, 32));
        }
        else
        {
            lblIcon->setPixmap(MainWindow::pixmapFromData(iconData, 32, 32));
        }
    }
    else
    {
        setWindowTitle(_comm->title());
        lblIcon->setPixmap(QIcon(":/images/icon.svg").pixmap(48, 48));
    }
    
    lblDescription->setText(_comm->description());
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    
    switch (_comm->returnType())
    {
        case Communication::String:
        {
            lblChoiceType->setText(tr("Entrez une chaîne de caractère :"));
            
            QLineEdit *edit = new QLineEdit(this);
            edit->setText(comm->defaultString());
            verticalLayout->insertWidget(2, edit);
            
            connect(edit, SIGNAL(textChanged(QString)), this, SLOT(updateValues()));
            
            inputWidget = edit;
            inputProperty = "text";
        }    
            break;
        case Communication::Integer:
        {
            lblChoiceType->setText(tr("Entrez un nombre entier :"));
            
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setMaximum(INT_MAX);
            spinBox->setValue(comm->defaultInt());
            verticalLayout->insertWidget(2, spinBox);
            
            connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateValues()));
            
            inputWidget = spinBox;
            inputProperty = "value";
        }
            break;
        case Communication::Float:
        {
            lblChoiceType->setText(tr("Entrez un nombre décimal :"));
            
            QDoubleSpinBox *doubleSpinBox = new QDoubleSpinBox(this);
            doubleSpinBox->setValue(comm->defaultDouble());
            verticalLayout->insertWidget(2, doubleSpinBox);
            
            connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateValues()));
            
            inputWidget = doubleSpinBox;
            inputProperty = "value";
        }
            break;
        case Communication::MultiChoice:
        case Communication::SingleChoice:
            for (int i=0; i<comm->choicesCount(); ++i)
            {
                Communication::Choice c = comm->choice(i);
                QAbstractButton *btn;
                
                if (comm->returnType() == Communication::SingleChoice)
                {
                    lblChoiceType->setText(tr("Sélectionner un choix :"));
                    btn = new QRadioButton(this);
                }
                else
                {
                    lblChoiceType->setText(tr("Sélectionner un ou plusieurs choix :"));
                    btn = new QCheckBox(this);
                }
                
                btn->setText(c.title);
                btn->setChecked(c.selected);
                
                connect(btn, SIGNAL(toggled(bool)), this, SLOT(updateValues()));
                
                listWidgets.append(btn);
            }
            
            for (int i=listWidgets.count() - 1; i >= 0; --i)
            {
                verticalLayout->insertWidget(2, listWidgets.at(i));
            }
            
            break;
        default:
            break;
    }
    
    updateValues();     // Voir si les valeurs par défaut sont bonnes
}

void CommunicationDialog::updateValues()
{
    // Définir les valeurs
    switch (comm->returnType())
    {
        case Communication::String:
            comm->setValue(inputWidget->property(inputProperty).toString());
            break;
        case Communication::Integer:
            comm->setValue(inputWidget->property(inputProperty).toInt());
            break;
        case Communication::Float:
            comm->setValue(inputWidget->property(inputProperty).toDouble());
            break;
        case Communication::SingleChoice:
        case Communication::MultiChoice:
            for (int i=0; i<listWidgets.count(); ++i)
            {
                QAbstractButton *btn = listWidgets.at(i);
                
                comm->enableChoice(i, btn->isChecked());
            }
            
            break;
        default:
            break;
    }
    
    if (!comm->isEntryValid())
    {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
        lblError->setText(QString("<span style=\"color: #ff0000\">%1</span>").arg(comm->entryValidationErrorString()));
    }
    else
    {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        lblError->setText(QString());
    }
}

#include "communicationdialog.moc"
