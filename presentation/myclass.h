/*
 * myclass.h
 * This file is part of Logram
 *
 * Copyright (C) 2009 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#ifndef __MYCLASS_H__
#define __MYCLASS_H__

//KDE
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <kurl.h>
#include <kparts/browserextension.h>

// Qt
#include <QDesktopWidget>
#include <QLayout>
#include <QObject>

class MyClass : public QObject
{
    Q_OBJECT
    public:
        MyClass(QObject *parent, KHTMLPart *_w);
        
        KHTMLPart *w;
        
    public slots:
        void openUrlRequestDelayed( const KUrl &url,
                              const KParts::OpenUrlArguments& arguments,
                              const KParts::BrowserArguments &browserArguments );
};

#endif