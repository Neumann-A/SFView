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
 * $Id: ChangeListV1.cpp 69 2017-02-26 16:15:45Z uhei $
 */

#include "ChangeListV1.h"

QDataStream & operator<< (QDataStream & d, const ChangeListEntryV1 & c)
{
    d << c.globalIndex << c.oldValue.real() << c.oldValue.imag() << c.newValue.real() << c.newValue.imag();
    return d;
}

QDataStream & operator>> (QDataStream & d, ChangeListEntryV1 & c)
{
    double v[4];
    d >> c.globalIndex;
    for ( unsigned int i=0; i<4; i++ )
        d >> v[i];
    c.oldValue = SystemMatrix::complex(v[0],v[1]);
    c.newValue = SystemMatrix::complex(v[2],v[3]);
    return d;
}

QTextStream & operator << (QTextStream & ts, const ChangeListEntryV1 & c)
{
    ts << c.globalIndex << ": "
          "(" << c.oldValue.real() << "," << c.oldValue.imag() << ") => "
          "(" << c.newValue.real() << "," << c.newValue.imag() << ")";
    return ts;
}

typedef QList<ChangeListEntryV1> ChangeListV1;

QTextStream & operator<< (QTextStream & ts, const ChangeListV1 & cl)
{
    foreach(const ChangeListEntryV1 & c, cl)
        ts << c << "\n";
    return ts;
}
