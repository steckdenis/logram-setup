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

struct FileFile
{
    int index;          // index dans knownFiles
    
    FileFile *parent;   // Dossier parent (un dossier est un fichier)
    int package_index;  // Index du parent, inutilisé si dossier
    int name_index;     // Index du nom
    int flags;          // Flags (voir PACKAGE_FILE_FLAGS)
    
    FileFile *next;         // Prochain fichier du dossier
    FileFile *package_next; // Prochain fichier du paquet
    
    FileFile *first_child;  // Premier enfant
};

DatabaseWriter::DatabaseWriter(PackageSystem *_parent)
{
    parent = _parent;
}

bool DatabaseWriter::download(const QString &source, const QString &url, Repository::Type type, FileDataType datatype, bool gpgCheck)
{
    // Calculer le nom du fichier
    QString arch = url.section('/', -2, -2);
    QString distro = url.section('/', -3, -3);

    QString fname = QString("%1.%2.%3.%4.%5.xz").arg(source).arg(distro).arg(arch).arg(datatype).arg(type);
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
    if (gpgCheck)
    {
        if (!parent->download(type, url + ".sig", fileName + ".sig", true, unused))
        {
            return false;
        }
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

int DatabaseWriter::fileStringIndex(const QByteArray &str)
{
    int rs = fileStringsPtrs.value(str, -1);
    
    if (rs != -1)
    {
        return rs;
    }
    
    // Pas trouvé, ajouter
    rs = fileStrPtr;
    fileStringsPtrs.insert(str, rs);
    fileStrings.append(str);
    
    fileStrPtr += str.length() + 1;
    
    return rs;
}

int DatabaseWriter::stringIndex(const QByteArray &str, int pkg, bool isTr, bool create)
{
    int rs;
    bool found = false;
    _String *mstr;
    
    if (isTr)
    {
        // TODO: Optimisation (voir fileStringIndex)
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
        
        QByteArray name, version;
        int op = parent->parseVersion(dep, name, version);

        if (op == DEPEND_OP_NOVERSION)
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
            // Créer le depend
            depend = new _Depend;
            depend->type = type;
            depend->op = op;
            depend->pkgname = stringIndex(name, 0, false, false);
            depend->pkgver = stringIndex(version, 0, false, false);

            depends[pkg->deps].append(depend);

            // Gérer les dépendances inverses
            if (type == DEPEND_TYPE_DEPEND || type == DEPEND_TYPE_CONFLICT || DEPEND_TYPE_REPLACE)
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
    FileFile *currentDir = 0, *firstFile = 0;
    int pass;
    QList<char *> buffers;
    char *buffer;

    strPtr = 0;
    transPtr = 0;
    fileStrPtr = 0;

    // Première étape
    int progress = parent->startProgress(Progress::UpdateDatabase, 7);
    
    if (!parent->sendProgress(progress, 0, tr("Lecture des listes")))
    {
        return false;
    }

    // On lit également la liste des fichiers installés
    QString ipackagelist = parent->varRoot() + "/var/cache/lgrpkg/db/installed_packages.list";
    int installedPackagesListIndex = -1;
    
    if (QFile::exists(ipackagelist))
    {
        installedPackagesListIndex = cacheFiles.count();
        cacheFiles.append(ipackagelist);
    }
    
    for (pass=0; pass<2; ++pass)
    {
        for (int cfIndex=0; cfIndex < cacheFiles.count(); ++cfIndex)
        {
            const QString &file = cacheFiles.at(cfIndex);
            
            QString fname = file;
            QStringList parts;
            QString reponame, distroname, arch, method;
            int strDistro, strRepo;
            FileDataType datatype = PackagesList;

            bool isInstalledPackages = (cfIndex == installedPackagesListIndex);
            
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
                datatype = (FileDataType)parts.at(3).toInt();
                method = parts.at(4);
            }

            // Pas de passe 1 pour translate
            if (pass == 0 && datatype != PackagesList) continue;
            
            // Préparation pour les traductions
            if (datatype != PackagesList)
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
                    /*PackageError *err = new PackageError;
                    err->type = PackageError::SignatureError;
                    err->info = file;
                    
                    parent->setLastError(err);
                    */
                    
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
                
                if (datatype == PackagesList)
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
                else if (datatype == Translations)
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
                else if (datatype == FilesList)
                {
                    // Avancer jusqu'à la fin de la ligne, on gère tout plus tard
                    while (fpos < flength && *buffer != '\n')
                    {
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
                        pkg->used = 0;
                        pkg->first_file = 0;
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
                        else if (key == "Provides" || key == "Replaces")
                        {
                            // Insérer des knownPackages pour chaque provide
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
        
                                QByteArray name, version;
                                parent->parseVersion(dep, name, version);
                                
                                if (version.isNull())
                                {
                                    version = pkgver;
                                }

                                // Ajouter <dep>=<version> dans knownPackages
                                knownEntry *entry = new knownEntry;
                                knownEntries.append(entry);
                                
                                entry->pkg = pkg;
                                entry->version = version;
                                entry->index = index;
                                
                                knownPackages[name].append(entry);
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
                        else if (key == "InstalledBy")
                        {
                            pkg->iby = value.toInt();
                        }
                        else if (key == "ShortDesc")
                        {
                            pkg->short_desc = stringIndex(QByteArray::fromBase64(value), index, true, false);
                        }
                        else if (key == "Flags")
                        {
                            pkg->flags = value.toInt();
                        }
                        else if (key == "Used")
                        {
                            pkg->used = value.toInt();
                        }
                    }
                }
                else if (pass == 1)
                {
                    // Si ce sont les traductions, savoir à quel paquet elles appartiennent
                    if (datatype == Translations)
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
                    else if (datatype == FilesList)
                    {
                        /*
                        FileFile *currentDir = 0, *firstFile = 0;
                        
                        struct FileFile
                        {
                            int index;          // index dans knownFiles

                            FileFile *parent;   // Dossier parent (un dossier est un fichier)
                            int package_index;  // Index du parent, inutilisé si dossier
                            int name_index;     // Index du nom
                            int flags;          // Flags (voir PACKAGE_FILE_FLAGS)
                            
                            FileFile *next;         // Prochain fichier du dossier
                            FileFile *package_next; // Prochain fichier du paquet
                            
                            FileFile *first_child;  // Premier enfant
                        };
                        */
                        
                        if (cline[0] == ':')
                        {
                            if (cline[1] == ':')
                            {
                                // On remonte d'un dossier
                                currentDir = currentDir->parent;
                            }
                            else
                            {
                                // On entre dans un dossier, cline[1:-] contient son nom
                                // Explorer les enfants de currentDir à la recherche d'un qui
                                // a le bon nom (bon index). Si pas trouvé, en ajouter un.
                                cline++;
                                int name_index = fileStringIndex(QByteArray::fromRawData(cline, linelength-1));
                                
                                FileFile *file = 0;
                                
                                if (currentDir)
                                {
                                    file = currentDir->first_child;
                                }
                                
                                while (file != 0 && 
                                    (file->name_index != name_index || file->flags != PACKAGE_FILE_DIR)
                                    ) // NOTE: != et pas !( & ), car un dossier n'a que ça comme flags
                                {
                                    file = file->next;
                                }
                                
                                if (file != 0)
                                {
                                    // On a un dossier du bon nom
                                    // L'utiliser comme dossier courant
                                    currentDir = file;
                                }
                                else
                                {
                                    // file == 0, on n'a rien trouvé, créer un dossier
                                    // du bon nom
                                    file = new FileFile;
                                    
                                    file->index = knownFiles.count();
                                    file->parent = currentDir;
                                    file->package_index = 0;
                                    file->name_index = name_index;
                                    file->flags = PACKAGE_FILE_DIR;
                                    file->package_next = 0;
                                    file->first_child = 0;
                                    
                                    // L'insérer en premier, son suivant est l'actuel premier
                                    if (currentDir)
                                    {
                                        file->next = currentDir->first_child;
                                        currentDir->first_child = file;
                                    }
                                    else
                                    {
                                        file->next = firstFile;
                                        firstFile = file;
                                    }
                                    
                                    // Enregistrer dans knownFiles
                                    knownFiles.append(file);
                                    
                                    // Le passer comme dossier courant
                                    currentDir = file;
                                }
                            }
                        }
                        else
                        {
                            // On a un fichier.
                            // D'abord, récupérer des informations :
                            // package_name|flags|file_name.
                            // cline pointe sur le début de cette ligne
                            const char *pkname = cline;
                            int length = 0, flags = 0;
                            
                            // Avancer jusqu'au |
                            while (*cline != '|')
                            {
                                cline++;
                                length++;
                                linelength--;
                            }
                            
                            pkgname = QByteArray::fromRawData(pkname, length);
                            
                            // Les flags sont 4 octets, simplement sauter le |
                            cline++;
                            flags = *(int *)cline;
                            cline += 4;
                            
                            // Tout le reste est le nom, pas de | à sauter
                            linelength -= (1 + sizeof(int)); // Garder la bonne taille
                            
                            name = QByteArray::fromRawData(cline, linelength);
                            
                            // Obtenir une position de chaîne pour le nom
                            int name_index = fileStringIndex(name);
                            
                            // Trouver le paquet du bon nom dans le bon dépôt
                            const QList<knownEntry *> &entries = knownPackages.value(pkgname);

                            index = -1;
                            foreach(knownEntry *entry, entries)
                            {
                                if (entry->pkg->distribution == strDistro && entry->pkg->repo == strRepo)
                                {
                                    index = entry->index;
                                    break;
                                }
                            }
                            
                            if (index == -1)
                            {
                                // Oh ? Un fichier avec un mauvais paquet ?
                                qDebug() << "Index = -1 found for file" << name << ", package" << pkgname;
                                continue;
                            }
                            
                            // Explorer les fichiers de currentDir, pour voir si un fichier
                            // du même nom et même paquet existe déjà. Si c'est le cas, 
                            // remplacer ses flags (en effet, les flags utilisateurs sont
                            // lus après ceux de l'empaqueteur, et ont la priorité dessus).
                            FileFile *file = new FileFile;
                            
                            file->index = knownFiles.count();
                            file->parent = currentDir;
                            file->package_index = index;
                            file->name_index = name_index;
                            file->flags = flags;
                            file->first_child = 0;
                            file->package_next = 0;
                            
                            // Ajouter à la liste des fichiers connus
                            knownFiles.append(file);
                            
                            // Le relier à l'arbre des fichiers
                            if (currentDir)
                            {
                                file->next = currentDir->first_child;
                                currentDir->first_child = file;
                            }
                            else
                            {
                                file->next = firstFile;
                                firstFile = file;
                            }
                            
                            // Le relier à la liste des fichiers de son paquet
                            _Package *pkg = packages.at(index);
                            
                            if (pkg->first_file != 0)
                            {
                                file->package_next = knownFiles.at(pkg->first_file);
                            }
                            
                            pkg->first_file = file->index;
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
                        
                        if (value.isNull()) continue;

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
                        if (key == "Depends")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_DEPEND);
                        }
                        else if (key == "Suggest")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_SUGGEST);
                        }
                        else if (key == "Conflicts")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_CONFLICT);
                        }
                        else if (key == "Provides" || key == "Replaces")
                        {
                            // Quand on remplace un paquet, on le fournit
                            setDepends(pkg, value, DEPEND_TYPE_PROVIDE);
                            
                            if (key == "Replaces")
                            {
                                // Pour info et pour le solveur
                                setDepends(pkg, value, DEPEND_TYPE_REPLACE);
                            }

                            // Parser la chaîne
                            QList<QByteArray> deps = value.split(';');
                            QByteArray dep;
                            
                            foreach (const QByteArray &_dep, deps)
                            {
                                dep = _dep.trimmed();
        
                                int32_t oldver;
                                
                                QByteArray name, version;
                                parent->parseVersion(dep, name, version);
                                
                                if (!version.isNull())
                                {
                                    oldver = pkg->version;
                                    pkg->version = stringIndex(version, index, false, false);
                                }
                                
                                // Simplement créer une chaîne
                                stringIndex(name, index, false, true);
                                
                                if (!version.isNull())
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
    for (int cfIndex=0; cfIndex < cacheFiles.count(); ++cfIndex)
    {
        const QString &file = cacheFiles.at(cfIndex);

        if (cfIndex != installedPackagesListIndex /* TODO: && != filesListIndex */)
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
    
    // Liste des fichiers
    fl.close();
    if (!parent->sendProgress(progress, 2, tr("Enregistrement de la liste des fichiers")))
    {
        return false;
    }
    
    fl.setFileName(parent->varRoot() + "/var/cache/lgrpkg/db/files");
    
    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        parent->setLastError(err);
        return false;
    }
    
    length = knownFiles.count(); // Nombre de fichiers dans la base
    fl.write((const char *)&length, sizeof(int32_t));
    length = firstFile->index;   // Index du premier enfant du dossier racine
    fl.write((const char *)&length, sizeof(int32_t));
    
    _File file;
    
    foreach (FileFile *mfile, knownFiles)
    {
        file.package = mfile->package_index;
        file.flags = mfile->flags;
        file.name_ptr = mfile->name_index;
        file.parent_dir = -1;
        file.next_file_dir = -1;
        file.next_file_pkg = -1;
        file.itime = 0;
        
        if (mfile->next)
        {
            file.next_file_dir = mfile->next->index;
        }
        
        if (mfile->package_next)
        {
            file.next_file_pkg = mfile->package_next->index;
        }
        
        if (mfile->parent)
        {
            file.parent_dir = mfile->parent->index;
        }
        
        // Écriture
        fl.write((const char *)&file, sizeof(_File));
    }
    
    for (int i=0; i<fileStrings.count(); ++i)
    {
        const QByteArray &str = fileStrings.at(i);
        fl.write(str.constData(), str.length());
        fl.write(&zero, 1);
    }

    // Chaînes de caractères
    fl.close();
    if (!parent->sendProgress(progress, 3, tr("Écriture des chaînes de caractère")))
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
    if (!parent->sendProgress(progress, 4, tr("Écriture des traductions")))
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
    if (!parent->sendProgress(progress, 5, tr("Enregistrement des dépendances")))
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
    if (!parent->sendProgress(progress, 6, tr("Enregistrement des données supplémentaires")))
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