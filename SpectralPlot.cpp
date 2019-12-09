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
 * $Id: SpectralPlot.cpp 77 2017-03-03 17:20:27Z uhei $
 */

// System includes
#include <limits>

// Qt includes
#include <QtCore/QPointer>
#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>

// Local includes
#include "SpectralPlot.h"
#include "SystemMatrix.h"


struct SpectralPlot::Impl
{
    QPointer<SystemMatrix> systemMatrix;
    QList<QColor> stdColors;
    QList<int> channelOrder;

    QVector<QtCharts::QLineSeries*> traces;
    QMap<QAction*,QtCharts::QLineSeries*> traceActions;
    QPointer<QtCharts::QLineSeries> markerLine;
    QtCharts::QChartView * chartView;
    QtCharts::QChart * chart;
    QtCharts::QValueAxis * frequencyAxis;
    QtCharts::QLogValueAxis * snrAxis;

    QToolBar * toolBar;
};

SpectralPlot::SpectralPlot(QWidget *parent) :
    QWidget(parent), d(new Impl)
{
    d->stdColors.append(Qt::red);
    d->stdColors.append(Qt::green);
    d->stdColors.append(Qt::blue);
    d->stdColors.append(Qt::yellow);
    d->stdColors.append(Qt::cyan);
    d->stdColors.append(Qt::magenta);

    QLayout * l = new QVBoxLayout(this);
    d->toolBar = new QToolBar;
    d->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    l->addWidget(d->toolBar);
    d->chartView = new QtCharts::QChartView(this);
    l->addWidget(d->chartView);

    d->chart = new QtCharts::QChart();
    d->frequencyAxis = new QtCharts::QValueAxis;
    d->frequencyAxis->setTitleText(tr("Frequency / kHz"));
    d->chart->addAxis(d->frequencyAxis, Qt::AlignBottom);

    d->snrAxis = new QtCharts::QLogValueAxis;
    d->snrAxis->setTitleText(tr("SNR"));
    d->snrAxis->setBase(10);
    d->chart->addAxis(d->snrAxis, Qt::AlignLeft);

    d->chartView->setChart(d->chart);
    d->chartView->setRubberBand(QtCharts::QChartView::RectangleRubberBand);
    d->chartView->show();

    QAction * a = new QAction(QIcon(":/zoomReset"),tr("Reset zoom"));
    d->toolBar->addAction(a);
    connect(a,SIGNAL(triggered(bool)),SLOT(resetZoom()));

    a = new QAction(QIcon(":/zoomIn"),tr("Zoom in"));
    d->toolBar->addAction(a);
    connect(a,SIGNAL(triggered(bool)),SLOT(zoomIn()));

    a = new QAction(QIcon(":/zoomOut"),tr("Zoom out"));
    d->toolBar->addAction(a);
    connect(a,SIGNAL(triggered(bool)),SLOT(zoomOut()));
}

SpectralPlot::~SpectralPlot()
{
    delete d;
}

void SpectralPlot::setSystemMatrix(const SystemMatrix *s)
{
    d->systemMatrix = const_cast<SystemMatrix*>(s);
    d->channelOrder.clear();
    d->traces.clear();
    foreach(QAction * a,d->traceActions.keys())
        delete a;
    d->traceActions.clear();
    d->chart->removeAllSeries();
    if ( d->systemMatrix )
    {
        for ( int i=0; i<d->systemMatrix->maxGlobalIndex(); i++)
        {
            int globalIndex = d->systemMatrix->globalIndex(i); // Lookup in SNR order
            int receiver = d->systemMatrix->receiver(globalIndex);
            if ( !d->channelOrder.contains(receiver) )
            {
                d->channelOrder.append(receiver);
                if ( d->channelOrder.count()==d->systemMatrix->numberOfReceivers() )
                    break;
            }
        }
    }

    d->traces.resize(d->systemMatrix->numberOfReceivers());
    if ( d->systemMatrix )
    {

       d->markerLine = new QtCharts::QLineSeries;
       d->markerLine->setColor(Qt::black);
       d->chart->addSeries(d->markerLine);
       d->markerLine->attachAxis(d->frequencyAxis);
       d->markerLine->attachAxis(d->snrAxis);

       QMap<int,QAction *> actions;
       for ( int c=0; c<d->systemMatrix->numberOfReceivers(); c++)
       {
           int receiver = d->channelOrder.at(c);
           QColor color = d->stdColors.at ( receiver % d->stdColors.count() );

           QtCharts::QLineSeries * trace = new QtCharts::QLineSeries(this);
           d->traces[receiver] = trace;
           trace->setName(QString::number(receiver+1));
           trace->setColor(color);
           QPixmap icon(128,128);
           icon.fill(color);
           QAction * traceAction = new QAction(QIcon(icon),tr("%1").arg(receiver+1));
           traceAction->setCheckable(true);
           traceAction->setChecked(true);
           connect(traceAction,SIGNAL(toggled(bool)),SLOT(setTraceVisible(bool)));
           actions [receiver] = traceAction;
           d->traceActions.insert(traceAction,trace);
           connect(trace,SIGNAL(clicked(QPointF)),SLOT(selectPoint(QPointF)));

           for ( int i=0; i<d->systemMatrix->numberOfFrequencies(); i++)
           {
               int globalIndex = d->systemMatrix->globalIndex(receiver,i);
               trace->append(d->systemMatrix->frequency(globalIndex)/1000,d->systemMatrix->snr(globalIndex));
           }
           d->chart->addSeries(trace);
           trace->attachAxis(d->frequencyAxis);
           trace->attachAxis(d->snrAxis);
       }
       foreach(int r,actions.keys())
           d->toolBar->addAction(actions.value(r));
       d->frequencyAxis->setRange(0.0,d->systemMatrix->bandwidth()/1e3);
       d->frequencyAxis->applyNiceNumbers();
       double maxSnr = d->systemMatrix->snr(d->systemMatrix->globalIndex(0));
       d->snrAxis->setRange(1.0,maxSnr);
       d->chart->legend()->hide();
       d->markerLine->setColor(Qt::black);

    }
}

const SystemMatrix * SpectralPlot::systemMatrix() const
{
    return d->systemMatrix;
}

void SpectralPlot::highlightGlobalIndex(int globalIndex)
{
    if ( !d->systemMatrix )
        return;
    double frequency = d->systemMatrix->frequency(globalIndex)/1e3;
    double maxSnr = d->systemMatrix->snr(d->systemMatrix->globalIndex(0));

    QVector<QPointF> line;
    line.append(QPointF(frequency,1.0));
    line.append(QPointF(frequency,maxSnr));
    d->markerLine->replace(line);
    update();

}

void SpectralPlot::selectPoint(const QPointF & p)
{
    int receiver=-1;
    QtCharts::QLineSeries * trace=0;

    if ( ! d->systemMatrix )
        return;
    for ( int i=0; i<d->traces.count(); i++ )
    {
        if ( d->traces.at(i) == sender() )
        {
            receiver=i;
            trace=d->traces.at(i);
            break;
        }
    }
    if ( receiver==-1 || trace==0 )
        return;
    int index=0;
    double delta=0.5e-3*d->systemMatrix->bandwidth()/(d->systemMatrix->numberOfFrequencies()-1);

    foreach( const QPointF & q, trace->points() )
    {
        if (  fabs(q.x()-p.x())<=delta )
        {
            emit globalIndexSelect(d->systemMatrix->globalIndex(receiver,index));
            return;
        }
        index++;
    }
}

void SpectralPlot::zoomIn()
{
    QtCharts::QChart * chart = d->chartView->chart();
    if ( chart )
        chart->zoomIn();
}

void SpectralPlot::zoomOut()
{
    QtCharts::QChart * chart = d->chartView->chart();
    if ( chart )
        chart->zoomOut();
}

void SpectralPlot::resetZoom()
{
    QtCharts::QChart * chart = d->chartView->chart();
    if ( chart )
        chart->zoomReset();
}

void SpectralPlot::setTraceVisible(bool b)
{
    QtCharts::QChart * chart = d->chartView->chart();
    QAction * a = qobject_cast<QAction*>(sender());
    if ( a!=0 && chart!=0 )
    {
        QtCharts::QLineSeries * trace = d->traceActions[a];
        if ( trace==0 )
            return;
        if ( b )
        {
            chart->addSeries( trace );
            trace->attachAxis(d->frequencyAxis);
            trace->attachAxis(d->snrAxis);
        }
        else
            chart->removeSeries( trace );
    }
}
