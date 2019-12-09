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
 * $Id: PlotWidget.h 72 2017-02-28 22:45:42Z uhei $
 */

#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

// Qt Includes
#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QFrame>
#else
#include <QtGui/QFrame>
#endif

// Local includes
#include "SystemMatrix.h"
#include "MatrixPosition.h"

// Forward declarations
class ColorScale;
class ColorScaleManager;

class PlotWidget : public QFrame
{
    Q_OBJECT
public:
    PlotWidget( QWidget * parent=0);
    virtual ~PlotWidget();
    Qt::Axis horizontalAxis() const;
    Qt::Axis verticalAxis() const;
    Qt::Axis sliceDirection() const;
    bool smoothScaling() const;
    bool backgroundCorrection() const;
    SystemMatrix * systemMatrix() const;
    int index() const;
    int visibleSlice() const;
    bool legendEnabled() const;
    bool ticksEnabled() const;
    bool isVoxel(const QPoint & p, MatrixPosition * pos=0) const;
    int slice(const QPoint & p) const;
public slots:
    void setTitle(const QString & text);
    void setSliceDirection(Qt::Axis);
    void setSmoothScaling(bool);
    void setBackgroundCorrection(bool);
    void setGlobalIndex(int);
    void setSystemMatrix(SystemMatrix *);
    void setTicksEnabled(bool);
    void setLegendEnabled(bool);
    void imageToClipboard();
    void showSingleSlice(int);
    void showAllSlices();
    void setHighlightPosition(const MatrixPosition & pos);
signals:
    void currentPositionAndValue(const MatrixPosition & pos, const SystemMatrix::complex & value);
    void requestContextMenu(const QPoint & p, const MatrixPosition & pos);
protected:
    virtual void paintEvent(QPaintEvent* );
    virtual void resizeEvent(QResizeEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void wheelEvent(QWheelEvent *event);
private:
    QString axisName(Qt::Axis axis) const;
    const ColorScaleManager * colorScaleManager();
    const ColorScale * currentColorScale();
    void relayout();
    void drawTicks( QPainter *, QPoint, QSize );
    struct Impl;
    Impl * d;
};

#endif // PLOTWIDGET_H
