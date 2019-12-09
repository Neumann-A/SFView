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
 * $Id: SpectralPlot.h 77 2017-03-03 17:20:27Z uhei $
 */

#ifndef SPECTRALPLOT_H
#define SPECTRALPLOT_H

// Qt includes
#include <QWidget>

// Local includes
#include "SystemMatrix.h"

class SpectralPlot : public QWidget
{
    Q_OBJECT
public:
    explicit SpectralPlot(QWidget *parent = 0);
    virtual ~SpectralPlot();
    const SystemMatrix * systemMatrix() const;
signals:
    void globalIndexSelect(int globalIndex);
public slots:
    void setSystemMatrix( const SystemMatrix * s );
    void highlightGlobalIndex(int globalIndex);
private slots:
    void selectPoint(const QPointF &);
    void setTraceVisible(bool);
    void zoomIn();
    void zoomOut();
    void resetZoom();
private:
    struct Impl;
    Impl * d;
};

#endif // SPECTRALPLOT_H
