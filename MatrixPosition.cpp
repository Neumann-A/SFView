/*
 * SF Viewer - A program to visualize MPI system matrices
 * Copyright (C) 2014-2017 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
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
 * $Id: MatrixPosition.cpp 69 2017-02-26 16:15:45Z uhei $
 */


#include <QDataStream>
#include <QTextStream>

#include "MatrixPosition.h"

QDataStream & operator<< (QDataStream & d, const MatrixPosition & p)
{
    for ( unsigned int i=0; i<3; i++)
        d << p.index(i);
    return d;
}

QDataStream & operator>> (QDataStream & d, MatrixPosition & p)
{
    for ( unsigned int i=0; i<3; i++)
    {
        unsigned int v;
        d >> v;
        p.set(i,v);
    }
    return d;
}

QTextStream & operator<< (QTextStream & t, const MatrixPosition & p)
{
    t << p.x() << "/" << p.y() << "/" << p.z();
    return t;
}
