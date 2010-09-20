/*
 * mainwindow.cpp
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

#include "app.h"
#include "config.h"

#include <QMessageBox>
#include <QFile>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QDialogButtonBox>

#include <communication.h>
#include <packagesystem.h>
#include <databasereader.h>

#include <communicationdialog.h>
#include <installwizard.h>
#include <utils.h>
#include <installwizard.h>

#include <QtDebug>

using namespace Logram;
using namespace LogramUi;

App::App(int &argc, char **argv) : QApplication(argc, argv)
{
    _error = false;
    ps = 0;
    
    // Vérifier les paramètres
    if (arguments().count() < 2)
    {
        _error = true;
        return;
    }
    
    // Initialiser la gestion des paquets
    ps = new PackageSystem(this);
    
    ps->loadConfig();
    connect(ps, SIGNAL(communication(Logram::Package *, Logram::Communication *)), 
            this, SLOT(communication(Logram::Package *, Logram::Communication *)));
    
    if (!ps->init())
    {
        Utils::packageSystemError(ps);
        _error = true;
        return;
    }
}

App::~App()
{
    delete ps;
}

bool App::error() const
{
    return _error;
}

bool App::launch()
{
    // Ouvrir le fichier
    QFile fl(arguments().at(1));
    
    if (!fl.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, tr("Impossible d'ouvrir le fichier"), tr("Impossible d'ouvrir le fichier %1").arg(fl.fileName()));
        return false;
    }
    
    // Assistant d'installation
    InstallWizard wizard(ps, 0);
        
    wizard.setWindowIcon(QIcon(":/images/icon.svg"));

    // Lire chaque ligne du fichier
    QByteArray line;
    
    while (!fl.atEnd())
    {
        line = fl.readLine().trimmed();
        HandleType type;
        
        if (line.startsWith("InstallProvides"))
        {
            line.remove(0, 15);
            type = Provides;
        }
        else if (line.startsWith("InstallPackages"))
        {
            line.remove(0, 15);
            type = Packages;
        }
        else if (line.startsWith("InstallFiles"))
        {
            line.remove(0, 12);
            type = Files;
        }
        else
        {
            continue;
        }
        
        // Séparer la ligne en ce qui est à gauche (filtre éventuel) et à droite (paquet) du égal
        int indexOfEqual = line.indexOf('=');
        bool reject = false;
        
        QByteArray value = line;
        value.remove(0, indexOfEqual + 1);
        
        if (indexOfEqual != 0)
        {
            // Filtre avant le égal
            QByteArray filter = line;
            filter.resize(indexOfEqual);
            
            if (filter.at(0) != '(' || filter.at(filter.length() - 1) != ')')
            {
                // Fichier mal formatté
                QMessageBox::critical(0, tr("Fichier mal formatté"), tr("Le fichier de catalogue que vous essayez d'installer présente des erreurs !"));
                return false;
            }
            
            // Supprimer les parenthèses
            filter.remove(0, 1);
            filter.resize(filter.length() - 1);
            
            // Splitter en fonction des ; 
            QList<QByteArray> parts = filter.split(';');
            
            if (parts.at(0) != "logram")
            {
                // Pas destiné à Logram
                reject = true;
            }
            else if (parts.count() >= 2 && !ps->matchVersion(VERSION, parts.at(1), Depend::GreaterOrEqual))
            {
                reject = true;
            }
            else if (parts.count() >= 3 && parts.at(2) != SETUP_ARCH)
            {
                reject = true;
            }
        }
        
        // Si on doit rejeter la ligne, le faire
        if (reject)
        {
            continue;
        }
        
        // Trouver la liste des paquets correspondant à la valeur
        QList<QByteArray> elements = value.split(';');
        
        foreach (const QByteArray &element, elements)
        {
            QVector<DatabasePackage *> pkgs;
            
            if (type == Packages)
            {
                QVector<int> packages = ps->packagesByVString(QString::fromLatin1(element), QString(), Depend::NoVersion);
                
                if (packages.count() == 0)
                {
                    QMessageBox::critical(0, tr("Impossible de trouver un paquet"), tr("Impossible de trouver le paquet correspondant à %1").arg(QString::fromLatin1(element)));
                    return false;
                }
                
                int installedPackage = -1;
                
                foreach (int i, packages)
                {
                    DatabasePackage *p = ps->package(i);
                    
                    if (p->flags() & Package::Installed)
                    {
                        installedPackage = i;
                        break;
                    }
                    
                    pkgs.append(p);
                }
                
                // Si un des paquets est déjà installé, c'est celui-là le meilleur
                if (installedPackage != -1)
                {
                    pkgs.clear();
                    pkgs.append(ps->package(installedPackage));
                }
            }
            else if (type == Files)
            {
                QVector<PackageFile *> files = ps->files(element);
                
                if (files.count() == 0)
                {
                    QMessageBox::critical(0, tr("Impossible de trouver un fichier"), tr("Impossible de trouver le paquet contenant le fichier %1").arg(QString::fromLatin1(element)));
                    return false;
                }
                
                foreach (PackageFile *file, files)
                {
                    DatabaseFile *df = static_cast<DatabaseFile *>(file);
                    
                    df->setPackageBinded(false);
                    pkgs.append(static_cast<DatabasePackage *>(df->package()));
                    
                    delete df;
                }
            }
            else if (type == Provides)
            {
                DatabaseReader *dr = ps->databaseReader();
                
                // Traitement O(1) dans la base de donnée, peut être long
                int str = dr->string(false, element);
                QVector<int> packages = dr->packagesOfString(0, str, Depend::NoVersion);
                
                if (packages.count() == 0)
                {
                    QMessageBox::critical(0, tr("Impossible de trouver un paquet"), tr("Impossible de trouver le paquet fournissant %1").arg(QString::fromLatin1(element)));
                    return false;
                }
                
                foreach (int i, packages)
                {
                    pkgs.append(ps->package(i));
                }
            }
            
            // Si pkgs.count() > 1, proposer le choix
            DatabasePackage *pkg;
            
            if (pkgs.count() == 1)
            {
                pkg = pkgs.at(0);
            }
            else
            {
                QDialog *dialog = new QDialog(0);
                QVBoxLayout *layout = new QVBoxLayout(dialog);
                QLabel *lbl = new QLabel(dialog);
                QListWidget *list = new QListWidget(dialog);
                QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
                
                dialog->setWindowTitle(tr("Sélectionner un paquet"));
                dialog->setWindowIcon(QIcon(":/images/icon.svg"));
                dialog->setLayout(layout);
                
                lbl->setText(tr("Une des opérations demandée par le catalogue vous permet de choisir le paquet le plus approprié. Voici une liste de ceux disponibles :"));
                
                layout->addWidget(lbl);
                layout->addWidget(list);
                layout->addWidget(box, 0, Qt::AlignRight);
                
                connect(box, SIGNAL(accepted()), dialog, SLOT(accept()));
                connect(box, SIGNAL(rejected()), dialog, SLOT(reject()));
                
                foreach (DatabasePackage *p, pkgs)
                {
                    QListWidgetItem *item = new QListWidgetItem(list);
                    list->addItem(item);
                    
                    QWidget *widget = Utils::choiceWidget(
                        dialog,
                        false,
                        ps->fileSizeFormat(p->downloadSize()),
                        ps->fileSizeFormat(p->installSize()),
                        p->shortDesc(),
                        p->name(),
                        p->version(),
                        QString(),
                        Solver::Install,
                        p->flags()
                    );
                    
                    item->setSizeHint(widget->minimumSizeHint());
                    list->setItemWidget(item, widget);
                }
                
                list->setCurrentRow(0);
                
                // Afficher la boîte de dialogue
                dialog->exec();
                
                if (dialog->result() == QDialog::Rejected)
                {
                    delete dialog;
                    return false;
                }
                
                pkg = pkgs.at(list->currentRow());
                
                delete dialog;
            }
            
            // Ajouter le paquet à la liste globale
            pkg->setAction(Solver::Install);
            wizard.addPackage(pkg);
        }
    }
    
    // Installer les paquets
    wizard.exec();
    
    return (wizard.result() == QDialog::Accepted);
}

void App::communication(Logram::Package *sender, Logram::Communication *comm)
{
    if (comm->type() != Communication::Question)
    {
        // Les messages sont gérés par InstallWizard
        return;
    }
    
    CommunicationDialog dialog(sender, comm, 0);
    
    dialog.exec();
}

#include "app.moc"
