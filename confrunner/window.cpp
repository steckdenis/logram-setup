/*
 * window.cpp
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

#include "window.h"

#include <QtDebug>

#define FILENAME "/etc/laptop-mode/laptop-mode.conf"

Window::Window(const QString &_specFile)
{
    QString locale = QLocale::system().name().section("_", 0, 0);
    specFile = _specFile;
    specFile.replace("_LANG_", locale);
    
    if (!QFile::exists(specFile))
    {
        specFile = _specFile;
        specFile.replace("_LANG_", "en");
    }
    
    //Initialiser l'interface
    initInterface();
    
    //Charger les valeurs (en différé)
    QTimer::singleShot(0, this, SLOT(resetData()));
}

Window::~Window()
{
    fl->close();
    delete fl;
}

void Window::initInterface()
{
    //Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    this->setLayout(mainLayout);
    
    tb = new QTabWidget(this);
    mainLayout->addWidget(tb);
    
    //Boutons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOk, *btnApply, *btnCancel, *btnReset;
    
    btnOk = new QPushButton(tr("&Ok"), this);
    btnApply = new QPushButton(tr("&Apply"), this);
    btnCancel = new QPushButton(tr("&Cancel"), this);
    btnReset = new QPushButton(tr("&Reset"), this);
    
    connect(btnOk, SIGNAL(clicked(bool)), this, SLOT(okData()));
    connect(btnApply, SIGNAL(clicked(bool)), this, SLOT(applyData()));
    connect(btnCancel, SIGNAL(clicked(bool)), qApp, SLOT(quit()));
    connect(btnReset, SIGNAL(clicked(bool)), this, SLOT(resetData()));
    
    btnLayout->addWidget(btnReset);
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnApply);
    btnLayout->addWidget(btnCancel);
    
    mainLayout->addLayout(btnLayout);
    
    //Fichier de spécifications
    QFile *sfl = new QFile(specFile);
    if (!sfl->open(QIODevice::ReadOnly)) return;
    
    doc = new QDomDocument();
    doc->setContent(sfl);
    QDomElement el = doc->documentElement();
    this->setWindowTitle(el.attribute("title", el.attribute("configfile")));
    
    //Fichier
    fl = new QFile(el.attribute("configfile"));
   
    if (!fl->open(QIODevice::ReadWrite))
    {
        return;
    }
    
    //Explorer les onglets
    QDomElement tab = el.firstChildElement("tab");
    
    while (!tab.isNull())
    {
        QWidget *mtab = new QWidget(this);
        
        tb->addTab(mtab, tab.attribute("title"));
        
        initWidget(mtab, tab);
        
        tab = tab.nextSiblingElement("tab");
    }
}

void Window::initWidget(QWidget *wg, const QDomElement &el)
{
    //Créer le layout
    bool isFormLayout = false;
    QString ly = el.attribute("layout", "form");
    QFormLayout *formLayout;
    QBoxLayout *boxLayout;
    
    if (ly == "form")
    {
        isFormLayout = true;
        formLayout = new QFormLayout(wg);
        wg->setLayout(formLayout);
    }
    else if (ly == "vertical")
    {
        boxLayout = new QVBoxLayout(wg);
        wg->setLayout(boxLayout);
    }
    else if (ly == "horizontal")
    {
        boxLayout = new QHBoxLayout(wg);
        wg->setLayout(boxLayout);
    }
    else
    {
        qFatal("Unknown layout");
        return;
    }
    
    //Parser la chaine et ajouter les widgets
    QDomElement w = el.firstChildElement("widget");
    
    while (!w.isNull())
    {
        QWidget *widget;
        QString rdProp;
        bool isWidget = true;
        
        QString type = w.attribute("type");
        QString label = w.attribute("label");
        QString confProp = w.attribute("config");
        QString limit = w.attribute("limit");
        
        if (type == "spin")
        {
            QSpinBox *sp = new QSpinBox(wg);
            
            if (!limit.isEmpty()) sp->setMaximum(limit.toInt());
            
            widget = sp;
            rdProp = "value";
        }
        else if (type == "line")
        {
            QLineEdit *ln = new QLineEdit(wg);
            
            if (!limit.isEmpty()) ln->setMaxLength(limit.toInt());
            
            widget = ln;
            rdProp = "text";
        }
        else if (type == "text")
        {
            QTextEdit *txt = new QTextEdit(wg);
            
            widget = txt;
            rdProp = "plainText";
        }
        else if (type == "slider")
        {
            QSlider *sl = new QSlider(Qt::Horizontal, wg);
            
            if (!limit.isEmpty()) sl->setMaximum(limit.toInt());
            
            widget = sl;
            rdProp = "value";
        }
        else if (type == "check")
        {
            QCheckBox *ck = new QCheckBox(wg);
            
            if (!isFormLayout)
            {
                ck->setText(label);
            }
            
            widget = ck;
            rdProp = "checked";
        }
        else if (type == "combo")
        {
            QComboBox *cbo = new QComboBox(wg);
            
            cbo->setEditable(false);
            
            //Pas de limite, mais une suite de valeurs
            QStringList values = w.attribute("values").split(';', QString::SkipEmptyParts);
            
            foreach (const QString &value, values)
            {
                cbo->addItem(value);
            }
            
            widget = cbo;
            rdProp = "currentText";
        }
        else if (type == "separator")
        {
            QFrame *frm = new QFrame(wg);
            
            frm->setFrameShape(QFrame::HLine);
            
            widget = frm;
            isWidget = false;
        }
        else if (type == "group")
        {
            QGroupBox *grp = new QGroupBox(label, wg);
            
            initWidget(grp, w);
            
            widget = grp;
            isWidget = false;
        }
        else if (type == "widget")
        {
            QWidget *wd = new QWidget(wg);
            
            initWidget(wd, w);
            
            widget = wd;
            isWidget = false;
        }
        else
        {
            qFatal("Unknown widget type");
            return;
        }
        
        //Ajouter
        if (isFormLayout)
        {
            if (isWidget)
            {
                formLayout->addRow(label, widget);
            }
            else
            {
                formLayout->addRow(widget);
            }
        }
        else
        {
            boxLayout->addWidget(widget);
        }
        
        if (isWidget)
        {
            widgets.append(widget);
            widget->setProperty("ConfigProperty", confProp);
            widget->setProperty("ReadProperty", rdProp);
        }
        
        w = w.nextSiblingElement("widget");
    }
}

QString Window::configValue(const QString &key)
{
    //Lire le fichier
    fl->seek(0);
    
    while (!fl->atEnd())
    {
        QString ln = fl->readLine().trimmed();
        
        if (ln.contains("#") || ln.length() < 3) continue;
        
        if (key == ln.section("=", 0, 0))
        { 
            return ln.section("=", 1, 1);
        }
    }
    
    return QString();
}

void Window::setConfigValue(const QString &key, const QString &value)
{
    QString vl = value;
    
    if (vl == "true") vl = "1";
    if (vl == "false") vl = "0";
    
    //Lire chaque ligne du fichier et si c'est la bonne, la modifier
    QString buffer;
    
    fl->seek(0);
    
    while (!fl->atEnd())
    {
        QString ln = fl->readLine().trimmed();
        
        if (key == ln.section("=", 0, 0))
        {
            buffer += key + "=" + vl + "\n";
        }
        else
        {
            buffer += ln + "\n";
        }
    }
    
    //Ecrire le buffer
    fl->seek(0);
    
    fl->write(buffer.toUtf8());
    fl->flush();
}

void Window::resetData()
{   
    //Lire les données du fichier et les placer dans les controles
    foreach (QWidget *wg, widgets)
    {
        QString confProp = wg->property("ConfigProperty").toString();
        QString rdProp = wg->property("ReadProperty").toString();
        
        QString value = configValue(confProp);
        
        wg->setProperty(qPrintable(rdProp), value);
    }
}

void Window::applyData()
{
    foreach (QWidget *wg, widgets)
    {
        QString confProp = wg->property("ConfigProperty").toString();
        QString rdProp = wg->property("ReadProperty").toString();
        
        QString value = wg->property(qPrintable(rdProp)).toString();
        
        setConfigValue(confProp, value);
    }
}

void Window::okData()
{
    applyData();
    close();
}