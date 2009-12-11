/*
 * packagesource.cpp
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

#include "packagesource.h"
#include "packagemetadata.h"
#include "packagesystem.h"

#include <QHash>
#include <QProcess>
#include <QEventLoop>
#include <QFile>
#include <QDir>

#include <QtXml>
#include <QtDebug>

using namespace Logram;

struct PackageSource::Private
{
    PackageSystem *ps;
    PackageMetaData *md;
    PackageSource *src;
    
    QHash<Option, QVariant> options;
    
    // Processus
    QEventLoop loop;
    QString commandLine;
    QByteArray buffer;
    
    // Fonctions
    QByteArray script(const QString &key, const QString &type);
    bool runScript(const QByteArray &script, const QStringList &args, bool block = true);
};

PackageSource::PackageSource(PackageSystem *ps) : QObject(ps)
{
    d = new Private;
    
    d->ps = ps;
    d->src = this;
}

PackageSource::~PackageSource()
{
    delete d->md;
    
    delete d;
}

bool PackageSource::setMetaData(const QString &fileName)
{
    d->md = new PackageMetaData(d->ps);
    
    d->md->loadFile(fileName, QByteArray(), false);
    
    return !d->md->error();
}

void PackageSource::setOption(Option opt, const QVariant &value)
{
    d->options.insert(opt, value); // Remplace une éventuelle valeur déjà prévue
}

QVariant PackageSource::option(Option opt, const QVariant &defaultValue)
{
    return d->options.value(opt, defaultValue);
}

bool PackageSource::getSource(bool block)
{
    // Récupérer les sources du paquet, donc les télécharger (plus précisément lancer le script source)
    QString cDir = QDir::currentPath();
    QDir::setCurrent(option(SourceDir, cDir).toString());
    
    QByteArray script = d->script("source", "download");
    
    if (script.isNull())
    {
        return false;
    }
    
    if (!d->runScript(script, QStringList(), block))
    {
        return false;
    }
    
    return true;
}

/*****************************************************************************/

void PackageSource::processDataOut()
{
    QProcess *sh = static_cast<QProcess *>(sender());
    
    if (sh == 0)
    {
        return;
    }
    
    // Lire les lignes
    QByteArray line;
    
    while (sh->bytesAvailable() > 0)
    {
        line = sh->readLine().trimmed();
        
        d->buffer += line + '\n';
        
        // Envoyer la progression
        d->ps->sendProgress(PackageSystem::ProcessOut, 0, 1, line);
        
        // NOTE: Après le refactoring, on gèrera ici les communications
    }
}

void PackageSource::processTerminated(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Plus besoin du processus
    QProcess *sh = static_cast<QProcess *>(sender());
    
    if (sh != 0)
    {
        sh->deleteLater();
    }
    
    d->buffer.clear();
    
    // Savoir si tout est ok
    int rs = 0;
    
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = d->commandLine;
        err->more = d->buffer;
        
        d->ps->setLastError(err);
        
        rs = 1;
    }
    
    // Mettre un terme à la progression
    d->ps->endProgress(PackageSystem::ProcessOut, 1);
    
    // Quitter la boucle, si nécessaire
    if (d->loop.isRunning())
    {
        d->loop.exit(rs);
    }
}

/*****************************************************************************/

QByteArray PackageSource::Private::script(const QString &key, const QString &type)
{
    QDomElement package = md->documentElement().firstChildElement(key);
    
    if (package.isNull())
    {
        return QByteArray();
    }
    
    // Lire les éléments <script>, et voir si leur type correspond
    QDomElement script = package.firstChildElement("script");
    
    while (!script.isNull())
    {
        if (script.attribute("type") == type)
        {
            return script.text().toUtf8();
        }
        
        script = script.nextSiblingElement("script");
    }
    
    return QByteArray();
}

bool PackageSource::Private::runScript(const QByteArray &script, const QStringList &args, bool block)
{
    // Charger le script
    QFile scriptapi("/usr/libexec/scriptapi");
    
    if (!scriptapi.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = scriptapi.fileName();
        
        ps->setLastError(err);
        
        return false;
    }
    
    QByteArray header = scriptapi.readAll();
    scriptapi.close();
    
    // Lancer bash, en lui envoyant comme entrée le script ainsi que scriptapi
    QProcess *sh = new QProcess(src);
    
    QStringList arguments;
    arguments << "-s";
    arguments << "--";
    arguments << args;
    
    connect(sh, SIGNAL(readyReadStandardOutput()), 
            src, SLOT(processDataOut()));
    connect(sh, SIGNAL(finished(int, QProcess::ExitStatus)),
            src, SLOT(processTerminated(int, QProcess::ExitStatus)));
    
    commandLine = "/bin/sh " + arguments.join(" ");
    sh->setProcessChannelMode(QProcess::MergedChannels);
    sh->start("/bin/sh", arguments);
    
    if (!sh->waitForStarted())
    {
        delete sh;
        
        PackageError *err = new PackageError;
        err->type = PackageError::ProcessError;
        err->info = commandLine;
        
        ps->setLastError(err);
        
        return false;
    }
    
    // Écrire le script, d'abord /usr/libexec/scriptapi, puis script
    sh->write(header);
    sh->write(script);
    sh->write("\nexit 0\n");
    
    // Si on bloque, attendre
    if (block)
    {
        int rs = loop.exec();
        
        return (rs == 0);
    }
    
    return true;
}