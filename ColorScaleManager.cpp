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
#include <QActionGroup>
#include <QSettings>

// Local includes
#include "ColorScaleManager.h"

struct ColorScaleManager::Impl
{
    QList<ColorScale *> colorScales;
    QActionGroup * colorScaleActions;
    const ColorScale * currentColorScale;
};

ColorScaleManager::ColorScaleManager(QObject *parent) : QObject(parent), d(new Impl)
{
    QSettings settings;

    d->colorScales = ColorScale::defaultColorScales();
    if ( !d->colorScales.isEmpty() )
    {
        d->currentColorScale=d->colorScales.first();
        foreach (ColorScale * s,d->colorScales)
            s->setParent(this); // Assure autodelete
    }
    int key=1;
    d->colorScaleActions = new QActionGroup(this);
    connect (d->colorScaleActions,SIGNAL(triggered(QAction*)),SLOT(setCurrentColorScale(QAction*)));

    foreach ( ColorScale * colorScale, d->colorScales )
    {
        QAction * a = d->colorScaleActions->addAction ( colorScale->name() );
        a->setIcon( QIcon( colorScale->createIconEngine() ) );
        a->setCheckable( true );
        a->setShortcut(QKeySequence(tr("Ctrl+%1").arg(key++)));

        if ( colorScale->name() == settings.value("colorScale").toString() )
        {
            d->currentColorScale=colorScale;
            a->setChecked( true );
        }
    }
}

ColorScaleManager::~ColorScaleManager()
{
    delete d;

}

QList<QAction *> ColorScaleManager::colorScaleActions() const
{
    return d->colorScaleActions->actions();
}

void ColorScaleManager::setCurrentColorScale( const QString & colorScaleName )
{
    foreach ( const ColorScale * c, d->colorScales )
    {
        if ( c->name() == colorScaleName )
        {
            QSettings settings;
            d->currentColorScale=c;
            settings.setValue("colorScale", colorScaleName );
            emit changedColorScale(c);
            emit colorScaleChanged();
            break;
        }
    }
}

void ColorScaleManager::setCurrentColorScale( QAction *a )
{
    setCurrentColorScale(a->text());
}

const ColorScale * ColorScaleManager::currentColorScale() const
{
    return d->currentColorScale;
}

QStringList ColorScaleManager::colorScales() const
{
    QStringList res;
    foreach ( const ColorScale * c, d->colorScales )
        res << c->name();
    return res;
}
