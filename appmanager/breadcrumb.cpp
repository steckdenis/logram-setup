/*
 * mainwindow.h
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

#include "breadcrumb.h"

#include <QHBoxLayout>
#include <QAbstractButton>
#include <QPushButton>
#include <QVector>

#include <QPainter>
#include <QPalette>
#include <QLinearGradient>
#include <QPolygonF>
#include <QColor>

#include <QtDebug>

class Separator : public QWidget
{
    public:
        Separator(QWidget *parent) : QWidget(parent)
        {
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        }
        
        QSize sizeHint() const;
        QSize minimumSizeHint() const;
        
    protected:
        void paintEvent(QPaintEvent *event);
};

struct Breadcrumb::Private
{
    QHBoxLayout *layout;
    QVector<QAbstractButton *> buttons;
    QVector<Separator *> separators;
    Breadcrumb *b;
    
    QAbstractButton *button(const QString &text, const QIcon &icon);
    void appendButton(QAbstractButton *btn);
    void insertButton(int index, QAbstractButton *btn);
};

Breadcrumb::Breadcrumb(QWidget* parent): QWidget(parent)
{
    d = new Private;
    
    d->b = this;
    d->layout = new QHBoxLayout(this);
    d->layout->setContentsMargins(0, 0, 0, 0);
}

Breadcrumb::~Breadcrumb()
{
    delete d;
}

void Breadcrumb::addButton(const QString& text)
{
    d->appendButton(d->button(text, QIcon()));
}

void Breadcrumb::addButton(const QIcon& icon, const QString& text)
{
    d->appendButton(d->button(text, icon));
}

void Breadcrumb::addButton(QAbstractButton* button)
{
    d->appendButton(button);
}

void Breadcrumb::insertButton(int index, const QString& text)
{
    d->insertButton(index, d->button(text, QIcon()));
}

void Breadcrumb::insertButton(int index, const QIcon& icon, const QString& text)
{
    d->insertButton(index, d->button(text, icon));
}

void Breadcrumb::insertButton(int index, QAbstractButton* button)
{
    d->insertButton(index, button);
}

QAbstractButton* Breadcrumb::button(int index)
{
    return d->buttons.at(index);
}

void Breadcrumb::removeButton(int index)
{
    d->layout->removeWidget(d->buttons.at(index));
    d->buttons.remove(index);
    
    Separator *sep = d->separators.at(index);
    
    if (sep != 0)
    {
        d->layout->removeWidget(d->separators.at(index));
    }
    
    d->separators.remove(index);
}

void Breadcrumb::Private::appendButton(QAbstractButton* btn)
{
    if (buttons.count() != 0)
    {
        Separator *sep = new Separator(b);
        separators.append(sep);
        layout->addWidget(sep);
    }
    else
    {
        separators.append(0);
    }
    
    buttons.append(btn);
    layout->addWidget(btn);
}

void Breadcrumb::Private::insertButton(int index, QAbstractButton* btn)
{
    if (index != 0)
    {
        Separator *sep = new Separator(b);
        separators.insert(index, sep);
        layout->insertWidget((index * 2) - 1, sep);
    }
    else
    {
        separators.insert(index, 0);
    }
    
    buttons.insert(index, btn);
    layout->insertWidget(index * 2, btn);
}

QAbstractButton* Breadcrumb::Private::button(const QString& text, const QIcon& icon)
{
    QPushButton *rs = new QPushButton(b);
    
    rs->setText(text);
    if (!icon.isNull()) rs->setIcon(icon);
    
    rs->setFlat(true);
    
    return rs;
}

/////////////////////////////////////////

#define GRADLENGTH 8

QSize Separator::sizeHint() const
{
    return minimumSizeHint();
}

QSize Separator::minimumSizeHint() const
{
    return QSize(10 + GRADLENGTH, 0);
}

void Separator::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    QColor dark = palette().color(QPalette::Dark);
    QColor mid = palette().color(QPalette::Mid);
    QColor light = palette().color(QPalette::Light);
    QColor transp(0, 0, 0, 0);
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Dessiner la ligne claire en V
    QPolygonF vpoly;
    
    vpoly.append(QPointF(0.0, 0.0));
    vpoly.append(QPointF(float(width() - GRADLENGTH - 1), float(height()) / 2.0));
    vpoly.append(QPointF(0.0, float(height())));
    
    painter.setPen(light);
    painter.drawPolyline(vpoly);
    
    // Dessiner le dégradé supérieur
    QLinearGradient grad;
    QPolygonF gradpoly;
    
    gradpoly.append(vpoly.at(0));
    gradpoly.append(vpoly.at(1));
    
    gradpoly.translate(1.0, 0.0);
    
    gradpoly.append(QPointF(width(), float(height()) / 2.0));
    gradpoly.append(QPointF(GRADLENGTH + 1, 0.0));
    
    // Dégradé de cette partie
    float desc = vpoly.at(1).y() / vpoly.at(1).x();
    float cy = float(GRADLENGTH) / desc;
    
    grad.setStart(0, cy);
    grad.setFinalStop(GRADLENGTH, 0.0);
    
    grad.setColorAt(0.0, mid);
    grad.setColorAt(0.8, transp);
    
    painter.setPen(transp);
    painter.setBrush(grad);
    painter.drawConvexPolygon(gradpoly);
    
    // Dessigner le dégradé inférieur
    gradpoly.clear();
    
    gradpoly.append(vpoly.at(1));
    gradpoly.append(vpoly.at(2));
    
    gradpoly.translate(1.0, 0.0);
    
    gradpoly.append(QPointF(GRADLENGTH + 1, height()));
    gradpoly.append(QPointF(width(), float(height()) / 2.0));
    
    // Dégradé de cette partie
    cy = float(height()) - cy;
    
    grad.setStart(0, cy);
    grad.setFinalStop(GRADLENGTH, height());
    
    painter.setBrush(grad);
    painter.drawConvexPolygon(gradpoly);
    
    /*QLinearGradient grad;
    
    // Partie supérieure
    grad.setStart(0.0, 0.0);
    grad.setFinalStop(vpoly.at(1));
    
    grad.setColorAt(0.0, mid);
    grad.setColorAt(1.0, transp); */
    
    // Dessiner la ligne foncée en V
    vpoly.translate(1.0, 0.0);
    
    painter.setPen(dark);
    painter.drawPolyline(vpoly);
}

#include "breadcrumb.moc"
