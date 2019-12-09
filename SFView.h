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
 * $Id: SFView.h 78 2017-03-07 22:40:51Z uhei $
 */

#ifndef SFView_H
#define SFView_H

// Qt includes
#include <QtGlobal>
//#include <QtCore/QProcess>
#if QT_VERSION > 0x050000
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#else
#include <QtGui/QMainWindow>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>
#endif

// Local includes
#include "MatrixPosition.h"
#include "SystemMatrix.h"

// Forward declarations
class SystemMatrix;
class QResizeEvent;

class SFView : public QMainWindow
{
Q_OBJECT
public:
    enum Mode { Viewer, Editor };
    enum MixingUpdate { KeepMixingTerms, UpdateMixingTerms };
    SFView(Mode mode=Viewer);
    virtual ~SFView();
public slots:
    void loadSystemMatrix(const QString & procnoPath);
protected slots:
    void setSliceDirection(int);
    void setSliceDirection(QAction *);
    void setChannel(int);
    void setFrame(int);
    void setSnrIndex(int);
    void setMixing(int);
    void openFile();
    void openRecentFile();
    void showAbout();
    void configure();
    void showContextMenu(const QPoint &, const MatrixPosition & pos);
    void undo();
    void undoAll();
    void showPositionAndValue(const MatrixPosition & pos, const SystemMatrix::complex & value);
    void setBackgroundCorrection(bool b);
    void setIconSize(int size);
    void setToolButtonStyle(int);
    void setToolButtonStyle(Qt::ToolButtonStyle style);
    void checkForUpdates(bool initialCheck=false);
protected slots:
    void setGlobalIndex(int, MixingUpdate updateMixingTerms=UpdateMixingTerms);
    void updateCheckResult(int);
protected:
    void setControlSignalsEnabled(bool b);
    void updateRecentFiles();
    void setupGui();
    void setupActions();
    void loadSettings();
    void storeSettings();
    void updateNavigation( int globalIndex, MixingUpdate updateMixingTerms=UpdateMixingTerms );
    void updateInfo();
    void updateUndo();
    SystemMatrix * systemMatrix() const;
    bool backgroundCorrection() const;
    virtual void hideEvent(QHideEvent *ev);
    virtual void showEvent(QShowEvent * ev);
    virtual void closeEvent(QCloseEvent * ev);
    virtual bool eventFilter( QObject *, QEvent * );
    bool interpolate(const MatrixPosition & pos, int globalIndex=-1);
private:
    enum { MaxRecentFiles = 10 };
    struct Impl;
    Impl * d;
};

#endif // SFView_H
