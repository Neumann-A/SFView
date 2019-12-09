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

#ifndef COLORSCALEMANAGER_H
#define COLORSCALEMANAGER_H

// Qt includes
#include <QAction>
#include <QList>
#include <QObject>

// Local includes
#include "ColorScale.h"

class ColorScaleManager : public QObject
{
    Q_OBJECT
public:
    explicit ColorScaleManager(QObject *parent = 0);
    virtual ~ColorScaleManager();
    QList<QAction *> colorScaleActions() const;
    const ColorScale * currentColorScale() const;
    QStringList colorScales() const;
signals:
    void colorScaleChanged();
    void changedColorScale(const ColorScale *);
public slots:
    void setCurrentColorScale(const QString & name);
protected slots:
    void setCurrentColorScale( QAction * a);
private:
    struct Impl;
    Impl * d;
};

#endif // COLORSCALEMANAGER_H
