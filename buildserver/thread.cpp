/*
 * thread.cpp
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

#include "app.h"
#include "thread.h"
#include "worker.h"

Thread::Thread(App *_app, const QSqlQuery &query) : QThread(0)
{
    worker = new Worker(_app, query);
    
    // C'est nous qui lanceront le worker
    worker->moveToThread(this);
    worker->eventLoop().moveToThread(this);
}

Thread::~Thread()
{
    delete worker;
}
        
int Thread::id() const
{
    return worker->id();
}

void Thread::run()
{
    worker->run();
}
