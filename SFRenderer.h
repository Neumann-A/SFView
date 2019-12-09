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
 * $Id: SFRenderer.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef SFRENDERER_H
#define SFRENDERER_H

// Qt includes
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtGui/QImage>

// Forward declarations
class ColorScale;
class ColorScaleManager;
class SystemMatrix;
class QPainter;
class QRect;

class SFRenderer : public QObject
{
    Q_OBJECT
public:
    enum Colorization { PerSlice, PerFrame };
    explicit SFRenderer(QObject* parent = 0);
    virtual ~SFRenderer();
    SystemMatrix * systemMatrix() const;
    Qt::Axis horizontalAxis() const;
    Qt::Axis verticalAxis() const;
    Qt::Axis sliceDirection() const;
    bool backgroundCorrection() const;
    QImage image ( int globalIndex, int slice, Colorization colorScale=PerFrame );
    void plotLegend( QPainter *, const QRect &, int globalIndex, int slice=-1 );
public slots:
    void setSystemMatrix (SystemMatrix * systemMatrix);
    void setAxes(Qt::Axis horizontal, Qt::Axis vertical);
    void setBackgroundCorrection(bool);
private:
    struct Impl;
    Impl * d;
};

#endif // SFRENDERER_H
