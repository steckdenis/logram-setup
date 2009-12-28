/*
 * packagemetadata.h
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

#ifndef __PACKAGEMETADATA_H__
#define __PACKAGEMETADATA_H__

#include <QtXml/QDomDocument>

#include <QObject>
#include <QDateTime>
#include <QProcess>

namespace Logram
{

class Package;
class PackageSystem;
class Templatable;
class Communication;

struct ChangeLogEntry
{
    QString version;
    QString author, email;
    QString distribution;
    QDateTime date;
    QString text;
};

class PackageMetaData : public QObject, public QDomDocument
{
    Q_OBJECT
    
    public:
        PackageMetaData(PackageSystem *ps);
        PackageMetaData(PackageSystem *ps, QObject *parent);
        ~PackageMetaData();
        bool error() const;
        
        void loadFile(const QString &fileName, const QByteArray &sha1hash, bool decompress);
        void loadData(const QByteArray &data);
        void bindPackage(Package *pkg);
        void setPackage(Package *pkg);
        
        void setTemplatable(Templatable *tpl);
        
        
        QString primaryLang() const;
        QString packageDescription() const;
        QString packageTitle() const;
        QString currentPackage() const;
        QString upstreamUrl() const;
        
        QList<ChangeLogEntry *> changelog() const;
        
        void setCurrentPackage(const QString &name);
        QDomElement currentPackageElement() const;
        
        static QString stringOfKey(const QDomElement &element, const QString &primaryLang);
        QString stringOfKey(const QDomElement &element) const;
        
        QByteArray script(const QString &key, const QString &type);
        bool runScript(const QByteArray &script, const QStringList &args);
        bool runScript(const QString &key, const QString &type, const QString &currentDir, const QStringList &args);
        QByteArray scriptOut() const;
        
    private slots:
        void processDataOut();
        void processTerminated(int exitCode, QProcess::ExitStatus exitStatus);
        
    signals:
        void processLineOut(QProcess *process, const QByteArray &line);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif