/*
 * filepackage.cpp
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

#include "filepackage.h"
#include "packagesystem.h"

// Qt
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QLocale>
#include <QDir>

#include <QDebug>

// LibArchive
#include <archive.h>
#include <archive_entry.h>

using namespace Logram;

struct FilePackage::Private
{
    PackageSystem *ps;
    QString fileName;
    bool valid;
    
    bool isGui;
    qint64 size;
    QString name;
    QString version;
    QString maintainer;
    QString shortDesc;
    QString source;
    QString section;
    QString distribution;
    QString license;
    QString arch;
    QString primaryLang;
    
    QStringList fileList;
    QByteArray metadataContents;
    
    QList<Depend *> depends;
    
    void addDeps(const QByteArray &str, int8_t type);
};

struct KeyValue
{
    QByteArray key, value;
};

static QList<KeyValue> keyValues(const char *data, char sep, int size)
{
    // Explorer les ligne
    int pos = 0;
    const char *keystart, *valuestart;
    int keylen, valuelen;
    
    QList<KeyValue> rs;
    
    while (pos < size)
    {
        // Si la ligne commence par '[' ou est vide, continuer
        if (*data == '[')
        {
            while (pos < size && *data != '\n')
            {
                data++;
                pos++;
            }
            
            data++;
            pos++;
            
            continue;
        }
        
        if (*data == '\n')
        {
            data++;
            pos++;
            
            continue;
        }
        
        // Lire la clef
        keystart = data;
        keylen = 0;
        
        while (pos < size && *data != sep)
        {
            data++;
            pos++;
            keylen++;
        }
        
        // Sauter le égal
        data++;
        pos++;
        
        if (pos >= size)
        {
            break;
        }
        
        // Lire la valeur
        valuestart = data;
        valuelen = 0;
        
        while (pos < size && *data != '\n')
        {
            data++;
            pos++;
            valuelen++;
        }
        
        // Interpréter la clef et la valeur
        KeyValue kv;
        kv.key = QByteArray(keystart, keylen);
        kv.value = QByteArray(valuestart, valuelen);
        
        rs.append(kv);
    }
    
    return rs;
}

FilePackage::FilePackage(const QString &fileName, PackageSystem *ps, DatabaseReader *psd, Solver::Action _action)
    : Package(ps, psd, _action)
{
    d = new Private;
    d->ps = ps;
    if (QDir::isAbsolutePath(fileName))
    {
        d->fileName = fileName;
    }
    else
    {
        d->fileName = QDir::currentPath() + "/" + fileName;
    }
    d->valid = true;
    
    // Architecture et taille
    d->arch = fileName.section('.', -2, -2);
    QFileInfo fi(fileName);
    d->size = fi.size();
    
    QString name = fileName.section('~', 0, 0);
    
    // Lire l'archive .tar.tlz qu'est un paquet, et récupérer les métadonnées et le fichier .control
    struct archive *a;
    struct archive_entry *entry;
    int r;
    
    a = archive_read_new();
    archive_read_support_compression_lzma(a);
    archive_read_support_compression_xz(a);
    archive_read_support_format_all(a);
    
    r = archive_read_open_filename(a, qPrintable(fileName), 10240);
    
    if (r != ARCHIVE_OK)
    {
        d->valid = false;
        return;
    }
    
    // Lire les entrées
    QByteArray path;
    int size;
    char *buffer;
    
    QString lang = QLocale::system().name().section('_', 0, 0);
    
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        path = QByteArray(archive_entry_pathname(entry));
        
        if (!path.startsWith("__LOGRAM"))
        {
            d->fileList.append(path);
        }
        
        // Savoir quel type de fichier on a lu
        if (path == "__LOGRAM/metadata.xml")
        {
            // Lire le fichier
            size = archive_entry_size(entry);
            buffer = new char[size];
            archive_read_data(a, buffer, size);
            
            d->metadataContents = QByteArray(buffer, size);
            
            delete[] buffer;
        }
        else if (path == "__LOGRAM/control")
        {
            // Lire le fichier
            size = archive_entry_size(entry);
            buffer = new char[size];
            archive_read_data(a, buffer, size);
            
            QList<KeyValue> kvs = keyValues(buffer, '=', size);
            
            foreach (const KeyValue &kv, kvs)
            {
                if (kv.key == "Name")
                {
                    d->name = kv.value;
                }
                else if (kv.key == "Version")
                {
                    d->version = kv.value;
                }
                else if (kv.key == "Source")
                {
                    d->source = kv.value;
                }
                else if (kv.key == "Maintainer")
                {
                    d->maintainer = kv.value;
                }
                else if (kv.key == "Section")
                {
                    d->section = kv.value;
                }
                else if (kv.key == "Distribution")
                {
                    d->distribution = kv.value;
                }
                else if (kv.key == "License")
                {
                    d->license = kv.value;
                }
                else if (kv.key == "PrimaryLang")
                {
                    d->primaryLang = kv.value;
                }
                else if (kv.key == "Gui")
                {
                    d->isGui = (kv.value == "true");
                }
                else if (kv.key == "Depends")
                {
                    d->addDeps(kv.value, DEPEND_TYPE_DEPEND);
                }
                else if (kv.key == "Provides")
                {
                    d->addDeps(kv.value, DEPEND_TYPE_PROVIDE);
                }
                else if (kv.key == "Replaces")
                {
                    d->addDeps(kv.value, DEPEND_TYPE_REPLACE);
                }
                else if (kv.key == "Suggest")
                {
                    d->addDeps(kv.value, DEPEND_TYPE_SUGGEST);
                }
                else if (kv.key == "Conflicts")
                {
                    d->addDeps(kv.value, DEPEND_TYPE_CONFLICT);
                }
            }
            
            delete[] buffer;
        }
        else if (path.startsWith("__LOGRAM/descriptions."))
        {
            // Trouver l'extension de langue
            QList<QByteArray> parts = path.split('.');
            QByteArray loc = parts.at(parts.count()-1);
            
            // Lire le fichier
            size = archive_entry_size(entry);
            buffer = new char[size];
            archive_read_data(a, buffer, size);
            
            QList<KeyValue> kvs = keyValues(buffer, ':', size);
            
            foreach (const KeyValue &kv, kvs)
            {
                if (kv.key == name)
                {
                    // Bon paquet
                    d->shortDesc = kv.value;
                    
                    // Si on a la langue préférée de l'utilisateur, alors on peut partir
                    if (loc == lang)
                    {
                        break;
                    }
                }
            }
            
            delete[] buffer;
        }
    }
    
    r = archive_read_finish(a);
    
    if (r != ARCHIVE_OK)
    {
        d->valid = false;
    }
}

FilePackage::FilePackage(const FilePackage &other) : Package(other)
{
    d = new Private;
    
    d->ps = other.d->ps;
    d->fileName = other.d->fileName;
    d->valid = other.d->valid;
    
    d->isGui = other.d->isGui;
    d->size = other.d->size;
    d->name = other.d->name;
    d->version = other.d->version;
    d->maintainer = other.d->maintainer;
    d->shortDesc = other.d->shortDesc;
    d->source = other.d->source;
    d->section = other.d->section;
    d->distribution = other.d->distribution;
    d->license = other.d->license;
    d->arch = other.d->arch;
    d->primaryLang = other.d->primaryLang;
    
    d->fileList = other.d->fileList;
    d->metadataContents = other.d->metadataContents;
    
    foreach(Depend *dep, other.d->depends)
    {
        d->depends.append(new FileDepend(dep->type(), dep->op(), dep->name(), dep->version()));
    }
}

FilePackage::~FilePackage()
{
    qDeleteAll(d->depends);
    
    delete d;
}

void FilePackage::Private::addDeps(const QByteArray &str, int8_t type)
{
    if (str.isEmpty())
    {
        return;
    }
    
    // Parser la chaîne
    QList<QByteArray> deps = str.split(';');
    QByteArray dep;
    Depend *depend;
    
    foreach (const QByteArray &_dep, deps)
    {
        dep = _dep.trimmed();

        if (!dep.contains('('))
        {
            // Dépendance non-versionnée
            depend = new FileDepend(type, DEPEND_OP_NOVERSION, dep, QString());

            // Ajouter la dépendance
            depends.append(depend);
        }
        else
        {
            // Retirer les parenthèses      // machin (>= version)
            dep.replace('(', "");           // machin >= version)
            dep.replace(')', "");           // machin >= version

            // Splitter avec les espaces
            QList<QByteArray> parts = dep.split(' ');
            
            if (parts.count() != 3) return;
            
            const QByteArray &name = parts.at(0);
            const QByteArray &_op = parts.at(1);
            const QByteArray &version = parts.at(2);

            // Trouver le bon opérateur
            int8_t op = 0;

            if (_op == "=")
            {
                op = DEPEND_OP_EQ;
            }
            else if (_op == ">")
            {
                op = DEPEND_OP_GR;
            }
            else if (_op == ">=")
            {
                op = DEPEND_OP_GREQ;
            }
            else if (_op == "<")
            {
                op = DEPEND_OP_LO;
            }
            else if (_op == "<=")
            {
                op = DEPEND_OP_LOEQ;
            }
            else if (_op == "!=")
            {
                op = DEPEND_OP_NE;
            }

            // Créer le depend
            depend = new FileDepend(type, op, name, version);

            depends.append(depend);
        }
    }
}

bool FilePackage::download()
{
    // Pas besoin de télécharger
    emit downloaded(true);
    
    return true;
}

QString FilePackage::tlzFileName()
{
    return d->fileName;
}

bool FilePackage::isValid()
{
    return d->valid;
}

Package::Origin FilePackage::origin()
{
    return Package::File;
}

QStringList FilePackage::files()
{
    return d->fileList;
}

QString FilePackage::name()
{
    return d->name;
}

QString FilePackage::version()
{
    return d->version;
}

QString FilePackage::maintainer()
{
    return d->maintainer;
}

QString FilePackage::shortDesc()
{
    return d->shortDesc;
}

QString FilePackage::source()
{
    return d->source;
}

QString FilePackage::repo()
{
    return "local";
}

QString FilePackage::section()
{
    return d->section;
}

QString FilePackage::distribution()
{
    return d->distribution;
}

QString FilePackage::license()
{
    return d->license;
}

QString FilePackage::arch()
{
    return d->arch;
}

QByteArray FilePackage::packageHash()
{
    return QByteArray();
}

QByteArray FilePackage::metadataHash()
{
    return QByteArray();
}

bool FilePackage::isGui()
{
    return d->isGui;
}

QDateTime FilePackage::installedDate()
{
    return QDateTime();
}

int FilePackage::installedBy()
{
    return 0;
}

int FilePackage::status()
{
    return PACKAGE_STATE_NOTINSTALLED;
}

int FilePackage::downloadSize()
{
    return 0;
}

int FilePackage::installSize()
{
    return d->size;
}

QList<Depend *> FilePackage::depends()
{
    return d->depends;
}

void FilePackage::registerState(int idate, int iby, int state)
{
    // TODO: Pas possible d'ajouter un élément dans la BDD. Peut-être déclencher un update()
    (void) idate;
    (void) iby;
    (void) state;
}

QByteArray FilePackage::metadataContents()
{
    return d->metadataContents;
}

/* Dépendances */

struct FileDepend::Private
{
    int8_t type, op;
    QString name, version;
};

FileDepend::FileDepend(int8_t type, int8_t op, const QString &name, const QString &version)
{
    d = new Private;
    
    d->type = type;
    d->op = op;
    d->name = name;
    d->version = version;
}

FileDepend::~FileDepend()
{
    delete d;
}

QString FileDepend::name()
{
    return d->name;
}

QString FileDepend::version()
{
    return d->version;
}

int8_t FileDepend::type()
{
    return d->type;
}

int8_t FileDepend::op()
{
    return d->op;
}