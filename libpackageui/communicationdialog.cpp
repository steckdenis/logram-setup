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
#include "utils.h"
#include "ui_communicationdialog.h"

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
using namespace LogramUi;

struct CommunicationDialog::Private
{
    Logram::Communication *comm;
    Logram::Package *pkg;
    
    QVector<QAbstractButton *> listWidgets;
    QWidget *inputWidget;
    const char *inputProperty;
    
    Ui_CommunicationDialog *ui;
};

CommunicationDialog::CommunicationDialog(Package *_pkg, Communication* _comm, QWidget* parent): QDialog(parent)
{
    d = new Private;
    d->comm = _comm;
    d->pkg = _pkg;
    d->ui = new Ui_CommunicationDialog();
    
    d->ui->setupUi(this);
    
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
            d->ui->lblIcon->setPixmap(QIcon(":/images/package.png").pixmap(32, 32));
        }
        else
        {
            d->ui->lblIcon->setPixmap(Utils::pixmapFromData(iconData, 32, 32));
        }
    }
    else
    {
        setWindowTitle(_comm->title());
        d->ui->lblIcon->setPixmap(QIcon(":/images/icon.svg").pixmap(48, 48));
    }
    
    d->ui->lblDescription->setText(_comm->description());
    d->ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    
    switch (_comm->returnType())
    {
        case Communication::String:
        {
            d->ui->lblChoiceType->setText(tr("Entrez une chaîne de caractère :"));
            
            QLineEdit *edit = new QLineEdit(this);
            edit->setText(_comm->defaultString());
            d->ui->verticalLayout->insertWidget(2, edit);
            
            connect(edit, SIGNAL(textChanged(QString)), this, SLOT(updateValues()));
            
            d->inputWidget = edit;
            d->inputProperty = "text";
        }    
            break;
        case Communication::Integer:
        {
            d->ui->lblChoiceType->setText(tr("Entrez un nombre entier :"));
            
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setMaximum(INT_MAX);
            spinBox->setValue(_comm->defaultInt());
            d->ui->verticalLayout->insertWidget(2, spinBox);
            
            connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateValues()));
            
            d->inputWidget = spinBox;
            d->inputProperty = "value";
        }
            break;
        case Communication::Float:
        {
            d->ui->lblChoiceType->setText(tr("Entrez un nombre décimal :"));
            
            QDoubleSpinBox *doubleSpinBox = new QDoubleSpinBox(this);
            doubleSpinBox->setValue(_comm->defaultDouble());
            d->ui->verticalLayout->insertWidget(2, doubleSpinBox);
            
            connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateValues()));
            
            d->inputWidget = doubleSpinBox;
            d->inputProperty = "value";
        }
            break;
        case Communication::MultiChoice:
        case Communication::SingleChoice:
            for (int i=0; i<_comm->choicesCount(); ++i)
            {
                Communication::Choice c = _comm->choice(i);
                QAbstractButton *btn;
                
                if (_comm->returnType() == Communication::SingleChoice)
                {
                    d->ui->lblChoiceType->setText(tr("Sélectionner un choix :"));
                    btn = new QRadioButton(this);
                }
                else
                {
                    d->ui->lblChoiceType->setText(tr("Sélectionner un ou plusieurs choix :"));
                    btn = new QCheckBox(this);
                }
                
                btn->setText(c.title);
                btn->setChecked(c.selected);
                
                connect(btn, SIGNAL(toggled(bool)), this, SLOT(updateValues()));
                
                d->listWidgets.append(btn);
            }
            
            for (int i=d->listWidgets.count() - 1; i >= 0; --i)
            {
                d->ui->verticalLayout->insertWidget(2, d->listWidgets.at(i));
            }
            
            break;
        default:
            break;
    }
    
    updateValues();     // Voir si les valeurs par défaut sont bonnes
}

CommunicationDialog::~CommunicationDialog()
{
    delete d;
}

void CommunicationDialog::updateValues()
{
    // Définir les valeurs
    switch (d->comm->returnType())
    {
        case Communication::String:
            d->comm->setValue(d->inputWidget->property(d->inputProperty).toString());
            break;
        case Communication::Integer:
            d->comm->setValue(d->inputWidget->property(d->inputProperty).toInt());
            break;
        case Communication::Float:
            d->comm->setValue(d->inputWidget->property(d->inputProperty).toDouble());
            break;
        case Communication::SingleChoice:
        case Communication::MultiChoice:
            for (int i=0; i<d->listWidgets.count(); ++i)
            {
                QAbstractButton *btn = d->listWidgets.at(i);
                
                d->comm->enableChoice(i, btn->isChecked());
            }
            
            break;
        default:
            break;
    }
    
    if (!d->comm->isEntryValid())
    {
        d->ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
        d->ui->lblError->setText(QString("<span style=\"color: #ff0000\">%1</span>").arg(d->comm->entryValidationErrorString()));
    }
    else
    {
        d->ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        d->ui->lblError->setText(QString());
    }
}

#include "communicationdialog.moc"
