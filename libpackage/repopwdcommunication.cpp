/*
 * repopwdcommunication.cpp
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

#include "repopwdcommunication.h"

using namespace Logram;

struct RepoCommunication::Private
{
    QString pwd;
};

RepoCommunication::RepoCommunication(QObject *parent) : Communication(parent)
{
    d = new Private;
}

RepoCommunication::~RepoCommunication()
{
    delete d;
}

bool RepoCommunication::error() const
{
    return false;
}

Communication::Type RepoCommunication::type() const
{
    return Communication::Question;
}

Communication::ReturnType RepoCommunication::returnType() const
{
    return Communication::String;
}

Communication::Origin RepoCommunication::origin() const
{
    return Communication::System;
}

QString RepoCommunication::title() const
{
    return tr("Mot de passe GPG");
}

QString RepoCommunication::description() const
{
    return tr("Saisissez le mot de passe nécessaire pour débloquer la clef GPG");
}

QString RepoCommunication::defaultString() const
{
    return QString();
}

int RepoCommunication::defaultInt() const
{
    return 0;
}

double RepoCommunication::defaultDouble() const
{
    return 0.0;
}

int RepoCommunication::defaultIndex() const
{
    return 0;
}

int RepoCommunication::choicesCount()
{
    return 0;
}

Communication::Choice RepoCommunication::choice(int i)
{
    return Communication::Choice();
    
    (void) i;
}

void RepoCommunication::enableChoice(int i, bool enable)
{
    (void) i;
    (void) enable;
}

void RepoCommunication::setValue(const QString &value)
{
    d->pwd = value;
}

void RepoCommunication::setValue(int value)
{
    (void) value;
}

void RepoCommunication::setValue(double value)
{
    (void) value;
}

bool RepoCommunication::isEntryValid() const
{
    return true;
}

QString RepoCommunication::entryValidationErrorString() const
{
    return QString();
}

QString RepoCommunication::processData() const
{
    return d->pwd;
}