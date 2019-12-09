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

// Qt includes
#include <QList>
#include <QCoreApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPointer>
#include <QTranslator>


// Local includes
#include "ColorScale.h"
#include "utility.h"

class ColorScaleIconEngine : public QIconEngine
{
public:
    ColorScaleIconEngine(ColorScale * colorScale)
    {
        m_colorScale = colorScale;
    }
    virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode /*mode*/, QIcon::State /*state*/)
    {
        painter->setPen(Qt::black);
        int w=rect.width();
        int h=rect.height();
        for (int i=0; i<w; i++)
            for (int j=0;j<h;j++)
            {
                double x=double(i)/w-0.5;
                double y=double(j)/h-0.5;
                double x1=1.0-4.0*fabs(fabs(x)-0.25);
                double y1=1.0-4.0*fabs(fabs(y)-0.25);
                double r=std::min(x1,y1);
                std::complex<double> c=std::polar(r,atan2(y,x));
                if ( m_colorScale )
                    painter->setPen(m_colorScale->color( c ));
                painter->drawPoint(rect.topLeft()+QPoint(i,j));
            }
    }
    virtual ColorScaleIconEngine * clone() const
    {
        return new ColorScaleIconEngine(m_colorScale);
    }
    QPointer<ColorScale> m_colorScale;
};

struct ColorScale::Impl
{
    QString name;
    QVector<QColor> colors;
};

class PhaseColorScale : public ColorScale {
public:
    PhaseColorScale() : ColorScale( qApp->translate("ColorScale","Phase") )
    {
    }
    virtual QColor color(const std::complex<double> & value) const
    {
        double gray = std::min(1.0,abs ( value ) );
        double hue = 0.5 * arg( value ) / M_PI;
        if ( hue<0.0 ) hue+=1.0;
        return QColor::fromHsvF( hue , 1.0, gray);
    }
    void drawLegend(QPainter * p, const QRect &area, double min, double max) const
    {
        QFont labelFont ("Helvetica", 8);
        QFontMetrics fm (labelFont);

        QImage image( 1,256,QImage::Format_RGB32 );
        for ( int i=0; i<256; i++)
            image.setPixel(0,255-i, qRgb(i,i,i));
        image = image.scaled(area.width()/5,area.height()-2*fm.height()-10);
        QRect colorBar(area.topLeft(),image.size());
        colorBar.moveTo(area.left()+area.width()/5*3,area.top()+fm.height()+5);

        p->save();
        p->setFont(labelFont);
        p->drawImage(colorBar,image);
        p->setPen ( Qt::black );
        p->drawRect(colorBar);
        QPoint tickStart = colorBar.topRight();
        tickStart.rx() += 1;
        QPoint tickEnd = tickStart;
        tickEnd.rx() +=4;
        p->drawLine(tickStart,tickEnd);
        QRect r(tickEnd.x()+4,tickEnd.y()-fm.height()/2,fm.width("XXXXXXXX"),fm.height());
        p->drawText(r,QString::number(max,'f',0),Qt::AlignLeft|Qt::AlignVCenter);
        tickStart.ry()+=colorBar.height();
        tickEnd.ry()+=colorBar.height();
        p->drawLine(tickStart,tickEnd);
        r.translate(0,colorBar.height());
        p->drawText(r,QString::number(min,'f',0),Qt::AlignLeft|Qt::AlignVCenter);

        image =QImage( 1, 360, QImage::Format_RGB32 );
        for ( unsigned int i=0; i<360; i++ )
            image.setPixel(0,359-i, QColor::fromHsvF( 1.0/360.0*i, 1.0, 1.0).rgb() );
        image = image.scaled(area.width()/5,area.height()-2*fm.height()-10);
        colorBar.moveTo(area.left()+area.width()/5,area.top()+fm.height()+5);
        p->drawImage(colorBar,image);
        p->drawRect(colorBar);
        for ( unsigned int i=0; i<=360; i+=90 )
        {
            QPoint tickStart = QPoint(colorBar.bottomLeft());
            tickStart.rx()-=1;
            tickStart.ry()-=i*colorBar.height()/360.0-1;
            QPoint tickEnd=tickStart;
            tickEnd.rx() -= 4;
            p->drawLine(tickStart,tickEnd);
            QRect r = QRect(tickEnd.x()-4-fm.width("360x"),tickEnd.y()-fm.height()/2,fm.width("360x"),fm.height());
            p->drawText(r,QString::number(i)+QChar(0xb0),Qt::AlignVCenter|Qt::AlignRight);
        }

        p->restore();

    }

};

QList<ColorScale *> ColorScale::defaultColorScales()
{
    QList<ColorScale *> res;

    QVector<QColor> scale ( 256 );
    for ( unsigned int i = 0; i < 256; i++ )
        scale[i] = qRgb ( i, i, i );

    res.append(new ColorScale( qApp->translate("ColorScale","Gray"), scale ));

    for ( unsigned int i = 0; i < 256; i++ )
    {
        if ( i < 32 )
            scale[i] = qRgb ( 0, 0, 0x7f + i * 4 );
        else if ( i < 96 )
            scale[i] = qRgb ( 0, ( i - 32 ) * 4, 0xff );
        else if ( i < 160 )
            scale[i] = qRgb ( ( i - 96 ) * 4, 0xff, ( 159 - i ) * 4 );
        else if ( i < 224 )
            scale[i] = qRgb ( 0xff, ( 223 - i ) * 4, 0 );
        else
            scale[i] = qRgb ( ( 255 - i ) * 4 + 0x7f, 0, 0 );
     }

     res.append(new ColorScale( qApp->translate("ColorScale","Jet"), scale ));

     res.append( new PhaseColorScale );

     return res;
}

ColorScale::ColorScale(const QString & name, const QVector<QColor> & colors, QObject *parent) :QObject(parent), d(new Impl)
{
    d->name = name;
    d->colors = colors;
}

ColorScale::ColorScale(const QString & name, QObject *parent) : QObject(parent), d(new Impl)
{
    d->name = name;
}

ColorScale::~ColorScale()
{
    delete d;
}

const QString & ColorScale::name() const
{
    return d->name;
}

QColor ColorScale::color(const std::complex<double> &value) const
{
    double v=abs(value);
    v=std::min(v,1.0);
    v=std::max(v,0.0);
    int index=v*(d->colors.size()-1);
    return d->colors[index];
}

void ColorScale::drawLegend(QPainter * p, const QRect &area, double min, double max) const
{
    QFont labelFont ("Helvetica", 8);
    QFontMetrics fm (labelFont);

    QImage image( 1,d->colors.size(),QImage::Format_RGB32 );
    int i=d->colors.size();
    foreach (QColor color, d->colors)
        image.setPixel(0,--i, color.rgb());
    image = image.scaled(area.width()/5,area.height()-2*fm.height()-10);
    QRect colorBar(area.topLeft(),image.size());
    colorBar.moveTo(area.left()+area.width()/5*2,area.top()+fm.height()+5);
    p->save();
    p->setFont(labelFont);
    p->drawImage(colorBar,image);
    p->setPen ( Qt::black );
    p->drawRect(colorBar);
    QPoint tickStart = colorBar.topRight();
    tickStart.rx() += 1;
    QPoint tickEnd = tickStart;
    tickEnd.rx() +=4;
    p->drawLine(tickStart,tickEnd);
    QRect r(tickEnd.x()+4,tickEnd.y()-fm.height()/2,fm.width("XXXXXXXX"),fm.height());
    p->drawText(r,QString::number(max,'f',0),Qt::AlignLeft|Qt::AlignVCenter);
    tickStart.ry()+=colorBar.height();
    tickEnd.ry()+=colorBar.height();
    p->drawLine(tickStart,tickEnd);
    r.translate(0,colorBar.height());
    p->drawText(r,QString::number(min,'f',0),Qt::AlignLeft|Qt::AlignVCenter);
    p->restore();
}

QIconEngine *ColorScale::createIconEngine()
{
    return new ColorScaleIconEngine(this);
}
