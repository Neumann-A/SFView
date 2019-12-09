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
 * $Id: ChangeListV1.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef CHANGELISTV1_H
#define CHANGELISTV1_H

#include <QList>
#include <QDateTime>
#include <QTextStream>
#include <QDataStream>
#include "SystemMatrix.h"
#include "MatrixPosition.h"

struct ChangeListEntryV1
{
    ChangeListEntryV1() : globalIndex(-1),oldValue(0.0),newValue(0.0) {}
    ChangeListEntryV1(int globalIndex, const SystemMatrix::complex & oldValue, const SystemMatrix::complex & newValue)
        : globalIndex(globalIndex),oldValue(oldValue),newValue(newValue) {}
    int globalIndex;
    SystemMatrix::complex oldValue,newValue;
};

typedef QList<ChangeListEntryV1> ChangeListV1;

QDataStream & operator<< (QDataStream & d, const ChangeListEntryV1 & c);
QDataStream & operator>> (QDataStream & d, ChangeListEntryV1 & c);
QTextStream & operator<< (QTextStream & ts, const ChangeListEntryV1 & c);
QTextStream & operator<< (QTextStream & ts, const ChangeListV1 & cl);

#endif // CHANGELISTV1_H
