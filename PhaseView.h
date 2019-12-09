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
 * $Id: PhaseView.h 84 2017-03-12 00:08:59Z uhei $
 */

#ifndef PHASEVIEW_H
#define PHASEVIEW_H

// Qt includes
#include <QFrame>

// Local includes
#include "SystemMatrix.h"

class PhaseView : public QFrame
{
    Q_OBJECT
public:
    explicit PhaseView(QWidget *parent = 0);
    virtual ~PhaseView();
    const SystemMatrix * systemMatrix() const;
    int globalIndex() const;
    bool backgroundCorrection() const;
signals:
    void currentPositionAndValue(const MatrixPosition & pos, const SystemMatrix::complex & value);
public slots:
    void setSystemMatrix( const SystemMatrix * s );
    void setGlobalIndex(int globalIndex);
    void setBackgroundCorrection(bool b);
    void highlightPosition(const MatrixPosition & pos, const SystemMatrix::complex & value);
protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    QPoint toQPoint(const SystemMatrix::complex & c) const;
protected slots:
    void recalculate();
private:
    struct Impl;
    Impl * d;
};

#endif // PHASEVIEW_H
