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
 * $Id: PlotWidget.cpp 72 2017-02-28 22:45:42Z uhei $
 */

// Qt includes
#include <QtGlobal>
#include <QtCore/QFile>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
#include <QtGui/QClipboard>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPicture>
#include <QtGui/QStaticText>
#include <QtGui/QTextOption>

// Local includes
#include "PlotWidget.h"
#include "SFRenderer.h"

#define TO_STRING(s) X_TO_STRING(s)
#define X_TO_STRING(s) #s

struct PlotWidget::Impl {
    SFRenderer * renderer;
    QList<QRect> areaList;
    QSize sliceSize;
    QRect area;
    int legendWidth;
    int index;
    Qt::TransformationMode transformationMode;
    bool showTicks;
    QFont tickFont;
    QFont labelFont;
    QFont titleFont;
    int margin;
    int spacing;
    int tickLength;
    int legendWith;
    int singleSlice;
    bool showLegend;
    QStaticText title;
    QPicture decoration;
    QMap<Qt::Axis,QPair<Qt::Axis,Qt::Axis> > directions;
    MatrixPosition highlightPosition;
};

PlotWidget::PlotWidget(QWidget * parent) : QFrame(parent), d(new Impl)
{
    d->renderer = new SFRenderer(this);
    d->legendWidth = 0;
    d->transformationMode = Qt::SmoothTransformation;
    d->index = 0;
    d->showTicks = true;
    d->legendWidth = 100;
    d->singleSlice = -1;
    d->showLegend = true;
    d->tickFont.setFamily( "Arial" );
    d->tickFont.setPointSize( 8 );
    d->labelFont.setFamily( "Arial" );
    d->labelFont.setPointSize( 9 );
    d->titleFont.setFamily( "Times" );
    d->titleFont.setPointSize( 14 );
    d->margin=30;
    d->spacing=40;
    d->tickLength=4;

    d->directions[Qt::XAxis]=qMakePair(Qt::YAxis,Qt::ZAxis);
    d->directions[Qt::YAxis]=qMakePair(Qt::XAxis,Qt::ZAxis);
    d->directions[Qt::ZAxis]=qMakePair(Qt::XAxis,Qt::YAxis);
    setFrameShadow( Sunken );
    setFrameStyle( StyledPanel );

    setMouseTracking( true );
}

PlotWidget::~PlotWidget()
{
    delete d;
}

void PlotWidget::setTitle(const QString &text)
{
    d->title.setText( text );
    d->title.setTextOption( QTextOption(Qt::AlignCenter) );
    d->title.prepare( QTransform(), d->titleFont );
}

void PlotWidget::setSliceDirection(Qt::Axis a)
{
    d->renderer->setAxes(d->directions[a].first,d->directions[a].second);
    relayout();
}

Qt::Axis PlotWidget::horizontalAxis() const
{
    return d->renderer->horizontalAxis();
}

Qt::Axis PlotWidget::verticalAxis() const
{
    return d->renderer->verticalAxis();
}

Qt::Axis PlotWidget::sliceDirection() const
{
    return d->renderer->sliceDirection();
}

void PlotWidget::setBackgroundCorrection(bool b)
{
    d->renderer->setBackgroundCorrection(b);
    update();
}

bool PlotWidget::backgroundCorrection() const
{
    return d->renderer->backgroundCorrection();
}

void PlotWidget::setSmoothScaling(bool b)
{
    if ( b )
        d->transformationMode = Qt::SmoothTransformation;
    else
        d->transformationMode = Qt::FastTransformation;
    update();
}

bool PlotWidget::smoothScaling() const
{
    return d->transformationMode==Qt::SmoothTransformation;
}

void PlotWidget::setSystemMatrix(SystemMatrix * systemMatrix)
{
    d->renderer->setSystemMatrix(systemMatrix);
    relayout();
}

SystemMatrix* PlotWidget::systemMatrix() const
{
    return d->renderer->systemMatrix();
    
}

void PlotWidget::setGlobalIndex(int index)
{
    d->index = index;
    update();
}

int PlotWidget::index() const
{
    return d->index;
}

void PlotWidget::setLegendEnabled ( bool  b ) {
    d->showLegend = b;
    relayout();
}

bool PlotWidget::legendEnabled() const {
    return d->showLegend;
}

void PlotWidget::setTicksEnabled(bool b)
{
    d->showTicks = b;
    relayout();
}

bool PlotWidget::ticksEnabled() const
{
    return d->showTicks;
}

void PlotWidget::showSingleSlice(int slice)
{
    if ( 0==systemMatrix() )
        return;
    if ( slice<0 || slice>=systemMatrix()->dimension(sliceDirection()) )
        return;
    d->singleSlice=slice;
    relayout();
}

void PlotWidget::showAllSlices()
{
    d->singleSlice=-1;
    relayout();
}

void PlotWidget::setHighlightPosition(const MatrixPosition & pos)
{
    d->highlightPosition=pos;
    update();
}

void PlotWidget::relayout()
{
    d->areaList.clear();
    if ( 0==systemMatrix() )
        return;
    
    QFontMetrics tickFontMetrics(d->tickFont);
    QFontMetrics labelFontMetrics(d->labelFont);

    // Define active area
    d->area=contentsRect();
    d->area.translate(d->margin,d->margin);
    d->area.setHeight(contentsRect().height()- 2*d->margin);
    d->area.setWidth(contentsRect().width() - 2*d->margin);
    if ( d->showLegend )
        d->area.setWidth(d->area.width() - d->legendWidth);

    Qt::Axis direction=sliceDirection();
    Qt::Axis horizontal=d->directions[direction].first;
    Qt::Axis vertical=d->directions[direction].second;
    int numSlices = 1;
    if ( d->singleSlice==-1 )
        numSlices = systemMatrix()->numSlices(direction);
    QSize sliceSize = systemMatrix()->sliceMatrix(horizontal,vertical);
    QSizeF sliceFov = systemMatrix()->sliceFov(horizontal,vertical);
    double resH=sliceFov.width()/sliceSize.width();
    double resV=sliceFov.height()/sliceSize.height();
    double ratio=resH/resV;
    if ( ratio>1.0 )
        sliceSize.setWidth(ratio*sliceSize.width());
    else
        sliceSize.setHeight(1.0/ratio*sliceSize.height());

    // Determine layout
    int columns = ceil ( sqrt ( static_cast<double> ( numSlices ) ) * 4.0 * sliceSize.height() * d->area.width() / ( ( sliceSize.width() + sliceSize.height() )*( d->area.width()+ d->area.height()) ) );
    columns = qMax(columns,1);
    int rows = ceil ( static_cast<double> ( numSlices ) / columns );

    // Determine plottable area
    QSize availableSize((d->area.width()-(columns-1)*d->spacing)/columns,(d->area.height()-(rows-1)*d->spacing)/rows);
    setMinimumSize(2*d->margin+d->spacing*(columns-1)+sliceSize.width()*columns+(d->showLegend?d->legendWidth:0),2*d->margin+d->spacing*(rows-1)+sliceSize.height()*rows);
    QSize decorationSize(6*d->tickLength,6*d->tickLength);
    decorationSize.rwidth() += tickFontMetrics.width("-XXXX mm"); // left side
    decorationSize.rwidth() += labelFontMetrics.width("X"); // right side
    decorationSize.rwidth() += 10; // Gap between ticks and labels
    decorationSize.rheight() += tickFontMetrics.height(); // bottom
    decorationSize.rheight() += labelFontMetrics.height(); // top
    decorationSize.rheight() += labelFontMetrics.height(); // bottom
    decorationSize.rheight() += 15; // gaps

    QSize testSize = availableSize-decorationSize;
    QPoint frameCenter;
    if ( testSize.width() < sliceSize.width() || testSize.height() < sliceSize.height() || ! d->showTicks )
    {
        sliceSize.scale(availableSize,Qt::KeepAspectRatio);
        frameCenter = QPoint( sliceSize.width()/2,sliceSize.height()/2 );
        d->decoration=QPicture(); // clear
    }
    else
    {
        sliceSize.scale(testSize,Qt::KeepAspectRatio);
        frameCenter = QPoint( sliceSize.width()/2+3*d->tickLength+tickFontMetrics.width("-XXXX mm"),
                              sliceSize.height()/2+3*d->tickLength+tickFontMetrics.height() );
        QPainter p(& d->decoration);
        p.setPen ( Qt::black );
        drawTicks(&p,QPoint(availableSize.width()/2-1,availableSize.height()/2-1),sliceSize);
    }

    d->sliceSize = sliceSize;
    int p=0;
    for ( int i=0; i<rows && p<numSlices; i++ )
    {
        int y=d->area.top()+i*(availableSize.height()+d->spacing);
        for ( int j=0; j<columns && p<numSlices; j++, p++ )
        {
            int x=d->area.left()+j*(availableSize.width()+d->spacing);
            d->areaList.append(QRect(QPoint(x,y),availableSize));
        }
    }
    update();
}

void PlotWidget::paintEvent(QPaintEvent* ev)
{
    QFontMetrics tickFontMetrics(d->tickFont);
    QPainter p(this);
    p.fillRect(ev->rect(),Qt::white);
    if ( 0==systemMatrix() )
    {
        QPoint pos = contentsRect().center();
        pos.rx() -= d->title.size().width()/2;
        pos.ry() -= d->title.size().height()/2;
        p.setFont( d->titleFont );
        p.drawStaticText(pos,d->title);
        return;
    }
    int slice=0;
    SFRenderer::Colorization cm = SFRenderer::PerFrame;
    foreach(QRect r, d->areaList)
    {
        if ( d->singleSlice!=-1 )
        {
            slice=d->singleSlice;
            cm = SFRenderer::PerSlice;
        }
        QImage image = d->renderer->image( d->index, slice, cm ).scaled(d->sliceSize,Qt::IgnoreAspectRatio, d->transformationMode );
        p.drawPicture( r.topLeft(),d->decoration);
        if ( ! d->decoration.isNull() )
        {
            QString label("%1 = %2 mm");
            label = label.arg(axisName(sliceDirection())).arg(systemMatrix()->slicePosition(sliceDirection(),slice));
            QRect q=image.rect().adjusted(-50,0,50,0);
            q.moveCenter(r.center());
            q.translate(0,image.height()+7*d->tickLength+tickFontMetrics.height());
            p.save();
            p.setFont(d->labelFont);
            p.drawText( q, Qt::AlignHCenter | Qt::AlignTop , label );
            p.restore();
        }
        QPoint pos=r.center();
        pos.rx()-= image.width()/2;
        pos.ry()-= image.height()/2;
        p.drawImage(pos,image);
        if ( d->highlightPosition.isValid() && d->highlightPosition.index(sliceDirection())==slice )
        {
            p.save();
            p.setPen( Qt::white );
            int x = pos.x()+(static_cast<double>(image.width())/(systemMatrix()->dimension(horizontalAxis()))*(0.5+d->highlightPosition.index(horizontalAxis()))-2.5);
            int y = pos.y()+(static_cast<double>(image.height())/(systemMatrix()->dimension(verticalAxis()))*(0.5+d->highlightPosition.index(verticalAxis()))-2.5);
            p.drawEllipse(x,y,5,5);
            p.restore();
        }
        slice++;
    }
    if ( d->showLegend )
    {
        QRect r(contentsRect().width()-d->margin-d->legendWidth,d->margin,d->legendWidth,contentsRect().height()-2*d->margin);
        int slice=-1;
        if ( cm == SFRenderer::PerSlice )
            slice=d->singleSlice;
        d->renderer->plotLegend( &p,r, d->index, slice );
    }
}

void PlotWidget::drawTicks ( QPainter * p, QPoint pos, QSize size ) {
    
    const char * labels[] = { "YZ", "XZ", "XY" };
    p->setFont ( d->tickFont );
    Qt::Axis direction = sliceDirection();
    QSizeF extent = systemMatrix()->sliceFov ( d->directions[direction].first, d->directions[direction].second );
    int xEnds = floor ( extent.width() / 2 );
    int yEnds = floor ( extent.height() / 2 );

    for ( int j = -xEnds; j <= xEnds; j++ ) {
        // Draw ticks along horizontal axis
        double tick = d->tickLength;

        if ( j % 10 == 0 )
            tick *= 3;
        else if ( j % 5 == 0 )
            tick *= 2;

        QPoint p1 = pos;
        p1.rx() += j * size.width() / extent.width();
        p1.ry() -= ( size.height() / 2 + tick );
        QPoint p2 = p1;
        p2.ry() += size.height() + 2 * tick;
        p->drawLine ( p1, p2 );

        if ( (xEnds >= 10 && j % 10 == 0) || (xEnds < 10 && j % 5 == 0) ) {
            p2.ry() += 5;
            p2.rx() -= 30;
            p1 = p2;
            p1.rx() += 60;
            p1.ry() += 20;
            QRect r ( p2, p1 );
            p->drawText ( r, Qt::AlignCenter, tr ( "%1 mm" ).arg ( j ) );
        }
    }
    {
        // Draw horizontal axis label
        QPoint p1=pos;
        p1.rx() -=30;
        p1.ry() -= ( size.height() / 2 + 4*d->tickLength + 20);
        QPoint p2=p1;
        p2.rx() +=60;
        p2.ry() +=20;
        QRect r( p1, p2);
        p->drawText( r, Qt::AlignHCenter | Qt::AlignBottom, QString ( labels[static_cast<int>(sliceDirection())][0] ) );
    }
    for ( int j = -yEnds; j <= yEnds; j++ ) {
        // Draw vertical axis ticks
        double tick = d->tickLength;

        if ( j % 10 == 0 )
            tick *= 3;
        else if ( j % 5 == 0 )
            tick *= 2;

        QPoint p1 = pos;
        p1.rx() -= ( size.width() / 2 + tick );
        p1.ry() += j * size.height() / extent.height();
        QPoint p2 = p1;
        p2.rx() += size.width() + 2 * tick;
        p->drawLine ( p1, p2 );

        if ( (yEnds >= 10 && j % 10 == 0) || (yEnds < 10 && j % 5 == 0) ) {
            QPoint p1 = pos;
            p1.rx() -= ( size.width() / 2 + tick + 10 );
            p1.ry() += j * size.height() / extent.height() - 10;
            p2 = p1;
            p1.rx() -= 100;
            p2.ry() += 20;
            QRect r ( p1, p2 );
            p->drawText ( r, Qt::AlignVCenter | Qt::AlignRight, tr ( "%1 mm" ).arg ( j ) );
        }
    }
    {
        // Draw vertical axis label
        QPoint p1=pos;
        p1.rx() += (size.width()/2 + 4*d->tickLength );
        p1.ry() -= 20;
        QPoint p2=p1;
        p2.rx() += 30;
        p2.ry() += 40;
        QRect r ( p1, p2 );
        p->drawText( r, Qt::AlignVCenter | Qt::AlignLeft, QString ( labels[static_cast<int>(sliceDirection())][1] ) );
    }
    
}

QString PlotWidget::axisName(Qt::Axis axis) const
{
    switch ( axis )
    {
        case Qt::XAxis : return "X";
        case Qt::YAxis : return "Y";
        case Qt::ZAxis : return "Z";
        default: return "";
    }
}

void PlotWidget::resizeEvent(QResizeEvent* ev)
{
    QFrame::resizeEvent(ev);
    relayout();
}

void PlotWidget::mouseMoveEvent(QMouseEvent * mouseEvent)
{
    MatrixPosition pos;
    SystemMatrix::complex c;
    if ( isVoxel(mouseEvent->pos(),&pos ) )
        c = systemMatrix()->dataPoint(d->index,pos,backgroundCorrection());
    emit currentPositionAndValue(pos,c);
}

void PlotWidget::mousePressEvent(QMouseEvent * mouseEvent)
{
    MatrixPosition pos;
    if ( isVoxel(mouseEvent->pos(),&pos) && mouseEvent->button() == Qt::RightButton )
        emit requestContextMenu(mouseEvent->globalPos(),pos);
}

void PlotWidget::mouseDoubleClickEvent(QMouseEvent * mouseEvent)
{
    MatrixPosition pos;
    if ( isVoxel(mouseEvent->pos(),&pos) && mouseEvent->button() == Qt::LeftButton )
    {
        int slice=this->slice(mouseEvent->pos());
        if ( slice!=-1 )
        {
            if ( visibleSlice()==-1 )
                showSingleSlice(slice);
            else
                showAllSlices();
        }
    }
}

void PlotWidget::wheelEvent(QWheelEvent * wheelEvent)
{
    int currentSlice=visibleSlice();
    MatrixPosition pos;
    if ( isVoxel(wheelEvent->pos(),&pos) && currentSlice!=-1 )
    {
        if ( wheelEvent->angleDelta().y() > 0 )
            currentSlice++;
        else if ( wheelEvent->angleDelta().y()<0 )
            currentSlice--;
        showSingleSlice(currentSlice);
        SystemMatrix::complex c = systemMatrix()->dataPoint(d->index,pos,backgroundCorrection());
        emit currentPositionAndValue(pos,c);
    }

}

int PlotWidget::visibleSlice() const
{
    return d->singleSlice;
}

int PlotWidget::slice(const QPoint &pos) const
{
    if ( 0==systemMatrix() )
        return false;
    unsigned int slice=0;
    foreach (QRect r,d->areaList)
    {
        if (r.contains(pos))
        {
            QRect imageArea(r.center(),d->sliceSize);
            imageArea.translate(-d->sliceSize.width()/2,-d->sliceSize.height()/2);
            if ( imageArea.contains(pos))
                return slice;
        }
        slice++;
    }
    return -1;
}

bool PlotWidget::isVoxel(const QPoint &widgetPos, MatrixPosition * matrixPos) const
{
    if ( 0==systemMatrix() )
        return false;
    unsigned int slice=0;
    if ( d->singleSlice!=-1 )
        slice=d->singleSlice;
    foreach (QRect r,d->areaList)
    {
        if (r.contains(widgetPos))
        {
            QRect imageArea(r.center(),d->sliceSize);
            imageArea.translate(-d->sliceSize.width()/2,-d->sliceSize.height()/2);
            if ( imageArea.contains(widgetPos))
            {
                if ( 0!=matrixPos )
                {
                    QPointF p(widgetPos-imageArea.topLeft());
                    p.rx() /= d->sliceSize.width();
                    p.ry() /= d->sliceSize.height();
                    p.rx() *= systemMatrix()->dimension(d->renderer->horizontalAxis());
                    p.ry() *= systemMatrix()->dimension(d->renderer->verticalAxis());
                    matrixPos->set(d->renderer->horizontalAxis(),floor(p.x()));
                    matrixPos->set(d->renderer->verticalAxis(),floor(p.y()));
                    matrixPos->set(d->renderer->sliceDirection(),slice);
                }
                return true;
            }
        }
        slice++;
    }
    return false;
}

void PlotWidget::imageToClipboard()
{
    QClipboard * clipboard = QApplication::clipboard();
    
    QImage image(contentsRect().size(),QImage::Format_RGB32);
    render(&image,QPoint(),contentsRect());
    clipboard->setImage(image);
}

