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
 * $Id: PhaseView.cpp 84 2017-03-12 00:08:59Z uhei $
 */

// System includes
#include <limits>
#include <cmath>

// Qt includes
#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

// Local includes
#include "PhaseView.h"
#include "utility.h"

struct PhaseView::Impl
{
    QPointer<SystemMatrix> systemMatrix;
    int globalIndex;
    bool showHighlight;
    QPoint highlightPosition;
    typedef QPair<QPoint,QColor> PositionEntry;
    typedef QMap<MatrixPosition,PositionEntry > PositionMap;
    PositionMap positionMap;
    bool backgroundCorrection;
    int minDim,tickLen,gapSize;
    double dotsPerMM,tickLenMM,gapMM;
    SystemMatrix::complex maxVal;
    int variance;
    MatrixPosition lastPosition;
};

PhaseView::PhaseView(QWidget *parent) :
    QFrame(parent), d(new Impl)
{
    d->globalIndex=-1;
    d->showHighlight=false;
    d->backgroundCorrection=false;
    d->minDim=std::min(width(),height())/2;
    d->dotsPerMM=5.0; // start value;
    d->tickLenMM=2.0;
    d->gapMM=1.0;
    d->maxVal=1.0;
    d->variance=0.0;

    setMouseTracking( true );
}

PhaseView::~PhaseView()
{
    delete d;
}

void PhaseView::setSystemMatrix(const SystemMatrix *s)
{
    d->systemMatrix = const_cast<SystemMatrix*>(s);
    connect(d->systemMatrix,SIGNAL(dataChange()),SLOT(recalculate()));
    recalculate();
}

void PhaseView::setGlobalIndex(int globalIndex)
{
    d->globalIndex=globalIndex;
    recalculate();
}

void PhaseView::setBackgroundCorrection(bool b)
{
    d->backgroundCorrection=b;
    recalculate();
}

void PhaseView::highlightPosition(const MatrixPosition & pos, const SystemMatrix::complex &value)
{
    d->showHighlight=pos.isValid();
    d->highlightPosition=toQPoint(value);
    update();
}

const SystemMatrix * PhaseView::systemMatrix() const
{
    return d->systemMatrix;
}

int PhaseView::globalIndex() const
{
    return d->globalIndex;
}

bool PhaseView::backgroundCorrection() const
{
    return d->backgroundCorrection;
}

void PhaseView::recalculate()
{
    d->positionMap.clear();
    if ( d->systemMatrix.isNull() || d->globalIndex<0 || d->globalIndex>d->systemMatrix->maxGlobalIndex() )
        return;

    const SystemMatrix::complex * p = systemMatrix()->rawData(d->globalIndex,d->backgroundCorrection);

    int numVoxels=1;
    for ( unsigned int i=0; i<3; i++ )
    {
        numVoxels *= systemMatrix()->dimension((Qt::Axis)i);
    }

    double min = std::numeric_limits<double>::max();
    double maxAbs=0.0;
    d->maxVal=0.0;

    if ( false /* colorScale == PerFrame */ )
    {
    }
    else
    {
        for ( int i = 0; i < numVoxels; i++ )
        {
            double q = abs ( *(p+i) );
            if ( q > maxAbs )
            {
                maxAbs = q;
                d->maxVal = *(p+i);
            }
            if ( q < min )
                min = q;
        }
    }

    for ( int i=0; i<d->systemMatrix->dimension(Qt::XAxis); i++)
    {
        for ( int j=0; j<d->systemMatrix->dimension(Qt::YAxis); j++)
        {
            for ( int k=0; k<d->systemMatrix->dimension(Qt::ZAxis); k++)
            {
                MatrixPosition pos(i,j,k);
                SystemMatrix::complex c=d->systemMatrix->dataPoint(d->globalIndex,pos,d->backgroundCorrection);
                double hue = 0.5 * arg( c ) / M_PI;
                if ( hue<0.0 ) hue+=1.0;
                double gray = abs( c ) / maxAbs;
                d->positionMap.insert(pos,qMakePair(toQPoint(c),QColor::fromHsvF(hue,1.0,gray)));
            }
        }
    }


    d->variance=sqrt(systemMatrix()->backgroundVariance(d->globalIndex))/maxAbs*d->minDim;

    update();
}

QPoint PhaseView::toQPoint(const SystemMatrix::complex &c) const
{
    QRect g=geometry();
    double maxAbs=abs(d->maxVal);
    int x=c.real()/maxAbs*d->minDim+g.width()/2+g.left();
    int y=-c.imag()/maxAbs*d->minDim+g.height()/2+g.top();
    return QPoint(x,y);
}

void PhaseView::resizeEvent(QResizeEvent * ev)
{
    QFrame::resizeEvent(ev);
    QFont font = qApp->font();
    font.setPointSize(10);
    QFontMetrics fm(font);
    int h=fm.height();
    int w=fm.width(QString("180")+QChar(0xb0));
    d->dotsPerMM=std::max(physicalDpiX()/25.4,physicalDpiY()/25.4);
    d->tickLen=d->tickLenMM*d->dotsPerMM;
    d->gapSize=d->gapMM*d->dotsPerMM;
    d->minDim=std::min(width()-2*w,height()-2*h)/2-2*(d->tickLen+d->gapSize);
    d->showHighlight=false;
    recalculate();
}

void PhaseView::mouseMoveEvent(QMouseEvent * ev)
{
    const int captureRange=25;
    if ( d->positionMap.empty() )
        return;
    MatrixPosition pos;
    QPoint p;
    int dist=(d->positionMap.begin()->first-ev->pos()).manhattanLength();
    for ( Impl::PositionMap::iterator i=d->positionMap.begin(); i!=d->positionMap.end(); ++i)
    {
        int q=(i->first-ev->pos()).manhattanLength();
        if ( q<=dist && q<=captureRange )
        {
            pos=i.key();
            p=i.value().first;
            dist=q;
        }
    }
    if ( pos!=d->lastPosition )
    {
        d->showHighlight = pos.isValid();
        d->highlightPosition = p;
        emit currentPositionAndValue(pos,d->systemMatrix->dataPoint(d->globalIndex,pos,d->backgroundCorrection));
        d->lastPosition=pos;
        update();
    }
}

void PhaseView::paintEvent(QPaintEvent * /*ev*/)
{
    QSettings settings;
    QPainter painter(this);
    QRect g=geometry();

    painter.setBackground( Qt::black );
    painter.eraseRect(g);
    QFont font = qApp->font();
    font.setPointSize(10);
    QFontMetrics fm(font);

    QPainterPath ellipse;
    ellipse.addEllipse(g.center(),d->minDim,d->minDim);
    painter.fillPath(ellipse,Qt::gray);
    painter.setPen(Qt::white);
    painter.drawPath(ellipse);

    painter.drawLine(g.center()-QPoint(d->minDim+d->tickLen,0),g.center()+QPoint(d->minDim+d->tickLen,0));
    painter.drawLine(g.center()-QPoint(0,d->minDim+d->tickLen),g.center()+QPoint(0,d->minDim+d->tickLen));
    int lwWidth=width()/2-d->minDim-d->tickLen-d->gapSize;

    painter.drawText(QRect(g.width()/2+d->minDim+d->tickLen+d->gapSize,g.top(),lwWidth,height()),Qt::AlignLeft|Qt::AlignVCenter,QString("0")+QChar(0x00b0));
    painter.drawText(QRect(g.left(),g.top(),lwWidth,g.height()),Qt::AlignRight|Qt::AlignVCenter,QString("180")+QChar(0x00b0));
    painter.drawText(QRect(g.left(),g.top(),g.width(),g.height()/2-d->minDim-d->tickLen-d->gapSize),Qt::AlignBottom|Qt::AlignHCenter,QString("90")+QChar(0x00b0));
    painter.drawText(QRect(g.left(),g.top()+g.height()/2+d->minDim+d->tickLen+d->gapSize,g.width(),g.height()/2-d->minDim-d->tickLen-d->gapSize),Qt::AlignTop|Qt::AlignHCenter,QString("270")+QChar(0x00b0));

    if ( d->positionMap.isEmpty() )
        return;

    // Draw maximum value
    const int offset=5;
    QRect textArea(g.left()+offset,g.top()+offset,g.width()/2-d->minDim*0.71-offset,g.height()/2-d->minDim*0.71-offset);
    Qt::Alignment align=Qt::AlignTop | Qt::AlignLeft;
    int quadrant = ( static_cast<int>(floor( 2.0*arg ( d->maxVal )/M_PI + 4.0) ) ) % 4;

    switch ( quadrant )
    {
        case 0: textArea.moveTopRight(QPoint(g.width()-1-offset,g.top()));
                align=Qt::AlignTop|Qt::AlignRight;
                break;
        case 1: break;
        case 2: textArea.moveBottomLeft(QPoint(g.left(),g.height()-1-offset));
                align=Qt::AlignBottom|Qt::AlignLeft;
                break;
        case 3: textArea.moveBottomRight(QPoint(g.width()-1-offset,g.height()-1-offset));
                align=Qt::AlignBottom|Qt::AlignRight;
                break;
    }

    // Write absolute maximum and connect it via line
    QPoint maxPoint=toQPoint(d->maxVal);
    QString maxVal=QString("%1").arg(abs(d->maxVal),0,'f',0);
    QRect textRect=painter.boundingRect(textArea,align,maxVal);
    QPoint dist=textRect.center()-maxPoint;
    int delta=std::min(abs(dist.x()),abs(dist.y()));
    switch ( quadrant )
    {
        case 0 : dist=maxPoint+QPoint(delta,-delta);break;
        case 1 : dist=maxPoint+QPoint(-delta,-delta);break;
        case 2 : dist=maxPoint+QPoint(-delta,delta);break;
        case 3 : dist=maxPoint+QPoint(delta,delta);break;
    }
    painter.drawLine(maxPoint,dist);
    painter.drawLine(dist,textRect.center());
    painter.fillRect(textRect,Qt::black);
    painter.drawText(textArea, maxVal, align );

    // Draw actual data
    foreach( Impl::PositionEntry p, d->positionMap )
    {
        painter.setPen(p.second);
        const int & x = p.first.x();
        const int & y = p.first.y();
        painter.drawLine(x-3,y,x+3,y);
        painter.drawLine(x,y-3,x,y+3);
    }

    if ( settings.value("ShowVarianceInPhasePlot",true).toBool() )
    {
        int v=d->variance;
        if ( d->variance>d->minDim)
        {
            v=d->minDim;
            painter.setPen(Qt::red);
            painter.drawPath(ellipse);
        }
        else
        {
            painter.setPen(Qt::white);
            painter.drawEllipse(geometry().center(),v, v );
        }
    }

    if ( d->showHighlight )
    {
        int x=d->highlightPosition.x();
        int y=d->highlightPosition.y();
        painter.setPen( Qt::white );
        painter.drawLine(x-7,y,x+7,y);
        painter.drawLine(x,y-7,x,y+7);
    }


}

