/*
 * progressdialog.h
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

#ifndef __PROGRESSDIALOG_H__
#define __PROGRESSDIALOG_H__

#include <QHash>
#include <QDialog>

#include <packagesystem.h>

class QVBoxLayout;
class QPushButton;
class QTimer;

namespace LogramUi
{

class ProgressList;

class ProgressDialog : public QDialog
{
    Q_OBJECT
    
    public:
        ProgressDialog(QWidget *parent);
        ~ProgressDialog();
        
        void addProgress(Logram::Progress *progress);
        void updateProgress(Logram::Progress *progress);
        void endProgress(Logram::Progress *progress);
        
        bool canceled();
        
    private slots:
        void cancelClicked();
        void hideElapsed();
        void showElapsed();
        
    protected:
        void closeEvent(QCloseEvent *event);
        
    private:
        struct Private;
        Private *d;
};

}

#endif