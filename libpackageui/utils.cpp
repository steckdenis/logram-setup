/*
 * utils.cpp
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#include "utils.h"

#include <QImage>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QIcon>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>

#include <packagesystem.h>
#include <installwizard.h>
#include <package.h>

#include <string>
#include <sstream>

using namespace LogramUi;
using namespace Logram;

QPixmap Utils::pixmapFromData(const QByteArray &data, int width, int height)
{
    QImage img = QImage::fromData(data);
    return QPixmap::fromImage(img).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QString Utils::markdown(const QString &source)
{
    QProcess mdown;
    
    mdown.start("markdown", QStringList() << "/dev/stdin");
    
    if (!mdown.waitForStarted())
    {
        return source;
    }
    
    mdown.write(source.toUtf8());
    mdown.closeWriteChannel();
    
    if (!mdown.waitForFinished())
    {
        return source;
    }
    
    return QString::fromUtf8(mdown.readAll());
}

QString Utils::actionNameInf(Logram::Solver::Action action)
{
    switch (action)
    {
        case Solver::Install:
            return QApplication::translate("Utils", "Installer");
            break;
        case Solver::Remove:
            return QApplication::translate("Utils", "Supprimer");
            break;
        case Solver::Purge:
            return QApplication::translate("Utils", "Supprimer totalement");
            break;
        default:
            break;
    }

    return QString();
}

void Utils::packageSystemError(PackageSystem* ps)
{
    PackageError *err = ps->lastError();
    QString s;
    
    if (err == 0)
    {
        s = QApplication::translate("Utils", "Pas d'erreur ou erreur inconnue");
    }
    else
    {
        switch (err->type)
        {
            case PackageError::OpenFileError:
                s = QApplication::translate("Utils", "Impossible d'ouvrir le fichier ");
                break;

            case PackageError::MapFileError:
                s = QApplication::translate("Utils", "Impossible de mapper le fichier ");
                break;

            case PackageError::ProcessError:
                s = QApplication::translate("Utils", "Erreur dans la commande ");
                break;

            case PackageError::DownloadError:
                s = QApplication::translate("Utils", "Impossible de télécharger ");
                break;

            case PackageError::ScriptException:
                s = QApplication::translate("Utils", "Erreur dans le QtScript ");
                break;
                
            case PackageError::SignatureError:
                s = QApplication::translate("Utils", "Mauvaise signature GPG du fichier ");
                break;

            case PackageError::SHAError:
                s = QApplication::translate("Utils", "Mauvaise somme SHA1, fichier corrompu : ");
                break;

            case PackageError::PackageNotFound:
                s = QApplication::translate("Utils", "Paquet inexistant : ");
                break;

            case PackageError::BadDownloadType:
                s = QApplication::translate("Utils", "Mauvais type de téléchargement, vérifier sources.list : ");
                break;

            case PackageError::OpenDatabaseError:
                s = QApplication::translate("Utils", "Impossible d'ouvrir la base de donnée : ");
                break;

            case PackageError::QueryError:
                s = QApplication::translate("Utils", "Erreur dans la requête : ");
                break;

            case PackageError::SignError:
                s = QApplication::translate("Utils", "Impossible de vérifier la signature : ");
                break;
                
            case PackageError::InstallError:
                s = QApplication::translate("Utils", "Impossible d'installer le paquet ");
                break;

            case PackageError::ProgressCanceled:
                s = QApplication::translate("Utils", "Opération annulée : ");
                break;
        }
    }
    
    s += err->info;
    
    if (!err->more.isEmpty())
    {
        s += "<br /><br />";
        s += err->more;
    }
    
    QMessageBox::critical(QApplication::activeWindow(), QApplication::translate("Utils", "Erreur"), s);
}

QWidget *Utils::choiceWidget(QWidget *parent, 
                             bool first, 
                             const QString &downloadString, 
                             const QString &installString, 
                             const QString &shortDesc, 
                             const QString &name, 
                             const QString &version, 
                             const QString &updateVersion, 
                             Solver::Action action, 
                             int flags)
{
    QWidget *widget = new QWidget(parent);
    QLabel *lblActionIcon = new QLabel(widget);
    QLabel *lblActionString = new QLabel(widget);
    QFrame *vline = new QFrame(widget);
    QLabel *lblShortDesc = new QLabel(widget);
    QLabel *dlIcon = new QLabel(widget);
    QLabel *dlString = new QLabel(widget);
    QLabel *instIcon = new QLabel(widget);
    QLabel *instString = new QLabel(widget);
    
    QGridLayout *layout = new QGridLayout(widget);
    widget->setLayout(layout);
    
    // Propriétés
    lblActionIcon->resize(22, 22);
    lblActionIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    
    lblActionString->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    
    dlIcon->resize(22, 22);
    dlIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    dlIcon->setPixmap(QIcon::fromTheme("download").pixmap(22, 22));
    
    dlString->setText(downloadString);
    dlString->setMinimumSize(QSize(100, 0));
    
    instIcon->resize(22, 22);
    instIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    instIcon->setPixmap(QIcon(":/images/repository.png").pixmap(22, 22));
    
    instString->setText(installString);
    instString->setMinimumSize(QSize(100, 0));
    
    vline->setFrameShape(QFrame::VLine);
    
    lblShortDesc->setText(shortDesc);
    lblShortDesc->setWordWrap(true);
    lblShortDesc->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    
    switch (action)
    {
        case Solver::Install:
            lblActionIcon->setPixmap(QIcon(":/images/pkg-install.png").pixmap(22, 22));
            lblActionString->setText(InstallWizard::tr("Installation de <b>%1</b>~%2").arg(name, version));
            break;
        case Solver::Remove:
            lblActionIcon->setPixmap(QIcon(":/images/pkg-remove.png").pixmap(22, 22));
            lblActionString->setText(InstallWizard::tr("Suppression de <b>%1</b>~%2").arg(name, version));
            break;
        case Solver::Purge:
            lblActionIcon->setPixmap(QIcon(":/images/pkg-purge.png").pixmap(22, 22));
            lblActionString->setText(InstallWizard::tr("Suppression totale de <b>%1</b>~%2").arg(name, version));
            break;
        case Solver::Update:
            lblActionIcon->setPixmap(QIcon(":/images/pkg-update.png").pixmap(22, 22));
            lblActionString->setText(InstallWizard::tr("Mise à jour de <b>%1</b>~%2 vers %3").arg(name, version, updateVersion));
            break;
        default:
            break;
    }
    
    // Ajouter les widgets au layout
    int c = 0;
    
    if (first)
    {
        // On aura des choix en-dessous, bien les séparer pour que l'utilisateur le voie
        QFrame *hline = new QFrame(widget);
        hline->setFrameShape(QFrame::HLine);
        layout->addWidget(hline, 0, 0, 1, 7);
        
        c++;
    }
    
    layout->addWidget(lblActionIcon, c, 0);
    layout->addWidget(lblActionString, c, 2);
    layout->addWidget(dlIcon, c, 3);
    layout->addWidget(dlString, c, 4);
    layout->addWidget(instIcon, c, 5);
    layout->addWidget(instString, c, 6);
    
    layout->addWidget(vline, c + 1, 1);
    
    if (flags & PACKAGE_FLAG_NEEDSREBOOT || flags & PACKAGE_FLAG_EULA)
    {
        layout->addWidget(lblShortDesc, c + 1, 2);
        
        if (flags & PACKAGE_FLAG_NEEDSREBOOT)
        {
            QLabel *lblRebootIcon = new QLabel(widget);
            QLabel *lblRebootString = new QLabel(widget);
            
            lblRebootIcon->resize(22, 22);
            lblRebootIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            lblRebootIcon->setPixmap(QIcon::fromTheme("system-reboot").pixmap(22, 22));
            
            lblRebootString->setText(InstallWizard::tr("Redémarrage"));
            
            layout->addWidget(lblRebootIcon, c + 1, 3, Qt::AlignTop);
            layout->addWidget(lblRebootString, c + 1, 4, Qt::AlignTop);
        }
        
        if (flags & PACKAGE_FLAG_EULA)
        {
            QLabel *lblEulaIcon = new QLabel(widget);
            QLabel *lblEulaString = new QLabel(widget);
            
            lblEulaIcon->resize(22, 22);
            lblEulaIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            lblEulaIcon->setPixmap(QIcon::fromTheme("mail-signed").pixmap(22, 22));
            
            lblEulaString->setText(InstallWizard::tr("Licence à approuver"));
            
            layout->addWidget(lblEulaIcon, c + 1, 5, Qt::AlignTop);
            layout->addWidget(lblEulaString, c + 1, 6, Qt::AlignTop);
        }
    }
    else
    {
        layout->addWidget(lblShortDesc, c + 1, 2, 1, 5);
    }
    
    return widget;
}
