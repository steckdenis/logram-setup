/*
 * buildpkg.cpp
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

#include "buildpkg.h"
#include "libpackage.h"

#include <QDomDocument>
#include <QDomElement>

#include <QFile>
#include <QProcess>

struct BuildPackage::Private
{
    QDomDocument doc;
    QString fileName;
    PackageSystem *ps;
};

BuildPackage::BuildPackage(const QString &fileName, PackageSystem *ps)
{
    QFile fl(fileName);

    d = new Private;

    d->doc.setContent(&fl);
    d->ps = ps;
    d->fileName = fileName;
}

void BuildPackage::getSource()
{
    QList<Source *> srcs = sources();

    // Télécharger les sources
    foreach (Source *src, srcs)
    {
        downloadSource(src);
    }

    qDeleteAll(srcs);
}

void BuildPackage::downloadSource(Source *src)
{
    if (src->method == Source::Git)
    {
        // Parser les options (dossier de destination)
        QString destdir;

        if (src->options.contains("destdir="))
        {
            destdir = src->options.section("destdir=", 1, 1).section(';', 0, 0);
        }

        QString command = "git clone --depth 1 " + src->url + " src/" + destdir;

        d->ps->sendMessage(tr("Clonage du dépôt git <%1> dans <%2>").arg(src->url).arg("src/" + destdir));

        // Lancer la commande
        if (QProcess::execute(command) != 0)
        {
            d->ps->raise(PackageSystem::ProcessError, command);
        }
    }

    // TODO: Le reste
}

void BuildPackage::preparePatch()
{
    if (!QFile::exists("src"))
    {
        // On n'a pas encore récupérer les sources
        getSource();
    }

    // Copier le src en src.orig, pour créer le diff.gz
    QProcess::execute("cp -r src src.orig");
}

void BuildPackage::genPatch()
{
    QString pkgname = d->doc.documentElement().attribute("name");

    QFile out(pkgname + ".diff.lzma");

    d->ps->sendMessage(tr("Patch écrit et compressé dans <%1>").arg(out.fileName()));

    if (!out.open(QIODevice::WriteOnly)) return;

    QProcess diff;
    QProcess lzma;

    diff.start("diff -ur src.orig src");

    if (!diff.waitForFinished())
    {
        d->ps->raise(PackageSystem::ProcessError, "diff -ur src.orig src");
        return;
    }

    lzma.start("lzma");

    if (!lzma.waitForStarted())
    {
        d->ps->raise(PackageSystem::ProcessError, "lzma");
        return;
    }

    lzma.write(diff.readAll());
    lzma.closeWriteChannel();

    if (!lzma.waitForFinished())
    {
        d->ps->raise(PackageSystem::ProcessError, "lzma");
        return;
    }

    out.write(lzma.readAll());
    out.close();
}

void BuildPackage::createSource()
{
    // Créer la liste des fichiers et dossiers qui doivent être empaquetés
    QString filenames = d->fileName;
    QString pkgname = d->doc.documentElement().attribute("name");
    QString diffname = pkgname + ".diff.lzma";
    
    // Si on a un patch, l'empaqueter
    if (QFile::exists(diffname))
    {
        d->ps->sendMessage(tr("Ajout du fichier de patch <%1>").arg(diffname));

        filenames += " " + diffname;
    }

    // Explorer les sources de type Embed
    QList<Source *> srcs = sources();

    foreach(Source *src, srcs)
    {
        if (src->method == Source::Embed)
        {
            d->ps->sendMessage(tr("Ajout du fichier inclus <%1>").arg(src->url));

            filenames += " " + src->url;
        }
    }

    // Empaqueter le tout dans un tar, puis compresser en LZMA
    QString command = "tar --lzma -cf " + pkgname + ".src.tlz " + filenames;

    if (QProcess::execute(command) != 0)
    {
        d->ps->raise(PackageSystem::ProcessError, command);
    }

    d->ps->sendMessage(tr("Le paquet a été créé dans <%1>, il peut être envoyé sur le serveur").arg(pkgname + ".src.tlz"));
}

void BuildPackage::unpackSource(const QString &fileName)
{
    // Juste décompresser
    QProcess::execute("tar --lzma -xf " + fileName);
}

void BuildPackage::patchSource()
{
    QString pkgname = d->doc.documentElement().attribute("name");

    // lzma -dc <pkgname>.diff.lzma | patch -p0
    QProcess lzma;
    QProcess patch;

    lzma.start("lzma -dc " + pkgname + ".diff.lzma");

    if (!lzma.waitForFinished())
    {
        d->ps->raise(PackageSystem::ProcessError, "lzma -dc " + pkgname + ".diff.lzma");
        return;
    }

    patch.start("patch -p0");

    if (!patch.waitForStarted())
    {
        d->ps->raise(PackageSystem::ProcessError, "patch -p0");
        return;
    }

    patch.write(lzma.readAll());
    patch.closeWriteChannel();

    if (!patch.waitForFinished())
    {
        d->ps->raise(PackageSystem::ProcessError, "patch -p0");
        return;
    }
}

QList<Source *> BuildPackage::sources()
{
    QList<Source *> rs;
    Source *src;
    QDomElement source = d->doc.documentElement().firstChildElement("download");
    
    while (!source.isNull())
    {
        src = new Source;
        src->url = source.attribute("url");

        // Type
        if (source.attribute("type", "main") == "main")
        {
            src->type = Source::Main;
        }
        else if (source.attribute("type") == "archive")
        {
            src->type = Source::Archive;
        }
        else if (source.attribute("type") == "plain")
        {
            src->type = Source::Plain;
        }
        else if (source.attribute("type") == "patch")
        {
            src->type = Source::Patch;
        }

        // Méthode
        if (source.attribute("method", "download") == "download")
        {
            src->method = Source::Download;
        }
        else if (source.attribute("method") == "embed")
        {
            src->method = Source::Embed;
        }
        else if (source.attribute("method") == "git")
        {
            src->method = Source::Git;
        }
        else if (source.attribute("method") == "svn")
        {
            src->method = Source::Svn;
        }
        else if (source.attribute("method") == "mercurial")
        {
            src->method = Source::Mercurial;
        }
        else if (source.attribute("method") == "bazaar")
        {
            src->method = Source::Bazaar;
        }
        else if (source.attribute("method") == "scp")
        {
            src->method = Source::Scp;
        }

        src->options = source.attribute("options");     // Facultatif (-p d'un patch, révision git/svn/etc, etc)

        rs.append(src);

        source = source.nextSiblingElement("download");
    }

    return rs;
}