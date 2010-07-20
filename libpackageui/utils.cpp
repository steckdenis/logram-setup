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
#include "markdown/markdown.h"

#include <QImage>
#include <QApplication>
#include <QMessageBox>

#include <packagesystem.h>

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
    std::string s = source.toStdString();
    
    markdown::Document doc(4);
    doc.read(s);
    
    std::stringstream str;
    doc.write(str);
    
    return QString::fromStdString(str.str());
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
