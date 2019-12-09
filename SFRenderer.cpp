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
 * $Id: SFRenderer.cpp 69 2017-02-26 16:15:45Z uhei $
 */

// Standard includes
#include <limits>

// Qt includes
#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#else
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#endif
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtGui/QColor>
#include <QtGui/QPainter>


// Local includes
#include "SFRenderer.h"
#include "SystemMatrix.h"
#include "ColorScale.h"
#include "ColorScaleManager.h"

struct SFRenderer::Impl {
        QPointer<SystemMatrix> systemMatrix;
        Qt::Axis horizontalAxis,verticalAxis,sliceDirection;
        bool smoothScaling;
        bool backgroundCorrection;
        mutable QPointer<ColorScaleManager> m_colorScaleManager;
        ColorScaleManager * colorScaleManager() const
        {
            if ( m_colorScaleManager == 0)
            {
                foreach(QWidget * w, QApplication::topLevelWidgets() )
                {
                    m_colorScaleManager=w->findChild<ColorScaleManager*>();
                    if ( m_colorScaleManager )
                        break;
                }
            }
            return m_colorScaleManager;
        }

        const ColorScale * currentColorScale() const
        {
            ColorScaleManager * csm = colorScaleManager();
            if ( csm )
                return csm->currentColorScale();
            return 0;
        }

        QColor color(const SystemMatrix::complex & v) const
        {
            const ColorScale * cs = currentColorScale();
            if ( cs )
                return cs->color(v);
            return Qt::black;
        }

};

SFRenderer::SFRenderer(QObject* parent): QObject(parent), d(new Impl)
{
    d->systemMatrix=0;
    d->horizontalAxis=Qt::XAxis;
    d->verticalAxis=Qt::YAxis;
    d->sliceDirection=Qt::ZAxis;
    d->smoothScaling=false;
    d->backgroundCorrection=false;
}

SFRenderer::~SFRenderer() {
    delete d;
}

void SFRenderer::setSystemMatrix(SystemMatrix* systemMatrix)
{
    d->systemMatrix=systemMatrix;
}

SystemMatrix* SFRenderer::systemMatrix() const
{
    return d->systemMatrix;
}

void SFRenderer::setAxes(Qt::Axis horizontal, Qt::Axis vertical)
{
    d->horizontalAxis = horizontal;
    d->verticalAxis = vertical;
    if ( Qt::XAxis != horizontal && Qt::XAxis != vertical )
        d->sliceDirection = Qt::XAxis;
    else if ( Qt::YAxis != horizontal && Qt::YAxis != vertical )
        d->sliceDirection = Qt::YAxis;
    else
        d->sliceDirection = Qt::ZAxis;
}

Qt::Axis SFRenderer::horizontalAxis() const
{
    return d->horizontalAxis;
}

Qt::Axis SFRenderer::verticalAxis() const
{
    return d->verticalAxis;
}

Qt::Axis SFRenderer::sliceDirection() const
{
    return d->sliceDirection;
}

void SFRenderer::setBackgroundCorrection(bool b)
{
    d->backgroundCorrection=b;
}

bool SFRenderer::backgroundCorrection() const
{
    return d->backgroundCorrection;
}

QImage SFRenderer::image(int globalIndex, int slice, Colorization colorScale)
{
    QSettings settings;

    int direction[3], inc[3], grid[3];

    for ( unsigned int i=0; i<3; i++ )
    {
        grid[i] = systemMatrix()->dimension((Qt::Axis)i);
    }
    
    if ( d->horizontalAxis==d->verticalAxis )
    {
        QImage dummy (1,1,QImage::Format_RGB32 );
        dummy.fill ( Qt::black );
        return dummy;
    }
        
    direction[0] = static_cast<int>(d->horizontalAxis);
    direction[1] = static_cast<int>(d->verticalAxis);
    direction[2] = static_cast<int>(d->sliceDirection);

    for ( unsigned int i = 0; i < 3; i++ ) {
        inc[i] = 1;

        for ( int j = 0; j < direction[i]; j++ )
            inc[i] *= grid[j];
    }

    QImage image ( grid[direction[0]], grid[direction[1]], QImage::Format_RGB32 );

    const SystemMatrix::complex * p = systemMatrix()->rawData(globalIndex,backgroundCorrection());
    if ( 0==p )
    {
        image.fill( Qt::black );
        return image;
    }

    double min = std::numeric_limits<double>::max(), max = 0;

    if ( colorScale == PerFrame )
    {
        int block = grid[0] * grid[1] * grid[2];
        for ( int i = 0; i < block; i++ ) {
            double q = abs ( p[i] );

            if ( q > max )
                max = q;

            if ( q < min )
                min = q;
        }
    }
    else
    {
        for ( int i = 0; i < grid[direction[0]]; i++ )
        {
            for ( int j = 0; j < grid[direction[1]]; j++ )
            {
                int index = i * inc[0] + j * inc[1] + slice * inc[2];
                double q = abs ( p[index] );

                if ( q > max )
                    max = q;

                if ( q < min )
                    min = q;
            }
        }
    }

    if ( settings.value("startColorScaleAtZero",true).toBool() )
        min=0.0;

    for ( int i = 0; i < grid[direction[0]]; i++ ) {
        for ( int j = 0; j < grid[direction[1]]; j++ ) {
            int index = i * inc[0] + j * inc[1] + slice * inc[2];

            SystemMatrix::complex v = p[index];
            double q = ( abs ( v ) - min ) / ( max - min );
            v = std::polar(q,arg(v));

            image.setPixel ( i, j, d->color(v).rgb() );
        }
    }

    return image;
}

void SFRenderer::plotLegend (QPainter * p, const QRect & area, int globalIndex , int slice)
{
    QSettings settings;

    const SystemMatrix::complex * c = systemMatrix()->rawData(globalIndex, backgroundCorrection());
    if ( 0==c )
    {
        return;
    }

    int direction[3], inc[3], grid[3];

    for ( unsigned int i=0; i<3; i++ )
    {
        grid[i] = systemMatrix()->dimension((Qt::Axis)i);
    }

    direction[0] = static_cast<int>(d->horizontalAxis);
    direction[1] = static_cast<int>(d->verticalAxis);
    direction[2] = static_cast<int>(d->sliceDirection);

    for ( unsigned int i = 0; i < 3; i++ ) {
        inc[i] = 1;

        for ( int j = 0; j < direction[i]; j++ )
            inc[i] *= grid[j];
    }

    int block = 1;
    for ( unsigned int i=0; i<3; i++ )
        block *= grid[i];

    double min = std::numeric_limits<double>::max(), max = 0;

    if ( slice==-1 )
    {
        for ( int i = 0; i < block; i++ ) {
            double q = abs ( c[i] );

            if ( q > max )
                max = q;

            if ( q < min )
                min = q;
        }
    }
    else
    {
        for ( int i = 0; i < grid[direction[0]]; i++ )
        {
            for ( int j = 0; j < grid[direction[1]]; j++ )
            {
                int index = i * inc[0] + j * inc[1] + slice * inc[2];
                double q = abs ( c[index] );

                if ( q > max )
                    max = q;

                if ( q < min )
                    min = q;
            }
        }
    }

    if ( settings.value("startColorScaleAtZero",true).toBool() )
        min=0.0;

    d->currentColorScale()->drawLegend(p,area,min,max);
}
