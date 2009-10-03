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
#include "libpackage.h"
#include "libpackage_p.h"
#include "package.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <QFile>
#include <QProcess>
#include <QtDebug>

DatabaseWriter::DatabaseWriter(PackageSystem *_parent)
{
    parent = _parent;
}

void DatabaseWriter::download(const QString &source, const QString &url, const QString &type, bool isTranslations)
{
    // Calculer le nom du fichier
    QString arch = url.section('/', -2, -2);
    QString distro = url.section('/', -3, -3);

    QString fname = QString("%1.%2.%3.%4.%5.lzma").arg(source).arg(distro).arg(arch).arg(isTranslations).arg(type);
    QString fileName("/var/cache/lgrpkg/download/");

    fileName += fname;
    cacheFiles.append(fileName);

    // Le télécharger
    parent->download(type, url, fileName);
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
            rs = translate.count()-1;
            translateIndexes.insert(str, rs);
        }
        else
        {
            mstr->ptr = strPtr;
            mstr->strpkg = strPackages.count();
            strPtr += str.length() + 1;

            strings.append(mstr);
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
            if (type == DEPEND_TYPE_DEPEND)
            {
                revdep(pkg, dep, QByteArray(), DEPEND_OP_NOVERSION);
            }
        }
        else
        {
            // Retirer les parenthèses      // machin (>= version)
            dep.replace('(', "");           // machin >= version)
            dep.replace(')', "");           // machin >= version

            // Splitter avec les espaces
            QList<QByteArray> parts = dep.split(' ');
            QByteArray name = parts.at(0);
            QByteArray _op = parts.at(1);
            QByteArray version = parts.at(2);

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
            if (type == DEPEND_TYPE_DEPEND)
            {
                revdep(pkg, name, version, op);
            }
        }
    }
}

void DatabaseWriter::revdep(_Package *pkg, const QByteArray &name, const QByteArray &version, int op)
{
    // Explorer tous les paquets connus
    QHash<QByteArray, _Package *>::const_iterator i = knownPackages.constBegin();
    QList<QByteArray> parts;
    _Depend *depend;
    
    while (i != knownPackages.constEnd())
    {
        // Splitter la valeur
        parts = i.key().split('=');

        // Nom dans parts[0], Version dans parts[1]. Vérifier que le nom est bon
        if (parts.at(0) == name)
        {
            // Si on ne précise pas d'opérateur, ou si la version correspond, ajouter une reverse dependency
            QString v1(parts.at(1));
            QString v2(version);
            
            if (op == DEPEND_OP_NOVERSION || PackageSystem::matchVersion(v1, v2, op))
            {
                depend = new _Depend;
                depend->type = DEPEND_TYPE_REVDEP;
                depend->op = DEPEND_OP_EQ;
                depend->pkgname = pkg->name;
                depend->pkgver = pkg->version;

                // Ajouter la revdep au paquet cible
                depends[i.value()->deps].append(depend);
            }
        }

        ++i;
    }
}

void DatabaseWriter::rebuild()
{
    // On utilise 2 passes (d'abord créer les paquets, puis les manipuler)
    int pass;

    strPtr = 0;
    transPtr = 0;

    // Première étape
    parent->sendProgress(PackageSystem::UpdateDatabase, 0, 6, tr("Lecture des listes"));

    for (pass=0; pass<2; ++pass)
    {
        foreach (const QString &file, cacheFiles)
        {
            QString fname = file;
            fname.replace(".lzma", "");
            
            if (pass == 0)
            {
                // Supprimer le fichier de sortie
                if (QFile::exists(fname))
                {
                    QFile::remove(fname);
                }
                
                QString cmd = QString("unlzma ") + file;

                if (QProcess::execute(cmd) != 0)
                {
                    parent->raise(PackageSystem::ProcessError, cmd);
                }
            }

            // Gérer le fichier
            QStringList parts = file.section('/', -1, -1).split('.');
            QString reponame = parts.at(0);
            QString distroname = parts.at(1);
            QString arch = parts.at(2);
            bool istr = (parts.at(3) == "1");
            QString method = parts.at(4);

            // Pas de passe 1 pour translate
            if (pass == 0 && istr) continue;

            // Lire toutes les lignes de ce fichier
            QFile fl(fname);

            if (!fl.open(QIODevice::ReadOnly))
            {
                parent->raise(PackageSystem::OpenFileError, fname);
            }

            QByteArray line, name, long_desc, pkgname, pkgver;
            _Package *pkg = 0;
            int number = 0;
            int index;
            bool waspoint = false;

            // Lire le fichier
            while (!fl.atEnd())
            {
                line = fl.readLine().trimmed();

                if (pass == 0)
                {
                    if (line.startsWith('['))
                    {
                        // On commence un paquet
                        pkg = new _Package;
                        name = line;
                        name.replace('[', "");
                        name.replace(']', "");

                        // Préparer pour la gestion des dépendances
                        pkg->deps = depends.count();

                        depends.append(QList<_Depend *>());

                        // Ajouter le paquet
                        packages.append(pkg);
                        QByteArray key = name + "_" + distroname.toUtf8();
                        index = packages.count()-1;
                        packagesIndexes.insert(key, index);

                        // Clef utiles
                        pkg->distribution = stringIndex(distroname.toUtf8(), index, false, false);
                        pkg->repo = stringIndex(reponame.toUtf8(), index, false, false);

                        // Initialisations
                        pkgname.clear();
                        pkgver.clear();
                    }

                    // Si la ligne ne contient pas un égal, on passe à la suivante (marche aussi quand la ligne commence par [)
                    if (!line.contains('=')) continue;
                    if (pkg == 0) continue;

                    // Lire les clefs et les valeurs
                    QList<QByteArray> parts = line.split('=');
                    QByteArray key = parts.takeAt(0);
                    QByteArray value;

                    for (int i=0; i<parts.count(); ++i)
                    {
                        if (i)
                            value += "=";
                        value += parts.at(i);
                    }

                    if (key == "Name")
                    {
                        pkgname = value;
                    }
                    else if (key == "Version")
                    {
                        pkgver = value;
                        pkg->version = stringIndex(value, index, false, false);
                        pkg->name = stringIndex(pkgname, index, false, true);

                        // Si le nom est aussi ok, ajouter le couple clef/valeur
                        if (!pkgname.isEmpty())
                        {
                            knownPackages.insertMulti(pkgname + "=" + value, pkg);
                        }
                    }
                    else if (key == "Source")
                    {
                        pkg->source = stringIndex(value, index, false, false);
                    }
                    else if (key == "Section")
                    {
                        pkg->section = stringIndex(value, index, false, false);
                    }
                    else if (key == "Licence")
                    {
                        pkg->license = stringIndex(value, index, false, false);
                    }
                    else if (key == "DownloadSize")
                    {
                        pkg->dsize = value.toInt();
                    }
                    else if (key == "InstallSize")
                    {
                        pkg->isize = value.toInt();
                    }
                    else if (key == "Url")
                    {
                        pkg->url = stringIndex(value, index, false, false);
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
                            knownPackages.insertMulti(dep + "=" + version, pkg);
                        }
                    }
                }
                else if (pass == 1)
                {
                    // Si ce sont les traductions, savoir à quel paquet elles appartiennent
                    if (istr)
                    {
                        // Si après un point, on a une ligne vide, alors on a fini un paquet
                        if (line == "" && waspoint)
                        {
                            if (pkg != 0)
                            {
                                // Description longue du paquet
                                pkg->long_desc = stringIndex(long_desc, index, true);

                                // Réinitialisations
                                number = 0;
                                pkg = 0;
                                waspoint = false;
                                long_desc.clear();
                            }

                            continue;
                        }

                        // On a un point. Si on a une ligne vide après, on a fini un paquet
                        waspoint = (line == ".");

                        if (waspoint) continue;
                        
                        if (number == 0)
                        {
                            // Nom du paquet qui recoit
                            name = line;
                            index = packagesIndexes.value(name + "_" + distroname.toUtf8());
                            pkg = packages.at(index);
                        }
                        else if (number == 1)
                        {
                            // Titre
                            pkg->title = stringIndex(line, index, true);
                        }
                        else if (number == 2)
                        {
                            // Description courte
                            pkg->short_desc = stringIndex(line, index, true);
                        }
                        else
                        {
                            // Description longue, ligne par ligne
                            long_desc += line + "\n";
                        }

                        number++;
                    }
                    else
                    {
                        // On a un format à la QSettings
                        if (line.startsWith('['))
                        {
                            // On commence un paquet
                            name = line;
                            name.replace('[', "");
                            name.replace(']', "");
                            index = packagesIndexes.value(name + "_" + distroname.toUtf8());
                            pkg = packages.at(index);
                        }

                        // Si la ligne ne contient pas un égal, on passe à la suivante (marche aussi quand la ligne commence par [)
                        if (!line.contains('=')) continue;
                        if (pkg == 0) continue;

                        // Lire les clefs et les valeurs
                        QList<QByteArray> parts = line.split('=');
                        QByteArray key = parts.takeAt(0);
                        QByteArray value;
                        
                        for (int i=0; i<parts.count(); ++i)
                        {
                            if (i)
                                value += "=";
                            value += parts.at(i);
                        }

                        if (key == "Depends")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_DEPEND);
                        }
                        else if (key == "Replaces")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_REPLACE);
                        }
                        else if (key == "Suggest")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_SUGGEST);
                        }
                        else if (key == "Conflicts")
                        {
                            setDepends(pkg, value, DEPEND_TYPE_CONFLICT);
                        }
                        else if (key == "Provides")
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

            fl.close();
        }
    }

    // Supprimer les fichiers temporaires
    foreach (const QString &file, cacheFiles)
    {
        QString fname = file;
        fname.replace(".lzma", "");

        QFile::remove(fname);
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

    /*** Écrire les listes dans les fichiers ***/
    int32_t length;
    
    // Liste des paquets
    parent->sendProgress(PackageSystem::UpdateDatabase, 1, 6, tr("Génération de la liste des paquets"));
    QFile fl("/var/cache/lgrpkg/db/packages");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        parent->raise(PackageSystem::OpenFileError, fl.fileName());
    }

    length = packages.count();
    fl.write((const char *)&length, sizeof(int32_t));

    foreach (_Package *pkg, packages)
    {
        // Écrire le paquet
        fl.write((const char *)pkg, sizeof(_Package));
    }

    // Chaînes de caractères
    fl.close();
    parent->sendProgress(PackageSystem::UpdateDatabase, 2, 6, tr("Écriture des chaînes de caractère"));
    fl.setFileName("/var/cache/lgrpkg/db/strings");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        parent->raise(PackageSystem::OpenFileError, fl.fileName());
    }

    length = strings.count();
    fl.write((const char *)&length, sizeof(int32_t));

    foreach (_String *str, strings)
    {
        // Ecrire la chaine
        fl.write((const char *)str, sizeof(_String));
    }

    for (int i=0; i<strings.count(); ++i)
    {
        // On écrit maintenant les valeurs
        QByteArray str = stringsIndexes.key(i);
        fl.write(str.constData(), str.length()+1); // +1 : nous avons besoin du \0 à la fin
    }

    // Chaînes traduites
    fl.close();
    parent->sendProgress(PackageSystem::UpdateDatabase, 3, 6, tr("Écriture des traductions"));
    fl.setFileName("/var/cache/lgrpkg/db/translate");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        parent->raise(PackageSystem::OpenFileError, fl.fileName());
    }

    length = translate.count();
    fl.write((const char *)&length, sizeof(int32_t));

    foreach (_String *str, translate)
    {
        // Ecrire la chaine
        fl.write((const char *)str, sizeof(_String));
    }

    for (int i=0; i<translate.count(); ++i)
    {
        // On écrit maintenant les valeurs
        QByteArray str = translateIndexes.key(i);
        fl.write(str.constData(), str.length()+1); // +1 : nous avons besoin du \0 à la fin
    }

    // Dépendances
    fl.close();
    parent->sendProgress(PackageSystem::UpdateDatabase, 4, 6, tr("Enregistrement des dépendances"));
    fl.setFileName("/var/cache/lgrpkg/db/depends");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        parent->raise(PackageSystem::OpenFileError, fl.fileName());
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
    }

    // StrPackages
    fl.close();
    parent->sendProgress(PackageSystem::UpdateDatabase, 5, 6, tr("Enregistrement de données supplémentaires"));
    fl.setFileName("/var/cache/lgrpkg/db/strpackages");

    if (!fl.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        parent->raise(PackageSystem::OpenFileError, fl.fileName());
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
    }

    // Fermer le fichier
    fl.close();

    // On a fini ! :-)
    parent->endProgress(PackageSystem::UpdateDatabase, 6);
}