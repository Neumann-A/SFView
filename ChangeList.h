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
 * $Id: ChangeList.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef CHANGELIST_H
#define CHANGELIST_H

#include <QList>
#include <QDateTime>
#include <QTextStream>
#include <QDataStream>
#include "SystemMatrix.h"
#include "MatrixPosition.h"

struct ChangeListItem
{
    ChangeListItem();
    ChangeListItem(int globalIndex_, const std::complex<double> * values_);
    ChangeListItem(const ChangeListItem &);
    int globalIndex_;
    SystemMatrix::complex values_[4]; // 0 & 2 = old values, 1 & 3 = new values, 0 & 1 = uncorrected, 2 & 3 corrected
};


QDataStream & operator << (QDataStream &, const ChangeListItem &);
QDataStream & operator >> (QDataStream &, ChangeListItem &);
QTextStream & operator << (QTextStream &, const ChangeListItem &);

struct ChangeListEntry
{
    ChangeListEntry();
    ChangeListEntry(const MatrixPosition & pos, const QList<ChangeListItem> & changes)
        : changeTime(QDateTime::currentDateTime()), position(pos), changeItems(changes) {}
    ChangeListEntry(const MatrixPosition & pos, const ChangeListItem & change);
    ChangeListEntry(const MatrixPosition & pos, int globalIndex, const std::complex<double> * values);
    ChangeListEntry(const ChangeListEntry & c)
        : changeTime(c.changeTime), position(c.position), changeItems(c.changeItems) {}
    QDateTime changeTime;
    MatrixPosition position;
    QList<ChangeListItem> changeItems;
};

QDataStream & operator << (QDataStream &, const ChangeListEntry &);
QDataStream & operator >> (QDataStream &, ChangeListEntry &);
QTextStream & operator << (QTextStream &, const ChangeListEntry &);

typedef QList<ChangeListEntry> ChangeList;

#endif // CHANGELIST_H
