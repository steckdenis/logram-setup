/*
 * categorydrawer.cpp
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

#include "categorydrawer.h"

#include <QFontMetrics>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QLinearGradient>

#include <QtDebug>

#define BORDER 2
#define ROUNDED 5

CategoryDrawer::CategoryDrawer(QWidget* parent): QWidget(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
}

void CategoryDrawer::setText(const QString& _text)
{
    text = _text;
    repaint();
}

void CategoryDrawer::setIcon(const QIcon& _icon)
{
    icon = _icon;
    repaint();
}

QSize CategoryDrawer::minimumSizeHint() const
{
    QFontMetrics fnt(font());
    
    int height = qMax(fnt.height(), 22) + 2 * BORDER;      // Marges, 22 = hauteur de l'icône
    
    return QSize(1, height);
}

QSize CategoryDrawer::sizeHint() const
{
    return minimumSizeHint();
}

void CategoryDrawer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    
    const QPalette &pal = palette();
    QColor textcolor = pal.color(QPalette::Text);
    QColor border = pal.color(QPalette::Background);
    QColor gradcolor(230, 230, 230);
    
    // Effacer avant de dessiner
    painter.setClipRect(event->rect());
    painter.fillRect(event->rect(), Qt::transparent);
    
    // Dessiner le dégradé
    qreal h = qreal(height());
    qreal w = qreal(width());
    
    QLinearGradient grad(0.0, 0.0, (h / w) * h, h - (h / w));
    grad.setColorAt(0.0, gradcolor);
    grad.setColorAt(1.0, QColor(0, 0, 0, 0));
    
    painter.setPen(QColor(0, 0, 0, 0));
    painter.setBrush(grad);
    painter.drawRoundedRect(0, 0, width(), height(), ROUNDED, ROUNDED);
    
    // Dessiner les bordues
    grad.setStart(ROUNDED, 0.0);
    grad.setFinalStop(float(width() - ROUNDED), 0.0);
    grad.setColorAt(0.0, border);
    
    painter.setPen(QPen(grad, 1.0));
    painter.drawLine(ROUNDED, 0, width(), 0);
    
    grad.setStart(0, ROUNDED);
    grad.setFinalStop(0, float(height()));
    
    painter.setPen(QPen(grad, 1.0));
    painter.drawLine(0, ROUNDED, 0, height());
    
    painter.setPen(border);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawArc(0, 0, 2 * (ROUNDED + 1), 2 * (ROUNDED + 1), 90 * 16, 90 * 16);
    
    // Dessiner l'icône
    QPixmap pix = icon.pixmap(22, 22);
    painter.drawPixmap(BORDER, BORDER, pix);
    
    // Dessigner le texte
    QFont fnt = font();
    fnt.setBold(true);
    
    painter.setFont(fnt);
    painter.setPen(textcolor);
    painter.drawText(BORDER + 22 + BORDER, 
                     BORDER, 
                     width() - BORDER - 22 - BORDER - BORDER, 
                     height() - BORDER - BORDER,
                     Qt::AlignVCenter | Qt::TextSingleLine, 
                     text);
}
