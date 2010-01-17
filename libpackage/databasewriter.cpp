/*
 * databasewriter.cpp
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

#include "databasewriter.h"
#include "databaseformat.h"
#include "packagesystem.h"
#include "package.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTime>

#include <QFile>
#include <QProcess>
#include <QtDebug>

#include <gpgme.h>

#include <iostream>
#include <fstream>

using namespace std;
using namespace Logram;

DatabaseWriter::DatabaseWriter(PackageSystem *_parent)
{
    parent = _parent;
}

bool DatabaseWriter::download(const QString &source, const QString &url, Repository::Type type, bool isTranslations, bool gpgCheck)
{
    // Calculer le nom du fichier
    QString arch = url.section('/', -2, -2);
    QString distro = url.section('/', -3, -3);

    QString fname = QString("%1.%2.%3.%4.%5.xz").arg(source).arg(distro).arg(arch).arg(isTranslations).arg(type);
    QString fileName(parent->varRoot() + "/var/cache/lgrpkg/download/");

    fileName += fname;
    cacheFiles.append(fileName);
    checkFiles.append(gpgCheck);

    // Le télécharger
    ManagedDownload *unused = new ManagedDownload;
    
    if (!parent->download(type, url, fileName, true, unused))
    {
        return false;
    }
    
    // Télécharger également la signature
    if (!parent->download(type, url + ".sig", fileName + ".sig", true, unused))
    {
        return false;
    }
    
    delete unused;
    
    return true;
}

bool DatabaseWriter::verifySign(const QString &signFileName, const QByteArray &sigtext, bool &rs)
{
    // Ouvrir les fichiers de signature
    QByteArray signature;
    QFile fl(signFileName);
    
    rs = false;
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        
        return false;
    }
    
    signature = fl.readAll();
    fl.close();
    
    // Vérifier la signature
    gpgme_ctx_t ctx;
    gpgme_data_t gpgme_text, gpgme_sig;
    gpgme_verify_result_t gpgme_result;
    
    gpgme_new(&ctx);
    gpgme_set_armor(ctx, 0);
    
    if (gpgme_data_new_from_mem(&gpgme_sig, signature.constData(), signature.size(), 0) != GPG_ERR_NO_ERROR)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::SignError;
        err->info = tr("Impossible de créer le tampon pour la signature.");
        
        parent->setLastError(err);
        
        gpgme_release(ctx);
        return false;
    }
    
    if (gpgme_data_new_from_mem(&gpgme_text, sigtext.constData(), sigtext.size(), 0) != GPG_ERR_NO_ERROR)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::SignError;
        err->info = tr("Impossible de créer le tampon pour les données.");
        
        parent->setLastError(err);
        
        gpgme_data_release(gpgme_sig);
        gpgme_data_release(gpgme_text);
        gpgme_release(ctx);
        return false;
    }
    
    if (gpgme_op_verify(ctx, gpgme_sig, gpgme_text, NULL) != GPG_ERR_NO_ERROR)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::SignError;
        err->info = tr("Impossible de vérifier la signature.");
        
        parent->setLastError(err);
        
        gpgme_data_release(gpgme_sig);
        gpgme_data_release(gpgme_text);
        gpgme_release(ctx);
        return false;
    }
    
    gpgme_result = gpgme_op_verify_result(ctx);
    
    
    if (gpgme_result->signatures && gpgme_result->signatures->summary != GPGME_SIGSUM_RED)
    {
        // Signature pas bonne
        rs = true;
    }
    
    QFile::remove(signFileName);
    
    // Nettoyage
    gpgme_data_release(gpgme_sig);
    gpgme_data_release(gpgme_text);
    gpgme_release(ctx);
    
    return true;
}

int DatabaseWriter::stringIndex(const QByteArray &str, int pkg, bool isTr, bool create)
{
    int rs;
    bool found = false;
    _String *mstr;
    
    if (isTr)
    {
        if (translateIndexes.contains(str))
        {
            rs = translateIndexes.value(str);
            found = true;
            mstr = translate.at(rs);
        }
    }
    else
    {
        if (stringsIndexes.contains(str))
        {
            rs = stringsIndexes.value(str);
            found = true;
            mstr = strings.at(rs);
        }
    }

    if (!found)
    {
        // Créer une nouvelle chaîne
        mstr = new _String;

        // Insérer la chaîne
        if (isTr)
        {
            mstr->ptr = transPtr;
            mstr->strpkg = strPackages.count();
            transPtr += str.length() + 1;

            translate.append(mstr);
            translateStrings.append(str);
            rs = translate.count()-1;
            translateIndexes.insert(str, rs);
        }
        else
        {
            mstr->ptr = strPtr;
            mstr->strpkg = strPackages.count();
            strPtr += str.length() + 1;

            strings.append(mstr);
            stringsStrings.append(str);
            rs = strings.count()-1;
            stringsIndexes.insert(str, rs);
        }

        strPackages.append(QList<_StrPackage *>());
    }

    // Gérer les StrPackages
    if (create)
    {
        _StrPackage *sp = new _StrPackage;
        _Package *p = packages.at(pkg);

        sp->version = p->version;
        sp->package = pkg;

        strPackages[mstr->strpkg].append(sp);
    }

    return rs;
}

void DatabaseWriter::setDepends(_Package *pkg, const QByteArray &str, int type)
{
    if (str.isEmpty())
    {
        return;
    }
    
    // Parser la chaîne
    QList<QByteArray> deps = str.split(';');
    QByteArray dep;
    _Depend *depend;
    
    foreach (const QByteArray &_dep, deps)
    {
        dep = _dep.trimmed();

        if (!dep.contains('('))
        {
            // Dépendance non-versionnée
            depend = new _Depend;
            depend->type = type;
            depend->op = DEPEND_OP_NOVERSION;
            depend->pkgname = stringIndex(dep, 0, false, false);
            depend->pkgver = 0;

            // Ajouter la dépendance
            depends[pkg->deps].append(depend);

            // Gérer les dépendances inverses
            if (type == DEPEND_TYPE_DEPEND || type == DEPEND_TYPE_CONFLICT)
            {
                int tp = DEPEND_TYPE_CONFLICT;
                
                if (type == DEPEND_TYPE_DEPEND)
                {
                    tp = DEPEND_TYPE_REVDEP;
                }
                
                revdep(pkg, dep, QByteArray(), DEPEND_OP_NOVERSION, tp);
            }
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
            depend = new _Depend;
            depend->type = type;
            depend->op = op;
            depend->pkgname = stringIndex(name, 0, false, false);
            depend->pkgver = stringIndex(version, 0, false, false);

            depends[pkg->deps].append(depend);

            // Gérer les dépendances inverses
            if (type == DEPEND_TYPE_DEPEND || type == DEPEND_TYPE_CONFLICT)
            {
                int tp = DEPEND_TYPE_CONFLICT;

                if (type == DEPEND_TYPE_DEPEND)
                {
                    tp = DEPEND_TYPE_REVDEP;
                }
                
                revdep(pkg, name, version, op, tp);
            }
        }
    }
}

void DatabaseWriter::revdep(_Package *pkg, const QByteArray &name, const QByteArray &version, int op, int type)
{
    // Explorer tous les paquets connus
    const QList<knownEntry *> &entries = knownPackages.value(name);
    _Depend *depend;
    
    foreach (knownEntry *entry, entries)
    {
        // Si on ne précise pas d'opérateur, ou si la version correspond, ajouter une reverse dependency
        
        if (op == DEPEND_OP_NOVERSION || PackageSystem::matchVersion(entry->version, version, op))
        {
            depend = new _Depend;
            depend->type = type;
            depend->op = DEPEND_OP_EQ;
            depend->pkgname = pkg->name;
            depend->pkgver = pkg->version;

            // Ajouter la revdep au paquet cible
            depends[entry->pkg->deps].append(depend);
        }
    }
}

bool DatabaseWriter::rebuild()
{
    // On utilise 2 passes (d'abord créer les paquets, puis les manipuler)
    int pass;
    int numFile;
    QList<char *> buffers;
    char *buffer;

    strPtr = 0;
    transPtr = 0;

    // Première étape
    int progress = parent->startProgress(Progress::UpdateDatabase, 6);
    
    if (!parent->sendProgress(progress, 0, tr("Lecture des listes")))
    {
        return false;
    }

    // On lit également la liste des fichiers installés
    QString ipackagelist = parent->varRoot() + "/var/cache/lgrpkg/db/installed_packages.list";
    bool hasInstalledPackages = false;
    
    if (QFile::exists(ipackagelist))
    {
        cacheFiles.append(ipackagelist);
        hasInstalledPackages = true;
    }
    
    for (pass=0; pass<2; ++pass)
    {
        numFile = 0;
        for (int cfIndex=0; cfIndex < cacheFiles.count(); ++cfIndex)
        {
            const QString &file = cacheFiles.at(cfIndex);
            numFile++;
            
            QString fname = file;
            QStringList parts;
            QString reponame, distroname, arch, method;
            int strDistro, strRepo;
            bool istr;

            bool isInstalledPackages = (numFile == cacheFiles.count() && hasInstalledPackages);
            
            if (!isInstalledPackages)
            {
                fname.remove(".xz");

                if (pass == 0)
                {
                    // Supprimer le fichier de sortie
                    if (QFile::exists(fname))
                    {
                        QFile::remove(fname);
                    }

                    QString cmd = QString("unxz ") + file;

                    if (QProcess::execute(cmd) != 0)
                    {
                        PackageError *err = new PackageError;
                        err->type = PackageError::ProcessError;
                        err->info = cmd;
                        
                        parent->setLastError(err);
                        return false;
                    }
                }

                // Gérer le fichier
                parts = file.section('/', -1, -1).split('.');
                reponame = parts.at(0);
                distroname = parts.at(1);
                arch = parts.at(2);
                istr = (parts.at(3) == "1");
                method = parts.at(4);
            }
            else
            {
                istr = false;
            }

            // Pas de passe 1 pour translate
            if (pass == 0 && istr) continue;
            
            // Préparation pour les traductions
            if (istr)
            {
                strDistro = stringsIndexes.value(distroname.toAscii());
                strRepo = stringsIndexes.value(reponame.toAscii());
            }

            // Lire toutes les lignes de ce fichier
            ifstream fd;
            
            fd.open(qPrintable(fname), ios::binary);

            if (fd.fail())
            {
                PackageError *err = new PackageError;
                err->type = PackageError::OpenFileError,
                err->info = fname;
                
                parent->setLastError(err);
                return false;
            }

            QByteArray name, long_desc, pkgname, pkgver;
            _Package *pkg = 0;
            int index;
            int linelength;
            bool ignorepackage = false;
            
            int flength, fpos;
            
            // Réserver le buffer mémoire pour le fichier
            fd.seekg(0, ios::end);
            flength = fd.tellg();
            fpos = 0;
            fd.seekg(0, ios::beg);
            
            buffer = new char[flength];
            buffers.append(buffer); //Pour deleter après
            fd.read(buffer, flength);
            
            fd.close();
            
            // Vérifier la signature
            bool signvalid;
            
            if (!isInstalledPackages && pass == 0 && checkFiles.at(cfIndex))
            {
                if (!verifySign(file + ".sig", QByteArray::fromRawData(buffer, flength), signvalid))
                {
                    return false;
                }
                
                if (!signvalid)
                {
                    PackageError *err = new PackageError;
                    err->type = PackageError::SignatureError;
                    err->info = file;
                    
                    //parent->setLastError(err);
                    
                    return false;
                }
            }

            // Lire le fichier
            while (fpos < flength)
            {
                // Lire une ligne
                char *cline = buffer;
                bool containsequal = false;
                int indexofequal;
                int hasquote = 0;   // int car utilisé dans des opérations de pointeurs
                linelength = 0;
                indexofequal = 0;
                
                if (!istr)
                {
                    // Trouver le égal, s'il existe
                    while (fpos < flength && *buffer != '\n')
                    {
                        // Si la ligne contient un égal, le savoir
                        if (!containsequal  && *buffer == '=')
                        {
                            containsequal = true;
                            indexofequal = linelength;
                        }
                        else if (*buffer == '"')
                        {
                            hasquote = 1;
                        }
                            
                        linelength++;
                        buffer++;
                        fpos++;
                    }
                }
                else
                {
                    // Trouver le :, s'il existe
                    while (fpos < flength && *buffer != '\n')
                    {
                        // Si la ligne contient un égal, le savoir
                        if (!containsequal && *buffer == ':')
                        {
                            containsequal = true;
                            indexofequal = linelength;
                        }
                            
                        linelength++;
                        buffer++;
                        fpos++;
                    }
                }
                
                if (fpos < flength)
                {
                    // Sauter le \n
                    buffer++;
                    fpos++;
                }
                
                // Si la ligne est vide, continuer
                if (linelength == 0) continue;

                if (pass == 0)
                {
                    if (cline[0] == '[')
                    {
                        // On commence un paquet
                        pkg = new _Package;
                        pkg->flags = 0;
                        name = QByteArray::fromRawData(cline + 1, linelength - 2); // -2 : sauter le ] et le [
                        
                        // Initialisations
                        pkgname.clear();
                        pkgver.clear();
                        ignorepackage = false;
                        
                        // Lire la ligne suivante
                        continue;
                    }

                    // Si la ligne ne contient pas un égal, on passe à la suivante (marche aussi quand la ligne commence par [)
                    if (!containsequal) continue;
                    if (pkg == 0) continue;

                    // Lire les clefs et les valeurs
                    QByteArray key = QByteArray::fromRawData(cline, indexofequal);
                    QByteArray value = QByteArray::fromRawData(cline + indexofequal + 1 + hasquote, 
                                                               linelength - indexofequal - 1 - hasquote - hasquote);

                    if (!ignorepackage)
                    {
                        if (key == "Name")
                        {
                            pkgname = value;
                        }
                        else if (key == "Version")
                        {
                            pkgver = value;

                            // Vérifier que ce paquet à cette version n'existe pas déjà
                            bool found = false;
                            
                            if (isInstalledPackages)
                            {
                                if (knownPackages.contains(pkgname))
                                {
                                    const QList<knownEntry *> &entries = knownPackages.value(pkgname);

                                    foreach(knownEntry *entry, entries)
                                    {
                                        if (entry->version == value)
                                        {
                                            found = true;
                                            delete pkg;
                                            pkg = entry->pkg;
                                            index = entry->index;
                                            entry->ignore = true;
                                            ignorepackage = true;
                                            
                                            break;
                                        }
                                    }
                                }
                            }
                            
                            // Si le nom est aussi ok, ajouter le couple clef/valeur
                            if (!found)
                            {
                                // Ajouter le paquet aux listes, on peut maintenant
                                pkg->deps = depends.count();

                                depends.append(QList<_Depend *>());

                                // Ajouter le paquet
                                packages.append(pkg);
                                index = packages.count()-1;

                                // Clef utiles
                                if (!isInstalledPackages)
                                {
                                    pkg->repo = stringIndex(reponame.toUtf8(), index, false, false);
                                    pkg->idate = 0;
                                    pkg->iby = 0;
                                    pkg->state = PACKAGE_STATE_NOTINSTALLED;
                                }
                                
                                knownEntry *entry = new knownEntry;
                                knownEntries.append(entry);
                                
                                entry->pkg = pkg;
                                entry->version = value;
                                entry->index = index;
                                entry->ignore = false;
                                
                                knownPackages[pkgname].append(entry);
                            }
                            
                            pkg->version = stringIndex(value, index, false, false);
                            pkg->name = stringIndex(pkgname, index, false, !found);
                        }
                        else if (key == "Source")
                        {
                            pkg->source = stringIndex(value, index, false, false);
                        }
                        else if (key == "Maintainer")
                        {
                            pkg->maintainer = stringIndex(value, index, false, false);
                        }
                        else if (key == "Distribution")
                        {
                            pkg->distribution = stringIndex(value, index, false, false);
                        }
                        else if (key == "Section")
                        {
                            pkg->section = stringIndex(value, index, false, false);
                        }
                        else if (key == "UpstreamUrl")
                        {
                            pkg->uurl = stringIndex(value, index, false, false);
                        }
                        else if (key == "License")
                        {
                            pkg->license = stringIndex(value, index, false, false);
                        }
                        else if (key == "PackageHash")
                        {
                            pkg->pkg_hash = stringIndex(value, index, false, false);
                        }
                        else if (key == "MetadataHash")
                        {
                            pkg->mtd_hash = stringIndex(value, index, false, false);
                        }
                        else if (key == "DownloadSize")
                        {
                            pkg->dsize = value.toInt();
                        }
                        else if (key == "InstallSize")
                        {
                            pkg->isize = value.toInt();
                        }
                        else if (key == "Arch")
                        {
                            pkg->arch = stringIndex(value, index, false, false);
                        }
                        else if (key == "Flags")
                        {
                            pkg->flags = value.toInt();
                        }
                        else if (key == "Provides")
                        {
                            // Insérer des knownPackages pour chaque provide
                            if (value.isEmpty())
                            {
                                continue;
                            }

                            // Parser la chaîne
                            QList<QByteArray> deps = value.split(';');
                            QByteArray dep;
                            QByteArray version(pkgver);
                            

                            foreach (const QByteArray &_dep, deps)
                            {
                                dep = _dep.trimmed();

                                bool pa = dep.contains('(');

                                if (pa)
                                {
                                    // Provide avec version
                                    // Parser la chaîne "nom (= version)"
                                    dep.replace('(', "");           // machin >= version)
                                    dep.replace(')', "");           // machin >= version

                                    // Splitter avec les espaces
                                    QList<QByteArray> parts = dep.split(' ');
                                    dep = parts.at(0);
                                    version = parts.at(2);
                                }

                                // Ajouter <dep>=<version> dans knownPackages
                                knownEntry *entry = new knownEntry;
                                knownEntries.append(entry);
                                
                                entry->pkg = pkg;
                                entry->version = version;
                                entry->index = index;
                                
                                knownPackages[dep].append(entry);
                            }
                        }
                    }

                    if (isInstalledPackages)
                    {
                        if (key == "InstalledDate")
                        {
                            pkg->idate = value.toInt();
                        }
                        else if (key == "InstalledRepo")
                        {
                            pkg->repo = stringIndex(value, index, false, false);
                        }
                        else if (key == "State")
                        {
                            pkg->state = value.toInt();
                        }
                        else if (key == "InstalledBy")
                        {
                            pkg->iby = value.toInt();
                        }
                        else if (key == "ShortDesc")
                        {
                            pkg->short_desc = stringIndex(QByteArray::fromBase64(value), index, true, false);
                        }
                    }
                }
                else if (pass == 1)
                {
                    // Si ce sont les traductions, savoir à quel paquet elles appartiennent
                    if (istr)
                    {
                        // Chaque ligne est de la forme "package:description courte"
                        QByteArray name = QByteArray::fromRawData(cline, indexofequal);
                        QByteArray value = QByteArray::fromRawData(cline + indexofequal + 1, linelength - indexofequal - 1);
                        
                        // Trouver le bon paquet
                        const QList<knownEntry *> &entries = knownPackages.value(name);

                        foreach(knownEntry *entry, entries)
                        {
                            if (entry->pkg->distribution == strDistro && entry->pkg->repo == strRepo)
                            {
                                index = entry->index;
                                
                                // Lui assigner sa description
                                entry->pkg->short_desc = stringIndex(value, index, true);
                                
                                break;
                            }
                        }
                    }
                    else
                    {
                        // On a un format à la QSettings
                        if (cline[0] == '[')
                        {
                            // On commence un paquet
                            name = QByteArray::fromRawData(cline + 1, linelength - 2); // -2 : sauter le ] et le [
                            ignorepackage = false;
                        }

                        // Si la ligne ne contient pas un égal, on passe à la suivante (marche aussi quand la ligne commence par [)
                        if (!containsequal) continue;
                        if (ignorepackage) continue;

                        // Lire les clefs et les valeurs
                        QByteArray key = QByteArray::fromRawData(cline, indexofequal);
                        QByteArray value = QByteArray::fromRawData(cline + indexofequal + 1 + hasquote,
                                                                   linelength - indexofequal - 1 - hasquote - hasquote);

                        if (key == "Version")
                        {
                            // Retrouver le paquet du bon nom et de la bonne version
                            const QList<knownEntry *> &entries = knownPackages.value(name);

                            foreach(knownEntry *entry, entries)
                            {
                                if (entry->version == value)
                                {
                                    pkg = entry->pkg;
                                    index = entry->index;
                                    ignorepackage = (entry->ignore && isInstalledPackages);
                                    
                                    break;
                                }
                            }
                        }
                        if (key == "Depends" && !value.isNull())
                        {
                            setDepends(pkg, value, DEPEND_TYPE_DEPEND);
                        }
                        else if (key == "Replaces" && !value.isNull())
                        {
                            setDepends(pkg, value, DEPEND_TYPE_REPLACE);
                        }
                        else if (key == "Suggest" && !value.isNull())
                        {
                            setDepends(pkg, value, DEPEND_TYPE_SUGGEST);
                        }
                        else if (key == "Conflicts" && !value.isNull())
                        {
                            setDepends(pkg, value, DEPEND_TYPE_CONFLICT);
                        }
                        else if (key == "Provides" && !value.isNull())
                        {
                            setDepends(pkg, value, DEPEND_TYPE_PROVIDE);
                            
                            if (value.isEmpty())
                            {
                                continue;
                            }

                            // Parser la chaîne
                            QList<QByteArray> deps = value.split(';');
                            QByteArray dep;

                            foreach (const QByteArray &_dep, deps)
                            {
                                dep = _dep.trimmed();
                                
                                bool pa = dep.contains('(');
                                int32_t oldver;
                                QByteArray version;

                                if (pa)
                                {
                                    // Provide avec version
                                    oldver = pkg->version;

                                    // Parser la chaîne "nom (= version)"
                                    dep.replace('(', "");           // machin >= version)
                                    dep.replace(')', "");           // machin >= version

                                    // Splitter avec les espaces
                                    QList<QByteArray> parts = dep.split(' ');
                                    dep = parts.at(0);
                                    version = parts.at(2);

                                    pkg->version = stringIndex(version, index, false, false);
                                }
                                
                                // Simplement créer une chaîne
                                stringIndex(dep, index, false, true);

                                if (pa)
                                {
                                    pkg->version = oldver;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Supprimer les fichiers temporaires
    numFile = 0;
    
    foreach (const QString &file, cacheFiles)
    {
        numFile++;

        if (numFile != cacheFiles.count())
        {
            QString fname = file;
            fname.replace(".xz", "");

            QFile::remove(fname);
        }
    }

    //qDebug() << packages;
    //qDebug() << packagesIndexes;
    //qDebug() << strings;
    //qDebug() << translate;
    //qDebug() << stringsIndexes;
    //qDebug() << translateIndexes;
    //qDebug() << strPackages;
    //qDebug() << knownPackages;
    //qDebug() << depends;
    //qDebug() << stringsStrings;

    /*** Écrire les listes dans les fichiers ***/
    int32_t length;
    char zero = 0;
    
    // Liste des paquets
    if (!parent->sendProgress(progress, 1, tr("Génération de la liste des paquets")))
    {
        return false;
    }
    
    QFile fl(parent->varRoot() +  "/var/cache/lgrpkg/db/packages");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }

    length = packages.count();
    fl.write((const char *)&length, sizeof(int32_t));

    foreach (_Package *pkg, packages)
    {
        // Écrire le paquet
        fl.write((const char *)pkg, sizeof(_Package));
        
        delete pkg;
    }

    // Chaînes de caractères
    fl.close();
    if (!parent->sendProgress(progress, 2, tr("Écriture des chaînes de caractère")))
    {
        return false;
    }
    
    fl.setFileName(parent->varRoot() + "/var/cache/lgrpkg/db/strings");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }

    length = strings.count();
    fl.write((const char *)&length, sizeof(int32_t));
    
    foreach (_String *str, strings)
    {
        // Ecrire la chaine
        fl.write((const char *)str, sizeof(_String));
        
        delete str;
    }

    for (int i=0; i<strings.count(); ++i)
    {
        // On écrit maintenant les valeurs
        const QByteArray &str = stringsStrings.at(i);
        fl.write(str.constData(), str.length());
        fl.write(&zero, 1);
    }

    // Chaînes traduites
    fl.close();
    if (!parent->sendProgress(progress, 3, tr("Écriture des traductions")))
    {
        return false;
    }
    
    fl.setFileName(parent->varRoot() + "/var/cache/lgrpkg/db/translate");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }

    length = translate.count();
    fl.write((const char *)&length, sizeof(int32_t));

    foreach (_String *str, translate)
    {
        // Ecrire la chaine
        fl.write((const char *)str, sizeof(_String));
        
        delete str;
    }

    for (int i=0; i<translate.count(); ++i)
    {
        // On écrit maintenant les valeurs
        const QByteArray &str = translateStrings.at(i);
        fl.write(str.constData(), str.length());
        fl.write(&zero, 1);
    }

    // Dépendances
    fl.close();
    if (!parent->sendProgress(progress, 4, tr("Enregistrement des dépendances")))
    {
        return false;
    }
    
    fl.setFileName(parent->varRoot() + "/var/cache/lgrpkg/db/depends");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }

    length = depends.count();
    fl.write((const char *)&length, sizeof(int32_t));

    _DependPtr dp;
    int dptr = 0;

    QList<_Depend *> alldeps;

    foreach (const QList<_Depend *> &l, depends)
    {
        // Ecrire le tableau des dépendances
        dp.count = l.count();
        dp.ptr = dptr;

        fl.write((const char *)&dp, sizeof(_DependPtr));

        // Adapter le pointeur
        dptr += l.count() * sizeof(_Depend);

        // Pour aller plus vite, déjà ajouter cette dépendance
        alldeps << l;
    }

    foreach(_Depend *dep, alldeps)
    {
        fl.write((const char *)dep, sizeof(_Depend));
        
        delete dep;
    }

    // StrPackages
    fl.close();
    if (!parent->sendProgress(progress, 5, tr("Enregistrement des données supplémentaires")))
    {
        return false;
    }
    
    fl.setFileName(parent->varRoot() + "/var/cache/lgrpkg/db/strpackages");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }

    length = strPackages.count();
    fl.write((const char *)&length, sizeof(int32_t));

    _StrPackagePtr spp;
    int spptr = 0;

    QList<_StrPackage *> allsp;

    foreach (const QList<_StrPackage *> &l, strPackages)
    {
        // Ecrire le tableau des dépendances
        spp.count = l.count();
        spp.ptr = spptr;

        fl.write((const char *)&spp, sizeof(_StrPackagePtr));

        // Adapter le pointeur
        spptr += l.count() * sizeof(_StrPackage);

        // Pour aller plus vite, déjà ajouter cette dépendance
        allsp << l;
    }

    foreach(_StrPackage *sp, allsp)
    {
        fl.write((const char *)sp, sizeof(_StrPackage));
        
        delete sp;
    }

    // Fermer le fichier
    fl.close();
    
    // Librérer les buffers
    foreach(char *buf, buffers)
    {
        delete[] buf;
    }
    
    // Nettoyer
    qDeleteAll(knownEntries);

    // On a fini ! :-)
    parent->endProgress(progress);
    
    return true;
}