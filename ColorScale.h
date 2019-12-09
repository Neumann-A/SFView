/*
 * SF Viewer - A program to visualize MPI system matrices
 * Copyright (C) 2014-2017  Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Id: ChangeList.h 58 2016-08-24 13:03:50Z uhei $
 */

#ifndef COLORSCALE_H
#define COLORSCALE_H


// Std headers
#include <complex>

// Qt headers
#include <QObject>
#include <QColor>
#include <QImage>
#include <QString>
#include <QVector>

// Forward declarations
class QIconEngine;
class ColorScaleManager;

class ColorScale : public QObject
{
    Q_OBJECT
public:
    virtual ~ColorScale();
    const QString & name() const;
    virtual QColor color(const std::complex<double> & value) const;
    virtual void drawLegend(QPainter *, const QRect & rect, double min, double max) const;
protected:
    QIconEngine * createIconEngine();
    static QList<ColorScale *> defaultColorScales();
    ColorScale(const QString & name, const QVector<QColor> & colors, QObject * parent=0);
    ColorScale(const QString & name, QObject * parent=0);
private:
    friend class ColorScaleManager;
    struct Impl;
    Impl * d;
};

#endif // COLORSCALE_H
