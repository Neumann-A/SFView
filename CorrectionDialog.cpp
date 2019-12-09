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
 * $Id: CorrectionDialog.cpp 69 2017-02-26 16:15:45Z uhei $
 */

// Qt includes
#include <QtGlobal>
#include <QtCore/QSettings>
#include <QtCore/QPointer>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QProgressBar>
#else
#include <QtGui/QProgressBar>
#endif

#include "CorrectionDialog.h"
#include "ui_CorrectionDialog.h"

#include "SystemMatrix.h"
#include "MatrixPosition.h"

struct CorrectionDialog::Impl
{
    QPointer<SystemMatrix> systemMatrix;
    int receiver;
    MatrixPosition pos;
    int numChanges;
    Ui::CorrectionDialog * ui;
};

int CorrectionDialog::performCorrection(SystemMatrix *matrix, int receiver, const MatrixPosition &pos, QWidget * parent)
{
    CorrectionDialog dialog(matrix, receiver, pos, parent);
    if ( QDialog::Accepted == dialog.exec())
        return dialog.numChanges();
    return 0;
}

CorrectionDialog::CorrectionDialog(SystemMatrix * matrix, int receiver, const MatrixPosition &pos, QWidget * parent, Qt::WindowFlags f )
    : QDialog(parent,f), d(new Impl)
{
    d->systemMatrix = matrix;
    d->receiver = receiver;
    d->pos = pos;
    d->numChanges = 0;

    d->ui = new Ui::CorrectionDialog;
    d->ui->setupUi(this);

    d->ui->introText->setText(tr("Replace voxel values at position (%1/%2/%3) in channel %4 by a value "
                                 "interpolated from the nearest neighbours.").arg(pos.x()).arg(pos.y()).arg(pos.z()).arg(QChar('X'+receiver)));
    QSettings settings;
    int threshold = settings.value("interpolationThreshold",10).toInt();
    d->ui->thresholdSpinBox->setValue(threshold);
    d->ui->thresholdSlider->setValue(threshold);
}

CorrectionDialog::~CorrectionDialog()
{
    delete d;
}

void CorrectionDialog::accept()
{
    if ( ! d->systemMatrix.isNull() )
    {
        qApp->setOverrideCursor(Qt::BusyCursor);
        QSettings settings;

        int threshold=d->ui->thresholdSpinBox->value();
        settings.setValue("interpolationThreshold",threshold);

        QList<int> indices;
        int n = d->systemMatrix->numberOfFrequencies();
        for ( int f=0; f<n; f++ )
            indices.append(d->systemMatrix->globalIndex(d->receiver,f));

        d->ui->progressBar->setRange(0,n-1);
        d->ui->progressBar->setValue(0);
        d->numChanges=d->systemMatrix->interpolateDatapoints(indices,d->pos,0.01*threshold,d->ui->progressBar,"setValue");
        qApp->restoreOverrideCursor();
    }
    QDialog::accept();
}

int CorrectionDialog::numChanges() const
{
    return d->numChanges;
}
