/*
 * communicationdialog.h
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

#ifndef __COMMUNICATIONDIALOG_H__
#define __COMMUNICATIONDIALOG_H__

#include <QDialog>
#include <QVector>
#include "ui_communicationdialog.h"

class QAbstractButton;

namespace Logram
{
    class Communication;
    class Package;
}

class CommunicationDialog : public QDialog, private Ui_CommunicationDialog
{
    Q_OBJECT
    
    public:
        CommunicationDialog(Logram::Package *_pkg, Logram::Communication *_comm, QWidget* parent = 0);
        
    private slots:
        void updateValues();
        
    private:
        Logram::Communication *comm;
        Logram::Package *pkg;
        
        QVector<QAbstractButton *> listWidgets;
        QWidget *inputWidget;
        const char *inputProperty;
};

#endif