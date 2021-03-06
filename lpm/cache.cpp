/*
 * cache.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include "app.h"

#include <databasepackage.h>
#include <filepackage.h>
#include <packagemetadata.h>

#include <QString>

#include <iostream>
using namespace std;

using namespace Logram;

void App::update()
{
    // Mettre à jour la base de donnée
    if (!ps->update(PackageSystem::Minimal))
    {
        error();
    }
}

void App::find(const QString &pattern)
{
    QVector<int> pkgs;
    QRegExp exp;
    
    exp.setCaseSensitivity(Qt::CaseSensitive);
    exp.setPatternSyntax(QRegExp::Wildcard);
    
    if (pattern.contains('*'))
    {
        exp.setPattern(pattern);
    }
    else
    {
        exp.setPattern("*" + pattern + "*");
    }
    
    if (!ps->packagesByName(exp, pkgs))
    {
        error();
        return;
    }

    // Afficher joliment les paquets
    for (int i=0; i<pkgs.count(); ++i)
    {
        int id = pkgs.at(i);
        
        DatabasePackage *pkg = ps->package(id);
        
        int sz = 0;
        
        if (pkg->name().length() > 20)
        {
            sz = pkg->name().length() - 20;
        }
        
        sz = 20 - sz;
        
        if (pkg->flags() & Package::Installed)
        {
            cout << COLOR("I ", "34");
        }
        else if (pkg->flags() & Package::Removed)
        {
            cout << COLOR("R ", "31");
        }
        else
        {
            cout << "  ";
        }
        
        cout
        << COLOR(pkg->name().leftJustified(20, ' ', false), "33") << ' '
        << COLOR(pkg->version().leftJustified(sz, ' ', false), "32") << ' '
        << qPrintable(pkg->shortDesc())
        << endl;
        
        delete pkg;
    }
}

static void outFlags(int flags)
{
    // Sortir les flags
    #define OUT_FLAG(flag_name, letter) \
        cout << (flags & flag_name ? letter : '-');
        
    cout << "\033[1m\033[33m";  // Fonction utilisée seulement en couleur, donc ok
    
    OUT_FLAG(PackageFile::Directory, 'd')
    OUT_FLAG(PackageFile::Installed, 'i')
    OUT_FLAG(PackageFile::DontRemove, 'r')
    OUT_FLAG(PackageFile::DontPurge, 'p')
    OUT_FLAG(PackageFile::Backup, 'b')
    OUT_FLAG(PackageFile::CheckBackup, 'c')
    OUT_FLAG(PackageFile::Overwrite, 'o')
    OUT_FLAG(PackageFile::Virtual, 'v')
    
    cout << "\033[0m";
        
    cout << ' ';
}

static void displayFile(PackageFile *file, bool colored)
{
    // Affichage de la sorte : «<jaune>---l--b</jaune> <bleu>/usr/share/truc/</bleu>chose 
    //                          appartient à <bleu>paquet</bleu>~<jaune>version</jaune>
    
    if (colored)
    {
        outFlags(file->flags());
    }
    
    // Afficher le chemin en couleur
    cout << COLOR("/" + file->path(), "32");
    
    if (file->package() == 0 || !file->package()->isValid())
    {
        cout << endl;
        return;
    }
    
    // Afficher «appartient à»
    if (colored)
    {
        cout << qPrintable(App::tr(" appartient à "));
    }
    else
    {
        // Plus facilement lisible d'un script
        cout << ':';
    }
    
    // Afficher le paquet en couleur
    cout << COLOR(file->package()->name(), "34");
    
    // Afficher la version du paquet
    cout << '~' << COLOR(file->package()->version(), "33");
    
    // Si le paquet est installé, afficher «(installé)» en rouge
    if (file->package()->flags() & Package::Installed)
    {
        cout << COLOR(App::tr(" (installé)"), "31");
    }
    
    // Fini
    cout << endl;
}

void App::infoFile(const QString &path)
{
    QVector<PackageFile *> files;
    
    if (path.startsWith('/'))
    {
        QString pth(path);
        pth.remove(0, 1);
        
        files = ps->files(pth);
    }
    else
    {
        // path = une expression régulière
        QRegExp exp;
    
        exp.setCaseSensitivity(Qt::CaseSensitive);
        exp.setPatternSyntax(QRegExp::Wildcard);
        exp.setPattern(path);
        
        files = ps->files(exp);
    }
    
    foreach (PackageFile *file, files)
    {
        displayFile(file, colored);
    }
}

void App::showFiles(const QString &packageName)
{
    QByteArray name, version;
    ps->parseVersion(packageName.toUtf8(), name, version);
    
    Package *pkg;
    
    if (!ps->package(name, (version.isNull() ? QString() : version), pkg))
    {
        error();
        return;
    }
    
    if (colored)
    {
        cout << COLOR(tr("Liste des fichiers installés par %1 :").arg(packageName), "32") << endl;
        cout << qPrintable(tr(
        "Légende : \n"
        "  * d: dossier\n"
        "  * i: installé\n"
        "  * r: ne pas supprimer, sauf si on purge\n"
        "  * p: ne pas supprimer, même si on purge\n"
        "  * b: toujours sauvegarder avant remplacement\n"
        "  * c: sauvegarder seulement si modifié localement depuis l'installation\n"
        "  * o: toujours écraser, même si un autre paquet le remplace\n"
        "  * v: n'est pas installé par le paquet, mais créé à l'utilisation\n"
        )) << endl << endl;
    }
    
    QString path;
    QStringList curParts, parts;
    int level;
    
    foreach (PackageFile *file, pkg->files())
    {
        path = file->path();
        
        if (!colored)
        {
            // Affichage "script-friendly" quand pas de couleurs
            cout << '/' << qPrintable(path) << endl;
            continue;
        }
        
        parts = path.split('/');
        
        // Trouver le nombre de parts communes entre curParts et parts.
        level = 0;
        for (int i=0; i<qMin(curParts.count()-1, parts.count()-1); ++i)
        {
            if (parts.at(i) == curParts.at(i))
            {
                level++;
            }
            else
            {
                break;
            }
        }
        
        // Sortir toutes les autres parties, avec indentation
        int level2 = level;
        for (int i=level; i<parts.count(); ++i)
        {
            outFlags(i != parts.count()-1 ? PackageFile::Directory : file->flags());
            
            for (int j=0; j<level2; ++j)
            {
                cout << "  ";
            }
            if (i == parts.count()-1)
            {
                // Fichier
                cout << "  " << qPrintable(parts.at(i)) << endl;
            }
            else
            {
                // Dossier
                cout << COLOR("* " + parts.at(i), "34") << endl;
            }
            level2++;
        }
        
        curParts = parts;
    }
}

void App::tagFile(const QString &fileName, const QString &tag)
{
    QVector<PackageFile *> files;
    QString fname, pname;
    Package *pkg = 0;
    DatabasePackage *dpkg = 0;
    
    // Savoir si fileName a une information sur le paquet contenant les fichiers
    fname = fileName;
    
    if (fileName.contains(':'))
    {
        fname = fileName.section(':', 1, -1); // Prendre juste le nom
        pname = fileName.section(':', 0, 0);  // Prendre le paquet
        
        QByteArray name, version;
        ps->parseVersion(pname.toUtf8(), name, version);
        
        if (!ps->package(name, (version.isNull() ? QString() : version), pkg))
        {
            error();
            return;
        }
        
        if (pkg != 0 && pkg->origin() == Package::Database)
        {
            dpkg = (DatabasePackage *)pkg;
        }
    }
    
    // Savoir si FileName est une regex
    if (fname.startsWith('/'))
    {
        // Non
        fname.remove(0, 1);
        files = ps->files(fname);
    }
    else
    {
        // Oui
        files = ps->files(QRegExp(fname, Qt::CaseSensitive, QRegExp::Wildcard));
    }
    
    // Si pkg != 0, éliminer tous les fichiers n'ayant pas le bon paquet
    if (dpkg != 0)
    {
        for (int i=0; i<files.count(); ++i)
        {
            DatabaseFile *file = (DatabaseFile *)files.at(i);
            
            if (((DatabasePackage *)file->package())->index() != dpkg->index())
            {
                files.remove(i);
                i--;
            }
        }
    }
    
    if (files.count() > 1)
    {
        cout << COLOR(tr("%1 fichiers vont être tagués, continuer ? ").arg(files.count()), "34") << std::flush;
        
        char in[2];
        
        getString(in, 2, "y", true);
        
        if (in[0] != 'y')
        {
            return;
        }
    }
    
    foreach (PackageFile *file, files)
    {
        int flag = 0;
        bool remove = false;
        QString t = tag;
        
        // Si t commence par "-", alors on retire le tag
        if (t.startsWith('-'))
        {
            remove = true;
            t.remove(0, 1);
        }
        
        if (t == "dontremove")
        {
            flag = PackageFile::DontRemove;
        }
        else if (t == "dontpurge")
        {
            flag = PackageFile::DontPurge;
        }
        else if (t == "backup")
        {
            flag = PackageFile::Backup;
        }
        else if (t == "overwrite")
        {
            flag = PackageFile::Overwrite;
        }
        else if (t == "checkbackup")
        {
            flag = PackageFile::CheckBackup;
        }
        else
        {
            cout << COLOR(tr("Tags disponibles :"), "37") << endl << endl;
            
            cout << qPrintable(tr("  * dontremove  : Ne pas supprimer quand son paquet est supprimé\n"
                                  "  * dontpurge   : Ne pas supprimer même si son paquet est purgé\n"
                                  "  * backup      : Toujours sauvegarder (.bak) avant remplacement\n"
                                  "  * checkbackup : Sauvegarder avant remplacement si modifié\n"
                                  "  * overwrite   : Ne jamais sauvegarder avant remplacement\n"));
                                  
            cout << endl;
            return;
        }
        
        if (remove)
        {
            // Supprimer le tag
            file->setFlags((PackageFile::Flag)(file->flags() & ~flag));
        }
        else
        {
            file->setFlags((PackageFile::Flag)(file->flags() | flag));
        }
    }
    
    (void) tag;
}

void App::tagPackage(const QString &packageName, const QString &tag)
{
    DatabasePackage *pkg;
    QVector<int> pkgs;
    QByteArray n, v;
    Depend::Operation op;
    
    // Parser le nom du paquet
    op = ps->parseVersion(packageName.toUtf8(), n, v);
    
    // Récupérer les paquets qui vont avec
    pkgs = ps->packagesByVString(n, v, op);
    
    // S'il y a plus d'un paquet, demander à l'utilisateur la confirmation
    if (pkgs.count() > 1)
    {
        cout << COLOR(tr("%1 paquets vont être tagués, continuer ? ").arg(pkgs.count()), "34") << std::flush;
        
        char in[2];
        
        getString(in, 2, "y", true);
        
        if (in[0] != 'y')
        {
            return;
        }
    }
    
    // Taguer les paquets
    foreach(int i, pkgs)
    {
        pkg = ps->package(i);
        
        int flag = 0;
        bool remove = false;
        QString t = tag;
        
        // Si t commence par "-", alors on retire le tag
        if (t.startsWith('-'))
        {
            remove = true;
            t.remove(0, 1);
        }
        
        if (t == "dontupdate")
        {
            flag = Package::DontUpdate;
        }
        else if (t == "dontinstall")
        {
            flag = Package::DontInstall;
        }
        else if (t == "dontremove")
        {
            flag = Package::DontRemove;
        }
        else if (t == "wanted")
        {
            flag = Package::Wanted;
        }
        else
        {
            cout << COLOR(tr("Tags disponibles :"), "37") << endl << endl;
            
            cout << qPrintable(tr("  * dontupdate  : Ne pas mettre à jour le paquet\n"
                                  "  * dontinstall : Ne pas installer le paquet\n"
                                  "  * dontremove  : Ne pas supprimer le paquet\n"
                                  "  * wanted      : Ne pas supprimer ce paquet s'il n'est plus nécessaire\n"));
                                  
            cout << endl;
            return;
        }
        
        if (remove)
        {
            // Supprimer le tag
            pkg->setFlags((Package::Flag)(pkg->flags() & ~flag));
        }
        else
        {
            pkg->setFlags((Package::Flag)(pkg->flags() | flag));
        }
    }
    
    (void) tag;
}

QStringList App::pkgFlags(Package *pkg)
{
    QStringList rs;
    QString tmp;
    int flags = pkg->flags();
    
    // Intégration KDE
    int kdeintegration = (flags & Package::KDEIntegration);
    
    tmp = App::tr("Intégration à KDE        : ");
    
    switch (kdeintegration)
    {
        case 0:
            tmp += App::tr("pas intégré");
            break;
        case 1:
            tmp += App::tr("utilisable");
            break;
        case 2:
            tmp += App::tr("bien intégré");
            break;
        case 3:
            tmp += App::tr("parfaitement intégré");
            break;
    }
    
    rs.append(tmp);
    
    #define YESNO(flag, string) \
        rs.append( \
            string \
            + ((flags & flag) != 0 ? syes : sno));
    
    QString syes = COLORS(App::tr("Oui"), "34");
    QString sno = App::tr("Non");
    
    // Graphique
    YESNO(Package::GUI,         App::tr("Paquet graphique         : "))
    
    // Principal
    YESNO(Package::Primary,     App::tr("Paquet principal         : "))
    
    // Ne pas mettre à jour
    YESNO(Package::DontUpdate,  App::tr("Ne pas mettre à jour     : "))
    
    // Ne pas installer
    YESNO(Package::DontInstall, App::tr("Ne pas installer         : "))
    
    // Ne pas supprimer
    YESNO(Package::DontRemove,  App::tr("Ne pas supprimer         : "))
    
    // Nécessite une CLUF
    YESNO(Package::Eula,        App::tr("Licence à approuver      : "))
    
    // Nécessite un redémarrage
    YESNO(Package::NeedsReboot, App::tr("Nécessite un redémarrage : "))
    
    // Ne pas être supprimé par dépendances auto
    YESNO(Package::Wanted,      App::tr("Installé manuellement    : "))
    
    return rs;
}

void App::showpkg(const QString &name, bool changelog, bool license)
{
    QString n = name.section('=', 0, 0);
    QString v;

    if (name.contains('='))
    {
        v = name.section('=', 1, -1);
    }

    Package *pkg;
    
    if (!ps->package(n, v, pkg))
    {
        error();
        return;
    }
    
    // Précharger les métadonnées
    PackageMetaData *metadata = pkg->metadata();
    
    if (metadata == 0)
    {
        // Pas Internet ?
        error();
        
        // On ne retourne pas, ça permet de garder LPM utilisable même hors ligne
    }
    
    if (metadata) metadata->setCurrentPackage(pkg->name());

    // Status du paquet
    QString status;

    if (pkg->flags() & Package::Removed)
    {
        status = tr("Supprimé");
    }
    else if (pkg->flags() & Package::Installed)
    {
        status = tr("Installé");
    }
    else
    {
        status = tr("Non installé");
    }

    // Afficher les informations
    cout << COLOR(tr("Nom                 : "), "33") << COLOR(pkg->name(), "34") << endl;
    cout << COLOR(tr("Version             : "), "33") << qPrintable(pkg->version()) << endl;
    if (metadata) 
    cout << COLOR(tr("Titre               : "), "33") << qPrintable(metadata->packageTitle()) << endl;
    cout << COLOR(tr("Drapeaux            : "), "33");
    
    QStringList flags = pkgFlags(pkg);
    bool fFirst = true;
    
    foreach (const QString &flag, flags)
    {
        if (!fFirst)
        {
            cout << "                      ";
        }
        
        fFirst = false;
        
        cout << "o " << qPrintable(flag) << endl;
    }
    
    cout << COLOR(tr("Section             : "), "33") << qPrintable(pkg->section()) << endl;
    
    Repository repo;
    ps->repository(pkg->repo(), repo);
    
    cout << COLOR(tr("Distribution        : "), "33") << qPrintable(pkg->distribution()) << endl;
    cout << COLOR(tr("Dépôt d'origine     : "), "33") << qPrintable(repo.description) << " (" << qPrintable(pkg->repo()) << ')' << endl;
    cout << COLOR(tr("Status              : "), "33") << qPrintable(status) << endl;
    cout << COLOR(tr("Utilisé par         : "), "33") << qPrintable(tr("%n paquet(s)", "", pkg->used())) << endl;
    
    if (pkg->flags() & Package::Installed)
    {
        cout << COLOR(tr("Date d'installation : "), "33") << qPrintable(pkg->installedDate().toString(Qt::DefaultLocaleLongDate)) << endl;
        // TODO: Uid vers nom d'utilisateur
    }
    else if (pkg->flags() & Package::Removed)
    {
        cout << COLOR(tr("Date de suppression : "), "33") << qPrintable(pkg->installedDate().toString(Qt::DefaultLocaleLongDate)) << endl;
        // TODO: Uid vers nom d'utilisateur
    }
    
    cout << COLOR(tr("Téléchargement      : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->downloadSize())) << endl;
    cout << COLOR(tr("Taille installée    : "), "33") << qPrintable(PackageSystem::fileSizeFormat(pkg->installSize())) << endl;
    cout << COLOR(tr("Paquet source       : "), "33") << qPrintable(pkg->source()) << endl;
    cout << COLOR(tr("Adresse de l'auteur : "), "33") << qPrintable(pkg->upstreamUrl()) << endl;
    cout << COLOR(tr("Licence             : "), "33") << qPrintable(pkg->license()) << endl;
    cout << COLOR(tr("Mainteneur          : "), "33") << qPrintable(pkg->maintainer()) << endl;
    cout << COLOR(tr("Description courte  : "), "33") << qPrintable(pkg->shortDesc()) << endl;
    
    if (metadata)
    {
        cout << endl << COLOR(tr("Description longue : "), "35") << endl << endl;
        printIndented(metadata->packageDescription().toUtf8(), 4);
        cout << endl;
    }

    // Afficher les dépendances
    QVector<Depend *> deps = pkg->depends();

    if (deps.count() != 0)
    {
        cout << endl;
        cout << COLOR(tr("Dépendances :"), "35") << endl;
        cout << qPrintable(tr("Légende : "))
            << COLOR(tr("D: Dépend "), "31")
            << COLOR(tr("S: Suggère "), "32")
            << COLOR(tr("C: Conflit "), "33")
            << COLOR(tr("P: Fourni "), "34")
            << COLOR(tr("R: Remplace "), "35")
            << COLOR(tr("N: Est requis par"), "37")
        << endl << endl;

        for (int i=0; i<deps.count(); ++i)
        {
            Depend *dep = deps.at(i);

            switch (dep->type())
            {
                case Depend::DependType:
                    cout << " D: ";
                    cout << COLOR(dep->name(), "31");
                    break;
                case Depend::Suggest:
                    cout << " S: ";
                    cout << COLOR(dep->name(), "32");
                    break;
                case Depend::Conflict:
                    cout << " C: ";
                    cout << COLOR(dep->name(), "33");
                    break;
                case Depend::Provide:
                    cout << " P: ";
                    cout << COLOR(dep->name(), "34");
                    break;
                case Depend::Replace:
                    cout << " R: ";
                    cout << COLOR(dep->name(), "35");
                    break;
                case Depend::RevDep:
                    cout << " N: ";
                    cout << COLOR(dep->name(), "37");
                    break;
                default:
                    break;
            }

            if (dep->op() != Depend::NoVersion)
            {
                switch (dep->op())
                {
                    case Depend::Equal:
                        cout << " (= ";
                        break;
                    case Depend::GreaterOrEqual:
                        cout << " (>= ";
                        break;
                    case Depend::Greater:
                        cout << " (> ";
                        break;
                    case Depend::LowerOrEqual:
                        cout << " (<= ";
                        break;
                    case Depend::Lower:
                        cout << " (< ";
                        break;
                    case Depend::NotEqual:
                        cout << " (!= ";
                        break;
                    default:
                        break;
                }

                cout << qPrintable(dep->version()) << ")";
            }

            cout << endl;
        }
    }

    // Versions disponibles
    if (pkg->origin() == Package::Database)
    {
        DatabasePackage *dpkg = (DatabasePackage*)pkg;
        
        cout << endl << COLOR(tr("Versions disponibles : "), "35") << endl;
        cout << qPrintable(tr("Légende : * = Disponible, I = installée, R = supprimée")) << endl << endl;

        QVector<DatabasePackage *> vers = dpkg->versions();

        foreach(DatabasePackage *ver, vers)
        {
            if (ver->flags() & Package::Installed)
            {
                cout << COLORC("  I ", "34");
            }
            else if (ver->flags() & Package::Removed)
            {
                cout << COLORC("  R ", "31");
            }
            else
            {
                cout << "  * ";
            }

            //cout << COLOR((ver->name() +ver->version().leftJustified(26, ' ', true), "32") << ' '
            //     << qPrintable(ver->shortDesc()) << endl;
            cout << qPrintable((COLORS(ver->name(), "34") + '~' + COLORS(ver->version(), "33")))
                 << ' '
                 << qPrintable(ver->shortDesc()) << endl;
        }

        cout << endl;
    }
    
    if (changelog && metadata)
    {
        cout << COLOR(tr("Historique des versions : "), "35") << endl << endl;
        
        // Afficher le changelog
        QVector<ChangeLogEntry *> entries = metadata->changelog();
        
        foreach(ChangeLogEntry *entry, entries)
        {
            QString t;
            
            switch (entry->type)
            {
                case ChangeLogEntry::LowPriority:
                    t = tr("Faible priorité : ");
                    break;
                    
                case ChangeLogEntry::Feature:
                    t = tr("Nouvelles fonctionnalités : ");
                    break;
                    
                case ChangeLogEntry::BugFix:
                    t = tr("Correction de bogues : ");
                    break;
                    
                case ChangeLogEntry::Security:
                    t = tr("Mise à jour de sécurité : ");
                    break;
            }
            
            cout
            << "  * "
            << COLOR(t, "37")
            << COLOR(entry->version, "32")
            << ", "
            << qPrintable(entry->date.toString(Qt::DefaultLocaleShortDate))
            << endl
            << "    "
            << qPrintable(tr("Par %1 <%2>").arg(entry->author, entry->email))
            << endl;
            
            cout << endl;
            
            // Ecrire le texte caractères par caractères
            printIndented(entry->text.toUtf8(), 8);
            
            cout << endl << endl;
        }
        
        qDeleteAll(entries);
    }
    
    if (license && (pkg->flags() & Package::Eula) && metadata)
    {
        cout <<  COLOR(tr("Texte de la licence : "), "35") << endl << endl;
        printIndented(metadata->packageEula().toUtf8(), 4);
        cout << endl << endl;
    }
}
