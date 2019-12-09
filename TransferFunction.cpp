/*
 * SF Viewer - A program to visualize MPI system matrices
 * Copyright (C) 2014-2016  Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
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
 * $Id: SpectralPlot.h 63 2016-12-11 15:01:06Z uhei $
 */

// Standard includes
#include <limits>
#include <vector>

// Qt includes
#include <QFile>
#include <QList>
#include <QTextStream>
#include <QVector>

// Local includes
#include "TransferFunction.h"
#include "utility.h"

#include <QDebug>

// Cubic spline implementation adapted from Numerical Recipes in C++

struct TransferFunction::Impl
{
    void init(const QVector<double> & _x, const QVector<complex> & _y)
    {
        int n=_x.count();
        if (n<1)
            return;
        x=_x;
        y=_y;
        y2.resize(n);
        y2.first()=0.0;
        y2.last()=0.0;
        if ( n<=2 )
            return;
        QVector<complex> h(n); // help vector
        h.first() = 0.0;
        for ( int i=1; i<n-1; i++ )
        {
            double s  =  (x[i]-x[i-1])/(x[i+1]-x[i-1]);
            complex p =  s*y2[i-1]+2.0;
            y2[i]     =  (s-1.0)/p;
            h[i]      =  (y[i+1]-y[i])/(x[i+1]-x[i])
                        -(y[i]-y[i-1])/(x[i]-x[i-1]);
            h[i]      = (6.0*h[i])/(x[i+1]-x[i-1])-s*h[i-1]/p;
        }
        for ( int i=n-1;i>0;i-- )
            y2[i-1]=y2[i-1]*y2[i]+h[i-1];

    }

    complex interpolate(double frequency) const
    {
        int l=0,h=x.count()-1;

        // Find correct interval for interpolation
        while (h-1>l)
        {
            int k=(l+h)/2;
            if ( x[k]>frequency )
                h=k;
            else
                l=k;
        }

        // Calculate fractions
        double interval=x[h]-x[l];
        double a=(x[h]-frequency)/interval;
        double b=1.0-a;

        return a*y[l]+b*y[h]+((a*a*a-a)*y2[l]+(b*b*b-b)*y2[h])*interval*interval/6.0;
    }
    QVector<double> x;
    QVector<complex> y,y2;
    bool valid;
};

TransferFunction::TransferFunction (const QString & calibrationFileName, QObject *parent)
    : QObject(parent),d(new Impl)
{
    d->valid=false;
    QFile file(calibrationFileName);
    if ( file.open(QIODevice::ReadOnly) )
    {
        QDataStream ds(&file);
        QList<double> f;
        QList<complex> t;
        double lastX=0.0;
        while ( !ds.atEnd() )
        {
            double x,yr,yi;
            ds >> x >> yr >> yi;
            if  ( QDataStream::Ok!=ds.status() )
            {
                qDebug() << "Read error in binary calibration file " << file.fileName();
                return;
            }
            if ( x<lastX )
            {
                qDebug() << "Error in binary calibration file " << file.fileName()
                         << ": Frequency values are not increasing monotonically.";
                return;
            }
            f.append(x);
            t.append(std::polar(pow(10,0.1*yr),M_PI/180.0*yi));
            lastX=x;
        }
        if ( f.isEmpty() )
            return;
        d->init(f.toVector(),t.toVector());
        d->valid=true;
        return;
    }

    file.setFileName(calibrationFileName+".csv");
    if ( file.open(QIODevice::ReadOnly) )
    {
        QTextStream ts(&file);
        QList<double> f;
        QList<complex> t;
        QString line;
        double lastX=0.0;
        while ( !(line=ts.readLine()).isNull() )
        {
            QStringList nums=line.split(",");
            if ( nums.count()!=3 )
            {
                qDebug() << "Read error in text-based calibration file " << file.fileName();
                return;
            }
            double x=nums.at(0).toDouble();
            if ( x<lastX )
            {
                qDebug() << "Error in binary calibration file " << file.fileName()
                         << ": Frequency values are not increasing monotonically.";
                return;
            }
            f.append(x);
            t.append(std::polar(pow(10,0.1*nums.at(1).toDouble()),M_PI/180.0*nums.at(2).toDouble()));
            lastX=x;
        }
        if ( f.isEmpty() )
            return;
        d->init(f.toVector(),t.toVector());
        d->valid=true;
        return;
    }
}

TransferFunction::~TransferFunction()
{
    delete d;
}

bool TransferFunction::isValid() const
{
    return d->valid;
}

TransferFunction::complex TransferFunction::correctionFactor(double frequency) const
{
    return 1.0/d->interpolate(frequency);
}

