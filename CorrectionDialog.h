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
 * $Id: CorrectionDialog.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef CORRECTIONDIALOG_H
#define CORRECTIONDIALOG_H

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

class MatrixPosition;
class SystemMatrix;

class CorrectionDialog : public QDialog
{
    Q_OBJECT
public:
    static int performCorrection(SystemMatrix * matrix, int receiver, const MatrixPosition & pos, QWidget *parent=0);
    virtual ~CorrectionDialog();
    int numChanges() const;
public slots:
    virtual void accept();
private:
    CorrectionDialog(SystemMatrix * matrix, int receiver, const MatrixPosition & pos, QWidget *parent=0, Qt::WindowFlags f=0);
    struct Impl;
    Impl * d;
};

#endif // CORRECTIONDIALOG_H
