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

#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

// Qt includes
#include <QObject>

// Local includes
#include "SystemMatrix.h"

// Forward declarations
class QString;

class TransferFunction : public QObject {
        Q_OBJECT
    public:
        typedef SystemMatrix::complex complex;
        TransferFunction (const QString & calibrationFileName, QObject * parent);
        ~TransferFunction();
        bool isValid() const;
        complex correctionFactor(double frequency) const;
    private:
       struct Impl;
       Impl * d;
};


#endif // TRANSFERFUNCTION_H
