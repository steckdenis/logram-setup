/*
 * communication.cpp
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

#include "communication.h"

#include "package.h"
#include "packagemetadata.h"
#include "libpackage.h"

#include <QtXml>
#include <QtDebug>

#include <QRegExp>
#include <QList>

struct Communication::Private
{
    PackageSystem *ps;
    Package *pkg;
    PackageMetaData *md;
    bool error;
    
    QDomElement element;
    
    QStringList keys;
    
    // Valeurs
    QString sValue;
    int iValue;
    double fValue;
    QList<Communication::Choice> choices;
    bool choicesFetched;
    
    QString validateErrorString;
};

Communication::Communication(PackageSystem *ps, Package *pkg, const QString &name) : QObject(pkg)
{
    d = new Private;
    
    d->error = false;
    d->ps = ps;
    d->pkg = pkg;
    d->md = pkg->metadata();
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
        }
        comm = comm.nextSiblingElement("communication");
    }
}

Communication::~Communication()
{
    delete d;
}

bool Communication::error() const
{
    return d->error;
}

void Communication::addKey(const QString &key, const QString &value)
{
    d->keys.append(key + "|" + value);
}

Communication::Type Communication::type() const
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

Communication::ReturnType Communication::returnType() const
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

QString Communication::templateString(const QString &tpl) const
{
    // Explorer les clefs et remplacer le motif {{clef}} dans tpl par la valeur de la clef
    QString rs = tpl;
    
    foreach(const QString &k, d->keys)
    {
        QString key = k.section('|', 0, 0);
        QString value = k.section('|', 1, 1);
        
        rs.replace("{{" + key + "}}", value);
    }
    
    return rs;
}

QString Communication::title() const
{
    QDomElement t = d->element.firstChildElement("title");
    
    if (t.isNull())
    {
        return QString();
    }
    
    return templateString(d->md->stringOfKey(t));
}

QString Communication::description() const
{
    QDomElement t = d->element.firstChildElement("description");
    
    if (t.isNull())
    {
        return QString();
    }
    
    return templateString(d->md->stringOfKey(t));
}

void Communication::setValue(const QString &value)
{
    d->sValue = value;
}

void Communication::setValue(int value)
{
    d->iValue = value;
}

void Communication::setValue(double value)
{
    d->fValue = value;
}

int Communication::choicesCount()
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
        choice.value = c.attribute("value");
        choice.selected = (c.attribute("selected", "false") == "true");
        
        d->choices.append(choice);
        
        c = c.nextSiblingElement("choice");
    }
    
    d->choicesFetched = true;
    return d->choices.count();
}

Communication::Choice Communication::choice(int i)
{
    // S'assurer que les choix sont lus
    choicesCount();
    
    return d->choices.at(i);
}

void Communication::enableChoice(int i, bool enable)
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

bool Communication::isEntryValid() const
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
                int min = rule.attribute("min").toInt();
                bool hasMax = rule.hasAttribute("max");
                int max = rule.attribute("max").toInt();
                
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

QString Communication::entryValidationErrorString() const
{
    return d->validateErrorString;
}

QString Communication::processData() const
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