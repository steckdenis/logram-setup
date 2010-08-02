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

#include "mainwindow.h"
#include "breadcrumb.h"
#include "introdisplay.h"
#include "packagelist.h"

#include <categoryview.h>
#include <utils.h>
#include <filterinterface.h>
#include <infopane.h>
#include <searchbar.h>
#include <communicationdialog.h>
#include <progressdialog.h>

#include <communication.h>
#include <packagemetadata.h>

#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QDate>
#include <QTimer>
#include <QDir>
#include <QScrollArea>

#include <QtXml>
#include <QCheckBox>

using namespace LogramUi;
using namespace Logram;

MainWindow::MainWindow() : QMainWindow(0)
{
    _error = false;
    
    setupUi(this);
    
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setDockNestingEnabled(true);
    statusBar();
    
     // Initialiser la gestion des paquets
    ps = new PackageSystem(this);
    
    ps->loadConfig();
    connect(ps, SIGNAL(communication(Logram::Package *, Logram::Communication *)), 
            this, SLOT(communication(Logram::Package *, Logram::Communication *)));
    connect(ps, SIGNAL(progress(Logram::Progress *)),
            this, SLOT(progress(Logram::Progress *)));
    
    if (!ps->init())
    {
        Utils::packageSystemError(ps);
        _error = true;
        return;
    }
    
    // Construire l'interface
    setWindowIcon(QIcon::fromTheme("applications-other"));
    
    btnHistory->setIcon(QIcon::fromTheme("go-previous"));
    btnChanges->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    btnUpdate->setIcon(QIcon(":/images/repository.png"));
    
    // Filtres
    filterInterface = new FilterInterface(this);
    
    // Sections
    sections = new CategoryView(ps, filterInterface, docSections);
    
    docSections->setWidget(sections);
    
    // Informations
    infoPane = new InfoPane(this);
    
    docInfos->setWidget(infoPane);
    
    // Recherche
    searchBar = new SearchBar(filterInterface, this);
    searchBar->layout()->setContentsMargins(0, 0, 0, 0);
    
    mainLayout->insertWidget(0, searchBar);
    
    // Fil d'ariane
    breadcrumb = new Breadcrumb(this);
    
    breadcrumb->addButton(QIcon::fromTheme("go-home"), tr("Accueil"));
    
    breadcrumbLayout->insertWidget(2, breadcrumb);
    
    // Affichage de l'introduction
    QScrollArea *scrollIntro = new QScrollArea(this);
    scrollIntro->setWidgetResizable(true);
    
    display = new IntroDisplay(this);
    scrollIntro->setWidget(display);
    
    stack->addWidget(scrollIntro);
    
    // Liste des paquets
    QScrollArea *scrollPackages = new QScrollArea(this);
    scrollPackages->setWidgetResizable(true);
    
    listPackages = new ::PackageList(this);
    scrollPackages->setWidget(listPackages);
    
    stack->addWidget(scrollPackages);
    
    // Progressions
    progressDialog = new ProgressDialog(this);
    progressDialog->setModal(true);
    
    // Restaurer l'état de la fenêtre
    QSettings settings("Logram", "AppManager");
    
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    // Mettre à jour la base de donnée si nécessaire
    QTimer::singleShot(0, this, SLOT(delayedInit()));
    
    // Signaux
    connect(filterInterface, SIGNAL(dataChanged()), this, SLOT(searchPackages()));
    //connect(listPackages, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(updateInfos()));
    connect(btnUpdate, SIGNAL(clicked(bool)), this, SLOT(updateDatabase()));
    connect(breadcrumb, SIGNAL(buttonPressed(int)), this, SLOT(breadcrumbPressed(int)));
    
    setMode(false);
    searchBar->setFocus();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings("Logram", "AppManager");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    delete listPackages;
    delete ps;
    delete progressDialog;
}

bool MainWindow::error() const
{
    return _error;
}

QVector <MainWindow::RatedPackage> &MainWindow::ratedPackages()
{
    return highestRated;
}

void MainWindow::delayedInit()
{
    // Vérifier s'il y a des choses à mettre à jour.
    QSettings settings("Logram", "AppManager");
    QDate lastUpdate = settings.value("LastUpdateDate").toDate();
    
    if (lastUpdate.isNull() || lastUpdate.daysTo(QDate::currentDate()) > 1)
    {
        // Pas de mise à jour de la liste des paquets depuis plus d'un jour
        updateDatabase();
    }
    else
    {
        readPackages();
    }
    
    // Afficher l'intro
    display->populate();
}

void MainWindow::setMode(bool packageList)
{
    if (!packageList)
    {
        // Cacher ce qu'il faut
        stack->setCurrentIndex(0);
        docInfos->hide();
        btnChanges->hide();
    }
    else
    {
        stack->setCurrentIndex(1);
        docInfos->show();
        btnChanges->show();
    }
}

void MainWindow::searchPackages()
{
    setMode(true);
    
    if (breadcrumb->count() == 1)
    {
        // Première recherche de l'utilisateur, lui ajouter un bouton pour dire où il est
        breadcrumb->addButton(QIcon(":/images/package.png"), tr("Recherche de paquets"));
    }
    else
    {
        // Prendre le bouton à l'index 1 et s'assurer qu'il affiche la bonne chose
        QAbstractButton *btn = breadcrumb->button(1);
        btn->setText(tr("Recherche de paquets"));
        btn->setIcon(QIcon(":/images/package.png"));
    }
}

void MainWindow::breadcrumbPressed(int index)
{
    if (index == 0)
    {
        // On revient à la page d'accueil
        setMode(false);
    }
    else if (index == 1)
    {
        // On affiche la dernière recherche de paquets
        setMode(true);
    }
}

void MainWindow::readPackages()
{
    PackageInfo pkg;
    
    QDir dir(ps->varRoot() + "/var/cache/lgrpkg/db");
    QStringList files = dir.entryList(QDir::Files);
    
    float minRated = 0.0;
    
    foreach (const QString &file, files)
    {
        if (!file.endsWith(".metadata")) continue;
        
        QString filename = dir.absoluteFilePath(file);
        QStringList parts = file.split('_');
        QString repo = parts.at(0);
        QString distro = parts.at(1).section('.', 0, 0);
        
        // Ouvrir le fichier
        QFile fl(filename);
        
        if (!fl.open(QIODevice::ReadOnly))
        {
            continue;
        }
        
        // Lire le contenu
        QDomDocument doc;
        doc.setContent(fl.readAll());
        QDomElement package = doc.documentElement().firstChildElement();
        
        while (!package.isNull())
        {
            QString key = repo + "/" + distro + "/" + package.attribute("name");
            QString primarylang = package.attribute("primarylang");
            
            pkg.title = PackageMetaData::stringOfKey(package.firstChildElement("title"), primarylang);
            pkg.votes = package.attribute("votes").toInt();
            pkg.total_votes = package.attribute("totalvotes").toInt();
            
            // Trouver l'icône
            QDomElement iconElement = package.firstChildElement("icon");
            QByteArray iconData;
            
            if (!iconElement.isNull())
            {
                // Trouver le noeud CDATA
                QDomNode node = iconElement.firstChild();
                QDomCDATASection section;
                
                while (!node.isNull())
                {
                    if (node.isCDATASection())
                    {
                        section = node.toCDATASection();
                        break;
                    }
                    
                    node = node.nextSibling();
                }
                
                if (!section.isNull())
                {
                    iconData = QByteArray::fromBase64(section.data().toAscii());
                }
            }
            
            if (!iconData.isNull())
            {
                pkg.icon = Utils::pixmapFromData(iconData, 32, 32);
            }
            
            // Insérer dans la liste globale
            packages.insert(key, pkg);
            
            // Voir si c'est un paquet qui a le meilleur score
            float score (pkg.total_votes == 0 ? 0 : float(pkg.votes) / float(pkg.total_votes));
            
            if (highestRated.count() < 5)
            {
                RatedPackage r;
                
                r.inf = pkg;
                r.repo = repo;
                r.distro = distro;
                r.score = score;
                
                highestRated.append(r);
                
                minRated = (minRated > score ? minRated : score);
            }
            else
            {
                if (score > minRated)
                {
                    // Éliminer de la liste le paquet avec le score le plus faible
                    int minIndex = 0;
                    float minScore;
                    
                    for (int i=0; i<highestRated.count(); ++i)
                    {
                        const RatedPackage &inf = highestRated.at(i);
                        
                        if (minIndex == -1)
                        {
                            minIndex = i;
                            minScore = inf.score;
                        }
                        else if (inf.score < minScore)
                        {
                            minIndex = i;
                            minScore = inf.score;
                        }
                    }
                    
                    RatedPackage r;
                    
                    r.inf = pkg;
                    r.repo = repo;
                    r.distro = distro;
                    r.score = score;
                    
                    highestRated.replace(minIndex, r);
                    minRated = score;
                }
            }
            
            package = package.nextSiblingElement();
        }
    }
}

void MainWindow::updateDatabase()
{
    // On met à jour la base de donnée binaire
    if (!ps->update(PackageSystem::Minimal | PackageSystem::Sections | PackageSystem::Metadata))
    {
        Utils::packageSystemError(ps);
        return;
    }
    
    if (!ps->reset())
    {
        Utils::packageSystemError(ps);
        return;
    }
    
    sections->reload();
    
    // Relire la liste des paquets
    readPackages();
    
    if (stack->currentIndex() == 1)
    {
        // Réactualiser l'affichage (TODO)
    }
    
    // Mise à jour effectuée
    QSettings settings("Logram", "AppManager");
    settings.setValue("LastUpdateDate", QDate::currentDate());
}

void MainWindow::communication(Logram::Package *sender, Logram::Communication *comm)
{
    if (comm->type() != Communication::Question)
    {
        // Les messages sont gérés par InstallWizard
        return;
    }
    
    CommunicationDialog dialog(sender, comm, this);
    
    dialog.exec();
}

void MainWindow::progress(Progress *progress)
{
    if (progress->action == Progress::Create)
    {
        progressDialog->addProgress(progress);
    }
    else if (progress->action == Progress::Update)
    {
        progressDialog->updateProgress(progress);
        
        if (progressDialog->canceled())
        {
            progress->canceled = true;
        }
    }
    else
    {
        progressDialog->endProgress(progress);
    }
}

#include "mainwindow.moc"
