/*
 * templatable.h
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

#include "templatable.h"

#include <QList>

using namespace Logram;

struct Templatable::Private
{
    struct Entry
    {
        QString key;
        QString value;
    };
    
    QList<Entry> entries;
};

Templatable::Templatable(QObject *parent) : QObject(parent)
{
    d = new Private;
}

Templatable::~Templatable()
{
    delete d;
}
        
void Templatable::addKey(const QString &key, const QString &value)
{
    for (int i=0; i<d->entries.count(); ++i)
    {
        Private::Entry &entry = d->entries[i];
        
        if (entry.key == key)
        {
            entry.value = value;
            return;
        }
    }
    
    Private::Entry entry;
    
    entry.key = key;
    entry.value = value;
    
    d->entries.append(entry);
}

void Templatable::removeKey(const QString &key)
{
    for (int i=0; i<d->entries.count(); ++i)
    {
        const Private::Entry &entry = d->entries.at(i);
        
        if (entry.key == key)
        {
            d->entries.removeAt(i);
            return;
        }
    }
}

QString Templatable::getKey(const QString &key) const
{
    foreach (const Private::Entry &entry, d->entries)
    {
        if (entry.key == key)
        {
            return entry.value;
        }
    }
    
    return QString();
}

bool Templatable::contains(const QString &key) const
{
    foreach (const Private::Entry &entry, d->entries)
    {
        if (entry.key == key)
        {
            return true;
        }
    }
    
    return false;
}
        
QString Templatable::templateString(const QString &tpl) const
{
    // Explorer les clefs et remplacer le motif {{clef}} dans tpl par la valeur de la clef
    QString rs = tpl;
    
    for (int i=0; i<d->entries.count(); ++i)
    {
        const Private::Entry &entry = d->entries.at(i);
        QString mkey = "{{" + entry.key + "}}";
        
        if (rs.contains(mkey))
        {
            rs.replace(mkey, entry.value);
            i = 0;  // Reboucler, pour permettre de remplacer des clefs par des clefs
        }
    }
    
    return rs;
}

QByteArray Templatable::templateString(const QByteArray &tpl) const
{
    return templateString(QString(tpl)).toUtf8();
}