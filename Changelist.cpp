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
 * $Id: Changelist.cpp 69 2017-02-26 16:15:45Z uhei $
 */


#include "ChangeList.h"

ChangeListItem::ChangeListItem()
    : globalIndex_(-1)
{
    for ( unsigned int i=0; i<4; i++)
        values_[i]=0.0;
}

ChangeListItem::ChangeListItem(int globalIndex, const std::complex<double> *values)
    : globalIndex_(globalIndex)
{
    for ( unsigned int i=0; i<4; i++)
        values_[i]=values[i];
}

ChangeListItem::ChangeListItem(const ChangeListItem & c)
    : globalIndex_(c.globalIndex_)
{
    for ( unsigned int i=0; i<4; i++)
        values_[i]=c.values_[i];
}

QDataStream & operator << (QDataStream & d, const ChangeListItem & c)
{
    d << c.globalIndex_;
    for ( unsigned int i=0; i<4; i++ )
        d << c.values_[i].real() << c.values_[i].imag();
    return d;
}

QDataStream & operator >> (QDataStream & d, ChangeListItem & c)
{
    d >> c.globalIndex_;
    for ( unsigned int i=0; i<4; i++ )
    {
        double a,b;
        d >> a >> b;
        c.values_[i]=std::complex<double>(a,b);
    }
    return d;
}

QTextStream & operator << (QTextStream & t, const ChangeListItem & c)
{
    t << c.globalIndex_ << ":";
    for ( unsigned int i=0; i<4; i++ )
    {
        if ( i%2 )
            t << " => ";
        else
            t << "\t";
        t << "(" << c.values_[i].real() << "," << c.values_[i].imag() << ")";
    }
    return t;
}

ChangeListEntry::ChangeListEntry()
    : changeTime(QDateTime::currentDateTime())
{
}

ChangeListEntry::ChangeListEntry(const MatrixPosition & pos, const ChangeListItem & change)
    : changeTime(QDateTime::currentDateTime()), position(pos)
{
    changeItems.append(change);
}

ChangeListEntry::ChangeListEntry(const MatrixPosition & pos, int globalIndex, const std::complex<double> * values)
    : changeTime(QDateTime::currentDateTime()), position(pos)
{
    changeItems.append(ChangeListItem(globalIndex,values));
}

QDataStream & operator << (QDataStream & d, const ChangeListEntry & c)
{
    d << c.changeTime
      << c.position
      << c.changeItems;
    return d;
}

QDataStream & operator >> (QDataStream & d , ChangeListEntry & c )
{
    d >> c.changeTime
      >> c.position
      >> c.changeItems;
    return d;
}

QTextStream & operator << (QTextStream & t, const ChangeListEntry & c )
{
    t << c.changeTime.toString(Qt::SystemLocaleShortDate) << ": Change of matrix position " << c.position << "\n";
    foreach(const ChangeListItem & i, c.changeItems)
        t << i << "\n";
    return t;
}

