/*
 * thread.h
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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <QThread>
#include <QSqlQuery>

class App;
class Worker;

class Thread : public QThread
{
    public:
        Thread(App *_app, const QSqlQuery &query);
        ~Thread();
        
        int id() const;
        
    protected:
        virtual void run();
        
    private:
        Worker *worker;
};

#endif