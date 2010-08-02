/*
 * filterinterface.h
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

#ifndef __FILTERINTERFACE_H__
#define __FILTERINTERFACE_H__

#include <QObject>
#include <QRegExp>

namespace LogramUi
{

class FilterInterface : public QObject
{
    Q_OBJECT
    
    public:
        FilterInterface(QObject *parent);
        ~FilterInterface();
        
        enum StatusFilter
        {
            NoFilter = 0,
            Installed = 1,
            NotInstalled = 2,
            Updateable = 3,
            Orphan = 4
        };
        
        void setNamePattern(const QString &pattern);
        void setNameSyntax(QRegExp::PatternSyntax syntax);
        void setStatusFilter(StatusFilter filter);
        void setRepository(const QString &repository);
        void setDistribution(const QString &distribution);
        void setSection(const QString &section);
        
        void updateViews();
        
        QRegExp regex() const;
        QRegExp::PatternSyntax nameSyntax() const;
        QString namePattern() const;
        StatusFilter statusFilter() const;
        QString repository() const;
        QString distribution() const;
        QString section() const;
        QString statusName(StatusFilter status) const;
        
    signals:
        void dataChanged();
        
    private:
        struct Private;
        Private *d;
};

}

#endif