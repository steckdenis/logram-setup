/*
 * solver.h
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

/**
 * @file solver_p.h
 * @brief Bindings QtScript pour le Solveur
 * 
 * Le Solveur de LPM utilise un script QtScript pour peser les différent
 * choix. Ce fichier contient des classes <em>wrapper</em> exposant
 * les structures internets de Solver au script.
 */

#ifndef __SOLVERP_H__
#define __SOLVERP_H__

#include "solver.h"

namespace Logram
{
    class Package;
}

class ScriptChild;

class ScriptNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *package READ package)
    Q_PROPERTY(int flags READ flags WRITE setFlags)
    Q_PROPERTY(bool hasError READ hasError)
    Q_PROPERTY(QObject *error READ error)
    Q_PROPERTY(int weight READ weight WRITE setWeight)
    Q_PROPERTY(QObjectList children READ children)
    
    public:
        ScriptNode(Logram::Solver::Node *node, QObject *parent);
        ~ScriptNode();
        
        QObject *package() const;
        int flags() const;
        void setFlags(int value);
        bool hasError() const;
        QObject *error();
        
        int weight() const;
        void setWeight(int value);
        
        QObjectList children();
        
    private:
        struct Private;
        Private *d;
};

class ScriptChild : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObjectList nodes READ nodes)
    Q_PROPERTY(QObject *node READ node)
    
    public:
        ScriptChild(Logram::Solver::Node::Child *child, ScriptNode *parent);
        ~ScriptChild();
        
        QObjectList nodes();
        QObject *node() const;
        
    private:
        struct Private;
        Private *d;
};

class ScriptError : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int type READ type)
    Q_PROPERTY(QObject *other READ other)
    Q_PROPERTY(QString pattern READ pattern)
    Q_PROPERTY(QObject *node READ node)
    
    public:
        ScriptError(Logram::Solver::Error *error, ScriptNode *parent);
        ~ScriptError();
        
        int type() const;
        QObject *other();
        QString pattern() const;
        
        QObject *node() const;
        
    private:
        struct Private;
        Private *d;
};

#endif