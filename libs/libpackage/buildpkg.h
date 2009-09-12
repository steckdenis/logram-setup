/*
 * buildpkg.h
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

#ifndef __BUILDPKG_H__
#define __BUILDPKG_H__

#include <QObject>
#include <QList>

struct Source
{
    enum Method
    {
        Embed,
        Download,
        Git,
        Svn,
        Mercurial,
        Bazaar,
        Scp
    };

    enum Type
    {
        Main,       // Source principale
        Archive,    // Archive à décompresser
        Plain,      // Fichier brut à télécharger
        Patch       // Patch à appliquer
    };

    Method method;
    Type type;
    QString url;
    QString options;
};

class PackageSystem;

class BuildPackage : public QObject
{
    public:
        BuildPackage(const QString &fileName, PackageSystem *ps);

        // Fonctions principales
        void getSource();
        void preparePatch();
        void genPatch();
        void createSource();
        void patchSource();
        static void unpackSource(const QString &fileName);

        // Utilitaires
        QList<Source *> sources();
        void downloadSource(Source *src);

    private:
        class Private;
        Private *d;
};

#endif