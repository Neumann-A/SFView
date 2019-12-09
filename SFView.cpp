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
 * $Id: SFView.cpp 84 2017-03-12 00:08:59Z uhei $
 */

// Qt includes
#include <QtGlobal>
#include <QtCore/QByteArray>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QRadioButton>
#include <QtGui/QDragEnterEvent>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QHeaderView>
#include <QtGui/QVector3D>
#include <QtCore/QMimeData>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QProgressBar>

#include <QDebug>

// Local includes
#include "SFView.h"
#include "CorrectionDialog.h"
#include "ColorScale.h"
#include "ColorScaleManager.h"
#include "ui_SFViewForm.h"
#include "SystemMatrix.h"
#include "PlotWidget.h"
#include "ui_SettingsDialog.h"
#include "SpectralPlot.h"
#include "PhaseView.h"
#include "utility.h"

#define TO_STRING(s) X_TO_STRING(s)
#define X_TO_STRING(s) #s



struct SFView::Impl
{
    Impl() : receiver ( 0 ),
             frame ( 0 ),
             undoAction( 0 ),
             spectralPlot( 0 ),
             phaseView( 0 ),
             colorScaleManager( 0 ),
             systemMatrix ( 0 ), ui(0) {}
    QButtonGroup * receiverSelect;
    QHBoxLayout * receiverButtonLayout;
    QSpinBox * mixSelect[3];
    int receiver;
    int frame;
    QAction * recentFiles[SFView::MaxRecentFiles];
    QAction * undoAction, * undoAllAction;
    PlotWidget * plotWidget;
    SpectralPlot * spectralPlot;
    PhaseView * phaseView;
    ColorScaleManager * colorScaleManager;
    SystemMatrix * systemMatrix;
    SFView::Mode mode;
    Ui::SFView * ui;
    QLabel * positionOutput, * valueOutput;
    bool backgroundCorrection;
    double interpolationThreshold;
    QString about;
    QMap<QDockWidget*,bool> dockWidgetVisibility;
};

SFView::SFView(Mode mode) : d( new Impl )
{
    d->mode = mode;
    d->backgroundCorrection = false;
    d->interpolationThreshold = 10.0;
    d->spectralPlot = 0;

    d->colorScaleManager = new ColorScaleManager(this);

    // Prepare about text
    QFile aboutText(":/title_and_license.txt");
    aboutText.open(QIODevice::ReadOnly);
    d->about = aboutText.readAll();
    d->about.replace("$appname$",(d->mode==Viewer)?"SFView":"SFEdit");
    d->about.replace("$version$",TO_STRING(VERSION));
    d->about.replace("$date$",TO_STRING(DATE));

    setupGui();
    setupActions();
    connect (d->plotWidget,SIGNAL(requestContextMenu(QPoint,MatrixPosition)),SLOT(showContextMenu(QPoint,MatrixPosition)));
    connect (d->plotWidget,SIGNAL(currentPositionAndValue(MatrixPosition,SystemMatrix::complex)),SLOT(showPositionAndValue(MatrixPosition,SystemMatrix::complex)));

    connect(d->plotWidget,SIGNAL(currentPositionAndValue(MatrixPosition,SystemMatrix::complex)),d->phaseView,SLOT(highlightPosition(MatrixPosition,SystemMatrix::complex)));
    connect(d->phaseView,SIGNAL(currentPositionAndValue(MatrixPosition,SystemMatrix::complex)),d->plotWidget,SLOT(setHighlightPosition(MatrixPosition)));
    connect(d->phaseView,SIGNAL(currentPositionAndValue(MatrixPosition,SystemMatrix::complex)),SLOT(showPositionAndValue(MatrixPosition,SystemMatrix::complex)));


    loadSettings();
}


SFView::~SFView() {
    if ( d->ui)
        delete d->ui;
    delete d;
}

void SFView::setupGui()
{
    d->ui = new Ui::SFView;
    d->ui->setupUi(this);

    d->receiverSelect = new QButtonGroup(this);
    connect(d->receiverSelect,SIGNAL(buttonClicked(int)),SLOT(setChannel(int)));
    QButtonGroup * sliceSelection = new QButtonGroup(d->ui->navigationTool);
    sliceSelection->addButton(d->ui->sliceDirX,0);
    sliceSelection->addButton(d->ui->sliceDirY,1);
    sliceSelection->addButton(d->ui->sliceDirZ,2);
    connect(sliceSelection,SIGNAL(buttonClicked(int)),SLOT(setSliceDirection(int)));

    QSettings settings;

    int maxMixingOrder=settings.value("maxMixingOrder",50).toInt();
    for ( unsigned int i=0; i<3; i++)
    {
        d->mixSelect[i] = new QSpinBox;
        d->ui->mixSelectLayout->addWidget(d->mixSelect[i],1);
        d->mixSelect[i]->setRange(-maxMixingOrder,maxMixingOrder);
        d->mixSelect[i]->setValue(0);
        connect(d->mixSelect[i],SIGNAL(valueChanged(int)),SLOT(setMixing(int)));
    }

    d->ui->mixingTable->clear();
    QStringList headers;
    headers << tr("X") << tr("Y") << tr("Z") << tr("Mixing Order") << tr("Effective Harmonic");
    d->ui->mixingTable->setHorizontalHeaderLabels(headers);

    d->plotWidget = new PlotWidget(this);
    setCentralWidget ( d->plotWidget );
    d->plotWidget->setAcceptDrops( true );
    d->plotWidget->setMouseTracking( true );
    d->plotWidget->installEventFilter( this );
    d->plotWidget->setTitle( d->about );
    connect( d->colorScaleManager, SIGNAL(colorScaleChanged()),d->plotWidget,SLOT(update()));

    d->phaseView = new PhaseView;
    d->ui->phaseViewTool->setWidget(d->phaseView);
    d->phaseView->show();

    d->spectralPlot = new SpectralPlot(this);
    d->ui->spectrumViewTool->setWidget(d->spectralPlot);
    connect(d->spectralPlot,SIGNAL(globalIndexSelect(int)),SLOT(setGlobalIndex(int)));

    d->positionOutput = new QLabel;
    d->positionOutput->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    statusBar()->addWidget(d->positionOutput);
    d->positionOutput->setMinimumWidth(100);
    d->positionOutput->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    d->valueOutput = new QLabel;
    d->valueOutput->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    d->valueOutput->setMinimumWidth(200);
    d->valueOutput->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    statusBar()->addWidget(d->valueOutput);

}

void SFView::setupActions()
{
    QSettings settings;

    for ( unsigned int i=0; i<MaxRecentFiles; i++ )
    {
        d->recentFiles[i] = new QAction(this);
        d->recentFiles[i]->setVisible( false );
        connect(d->recentFiles[i],SIGNAL(triggered()),SLOT(openRecentFile()));
        d->ui->recentFiles->addAction( d->recentFiles[i] );
    }

    updateRecentFiles();

    connect( d->ui->actionCopy, SIGNAL(triggered()), d->plotWidget, SLOT( imageToClipboard() ));

    if ( d->mode == Editor )
    {
        d->undoAction = new QAction( tr("Undo"), this );
        d->undoAction->setEnabled( false );
        d->undoAction->setShortcut(QKeySequence::Undo);
        connect(d->undoAction,SIGNAL(triggered()),SLOT(undo()));
        d->ui->menuEdit->addAction( d->undoAction );

        d->undoAllAction = new QAction( tr("Undo all..."), this);
        d->undoAllAction->setEnabled( false );
        connect(d->undoAllAction,SIGNAL(triggered()),SLOT(undoAll()));
        d->ui->menuEdit->addAction( d->undoAllAction );
    }

    bool b = settings.value("backgroundCorrection").toBool();
    d->ui->backgroundCorrection->setChecked( b );
    d->plotWidget->setBackgroundCorrection(b);
    connect ( d->ui->backgroundCorrection, SIGNAL ( toggled ( bool ) ), SLOT( setBackgroundCorrection ( bool ) ) );

    b = settings.value("smoothScaling").toBool();
    d->ui->smoothScaling->setChecked( b );
    d->plotWidget->setSmoothScaling( b );
    connect ( d->ui->smoothScaling, SIGNAL ( toggled ( bool ) ), d->plotWidget, SLOT ( setSmoothScaling ( bool ) ) );

    b = settings.value("legend").toBool();
    d->ui->showLegend->setChecked ( b );
    d->plotWidget->setLegendEnabled( b );
    connect ( d->ui->showLegend, SIGNAL ( toggled ( bool ) ), d->plotWidget, SLOT(setLegendEnabled( bool ) ) );
    
    b = settings.value("showTicks").toBool();
    d->ui->showTicks->setChecked( b );
    d->plotWidget->setTicksEnabled( b );
    connect ( d->ui->showTicks, SIGNAL ( toggled ( bool ) ), d->plotWidget, SLOT(setTicksEnabled( bool ) ) );

    d->ui->menuView->addSeparator()->setText ( tr("Color scale") );

    d->ui->menuView->addActions( d->colorScaleManager->colorScaleActions() );
    d->ui->viewToolbar->addActions( d->colorScaleManager->colorScaleActions() );

    QActionGroup * viewingDirection = new QActionGroup (this );
    viewingDirection->addAction(d->ui->actionSliceDirX);
    viewingDirection->addAction(d->ui->actionSliceDirY);
    viewingDirection->addAction(d->ui->actionSliceDirZ);
    connect ( viewingDirection, SIGNAL( triggered( QAction * ) ), SLOT( setSliceDirection( QAction * ) ) );
    
    if ( d->mode == Editor )
        d->ui->actionAbout->setText( tr("About SFEdit..." ));

    checkForUpdates(true);
}

void SFView::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    setBackgroundCorrection(settings.value("backgroundCorrection").toBool());
    d->plotWidget->setSmoothScaling(settings.value("smoothScaling").toBool());
    d->plotWidget->setLegendEnabled(settings.value("legend").toBool());
    d->plotWidget->setTicksEnabled(settings.value("showTicks").toBool());
    d->interpolationThreshold=settings.value("interpolationThreshold").toDouble();
    setIconSize(settings.value("iconSize",32).toInt());
    setToolButtonStyle(settings.value("toolButtonStyle",Qt::ToolButtonFollowStyle).toInt());
}

void SFView::storeSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("backgroundCorrection", backgroundCorrection());
    settings.setValue("smoothScaling", d->plotWidget->smoothScaling());
    settings.setValue("legend", d->plotWidget->legendEnabled());
    settings.setValue("showTicks", d->plotWidget->ticksEnabled());
    settings.setValue("interpolationThreshold", d->interpolationThreshold);
}

void SFView::setIconSize(int size)
{
    foreach(QToolBar * toolBar, findChildren<QToolBar *>())
    {
        toolBar->setIconSize(QSize(size,size));
    }
    QSettings settings;
    settings.setValue("iconSize",size);
}

void SFView::setToolButtonStyle(int style)
{
    setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(style));
}

void SFView::setToolButtonStyle(Qt::ToolButtonStyle style)
{
    foreach(QToolBar * toolBar, findChildren<QToolBar *>(QString(),Qt::FindDirectChildrenOnly))
    {
        toolBar->setToolButtonStyle(style);
    }
    QSettings settings;
    settings.setValue("toolButtonStyle",static_cast<int>(style));
}

void SFView::setGlobalIndex(int index, MixingUpdate updateMixingTerms)
{
    d->plotWidget->setGlobalIndex(index);
    d->phaseView->setGlobalIndex(index);
    d->spectralPlot->highlightGlobalIndex(index);

    updateNavigation( index, updateMixingTerms );
}

void SFView::setBackgroundCorrection(bool b)
{
    d->plotWidget->setBackgroundCorrection(b);
    d->phaseView->setBackgroundCorrection(b);
}

void SFView::setSliceDirection ( int a ) {
    d->plotWidget->setSliceDirection( static_cast<Qt::Axis>(a) );
    switch ( a )
    {
        case 0 : d->ui->actionSliceDirX->setChecked( true ); break;
        case 1 : d->ui->actionSliceDirY->setChecked( true ); break;
        case 2 : d->ui->actionSliceDirZ->setChecked( true ); break;
        default: break;
    }
}

void SFView::setSliceDirection ( QAction * a ) {
    if ( a==d->ui->actionSliceDirX )
    {
        d->ui->sliceDirX->setChecked(true);
        d->plotWidget->setSliceDirection( Qt::XAxis );
    }
    else if ( a==d->ui->actionSliceDirY )
    {
        d->ui->sliceDirY->setChecked(true);
        d->plotWidget->setSliceDirection( Qt::YAxis );
    }
    else if ( a==d->ui->actionSliceDirZ )
    {
        d->ui->sliceDirZ->setChecked(true);
        d->plotWidget->setSliceDirection( Qt::ZAxis );
    }
}

void SFView::setChannel ( int c ) {
    d->receiver = c - 1;
    
    if ( 0==systemMatrix() )
        return;
    int index = systemMatrix()->globalIndex( d->receiver, d->frame );

    setGlobalIndex(index);
}

void SFView::setFrame ( int f ) {
    d->frame = f;

    if ( 0==systemMatrix() )
        return;
    int index = systemMatrix()->globalIndex( d->receiver, d->frame );

    setGlobalIndex(index);
}

void SFView::setSnrIndex ( int snrIndex ) {
    if ( systemMatrix() == 0 ) return;

    int index=systemMatrix()->globalIndex( snrIndex );

    setGlobalIndex(index);
}

void SFView::setMixing( int ) {
    if ( systemMatrix() == 0 ) return;

    int mix[3];
    for ( unsigned int i=0; i<3; i++ )
        mix[i] = d->mixSelect[i]->value();
    int index = systemMatrix()->globalIndex(d->receiver,mix);

    setGlobalIndex(index,KeepMixingTerms);
}

void SFView::loadSystemMatrix ( const QString & procnoPath ) {
    QString error;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    SystemMatrix * newMatrix = 0;

    if ( d->mode==Viewer )
        newMatrix = new SystemMatrix( procnoPath, SystemMatrix::Viewer, this );
    else
        newMatrix = new SystemMatrix( procnoPath, SystemMatrix::Editor, this);

    if ( ! newMatrix->isValid ( &error ) )
    {
        QMessageBox::critical(this, tr( "System matrix load error"), error );
        delete newMatrix;
        QApplication::restoreOverrideCursor();
        return;
    }

    if ( d->systemMatrix )
        delete d->systemMatrix;
    d->systemMatrix = newMatrix;
    d->plotWidget->setSystemMatrix(newMatrix);
    d->phaseView->setSystemMatrix(newMatrix);
    if ( d->spectralPlot )
        d->spectralPlot->setSystemMatrix(newMatrix);

    setWindowTitle(tr("%1 - %2").arg((d->mode==Viewer)?"SFView":"SFEdit").arg(newMatrix->path()));

    int channels = systemMatrix()->numberOfReceivers();
    int frequencies = systemMatrix()->numberOfFrequencies();
    d->ui->indexSelect->setRange ( 0, frequencies - 1 );
    d->ui->snrSelect->setRange ( 0, channels * frequencies - 1 );
    d->ui->navigationTool->widget()->setEnabled( true );
    d->ui->informationTool->widget()->setEnabled( true );
    d->ui->actionCopy->setEnabled( true );
    d->ui->actionModifiable->setEnabled( true );

    if ( d->mode == Editor )
        updateUndo();

    foreach (QAbstractButton *a, d->receiverSelect->buttons())
        delete a;
    for ( int i=1; i<=channels; i++)
    {
        QRadioButton * r = new QRadioButton(QString("&%1").arg(i));
        d->ui->receiverSelectBox->addWidget(r,1);
        d->receiverSelect->addButton(r,i);
    }

    setSnrIndex(0);
    updateInfo();

    {
        QSettings settings;
        QStringList files=settings.value("recentFileList").toStringList();
        files.removeAll(procnoPath);
        files.prepend(procnoPath);
        while (files.size() > MaxRecentFiles)
            files.removeLast();
        settings.setValue("recentFileList", files);
    }

    updateRecentFiles();

    QApplication::restoreOverrideCursor();
}

void SFView::openRecentFile() {
    QAction * a=qobject_cast<QAction *>(sender());
    if  ( a )
        loadSystemMatrix( a->data().toString() );
}

void SFView::openFile() {
    QString procnoPath = QFileDialog::getExistingDirectory ( this, "Select System function procno directory" );

    if ( ! procnoPath.isEmpty() )
        loadSystemMatrix ( procnoPath );

    setSnrIndex ( 0 );
}

void SFView::updateRecentFiles() {
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i=0; i<numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i+1).arg(files[i]);
        d->recentFiles[i]->setText(text);
        d->recentFiles[i]->setData(files[i]);
        d->recentFiles[i]->setVisible(true);
    }
    for (int i=numRecentFiles; i<(int)MaxRecentFiles; ++i)
    {
        d->recentFiles[i]->setVisible(false);
    }
}

void SFView::hideEvent(QHideEvent * ev)
{
    d->dockWidgetVisibility.clear();
    d->dockWidgetVisibility[d->ui->navigationTool]=d->ui->navigationTool->isVisible();
    d->dockWidgetVisibility[d->ui->informationTool]=d->ui->informationTool->isVisible();
    d->dockWidgetVisibility[d->ui->phaseViewTool]=d->ui->phaseViewTool->isVisible();
    d->dockWidgetVisibility[d->ui->spectrumViewTool]=d->ui->spectrumViewTool->isVisible();
    QMainWindow::hideEvent(ev);
}

void SFView::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    foreach(QDockWidget * tool,d->dockWidgetVisibility.keys())
        tool->setVisible(d->dockWidgetVisibility.value(tool));
    d->dockWidgetVisibility.clear();
}

void SFView::closeEvent(QCloseEvent* ev)
{
    storeSettings();
    QMainWindow::closeEvent(ev);
}

bool SFView::eventFilter(QObject * obj, QEvent * ev)
{
    if ( obj==d->plotWidget )
    {
        if ( ev->type() == QEvent::DragEnter )
        {
            QDragEnterEvent * dragEnterEvent = static_cast<QDragEnterEvent*>(ev);
            const QMimeData * mimeData = dragEnterEvent->mimeData();
            if ( mimeData->hasUrls() )
            {
                ev->accept();
                return true;
            }
        }
        else if ( ev->type() == QEvent::Drop )
        {
            QDropEvent * dropEvent = static_cast<QDropEvent*>(ev);
            const QMimeData * mimeData = dropEvent->mimeData();
            if ( mimeData->hasUrls() )
            {
                QUrl url=mimeData->urls().first();
                if ( url.isLocalFile() )
                {
                    loadSystemMatrix( url.toLocalFile() );
                    ev->accept();
                    return true;
                }
            }
        }
    }
    return QObject::eventFilter( obj, ev);
}

void SFView::showPositionAndValue(const MatrixPosition & pos, const SystemMatrix::complex & value)
{
    if ( pos.isValid() )
    {
        QString posText = QString("[%1,%2,%3]");
        for(unsigned int i=0; i<3; i++)
            posText = posText.arg(pos.index(i));
        d->positionOutput->setText(posText);
        posText = QString("(%1;%2%3)").arg(std::abs(value)).arg(std::arg(value)*180.0/M_PI,0,'f',1).arg(QChar(0xb0));
        d->valueOutput->setText(posText);
    }
    else
    {
        d->positionOutput->clear();
        d->valueOutput->clear();
    }
}

void SFView::showContextMenu(const QPoint & pos, const MatrixPosition & matrixPos)
{
    if ( d->mode==Viewer )
        return;

    int globalIndex = systemMatrix()->globalIndex(d->receiver,d->frame);
    QMenu popup;
    QAction * interpolateOneFrequency=popup.addAction(tr("Interpolate this voxel for this frequency"));
    QAction * interpolateAllFrequencies=popup.addAction(tr("Interpolate this voxel for all frequencies..."));
    QAction * selected = popup.exec(pos);
    if ( selected==interpolateOneFrequency )
    {
        interpolate(matrixPos,globalIndex);
    }
    else if ( selected==interpolateAllFrequencies )
    {
        interpolate(matrixPos);
    }

}

void SFView::updateNavigation(int globalIndex, MixingUpdate updateMixingTerms )
{
    setControlSignalsEnabled( false );

    d->receiver = systemMatrix()->receiver( globalIndex );
    d->frame = systemMatrix()->frequencyIndex( globalIndex );

    QAbstractButton * b = d->receiverSelect->button( d->receiver+1 );
    if ( b ) b->setChecked( true );
    d->ui->indexSelect->setValue( d->frame );
    d->ui->snr->setText(tr("%L1").arg(systemMatrix()->snr(globalIndex),0,'f',4));
    d->ui->frequency->setText(tr("%L1 kHz").arg(systemMatrix()->frequency(globalIndex)/1000.0,0,'f',4));
    d->ui->snrSelect->setValue(systemMatrix()->snrIndex(globalIndex));
    SystemMatrix::complex background = systemMatrix()->background( globalIndex );
    double backgroundVariance = sqrt(systemMatrix()->backgroundVariance( globalIndex ));
    QString bg = QString ("(%1,%2) %4 %3").arg(background.real()).arg(background.imag()).arg(backgroundVariance).arg(QChar(0xb1));
    d->ui->background->setText(bg);

    int mixingTerms[3];
    if ( updateMixingTerms==UpdateMixingTerms )
    {
        d->ui->mixingOrder->setNum(systemMatrix()->mixingOrder(globalIndex,mixingTerms));
        for ( unsigned int i=0; i<3; i++)
        {
            d->mixSelect[i]->setMaximum(100000);
            d->mixSelect[i]->setMinimum(-100000);
            d->mixSelect[i]->setValue(mixingTerms[i]);
        }
    }
    else
    {
        int mixOrder=0;
        for ( unsigned int i=0; i<3; i++)
        {
            mixingTerms[i]=d->mixSelect[i]->value();
            mixOrder+=abs(mixingTerms[i]);
        }
        d->ui->mixingOrder->setNum(mixOrder);
    }

    int harmonic=0;
    for ( unsigned int i=0; i<3; i++)
        harmonic+=mixingTerms[i];

    d->ui->effectiveHarmonic->setNum(harmonic);


    // Update navigation limits for mixing terms
    for ( unsigned int i=0; i<3; i++)
    {
        int testMixing[3];
        for ( unsigned int j=0; j<3; j++)
            testMixing[j]=d->mixSelect[j]->value();
        while ( systemMatrix()->globalIndex(d->receiver,testMixing)>=0 )
            testMixing[i]++;
        d->mixSelect[i]->setMaximum(testMixing[i]-1);
        for ( unsigned int j=0; j<3; j++)
            testMixing[j]=d->mixSelect[j]->value();
        while ( systemMatrix()->globalIndex(d->receiver,testMixing)>=0 )
            testMixing[i]--;
        d->mixSelect[i]->setMinimum(testMixing[i]+1);
    }


    d->ui->mixingTable->clear();
    QStringList headers;
    headers << tr("X") << tr("Y") << tr("Z") << tr("Mixing Order") << tr("Effective Harmonic");
    d->ui->mixingTable->setHorizontalHeaderLabels(headers);

    QList<QVector3D> mixings=systemMatrix()->mixingTerms(globalIndex);
    int row=0;
    foreach(QVector3D v,mixings)
    {
        int mixingOrder = abs(v.x())+abs(v.y())+abs(v.z());
        int effectiveHarmonic = v.x()+v.y()+v.z();
        d->ui->mixingTable->setRowCount(row+1);
        QTableWidgetItem * item = new QTableWidgetItem(QString::number(v.x()));
        d->ui->mixingTable->setItem(row,0,item);
        item = new QTableWidgetItem(QString::number(v.y()));
        d->ui->mixingTable->setItem(row,1,item);
        item = new QTableWidgetItem(QString::number(v.z()));
        d->ui->mixingTable->setItem(row,2,item);
        item = new QTableWidgetItem(QString::number(mixingOrder));
        d->ui->mixingTable->setItem(row,3,item);
        item = new QTableWidgetItem(QString::number(effectiveHarmonic));
        d->ui->mixingTable->setItem(row,4,item);
        row++;
    }
    d->ui->mixingTable->resizeColumnsToContents();

    setControlSignalsEnabled( true );
}

void SFView::updateInfo()
{
    double steps[3] = { 5, 2, 1 };
    double factor = 1.0;

    double maxSnr = systemMatrix()->snr(systemMatrix()->globalIndex(0));
    while ( maxSnr > 10.0*factor ) factor*=10.0;

    d->ui->institutionName->setText( systemMatrix()->institution() );
    d->ui->systemName->setText( systemMatrix()->systemName() );

    d->ui->experimentName->setText( systemMatrix()->experimentName() );
    d->ui->acqDate->setText( systemMatrix()->experimentDate().toString(Qt::SystemLocaleShortDate) );
    d->ui->contrastAgent->setText( systemMatrix()->tracerName() );
    d->ui->concentration->setText( tr("%1 mol/L").arg(systemMatrix()->tracerConcentration()));
    d->ui->volume->setText( tr("%1 ul").arg(systemMatrix()->tracerVolume()));

    int xpoints = systemMatrix()->dimension( Qt::XAxis );
    int ypoints = systemMatrix()->dimension( Qt::YAxis );
    int zpoints = systemMatrix()->dimension( Qt::ZAxis );
    d->ui->geometryInfo->setText(QString("%1 x %2 x %3").arg(xpoints).arg(ypoints).arg(zpoints));

    double xFov = systemMatrix()->spatialExtent( Qt::XAxis );
    double yFov = systemMatrix()->spatialExtent( Qt::YAxis );
    double zFov = systemMatrix()->spatialExtent( Qt::ZAxis );
    d->ui->fovInfo->setText(QString("%1 mm x %2 mm x %3 mm").arg(xFov).arg(yFov).arg(zFov));

    d->ui->selectionField->setText(QString("%1 T/m").arg(systemMatrix()->selectionField()));
    QString driveField("%1 mT x %2 mT x %3 mT");
    for ( unsigned int i=0; i<3; i++ )
        driveField = driveField.arg(systemMatrix()->driveField(i));
    d->ui->driveField->setText(driveField);

    d->ui->averages->setText(QString::number(systemMatrix()->averages()));
    d->ui->bandWidth->setText(QString("%1 kHz").arg(1e-3*systemMatrix()->bandwidth()));
    d->ui->snrTable->clear();
    QStringList headers;
    headers << tr("Treshold") << tr("X") << tr ("Y") << tr("Z") << tr("Total");
    d->ui->snrTable->setHorizontalHeaderLabels(headers);
    int index=0, i=0;
    double threshold;
    int countX=0, countY=0, countZ=0, row=0;
    while ( (threshold=steps[index%3]*factor) >= 1.0 )
    {
        int globalIndex = systemMatrix()->globalIndex(i);
        while ( systemMatrix()->snr(globalIndex)> threshold )
        {
            switch( systemMatrix()->receiver(globalIndex) )
            {
                case 0 : countX++; break;
                case 1 : countY++; break;
                case 2 : countZ++; break;
                default: break;
            }
            globalIndex = systemMatrix()->globalIndex(++i);
        }
        int totalCount = countX+countY+countZ;

        if ( totalCount>0 )
        {
            d->ui->snrTable->setRowCount( row+1 );
            QTableWidgetItem * item = new QTableWidgetItem(tr("> %1").arg(threshold));
            d->ui->snrTable->setItem(row,0,item);
            item = new QTableWidgetItem(QString::number(countX));
            d->ui->snrTable->setItem(row,1,item);
            item = new QTableWidgetItem(QString::number(countY));
            d->ui->snrTable->setItem(row,2,item);
            item = new QTableWidgetItem(QString::number(countZ));
            d->ui->snrTable->setItem(row,3,item);
            item = new QTableWidgetItem(QString::number(totalCount));
            d->ui->snrTable->setItem(row,4,item);
            row++;
        }
        if ( (++index)%3 == 0 ) factor/=10.0;
    }
    d->ui->snrTable->resizeColumnsToContents();
    
    d->ui->highScoreTable->clear();
    headers.clear();
    headers << tr("Receiver") << tr("Index") << tr("SNR") << tr("Rank");
    d->ui->highScoreTable->setHorizontalHeaderLabels(headers);
    for ( int receiver=0; receiver<3; receiver++ )
    {
        int snrRank=0;
        int globalIndex=systemMatrix()->globalIndex(snrRank);
        while ( systemMatrix()->receiver(globalIndex)!=receiver && snrRank < systemMatrix()->maxGlobalIndex() )
            globalIndex=systemMatrix()->globalIndex(++snrRank);
        QTableWidgetItem * item = new QTableWidgetItem( QString( 'X'+receiver ));
        d->ui->highScoreTable->setItem(receiver,0,item);
        item = new QTableWidgetItem(QString::number(systemMatrix()->frequencyIndex(globalIndex)));
        d->ui->highScoreTable->setItem(receiver,1,item);
        item = new QTableWidgetItem(QString::number(systemMatrix()->snr(globalIndex)));
        d->ui->highScoreTable->setItem(receiver,2,item);
        item = new QTableWidgetItem(QString::number(snrRank));
        d->ui->highScoreTable->setItem(receiver,3,item);
    }
    d->ui->highScoreTable->resizeColumnsToContents();
}

bool SFView::interpolate(const MatrixPosition &pos, int globalIndex)
{
    bool changed=false;
    if ( 0==systemMatrix() || d->mode!=Editor )
        return changed;

    if ( globalIndex!=-1 )
    {
        changed = systemMatrix()->interpolateDatapoint(globalIndex,pos);
    }
    else
    {
        int numChanges=CorrectionDialog::performCorrection(systemMatrix(),d->receiver,pos,this);
        if ( numChanges>0 )
        {
            statusBar()->showMessage(tr("Voxel changed for %1 frequencies.").arg(numChanges),10000);
            changed=true;
        }
    }
    if ( changed )
    {
        d->plotWidget->update();
        updateNavigation(globalIndex,KeepMixingTerms);
        updateInfo();
        updateUndo();
    }
    return changed;
}

void SFView::updateUndo()
{
    QString lastChange;
    if ( systemMatrix() )
        lastChange = systemMatrix()->lastChangeDescription();

    if ( lastChange.isEmpty() )
    {
        d->undoAction->setText( tr("Undo") );
        d->undoAction->setEnabled( false );
        d->undoAllAction->setEnabled( false );
    }
    else
    {
        d->undoAction->setText( tr("Undo: %1").arg(lastChange) );
        d->undoAction->setEnabled( true );
        d->undoAllAction->setEnabled( true );
    }

}

void SFView::undo()
{
    if ( 0==systemMatrix() )
        return;
    systemMatrix()->undoLastChange();
    int index = systemMatrix()->globalIndex( d->receiver, d->frame );
    d->plotWidget->update();
    updateNavigation(index,KeepMixingTerms);
    updateInfo();
    updateUndo();
}

void SFView::undoAll()
{
    if ( 0==systemMatrix() )
        return;
    if ( QMessageBox::Yes !=
         QMessageBox::question(this,
                               tr("Undo all"),
                               tr("Reverting all changes, are you sure?"),
                               QMessageBox::Yes | QMessageBox::No) )
        return;
    QProgressBar * progress = new QProgressBar;
    progress->setRange(0,100);
    statusBar()->addWidget(progress,1);
    progress->show();
    qApp->setOverrideCursor(Qt::BusyCursor);
    systemMatrix()->undoAllChanges(progress,"setValue");
    qApp->restoreOverrideCursor();
    delete progress;

    int index = systemMatrix()->globalIndex( d->receiver, d->frame );
    d->plotWidget->update();
    updateNavigation(index,KeepMixingTerms);
    updateInfo();
    updateUndo();
}

void SFView::setControlSignalsEnabled(bool b)
{
    QList<QWidget*> controls = d->ui->navigationTool->findChildren<QWidget *>();
    foreach(QWidget * w,controls)
        w->blockSignals(!b);
}

SystemMatrix* SFView::systemMatrix() const
{
    return d->systemMatrix;
}

bool SFView::backgroundCorrection() const
{
    return d->plotWidget->backgroundCorrection();
}

void SFView::configure()
{
     QSettings settings;
     QDialog dialog;
     Ui::SettingsDialog * settingsDialog = new Ui::SettingsDialog;
     settingsDialog->setupUi(&dialog);
     settingsDialog->toolbarHeight->setValue(settings.value("iconSize",32).toInt());
     settingsDialog->toolbarConfig->setCurrentIndex(settings.value("toolButtonStyle",Qt::ToolButtonFollowStyle).toInt());
     connect(settingsDialog->toolbarHeight,SIGNAL(valueChanged(int)),SLOT(setIconSize(int)));
     connect(settingsDialog->toolbarConfig,SIGNAL(currentIndexChanged(int)),SLOT(setToolButtonStyle(int)));
     dialog.exec();
}

void SFView::showAbout()
{
    QMessageBox::about(this,tr("About SFView"),d->about);
}

void SFView::checkForUpdates(bool initialCheck)
{
    QProcess * update = new QProcess;
    update->setWorkingDirectory(QDir::currentPath());
    connect(update,SIGNAL(finished(int)),SLOT(updateCheckResult(int)));
    update->start("maintenancetool --checkupdates");
    if ( initialCheck && ! update->waitForStarted() )
    {
        delete d->ui->actionCheck_for_updates;
        d->ui->actionCheck_for_updates = 0;
    }
    else
        d->ui->actionCheck_for_updates->setEnabled(false);
}

void SFView::updateCheckResult(int res)
{
    QString info;
    QRegularExpression findVersion("version=\"([^\"]*)");
    if ( d->ui->actionCheck_for_updates )
        d->ui->actionCheck_for_updates->setEnabled(true);
    QProcess * update = qobject_cast<QProcess*>(sender());
    if ( update )
        info = QString::fromLocal8Bit(update->readAllStandardOutput());
    sender()->deleteLater();
    if ( 0==res && -1 )
    {
        QRegularExpressionMatch match = findVersion.match(info);
        QString newVersion=match.captured(1);
        QString name(d->mode==Editor?"SFEdit":"SFView");
        if ( QMessageBox::Yes ==
             QMessageBox::question(this,
                                   tr("Update available"),
                                   tr("<qt><p>A new version of %2 is available for download.</p>"
                                      "<p>New version: %1</p>"
                                      "<p>Terminate %2 and start update?</p></qt>").arg(newVersion).arg(name)))
        {
            QProcess * update = new QProcess;
            update->setWorkingDirectory(QDir::currentPath());
            update->start("maintenancetool --updater");
            if ( update->waitForStarted() )
                qApp->quit();
            else
                QMessageBox::warning(this,
                                     tr("Update problem"),
                                     tr("Failed to start update program."));
        }
    }
}
