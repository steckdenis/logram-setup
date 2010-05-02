/*
 * smtp.cpp
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

#include "smtp.h"
#include "worker.h"

#include <QSslSocket>
#include <QDateTime>

Mail::Mail(Worker *parent) : QObject(parent)
{
    worker = parent;
    state = None;
    socket = new QSslSocket(this);
    socket->ignoreSslErrors();
    socket->setProtocol(QSsl::TlsV1);

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

Mail::~Mail()
{
    // Noting
}
        
bool Mail::connectToHost(const QString &host, int port, bool encrypted)
{
    if (!encrypted)
    {
        socket->connectToHost(host, port, QIODevice::ReadWrite);
        
        if (!socket->waitForConnected())
        {
            worker->log(Error, "Unable to connect to the host " + host + ":" + QString::number(port));
            return false;
        }
    }
    else
    {
        socket->connectToHostEncrypted(host, port, QIODevice::ReadWrite);
    
        if (!socket->waitForEncrypted()) {
            worker->log(Error, "Unable to connect to encrypted host " + host + ":" + QString::number(port) + ": " + socket->errorString());
            return false;
        }
    }
    
    // Le server va envoyer sa signature
    if (loop.exec() != 0) return false;
    
    // ehlo
    state = Ehlo;
    if (!sendCommand("EHLO logram-build-server"))
    {
        return false;
    }
    
    return true;
}

bool Mail::startTls()
{
    state = Starttls;
    
    if (!sendCommand("STARTTLS"))
    {
        return false;
    }
    
    return true;
}

bool Mail::login(const QString &username, const QString &password)
{
    // Dire qu'on s'authentifie
    state = AuthCommand;
    
    if (!sendCommand("AUTH PLAIN"))
    {
        return false;
    }

    // Envoyer les identifiants
    state = AuthData;
    QByteArray data;
    
    data += '\0';
    data += username.toUtf8();
    data += '\0';
    data += password.toUtf8();
    
    if (!sendCommand(data.toBase64()))
    {
        return false;
    }
    
    return true;
}

bool Mail::sendMail(const QString &from, const QString &to, const QString &subject, const QString &content)
{
    state = MailFrom;
    
    if (!sendCommand("MAIL FROM: <" + from.toUtf8() + ">"))
    {
        return false;
    }
    
    state = RcptTo;
    
    if (!sendCommand("RCPT TO: <" + to.toUtf8() + ">"))
    {
        return false;
    }
    
    state = DataCommand;
    
    if (!sendCommand("DATA"))
    {
        return false;
    }
    
    // Adapter le mail
    state = DataData;
    QByteArray cnt = content.toUtf8();

    cnt.replace('\n', "\r\n");              // \r\n comme séparateur de ligne
    cnt.replace("\r\n.\r\n", "\r\n..\r\n"); // \r\n.\r\n signale la fin du mail, l'échapper.

    // Buffer à envoyer, sans le \r\n final ajouté par sendCommand
    QByteArray mail;
    
    mail += "Date: " + QDateTime::currentDateTime().toUTC().toString("ddd, dd MMM yyyy hh:mm:ss +0000") + "\r\n";
    mail += "To: " + to.toUtf8() + "\r\n";
    mail += "From: " + from.toUtf8() + "\r\n";
    mail += "Subject: " + subject.toUtf8() + "\r\n";
    mail += cnt;
    mail += "\r\n.";
    
    if (!sendCommand(mail))
    {
        return false;
    }
    
    // Quitter, on a fini
    state = Quit;
    
    if (!sendCommand("QUIT"))
    {
        return false;
    }
    
    // Se déconnecter
    socket->disconnectFromHost();
    
    return true;
}

bool Mail::sendCommand(const QByteArray& command)
{
    socket->write(command + "\r\n");
    socket->flush();

    return (loop.exec() == 0);
}

void Mail::sslErrors(const QList<QSslError> &errors)
{
    foreach (const QSslError &error, errors)
    {
        worker->log(Error, "SSL error : " + error.errorString());
    }
}

void Mail::readyRead()
{
    // Lire toutes les lignes, sauter celles dont le 4e cara n'est pas un espace
    QByteArray line;
    int code = -1;
    
    while (!socket->atEnd())
    {
        line = socket->readLine();
        
        if (line[3] == ' ')
        {
            code = line.left(3).toInt();
        }
    }
    
    if (code != -1)
    {   
        switch (state)
        {
            case None:
                if (code != 220) 
                {
                    worker->log(Error, "SMTP connection failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case Ehlo:
                if (code != 250) 
                {
                    worker->log(Error, "EHLO command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case Starttls:
                if (code != 220) 
                {
                    worker->log(Error, "STARTTLS command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                socket->startClientEncryption();
        
                if (!socket->waitForEncrypted(1000))
                {
                    worker->log(Error, "Unable to initiate a TLS connexion in STARTTLS");
                    loop.exit(1);
                    return;
                }
                
                break;
                
            case AuthCommand:
                if (code != 334) 
                {
                    worker->log(Error, "AUTH PLAIN command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case AuthData:
                if (code != 235) 
                {
                    worker->log(Error, "Authentication data sending failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case MailFrom:
                if (code != 250) 
                {
                    worker->log(Error, "MAIL FROM command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case RcptTo:
                if (code != 250) 
                {
                    worker->log(Error, "RCPT TO command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case DataCommand:
                if (code != 354) 
                {
                    worker->log(Error, "DATA command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case DataData:
                if (code != 250) 
                {
                    worker->log(Error, "Message data sending failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
                
            case Quit:
                if (code != 221) 
                {
                    worker->log(Error, "QUIT command failed with error code " + code);
                    worker->log(Message, line.trimmed());
                    loop.exit(1); 
                    return; 
                }
                
                break;
        }
        
        loop.exit(0);
    }
}

#include "smtp.moc"