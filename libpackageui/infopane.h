/*
 * infopane.h
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

#ifndef __INFOPANE_H__
#define __INFOPANE_H__

#include <QTabWidget>

class Ui_infoPane;

namespace LogramUi
{

class PackageDataProviderInterface;

class InfoPane : public QTabWidget
{
    Q_OBJECT
    
    public:
        InfoPane(QWidget *parent);
        ~InfoPane();
        
        void displayData(PackageDataProviderInterface *data);
        
    signals:
        void versionSelected(const QString &version);
        
    private slots:
        void websiteActivated(const QString &url);
        void licenseActivated(const QString &url);
        void showFlags();
        void versionIndexChanged();
        
    private:
        struct Private;
        Private *d;
};

}

#endif