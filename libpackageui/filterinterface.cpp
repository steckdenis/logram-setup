/*
 * filterinterface.cpp
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

#include "filterinterface.h"

using namespace LogramUi;

struct FilterInterface::Private
{
    QString nameFilter;
    QRegExp::PatternSyntax nameSyntax;
    FilterInterface::StatusFilter statusFilter;
    QString repo, distro, section;
};

FilterInterface::FilterInterface(QObject* parent): QObject(parent)
{
    d = new Private;
    d->nameSyntax = QRegExp::Wildcard;
    d->statusFilter = NoFilter;
}

FilterInterface::~FilterInterface()
{
    delete d;
}

void FilterInterface::setNamePattern(const QString& pattern)
{
    d->nameFilter = pattern;
}

void FilterInterface::setNameSyntax(QRegExp::PatternSyntax syntax)
{
    d->nameSyntax = syntax;
}

void FilterInterface::setStatusFilter(FilterInterface::StatusFilter filter)
{
    d->statusFilter = filter;
}


void FilterInterface::setRepository(const QString& repository)
{
    d->repo = repository;
}

void FilterInterface::setDistribution(const QString& distribution)
{
    d->distro = distribution;
}

void FilterInterface::setSection(const QString& section)
{
    d->section = section;
}

void FilterInterface::updateViews()
{
    emit dataChanged();
}

QRegExp FilterInterface::regex() const
{
    if (d->nameFilter.isNull())
    {
        return QRegExp();
    }
    else
    {
        return QRegExp(d->nameFilter, Qt::CaseSensitive, d->nameSyntax);
    }
}

QString FilterInterface::namePattern() const
{
    return d->nameFilter;
}

QRegExp::PatternSyntax FilterInterface::nameSyntax() const
{
    return d->nameSyntax;
}

FilterInterface::StatusFilter FilterInterface::statusFilter() const
{
    return d->statusFilter;
}

QString FilterInterface::repository() const
{
    return d->repo;
}

QString FilterInterface::distribution() const
{
    return d->distro;
}

QString FilterInterface::section() const
{
    return d->section;
}

QString FilterInterface::statusName(FilterInterface::StatusFilter status) const
{
    switch (status)
    {
        case NoFilter:
            return tr("Tous les paquets");
        case Installed:
            return tr("Paquets installés");
        case NotInstalled:
            return tr("Paquets non-installés");
        case Updateable:
            return tr("Paquets pouvant être mis à jour");
        case Orphan:
            return tr("Paquets orphelins");
    }
    
    return QString();
}

#include "filterinterface.moc"
