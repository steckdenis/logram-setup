/*
 * communication.h
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

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "templatable.h"

namespace Logram
{
    
class PackageSystem;
class Package;

class Communication : public Templatable
{
    public:
        Communication(PackageSystem *ps, Package *pkg, const QString &name);
        ~Communication();
        bool error() const;
        
        enum Type
        {
            None,
            Question,
            Message
        };
        
        enum ReturnType
        {
            Invalid,
            String,
            Integer,
            Float,
            SingleChoice,
            MultiChoice
        };
        
        struct Choice
        {
            QString title;
            QString value;
            bool selected;
        };
        
        Type type() const;
        ReturnType returnType() const;
        
        QString title() const;
        QString description() const;
        
        QString defaultString() const;
        int defaultInt() const;
        double defaultDouble() const;
        int defaultIndex() const;
        
        int choicesCount();
        Choice choice(int i);
        void enableChoice(int i, bool enable);
        
        void setValue(const QString &value);
        void setValue(int value);
        void setValue(double value);
        
        bool isEntryValid() const;   // Indique si ce que l'utilisateur a entré correspond à la règle de vérification
        QString entryValidationErrorString() const;
        QString processData() const; // Valeur à renvoyer au processus helperscript
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif