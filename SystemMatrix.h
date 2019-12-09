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
 * $Id: SystemMatrix.h 84 2017-03-12 00:08:59Z uhei $
 */

#ifndef SYSTEMMATRIX_H
#define SYSTEMMATRIX_H

// Standard includes
#include <complex>

// Qt includes
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QList>
#include <QtCore/QDateTime>
#include <QtGui/QVector3D>

// Local includes
#include "MatrixPosition.h"

class SystemMatrix  : public QObject {
        Q_OBJECT
    public:
        enum Mode { Viewer, Editor  };
        typedef std::complex<double> complex;
        SystemMatrix ( const QString & procnoPath, Mode=Viewer, QObject * parent=0 );
        virtual ~SystemMatrix();
        QString path() const;
        QString institution() const;
        QString systemName() const;
        QString systemId() const;
        QString manufacturer() const;
        QString experimentName() const;
        QDateTime experimentDate() const;
        int dimension( Qt::Axis ) const;
        double spatialExtent ( Qt::Axis ) const;
        int globalIndex( int receiver, int frequencyIndex) const;
        int globalIndex( int snrRank ) const;
        int globalIndex( int receiver, int mixingTerms[3] ) const;
        int receiver( int globalIndex ) const;
        int frequencyIndex( int globalIndex ) const;
        int snrIndex( int globalIndex ) const;
        double frequency( int globalIndex ) const;
        double snr( int globalIndex ) const;
        int mixingOrder( int globalIndex, int mixingTerms[3]=0 ) const;
        QList<QVector3D> mixingTerms(int globalIndex) const;
        int numSlices( Qt::Axis sliceDirection ) const;
        QString tracerName() const;
        double tracerConcentration() const;
        double tracerVolume() const;
        QSize sliceMatrix( Qt::Axis horizontal, Qt::Axis vertical ) const;
        QSizeF sliceFov( Qt::Axis horizontal, Qt::Axis vertical ) const;
        double slicePosition( Qt::Axis sliceDirection, int direction ) const;
        int numberOfReceivers() const;
        int numberOfFrequencies() const;
        int maxGlobalIndex() const;
        double selectionField() const;
        double driveField(unsigned int channel) const;
        int averages() const;
        double bandwidth() const;
        bool isModified() const;
        bool isValid ( QString * errorMsg = 0 ) const;
        const complex * rawData(int globalIndex, bool backgroundCorrection ) const;
        complex background( int globalIndex ) const;
        double backgroundVariance( int globalIndex ) const;
        double backgroundNoise( int globalIndex ) const;
        bool validPosition(const MatrixPosition & pos) const;
        complex dataPoint(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const;
        complex interpolated(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const;
        bool interpolateDatapoint(int globalIndex, const MatrixPosition & pos, double threshold=0.0);
        int interpolateDatapoints(const QList<int> & indices, const MatrixPosition & pos, double threshold, QObject * progressReceiver=0, const char * progressSlot=0);
        int increment( Qt::Axis direction) const;
        QString lastChangeDescription() const;
        void undoLastChange();
        void undoAllChanges(QObject * progressReceiver=0, const char * progressSlot=0);
    protected:
        /**
         * @brief setInstitution Set the name of the institution that operated the scanner
         * @param institution    Institution name
         */
        void setInstitution(const QString & institution);
        /**
         * @brief setSystemName  Set the name of the scanner that generated the data
         * @param systemName     Scanner name
         */
        void setSystemName(const QString & systemName);
        /**
         * @brief setSystemId    Set a unique ID of the scanner that generated the data, if applicable
         * @param systemId       Scanner id
         */
        void setSystemId(const QString & systemId);
        /**
         * @brief setManufacturer Set the name of the system manufacturer
         * @param manufacturer    System manufacturer
         */
        void setManufacturer(const QString & manufacturer);
        /**
         * @brief setExperimentName Set a descriptive name of the experiment
         * @param experimentName    Experiment name
         */
        void setExperimentName(const QString & experimentName);
        /**
         * @brief setExperimentDate Set time when the experiment was carried out
         * @param dateTime          Experiment time
         */
        void setExperimentDate(const QDateTime & dateTime);
        /**
         * @brief setTracerName Set the name of the used tracer
         * @param tracerName    Name of tracer
         */
        void setTracerName(const QString & tracerName);
        /**
         * @brief setTracerConcentration Set the used tracer concentration.
         * @param concentration          Tracer concentration (in Mol/L)
         */
        void setTracerConcentration(double concentration);
        /**
         * @brief setTracerVolume Set the used tracer volume
         * @param volume          Tracer volume ( in microL )
         */
        void setTracerVolume(double volume);
        /**
         * @brief setAverages     Set number of averages used for generating the data
         * @param averages        Number of averages
         */
        void setAverages(int averages);
    signals:
        void dataChange();
    private:
        struct Impl;
        Impl * d;
};

#endif // SYSTEMMATRIX_H
