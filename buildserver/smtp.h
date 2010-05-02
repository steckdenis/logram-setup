/*
 * smtp.h
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

#ifndef __SMTP_H__
#define __SMTP_H__

#include <QObject>
#include <QString>
#include <QEventLoop>
#include <QSslError>

class QSslSocket;
class Worker;

class Mail : public QObject
{
    Q_OBJECT
    
    public:
        Mail(Worker *parent);
        ~Mail();
        
        bool connectToHost(const QString &host, int port = 25, bool encrypted=false);
        bool startTls();
        
        bool login(const QString &username, const QString &password);
        bool sendMail(const QString &from, const QString &to, const QString &subject, const QString &content);
        
    private slots:
        void readyRead();
        void sslErrors(const QList<QSslError> &errors);
        
    private:
        Worker *worker;
        QSslSocket *socket;
        QEventLoop loop;
        
        enum State
        {
            None,
            Ehlo,
            Starttls,
            AuthCommand,
            AuthData,
            MailFrom,
            RcptTo,
            DataCommand,
            DataData,
            Quit
        };
        
        State state;
        
        bool sendCommand(const QByteArray &command);
};

#endif