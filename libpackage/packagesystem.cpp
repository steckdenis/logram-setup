/*
 * packagesystem.cpp
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

#include "packagesystem.h"
#include "databasereader.h"
#include "databasewriter.h"
#include "databasepackage.h"
#include "filepackage.h"
#include "solver.h"

#include <QSettings>
#include <QStringList>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QTextCodec>
#include <QMutex>

#include <cctype>

#include <gpgme.h>

#include <QtDebug>

using namespace Logram;

struct Logram::PackageSystem::Private
{
    DatabaseReader *dr;
    
    int progressCount;
    QHash<int, Progress *> progresses;
    int processOutProgress;
    
    QEventLoop loop;
    QNetworkAccessManager *nmanager;
    QString dlDest;
    QHash<QNetworkReply *, ManagedDownload *> managedDls;
    QSettings *set, *ipackages;
    
    PackageError *lastError;
    QMutex errorMutex;

    // Options
    bool installSuggests;
    int parallelInstalls, parallelDownloads;
    QString installRoot, confRoot, varRoot;
    int setParams;
    
    // Mirroirs
    QHash<QString, int> usedMirrors;
};

Logram::PackageSystem::PackageSystem(QObject *parent) : QObject(parent)
{
    d = new Private;
    
    d->dr = new DatabaseReader(this);
    d->nmanager = new QNetworkAccessManager(this);
    d->setParams = 0;
    d->lastError = 0;
    d->progressCount = 0;
    d->set = 0;
    d->ipackages = 0;
    
    d->processOutProgress = startProgress(Progress::ProcessOut, 1);
    
    // Initialiser GPG
    gpgme_check_version(NULL);
    
    connect(d->nmanager, SIGNAL(finished(QNetworkReply *)), this, SLOT(downloadFinished(QNetworkReply *)));
}

Logram::PackageSystem::~PackageSystem()
{
    endProgress(d->processOutProgress);
    
    if (d->ipackages) delete d->ipackages;
    if (d->set) delete d->set;
    
    delete d->nmanager;
    delete d->dr;
    delete d;
}

void Logram::PackageSystem::loadConfig()
{
    d->set = new QSettings(confRoot() + "/etc/lgrpkg/sources.list", QSettings::IniFormat, this);
    d->set->setIniCodec(QTextCodec::codecForCStrings());
    
    if ((d->setParams & PACKAGESYSTEM_OPT_INSTALLSUGGESTS) == 0)
    {
        d->installSuggests = d->set->value("InstallSuggests", true).toBool();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_PARALLELINSTALLS) == 0)
    {
        d->parallelInstalls = d->set->value("ParallelInstalls", 1).toInt();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_PARALLELDOWNLOADS) == 0)
    {
        d->parallelDownloads = d->set->value("ParallelDownloads", 2).toInt();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_INSTALLROOT) == 0)
    {
        d->installRoot = d->set->value("InstallRoot", "/").toString();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_CONFROOT) == 0)
    {
        d->confRoot = d->set->value("ConfigRoot", "/").toString();
    }
    if ((d->setParams & PACKAGESYSTEM_OPT_VARROOT) == 0)
    {
        d->varRoot = d->set->value("VarRoot", "/").toString();
    }
    
    d->ipackages = new QSettings(varRoot() + "/var/cache/lgrpkg/db/installed_packages.list", QSettings::IniFormat, this);
}

bool Logram::PackageSystem::init()
{
    return d->dr->init();
}

struct Enrg
{
    QString url, distroName, arch, sourceName;
    Repository::Type type;
    bool gpgCheck;
};

QList<Repository> Logram::PackageSystem::repositories() const
{
    QList<Repository> rs;
    Repository repo;
    
    foreach (const QString &repoName, d->set->childGroups())
    {
        if (repository(repoName, repo))
        {
            rs.append(repo);
        }
    }
    
    return rs;
}

bool Logram::PackageSystem::repository(const QString &name, Repository &rs) const
{
    if (!d->set->contains(name + "/Mirrors"))
    {
        return false;
    }
    
    d->set->beginGroup(name);
        
    QString t = d->set->value("Type", "remote").toString();
    
    if (t == "remote")
    {
        rs.type = Repository::Remote;
    }
    else if (t == "local")
    {
        rs.type = Repository::Local;
    }
    else
    {
        rs.type = Repository::Unknown;
    }
    
    rs.name = name;
    rs.description = d->set->value("Description").toString();
    
    rs.mirrors = d->set->value("Mirrors").toString().split(' ', QString::SkipEmptyParts);
    rs.distributions = d->set->value("Distributions").toString().split(' ', QString::SkipEmptyParts);
    rs.architectures = d->set->value("Archs").toString().split(' ', QString::SkipEmptyParts);
    
    rs.active = d->set->value("Active", true).toBool();
    rs.gpgcheck = d->set->value("Sign", true).toBool();
    
    d->set->endGroup();
    
    return true;
}

QSettings *Logram::PackageSystem::installedPackagesList() const
{
    return d->ipackages;
}

bool Logram::PackageSystem::update()
{
    // Explorer la liste des mirroirs dans /etc/setup/sources.list, format QSettings
    QString lang = d->set->value("Language", tr("fr", "Langue par défaut pour les paquets")).toString();
    
    DatabaseWriter *db = new DatabaseWriter(this);

    QList<Enrg *> enrgs;

    foreach (const Repository &repo, repositories())
    {
        // Explorer chaque distribution, et chaque architecture dedans
        if (!repo.active)
        {
            continue;
        }
        
        foreach (const QString &distroName, repo.distributions)
        {
            foreach (const QString &arch, repo.architectures)
            {
                Enrg *enrg = new Enrg;

                enrg->url = repo.mirrors.at(0);
                enrg->distroName = distroName;
                enrg->arch = arch;
                enrg->sourceName = repo.name;
                enrg->type = repo.type;
                enrg->gpgCheck = repo.gpgcheck;

                enrgs.append(enrg);
            }
        }
    }

    // Explorer les enregistrements et les télécharger
    int count = enrgs.count();
    int progress = startProgress(Progress::GlobalDownload, count*2);
    
    for (int i=0; i<count; ++i)
    {
        Enrg *enrg = enrgs.at(i);

        QString path = enrg->url + "/dists/" + enrg->distroName + "/" + enrg->arch;
        QString u = path + "/packages.xz";
        
        if (!sendProgress(progress, i*3, u))
        {
            return false;
        }
        
        if (!db->download(enrg->sourceName, u, enrg->type, DatabaseWriter::PackagesList, enrg->gpgCheck))
        {
            return false;
        }

        // Traductions
        u = path + "/translate." + lang + ".xz";
        
        if (!sendProgress(progress, i*3+1, u))
        {
            return false;
        }
        
        if (!db->download(enrg->sourceName, u, enrg->type, DatabaseWriter::Translations, enrg->gpgCheck))
        {
            return false;
        }
        
        // Liste des fichiers
        u = path + "/files.xz";
        
        if (!sendProgress(progress, i*3+2, u))
        {
            return false;
        }
        
        if (!db->download(enrg->sourceName, u, enrg->type, DatabaseWriter::FilesList, enrg->gpgCheck))
        {
            return false;
        }
        
        delete enrg;
    }

    endProgress(progress);

    if (!db->rebuild())
    {
        return false;
    }

    delete db;
    return true;
}

QList<Logram::DatabasePackage *> Logram::PackageSystem::upgradePackages()
{
    QList<UpgradeInfo> upds = d->dr->upgradePackages();
    QList<DatabasePackage *> rs;
    
    // Créer les paquets à mettre à jour
    foreach(const UpgradeInfo &inf, upds)
    {
        DatabasePackage *pkg = new DatabasePackage(inf.installedPackage, this, d->dr, Solver::Update);
        
        pkg->setUpgradePackage(inf.newPackage);
        
        rs.append(pkg);
    }
    
    return rs;
}

QList<Logram::DatabasePackage *> Logram::PackageSystem::orphans()
{
    QList<int> pkgs = d->dr->orphans();
    QList<DatabasePackage *> rs;
    
    foreach (int p, pkgs)
    {
        DatabasePackage *pkg = new DatabasePackage(p, this, d->dr, Solver::Remove);
        
        rs.append(pkg);
    }
    
    return rs;
}

bool Logram::PackageSystem::packagesByName(const QRegExp &regex, QList<int> &rs)
{
    return d->dr->packagesByName(regex, rs);
}

QList<int> Logram::PackageSystem::packagesByVString(const QString &name, const QString &version, int op)
{
    return d->dr->packagesByVString(name, version, op);
}

int Logram::PackageSystem::packages()
{
    return d->dr->packages();
}

bool Logram::PackageSystem::package(const QString &name, const QString &version, Package* &rs)
{
    // Si le nom se termine par .lpk, c'est un paquet local
    if (name.endsWith(".lpk"))
    {
        FilePackage *pkg = new FilePackage(name, this, d->dr, Solver::None);
        
        if (!pkg->isValid())
        {
            PackageError *err = new PackageError;
            err->type = PackageError::PackageNotFound;
            err->info = name;
            
            setLastError(err);
            
            return false;
        }
        
        rs = pkg;
        return true;
    }
    else
    {
        // Paquet dans la base de donnée
        int i;
        
        if (!d->dr->package(name, version, i))
        {
            return false;
        }
        
        rs = new DatabasePackage(i, this, d->dr);

        return true;
    }
}

DatabasePackage *Logram::PackageSystem::package(int id)
{
    return new DatabasePackage(id, this, d->dr);
}

PackageFile *Logram::PackageSystem::file(const QString &name)
{
    return d->dr->file(name);
}

Solver *Logram::PackageSystem::newSolver()
{
    return new Solver(this, d->dr);
}

bool Logram::PackageSystem::download(Repository::Type type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs)
{
    // Ne pas télécharger un fichier en cache
    if (QFile::exists(dest))
    {
        if (!block)
        {
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            
            emit downloadEnded(rs);
        }

        return true;
    }
    
    if (type == Repository::Remote)
    {
        // Lancer le téléchargement
        QNetworkReply *reply = d->nmanager->get(QNetworkRequest(QUrl(url)));
        connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(dlProgress(qint64, qint64)));

        if (!block)
        {
            // Ajouter le ManagedDownload dans la liste
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = reply;
            rs->url = url;
            rs->destination = dest;

            d->managedDls.insert(reply, rs);
            return true;
        }
        else
        {
            d->dlDest = dest;
            
            int rs = d->loop.exec();
            
            delete reply;
            
            return (rs == 0);
        }
    }
    else if (type == Repository::Local)
    {
        if (!QFile::copy(url, dest))
        {
            PackageError *err = new PackageError;
            err->type = PackageError::DownloadError;
            err->info = url;
            err->more = dest;
            
            setLastError(err);
            
            return false;
        }

        // Si on ne bloquait pas, dire que c'est fini quand-même
        if (!block)
        {
            rs = new ManagedDownload;
            rs->error = false;
            rs->reply = 0;
            rs->url = url;
            rs->destination = dest;
            
            emit downloadEnded(rs);
            return true;
        }
        
        return true;
    }
    
    PackageError *err = new PackageError;
    err->type = PackageError::BadDownloadType;
    err->info = type;
    
    setLastError(err);
    return false;
}

void Logram::PackageSystem::downloadFinished(QNetworkReply *reply)
{
    // Savoir si on bloquait
    ManagedDownload *md = d->managedDls.value(reply, 0);
    QString dlDest;

    if (md != 0)
    {
        dlDest = md->destination;
    }
    else
    {
        dlDest = d->dlDest;
    }
    
    // Terminer la progression
    int progress;
    QVariant prop = reply->property("__L_progress");
    
    if (!prop.isNull())
    {
        progress = prop.toInt();
        
        endProgress(progress);
    }
    
    // Voir s'il y a eu des erreurs
    if (reply->error() != QNetworkReply::NoError)
    {
        PackageError *err = new PackageError;
        err->type = PackageError::DownloadError;
        err->info = reply->url().toString();
        err->more = dlDest;
        
        setLastError(err);
        
        if (md == 0)
        {
            d->loop.exit(1);
            return;
        }
        else
        {
            md->error = true;
            emit downloadEnded(md);
            return;
        }
    }

    // Téléchargement
    QFile fl(dlDest);

    if (!fl.open(QIODevice::WriteOnly))
    {
        PackageError *err = new PackageError;
        err->type = PackageError::OpenFileError;
        err->info = fl.fileName();
        
        setLastError(err);
        
        if (md == 0)
        {
            d->loop.exit(1);
            return;
        }
        else
        {
            md->error = true;
            return;
        }
    }

    fl.write(reply->readAll());

    fl.close();

    if (md == 0)
    {
        // Quitter la boucle
        d->loop.exit(0);
    }
    else
    {
        // Envoyer le signal comme quoi on a fini
        emit downloadEnded(md);
    }
}

void Logram::PackageSystem::dlProgress(qint64 done, qint64 total)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply != 0)
    {
        // Mettre à jour la progression, ou la créer si nécessaire
        int progress;
        QVariant prop = reply->property("__L_progress");
        
        if (prop.isNull())
        {
            progress = startProgress(Progress::Download, total);
            reply->setProperty("__L_progress", progress);
        }
        else
        {
            progress = prop.toInt();
        }
        
        if (!sendProgress(progress, done, reply->url().toString()))
        {
            reply->abort();
        }
    }
}

/* Erreurs */

void Logram::PackageSystem::setLastError(PackageError *err)
{
    d->errorMutex.lock();
    
    if (d->lastError != 0)
    {
        delete d->lastError;
    }
    
    d->lastError = err;
    
    d->errorMutex.unlock();
}

void Logram::PackageSystem::setLastError(PackageError::Error type, const QString &info, const QString &more)
{
    PackageError *err = new PackageError;
    
    err->type = type;
    err->info = info;
    err->more = more;
    
    setLastError(err);
}

PackageError *Logram::PackageSystem::lastError()
{
    d->errorMutex.lock();
    
    PackageError *le = d->lastError;
    
    d->errorMutex.unlock();
    
    return le;
}

/* Utilitaires */

int Logram::PackageSystem::parseVersion(const QByteArray &verStr, QByteArray &name, QByteArray &version)
{
    const char *s = verStr.constData();
    char c, c2;
    int namesize = 0, versionsize = 0, op = DEPEND_OP_NOVERSION, pos = 0, len = verStr.size();
    const char *n = 0, *v = 0;
    
    // Le nom
    n = s;
    while (*s != 0 && pos < len)
    {
        c = *s;
        
        if (c == '>' || c == '<' || c == '!' || c == '=')
        {
            break;
        }
        
        s++;
        namesize++;
        pos++;
    }
    
    if (*s == 0 || pos == len)
    {
        // Juste un nom
        name = QByteArray(n, namesize);
        return op;
    }
    
    // L'opération
    // Ici, c est soit égal à >, soit à <, soit à !, soit à =.
    // Il suffit de comparer le caractère suivant
    s++;            // s pointe sur le premier caractère de la version, ou =, ou \0 s'il y a un problème
    pos++;
    c2 = *s;
    
    if (c2 == 0 || pos == len)
    {
        name = QByteArray(n, namesize);
        return op;
    }
    
    if (c2 == '=')
    {
        // Avancer d'un caractère pour la version
        s++;
        pos++;
    }
    
    switch (c)
    {
        case '>':
            op = (c2 == '=' ? DEPEND_OP_GREQ : DEPEND_OP_GR);
            break;
        case '<':
            op = (c2 == '=' ? DEPEND_OP_LOEQ : DEPEND_OP_LO);
            break;
        case '!':
            op = DEPEND_OP_NE;  // a!=b ou a!b est pris, side effect
            break;
        case '=':
            op = DEPEND_OP_EQ;  // a=b ou a==b est pris, side effect
            break;
    }
    
    // La version, simplement prendre de *s au bout
    v = s;
    
    while (*s && pos < len)
    {
        s++;
        versionsize++;
        pos++;
    }
    
    // Retour
    name = QByteArray(n, namesize);
    version = QByteArray(v, versionsize);
    return op;
}

bool Logram::PackageSystem::matchVersion(const QByteArray &v1, const QByteArray &v2, int op)
{
    // Comparer les versions
    int rs = compareVersions(v1, v2);

    // Retourner en fonction de l'opérateur
    switch (op)
    {
        case DEPEND_OP_EQ:
            return (rs == 0);
        case DEPEND_OP_GREQ:
            return ( (rs == 0) || (rs == 1) );
        case DEPEND_OP_GR:
            return (rs == 1);
        case DEPEND_OP_LOEQ:
            return ( (rs == 0) || (rs == -1) );
        case DEPEND_OP_LO:
            return (rs == -1);
        case DEPEND_OP_NE:
            return (rs != 0);
    }

    return true;
}

int Logram::PackageSystem::compareVersions(const QByteArray &v1, const QByteArray &v2)
{
    return compareVersions(v1.constData(), v2.constData());
}

int Logram::PackageSystem::compareVersions(const char *a, const char *b)
{
    if (strcmp(a, b) == 0) return 0;
    
    // Explorer chaque caractère
    int numa, numb;
    
    while (*a || *b)
    {
        //Sauter tout ce qui n'est pas nombre
        while (*a && !isdigit(*a)) a++;
        while (*b && !isdigit(*b)) b++;
        
        // On a donc maintenant des digits sous la dent, les transformer en entiers
        numa = 0;
        numb = 0;
        
        if (!*b && !*a)
        {
            // Les deux chaînes sont arrivées au bout, elles sont les mêmes
            return 0;
        }
        
        if (*a)
        {
            while (*a && isdigit(*a))
            {
                numa *= 10;
                numa += (*a - '0');
                a++;
            }
        }
        else
        {
            // Il manque un champ, l'ajouter
            numa = 0;
        }
        
        if (*b)
        {
            while (*b && isdigit(*b))
            {
                numb *= 10;
                numb += (*b - '0');
                b++;
            }
        }
        else
        {
            numb = 0;
        }
        
        // Comparer les nombres
        if (numa > numb)
        {
            return 1;
        }
        else if (numa < numb)
        {
            return -1;
        }
        
        // Deux nombres égaux, passer à la suite des champs
    }
    
    return 0;   // Les mêmes
}

QString Logram::PackageSystem::fileSizeFormat(int size)
{
    double fsize = double(size);
    
    if (size < 1024)
    {
        return QString::number(size) + " o";
    }
    else if (size < 1024*1024)
    {
        return QString::number(fsize/1024.0, 'f', 2) + " Kio";
    }
    else if (size < 1024*1024*1024)
    {
        return QString::number(fsize/1048576.0, 'f', 2) + " Mio";
    }
    else
    {
        return QString::number(fsize/1073741824.0, 'f', 2) + " Gio";
    }
}

QString Logram::PackageSystem::dependString(const QString &name, const QString &version, int op)
{
    QString rs(name);

    if (op != DEPEND_OP_NOVERSION)
    {
        switch (op)
        {
            case DEPEND_OP_EQ:
                rs += "=";
                break;
            case DEPEND_OP_GREQ:
                rs += ">=";
                break;
            case DEPEND_OP_GR:
                rs += ">";
                break;
            case DEPEND_OP_LOEQ:
                rs += "<=";
                break;
            case DEPEND_OP_LO:
                rs += "<";
                break;
            case DEPEND_OP_NE:
                rs += "!=";
                break;
        }
        
        rs += version;
    }
    
    return rs;
}

/* Options */

bool Logram::PackageSystem::installSuggests() const
{
    return d->installSuggests;
}

int Logram::PackageSystem::parallelDownloads() const
{
    return d->parallelDownloads;
}

int Logram::PackageSystem::parallelInstalls() const
{
    return d->parallelInstalls;
}

QString Logram::PackageSystem::installRoot() const
{
    return d->installRoot;
}

QString Logram::PackageSystem::confRoot() const
{
    return d->confRoot;
}

QString Logram::PackageSystem::varRoot() const
{
    return d->varRoot;
}

void Logram::PackageSystem::setConfRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_CONFROOT;
    
    d->confRoot = root;
}

void Logram::PackageSystem::setVarRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_VARROOT;
    
    d->varRoot = root;
}

void Logram::PackageSystem::setInstallSuggests(bool enable)
{
    d->setParams |= PACKAGESYSTEM_OPT_INSTALLSUGGESTS;
    
    d->installSuggests = enable;
}

void Logram::PackageSystem::setParallelDownloads(int num)
{
    d->setParams |= PACKAGESYSTEM_OPT_PARALLELDOWNLOADS;
    
    d->parallelDownloads = num;
}

void Logram::PackageSystem::setParallelInstalls(int num)
{
    d->setParams |= PACKAGESYSTEM_OPT_PARALLELINSTALLS;
    
    d->parallelInstalls = num;
}

void Logram::PackageSystem::setInstallRoot(const QString &root)
{
    d->setParams |= PACKAGESYSTEM_OPT_INSTALLROOT;
    
    d->installRoot = root;
}

/* Signaux */
int Logram::PackageSystem::startProgress(Progress::Type type, int tot)
{
    int pid = d->progressCount;
    Progress *p = new Progress;
    
    p->action = Progress::Create;
    p->type = type;
    p->total = tot;
    
    d->progresses.insert(pid, p);
    
    d->progressCount++;
    
    emit progress(p);
    
    return pid;
}

bool Logram::PackageSystem::sendProgress(int id, int num, const QString &msg, const QString &more, void *data)
{
    Progress *p = d->progresses.value(id);
    
    if (p == 0)
    {
        return false;
    }
    
    p->old = p->current;
    p->current = num;
    p->info = msg;
    p->more = more;
    p->data = data;
    p->canceled = false;
    
    p->action = Progress::Update;
    
    emit progress(p);
    
    if (p->canceled)
    {
        setLastError(PackageError::ProgressCanceled, msg, more);
        return false;
    }
    
    return true;
}

bool Logram::PackageSystem::processOut(const QString &command, const QString &line)
{
    return sendProgress(d->processOutProgress, 0, command, line);
}

void Logram::PackageSystem::endProgress(int id)
{
    Progress *p = d->progresses.take(id);       // take : plus besoin après, le slot s'occupe du delete
    
    if (p == 0)
    {
        return;
    }
    
    p->action = Progress::End;
    
    emit progress(p);
}

void Logram::PackageSystem::sendCommunication(Package *sender, Communication *comm)
{
    emit communication(sender, comm);
}

QString Logram::PackageSystem::bestMirror(const Repository &repo)
{
    // Explorer chaque mirroir du dépôt
    int minUsed = 0x7FFFFFFF;
    QString bmirror;
    
    foreach (const QString &mirror, repo.mirrors)
    {
        if (!d->usedMirrors.contains(mirror))
        {
            // Mirroir pas encore utilisé, c'est facile
            d->usedMirrors.insert(mirror, 1);
            return mirror;
        }
        
        if (d->usedMirrors.value(mirror) < minUsed)
        {
            // On a trouvé un mirroir moins utilisé que le précédant
            bmirror = mirror;
            minUsed = d->usedMirrors.value(mirror);
        }
    }
    
    // On a trouvé un mirroir
    if (bmirror.isEmpty())
    {
        return QString();
    }
    
    d->usedMirrors[bmirror]++;
    
    return bmirror;
}

void Logram::PackageSystem::releaseMirror(const QString &mirror)
{
    if (!d->usedMirrors.contains(mirror))
    {
        return;
    }
    
    // Retirer un jeton au mirroir
    d->usedMirrors[mirror]--;
}

DatabaseReader *Logram::PackageSystem::databaseReader()
{
    return d->dr;
}