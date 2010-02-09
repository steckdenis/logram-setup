/*
 * packagecommunication.cpp
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

#include "packagecommunication.h"

#include "package.h"
#include "packagemetadata.h"
#include "packagesystem.h"

#include <QtXml>
#include <QtDebug>

#include <QRegExp>
#include <QList>

using namespace Logram;

struct PackageCommunication::Private
{
    PackageSystem *ps;
    PackageMetaData *md;
    bool error;
    
    QDomElement element;
    
    // Valeurs
    QString sValue;
    int iValue;
    double fValue;
    QList<Communication::Choice> choices;
    bool choicesFetched;
    
    QString validateErrorString;
};

PackageCommunication::PackageCommunication(PackageSystem *ps, PackageMetaData *md, const QString &name) : Communication(ps)
{
    d = new Private;
    
    d->error = false;
    d->ps = ps;
    d->md = md;
    d->choicesFetched = false;
    
    if (d->md == 0)
    {
        // Les métadonnées n'ont pas pu être obtenues
        d->error = true;
        return;
    }
    
    // Trouver l'élément de la communication
    QDomElement comm = d->md->documentElement().firstChildElement("communication");
    
    while (!comm.isNull())
    {
        if (comm.attribute("name") == name)
        {
            d->element = comm;
            break;
        }
        comm = comm.nextSiblingElement("communication");
    }
}

PackageCommunication::~PackageCommunication()
{
    delete d;
}

bool PackageCommunication::error() const
{
    return d->error;
}

Communication::Origin PackageCommunication::origin() const
{
    return Communication::Package;
}

Communication::Type PackageCommunication::type() const
{
    QString t = d->element.attribute("type");
    
    if (t == "message")
    {
        return Message;
    }
    else if (t == "question")
    {
        return Question;
    }
    else
    {
        return None;
    }
}

Communication::ReturnType PackageCommunication::returnType() const
{
    if (type() != Question)
    {
        return Invalid;
    }
    
    QDomElement ret = d->element.firstChildElement("return");
    
    if (ret.isNull())
    {
        return Invalid;
    }
    
    QString t = ret.attribute("type");
    
    if (t == "string")
    {
        return String;
    }
    else if (t == "int")
    {
        return Integer;
    }
    else if (t == "float")
    {
        return Float;
    }
    else if (t == "singlechoice")
    {
        return SingleChoice;
    }
    else if (t == "multichoice")
    {
        return MultiChoice;
    }
    else
    {
        return Invalid;
    }
}

QString PackageCommunication::title() const
{
    QDomElement t = d->element.firstChildElement("title");
    
    if (t.isNull())
    {
        return QString();
    }
    
    return templateString(d->md->stringOfKey(t));
}

QString PackageCommunication::description() const
{
    QDomElement t = d->element.firstChildElement("description");
    
    if (t.isNull())
    {
        return QString();
    }
    
    return templateString(d->md->stringOfKey(t));
}

void PackageCommunication::setValue(const QString &value)
{
    d->sValue = value;
}

void PackageCommunication::setValue(int value)
{
    d->iValue = value;
}

void PackageCommunication::setValue(double value)
{
    d->fValue = value;
}

QString PackageCommunication::defaultString() const
{
    if (returnType() != String)
    {
        return QString();
    }
    
    QDomElement ret = d->element.firstChildElement("return");
    
    return templateString(ret.attribute("default"));
}

int PackageCommunication::defaultInt() const
{
    return defaultString().toInt();
}

double PackageCommunication::defaultDouble() const
{
    return defaultString().toDouble();
}

int PackageCommunication::defaultIndex() const
{
    int rs;
    
    // Lire les choix
    if (returnType() != SingleChoice && returnType() != MultiChoice)
    {
        return -1;
    }
    
    QDomElement ret = d->element.firstChildElement("return");
    
    if (ret.isNull())
    {
        return -1;
    }
    
    QDomElement c = ret.firstChildElement("choice");
    
    rs = 0;
    while (!c.isNull())
    {
        if (templateString(c.attribute("selected", "false")) == "true")
        {
            return rs;
        }
        
        c = c.nextSiblingElement("choice");
        rs++;
    }
    
    return -1;
}

int PackageCommunication::choicesCount()
{
    if (d->choicesFetched)
    {
        return d->choices.count();
    }
    
    // Lire les choix
    if (returnType() != SingleChoice && returnType() != MultiChoice)
    {
        return 0;
    }
    
    QDomElement ret = d->element.firstChildElement("return");
    
    if (ret.isNull())
    {
        return 0;
    }
    
    QDomElement c = ret.firstChildElement("choice");
    
    while (!c.isNull())
    {
        Choice choice;
        
        choice.title = templateString(d->md->stringOfKey(c));
        choice.value = templateString(c.attribute("value"));
        choice.selected = (templateString(c.attribute("selected", "false")) == "true");
        
        d->choices.append(choice);
        
        c = c.nextSiblingElement("choice");
    }
    
    d->choicesFetched = true;
    return d->choices.count();
}

Communication::Choice PackageCommunication::choice(int i)
{
    // S'assurer que les choix sont lus
    choicesCount();
    
    return d->choices.at(i);
}

void PackageCommunication::enableChoice(int i, bool enable)
{
    d->choices[i].selected = enable;
    
    // Si on est dans le cas d'un simple choix, s'assurer qu'un seul est sélectionné
    if (ReturnType() == SingleChoice && enable == true)
    {
        for (int j=0; j<d->choices.count(); ++j)
        {
            if (j != i)
            {
                d->choices[j].selected = false;
            }
        }
    }
}

bool PackageCommunication::isEntryValid() const
{
    // Pas d'entrée aux messages
    if (type() == Message)
    {
        return false;
    }
    
    // Explorer les rules de validation
    QDomElement ret = d->element.firstChildElement("return");
    
    if (ret.isNull())
    {
        return false;
    }
    
    QDomElement validate = ret.firstChildElement("validate");
    
    while (!validate.isNull())
    {
        QDomElement rule = validate.firstChildElement("rule");
        bool pass = true;
        
        while (!rule.isNull())
        {
            if (rule.attribute("type") == "regex")
            {
                // Vérifier que l'entrée est bonne, normalement de type texte
                if (returnType() != String)
                {
                    continue;
                }
                
                QRegExp regex(rule.text());
                
                if (!regex.exactMatch(d->sValue))
                {
                    pass = false;
                    break;
                }
            }
            else if (rule.attribute("type") == "limit")
            {
                // Max-Min des entiers et de la taille des chaînes
                bool hasMin = rule.hasAttribute("min");
                int min = templateString(rule.attribute("min")).toInt();
                bool hasMax = rule.hasAttribute("max");
                int max = templateString(rule.attribute("max")).toInt();
                
                if (returnType() == String)
                {
                    if (hasMin && d->sValue.length() < min)
                    {
                        pass = false;
                        break;
                    }
                    if (hasMax && d->sValue.length() > max)
                    {
                        pass = false;
                        break;
                    }
                }
                else if (returnType() == Integer)
                {
                    if (hasMin && d->iValue < min)
                    {
                        pass = false;
                        break;
                    }
                    if (hasMax && d->iValue > max)
                    {
                        pass = false;
                        break;
                    }
                }
                else if (returnType() == Float)
                {
                    if (hasMin && d->fValue < min)
                    {
                        pass = false;
                        break;
                    }
                    if (hasMax && d->fValue > max)
                    {
                        pass = false;
                        break;
                    }
                }
            }
            
            rule = rule.nextSiblingElement("rule");
        }
        
        // Si on ne passe pas le test, définir la chaîne d'erreur (si elle existe), et retourner false
        if (!pass)
        {
            d->validateErrorString = d->md->stringOfKey(validate.firstChildElement("errormessage"));
            return false;
        }
        
        validate = validate.nextSiblingElement("validate");
    }
    
    return true;
}

QString PackageCommunication::entryValidationErrorString() const
{
    return d->validateErrorString;
}

QString PackageCommunication::processData() const
{
    QString rs;
    
    if (type() == Question)
    {
        switch (returnType())
        {
            case String:
                return d->sValue;
                
            case Integer:
                return QString::number(d->iValue);
                
            case Float:
                return QString::number(d->fValue);
                
            case SingleChoice:
                for (int i=0; i<d->choices.count(); ++i)
                {
                    if (d->choices.at(i).selected)
                    {
                        return d->choices.at(i).value;
                    }
                }
                
            case MultiChoice:
                for (int i=0; i<d->choices.count(); ++i)
                {
                    if (d->choices.at(i).selected)
                    {
                        if (!rs.isNull())
                        {
                            rs += ' ';
                        }
                        
                        rs += d->choices.at(i).value;
                    }
                }
                
                return rs;
                
            default:
                break;
        }
    }
    
    return QString();
}