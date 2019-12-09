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
 * $Id: SystemMatrix.cpp 84 2017-03-12 00:08:59Z uhei $
 */

// Qt includes
#include <QtGlobal>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QCache>
#include <QtCore/QByteArray>
#include <QtGui/QVector3D>
#include <QtWidgets/QMessageBox>

// Local includes
#include "SystemMatrix.h"
#include "PvParameterFile.h"
#include "ChangeListV1.h"
#include "ChangeList.h"
#include "TransferFunction.h"

static int lcm(int n, int * a);

// Change if phase correction should modify amplitudes as well
static const bool correctPhaseOnly = true;

static const unsigned int changeTableVersion = 0x02;

template<typename T>
static T sqr(const T & a ) { return a*a; }

struct SystemMatrix::Impl {
    QString error;
    int baseFrequencyIndex[3];
    int maxMixingOrder;
    QString procnoPath;
    complex * rawDataUncorrected;
    complex * rawDataCorrected;
    typedef QCache<int,QByteArray> DataCache;
    DataCache dataCache;
    int grid[3];
    double fov[3];
    double offset[3];
    int numFrequencies, numChannels, positions, numBgPositions;
    double bandwidth;
    double * snrValueTable;
    QList<int> snrIndexTable;
    const complex * backgroundReference;
    const complex * allBackground;
    const double * backgroundVariance;
    QVector<double> backgroundNoise_;
    typedef QMap<int,QVector3D> MixTableType;
    MixTableType mixTable;
    PvParameterFile * methRecoParameters, * recoParameters, * acqpParameters, * methodParameters;
    Mode mode;
    bool snrInDFFOV;
    ChangeList changeList;
    QVector<TransferFunction*> transferFunction;
    typedef QMap<QFile*,uchar*> FileMappingTable;
    FileMappingTable mappedFiles;
    QString manufacturer,institution, systemName, systemId, experimentName,tracerName;
    QDateTime experimentDate;
    double tracerConcentration,tracerVolume;
    int averages;

    template<typename T>
    qint64 mapFile(const QString & fileName, T ** p, QFile::OpenMode mode, QString * error=0)
    {
        Q_ASSERT( p!=0 );

        FileMappingTable::iterator i=mappedFiles.begin();
        while ( i!=mappedFiles.end() )
        {
            QFile * file = i.key();
            uchar * q = i.value();
            if ( file->fileName()==fileName || reinterpret_cast<T*>(q)==*p )
            {
                file->unmap(q);
                i = mappedFiles.erase(i);
                delete file;
            }
            else
                ++i;
        }

        QFile * file = new QFile(fileName);
        if ( ! file->exists() )
        {
             if ( error ) *error = tr ( "File %1 does not exist.").arg(fileName);
             delete file;
             return 0;
        }
        if ( !file->open(mode) )
        {
            if ( error ) * error = tr ( "Cannot open file %1.").arg(fileName);
            delete file;
            return 0;
        }

        uchar * q = file->map(0,file->size());
        if ( 0 == q )
        {
             if ( error ) *error = tr ("Cannot map file %1 into memory.").arg(fileName);
             delete file;
             return 0;
        }
        *p = reinterpret_cast<T*>(q);
        mappedFiles.insert(file,q);
        file->close();
        return file->size();
    }

    bool unmapFile(const QString & fileName)
    {
        FileMappingTable::iterator i=mappedFiles.begin();
        while ( i!=mappedFiles.end() )
        {
            QFile * file = i.key();
            uchar * q = i.value();
            if ( file->fileName()==fileName )
            {
                file->unmap(q);
                i = mappedFiles.erase(i);
                delete file;
                return true;
            }
            else
                ++i;
        }
        return false;
    }

    void unmapAll()
    {
        FileMappingTable::iterator i=mappedFiles.begin();
        while ( i!=mappedFiles.end() )
        {
            QFile * file = i.key();
            uchar * q = i.value();
            file->unmap(q);
            i = mappedFiles.erase(i);
            delete file;
        }
    }

    bool validPosition(const MatrixPosition & pos) const
    {
        for ( unsigned int i=0; i<3; i++ )
        {
            if ( static_cast<int>(pos.index(i))>=grid[i] )
                return false;
        }
        return true;
    }

    double driveField(unsigned int channel) const
    {
        if ( ! methodParameters->isValid() )
            return 0.0;
        if ( ! methodParameters->isArray("PVM_MPI_DriveFieldStrength") )
            return 0.0;
        if ( static_cast<int>(channel) >= methodParameters->dimension("PVM_MPI_DriveFieldStrength") )
            return 0.0;
        return methodParameters->value<double>("PVM_MPI_DriveFieldStrength", channel);
    }

    double selectionField() const
    {
        if ( methodParameters->isValid() )
            return methodParameters->value<double>("PVM_MPI_SelectionFieldGradient");
        return 0.0;
    }

    complex dataPoint(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const
    {
        complex result(0.0,0.0);
        if ( validPosition(pos) && globalIndex>=0 && globalIndex < numChannels*numFrequencies )
        {
            size_t block = grid[0] * grid[1] * grid[2];
            complex * p = rawDataUncorrected;
            if (backgroundCorrection)
                p = rawDataCorrected;
            p += block*globalIndex;
            int offset=(pos.z()*grid[1]+pos.y())*grid[0]+pos.x();
            p+=offset;
            result = *p;
        }
        return result;
    }

    bool setDataPoint(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection, const complex & value)
    {
        if ( !validPosition(pos) || mode==Viewer )
            return false;
        if ( globalIndex<0 || globalIndex>=numFrequencies*numChannels )
            return false;
        size_t block=grid[0]*grid[1]*grid[2];
        size_t offset=(pos.z()*grid[1]+pos.y())*grid[0]+pos.x();
        complex * p=rawDataUncorrected;
        if ( backgroundCorrection)
            p = rawDataCorrected;

        p += block*globalIndex+offset;

        *p = value;
        return true;
    }

    complex interpolated(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const
    {
        MatrixPosition p;
        complex res(0.0,0.0);
        double weight=0.0;

        for ( int i=-1; i<=1; i++ )
        {
            p.setX(pos.x()+i);
            double dist=sqr(i*fov[0]/grid[0]);
            for ( int j=-1; j<=1; j++ )
            {
                p.setY(pos.y()+j);
                dist += sqr(j*fov[1]/grid[1]);
                for ( int k=-1; k<=1; k++ )
                {
                    p.setZ(pos.z()+k);
                    dist += sqr(k*fov[2]/grid[2]);
                    if ( i==0 && j==0 && k==0 )
                        continue;
                    dist=sqrt(dist);
                    if ( validPosition(p) )
                    {
                        double w=1.0/dist;
                        res+=w*dataPoint(globalIndex,p,backgroundCorrection);
                        weight+=w;
                    }
                }
            }
        }
        if (weight==0.0)
            return res;
        return res/weight;
    }

    double backgroundNoise(int globalIndex)
    {
        if ( globalIndex<0 || globalIndex>= numChannels*numFrequencies )
            return 0.0;
        if ( globalIndex>=backgroundNoise_.size() )
            return 0.0;
        double & res=backgroundNoise_[ globalIndex ];
        if ( 0.0==res )
        {
            const complex * p=allBackground + numBgPositions*globalIndex;

            complex mean = 0.0;
            QList<complex> values;
            values.reserve(numBgPositions);
            for ( int i=0; i<numBgPositions; ++i, ++p )
                mean += *p;

            mean /= numBgPositions;
            double noise=0.0;
            p=allBackground + numBgPositions*globalIndex;
            for ( int i=0; i<numBgPositions; ++i, ++p )
            {
                noise += abs(*p-mean);
            }
            noise /= numBgPositions;
            res = noise;
        }
        return res;
    }

    QString writeModificationTable()
    {
        QStringList errors;

        // Write binary table
        QFile binaryTable ( procnoPath + "/modificationTable.bin" );
        if ( changeList.empty() )
        {
            if ( !binaryTable.remove() )
                errors += tr( "Cannot remove %1 although no changes exist." ).arg(binaryTable.fileName());
        }
        else if ( ! binaryTable.open ( QIODevice::WriteOnly ) )
        {
            errors += tr ( "Cannot open %1 for writing.").arg(binaryTable.fileName());
        }
        else
        {
            QDataStream ds ( &binaryTable );
            ds.setByteOrder( QDataStream::LittleEndian );
            ds << changeTableVersion;
            ds << changeList;
        }

        // Write text table
        QFile textTable ( procnoPath + "/modificationTable.txt" );
        if ( changeList.empty() )
        {
            if ( !textTable.remove() )
                errors += tr( "Cannot remove %1 although no changes exist." ).arg(binaryTable.fileName());
        }
        else if ( ! textTable.open ( QIODevice::WriteOnly ) )
        {
            errors += tr ( "Cannot open %1 for writing.").arg(textTable.fileName());
        }
        else
        {
            QTextStream ts ( &textTable );
            ts << "List of changed voxels:\n";
            foreach(ChangeListEntry c, changeList )
            {
                ts << c << "\n";
            }
        }
        return errors.join("\n");
    }

    void recalcSNR(int globalIndex)
    {
        if ( globalIndex<0 || globalIndex>= numChannels*numFrequencies )
            return;
        double dfFov[3];
        for ( unsigned int i=0; i<3; i++)
        {
            dfFov[i] = driveField(i)/selectionField();
            if ( i<2 )
                dfFov[i] *= 2.0;
        }

        size_t block=grid[0]*grid[1]*grid[2];
        complex * p = rawDataCorrected;
        p+=block*globalIndex;

        double v=0.0;
        int count = 0;
        for ( int k=0; k<grid[2]; k++ )
        {
            double z= (fov[2]/grid[2])*k+offset[2]+0.5*fov[2]*(1.0/grid[2]-1.0);
            if ( fabs(z) > dfFov[2] && snrInDFFOV )
                continue;

            for ( int j=0; j<grid[1]; j++)
            {
                double y= (fov[1]/grid[1])*j+offset[1]+0.5*fov[1]*(1.0/grid[1]-1.0);
                if ( fabs(y) > dfFov[1] && snrInDFFOV )
                    continue;

                for ( int i=0; i<grid[0]; i++ )
                {
                    double x= (fov[0]/grid[0])*i+offset[0]+0.5*fov[0]*(1.0/grid[0]-1.0);
                    if ( fabs(x) > dfFov[0] && snrInDFFOV )
                        continue;

                    int offset=(k*grid[1]+j)*grid[0]+i;
                    v += abs(*(p+offset));
                    count++;
                }
            }
        }
        v /= count;
        v /= backgroundNoise(globalIndex);
        snrValueTable[globalIndex] = v;
    }


    void rebuildSNRIndex()
    {
        double * p = snrValueTable;
        QMultiMap<double, int> snrMap;
        for ( int i=0; i<numChannels*numFrequencies; ++i, ++p )
            snrMap.insert ( *p, i );

        snrIndexTable.clear();
        foreach ( int index, snrMap )
            snrIndexTable.prepend ( index );
    }

    void importChangeTableV1(QDataStream & ds)
    {
        QMap<MatrixPosition,ChangeListV1> changes;
        ds >> changes;
        changeList.clear();
        foreach( const MatrixPosition & pos, changes.keys() )
        {
            const QList<ChangeListEntryV1> & cl = changes[pos];
            QList<ChangeListItem> changeItems;
            foreach (const ChangeListEntryV1 & e1,cl)
            {
                complex value[4];
                value[0]=value[1]=dataPoint(e1.globalIndex,pos,false);
                complex v=interpolated(e1.globalIndex,pos,false);
                if ( abs(v)<abs(value[1]) && abs(value[1]-v)/abs(value[1])>0.1 )
                    value[1]=v;
                value[2]=e1.oldValue;
                value[3]=e1.newValue;
                changeItems.append(ChangeListItem(e1.globalIndex,value));
                recalcSNR(e1.globalIndex);
            }
            changeList.append(ChangeListEntry(pos,changeItems));
        }
        rebuildSNRIndex();
        error = writeModificationTable();
        if ( !error.isEmpty() )
            QMessageBox::warning(0,tr("File error"),error);
    }
};

SystemMatrix::SystemMatrix (const QString & procnoPath, Mode mode, QObject * parent ) : QObject(parent), d(new Impl)
{
    d->mode = mode;
    QFileInfo fi(procnoPath);
    if ( fi.isDir() )
        d->procnoPath = procnoPath;
    else
        d->procnoPath = fi.path();
    if ( ! d->procnoPath.contains("pdata") )
        d->procnoPath.append("/pdata/1");
    d->rawDataUncorrected = 0;
    d->rawDataCorrected = 0;
    d->snrValueTable = 0;
    d->backgroundReference = 0;
    d->backgroundVariance = 0;
    d->allBackground = 0;
    d->tracerVolume = 0.0;
    d->tracerConcentration = 0.0;
    d->averages = 0;
    d->dataCache.setMaxCost(10);

    QDir expno ( d->procnoPath );
    expno.cdUp();
    expno.cdUp();

    d->methRecoParameters = new PvParameterFile ( d->procnoPath + "/methreco", this );

    if ( !d->methRecoParameters->isValid ( &(d->error) ) )
        return;

    d->recoParameters = new PvParameterFile ( d->procnoPath + "/reco", this );

    if ( !d->recoParameters->isValid ( &(d->error) ) )
        return;

    d->acqpParameters = new PvParameterFile ( expno.absolutePath() + "/acqp", this );
    
    if ( !d->acqpParameters->isValid ( &(d->error ) ) )
        return;
    
    setInstitution(d->acqpParameters->value<QString>("ACQ_institution"));
    setSystemName(d->acqpParameters->value<QString>("ACQ_station"));
    setSystemId(d->acqpParameters->value<QString>("ACQ_system_order_number"));
    setExperimentName(d->acqpParameters->value<QString>("ACQ_scan_name"));
    setManufacturer("Bruker BioSpin MRI GmbH");
    QString date = d->acqpParameters->value<QString>("ACQ_time");
    setExperimentDate(QDateTime::fromString(date.left(19),"yyyy-MM-dd'T'HH:mm:ss"));
    setAverages(d->acqpParameters->value<int>("NA"));

    d->methodParameters = new PvParameterFile ( expno.absolutePath() + "/method", this );

    if ( !d->methodParameters->isValid( &(d->error ) ) )
        return;

    setTracerName(d->methodParameters->value<QString>("PVM_MPI_Tracer"));
    setTracerConcentration(d->methodParameters->value<double>("PVM_MPI_TracerConcentration"));
    setTracerVolume(d->methodParameters->value<double>("PVM_MPI_TracerVolume"));

    int dim = d->recoParameters->value<int> ( "RecoDim" );

    d->numFrequencies = d->methRecoParameters->value<int> ( "PVM_MPI_NrFrequencyComponents" );
    d->bandwidth = d->recoParameters->value<double> ( "RECO_sw", 0 );
    d->snrInDFFOV = d->methodParameters->value<QString>( "PVM_MPI_ActivateSNRWithinDFFov" )=="Yes";
    d->numBgPositions = d->methodParameters->value<int>("PVM_MPI_NrBackgroundMeasurementCalibrationAllScans") -
                        d->methodParameters->value<int>("PVM_MPI_NrBackgroundMeasurementCalibrationAdditionalScans");

    for ( unsigned int i = 0; i < 3; i++ )
        d->grid[i] = 1;

    d->positions=1;
    for ( int i = 0; i < dim; i++ )
    {
        d->grid[i] = d->recoParameters->value<int> ( "RECO_size", i );
        d->positions *= d->grid[i];
        d->fov[i] = d->recoParameters->value<double> ( "RECO_fov", i )*10;
        d->offset[i] = d->methodParameters->value<double> ( "PVM_MPI_FovCenter", i );
    }
    
    QIODevice::OpenMode fileMode=QIODevice::ReadWrite;

    if ( d->mode==Viewer )
        fileMode=QIODevice::ReadOnly;

    // Map uncorrected raw data
    qint64 dataSize1 = d->mapFile( d->procnoPath+"/systemMatrix", &d->rawDataUncorrected,fileMode,& d->error);
    if ( dataSize1==0 )
        return;

    // Map corrected raw data
    qint64 dataSize2 = d->mapFile( d->procnoPath+"/systemMatrixBG", &d->rawDataCorrected,fileMode, & d->error);
    if ( dataSize2==0 )
        return;

    if ( dataSize1!=dataSize2 )
    {
        d->error = tr ( "Sizes of corrected and uncorrected system matrices do not match.");
        return;
    }

    // Compute number of channels from raw data.
    // This would better be done from acqp parameters, but is more complicated that way with our simple parser,
    qint64 channels = dataSize1;
    channels /= sizeof(complex);
    channels /= d->positions;
    channels /= d->numFrequencies;
    d->numChannels = channels;

    d->transferFunction.resize(channels);

    // Load transfer functions
    for ( unsigned int i=0; i<channels; i++)
    {
        d->transferFunction[i]=0;
        QString calibFile = QString("%1/chan%2.rxcal").arg(expno.absolutePath()).arg(i+1);
        TransferFunction * tf =  new TransferFunction(calibFile,this);
        if ( tf->isValid() )
            d->transferFunction[i]=tf;
        else
            delete tf;
    }

    // Load SNR data
    qint64 mappedSize=d->mapFile( d->procnoPath+"/snr", & d->snrValueTable, fileMode, & d->error );
    if ( mappedSize==0 )
        return;

    qint64 expectedSize=static_cast<qint64>(d->numFrequencies)*d->numChannels*sizeof(double);
    if ( expectedSize!=mappedSize )
    {
        d->error = tr ("SNR file has wrong file size (%1 bytes instead of expected %2 bytes).").arg(mappedSize).arg(expectedSize);
        return;
    }
    d->rebuildSNRIndex();

    // Load background reference
    mappedSize=d->mapFile( d->procnoPath + "/backgroundReference", & d->backgroundReference, QFile::ReadOnly, & d->error);
    if ( mappedSize==0 )
        return;

    expectedSize = static_cast<qint64>(d->numFrequencies)*d->numChannels*sizeof(complex);
    if ( expectedSize != mappedSize )
    {
        d->error = tr ("Background reference file has wrong file size (%1 bytes instead of expected %2 bytes).").arg(mappedSize).arg(expectedSize);
        return;
    }

    // Load background variance
    mappedSize=d->mapFile( d->procnoPath + "/backgroundVariance", &d->backgroundVariance, QFile::ReadOnly, & d->error );
    if ( mappedSize==0 )
        return;

    expectedSize = static_cast<qint64>(d->numFrequencies)*d->numChannels*sizeof(double);
    if ( expectedSize != mappedSize )
    {
        d->error = tr ("Background variance file has wrong file size (%1 bytes instead of expected %2 bytes).").arg(mappedSize).arg(expectedSize);
        return;
    }

    // Load all background data, required for SNR recalculation in case of editor
    if ( mode==Editor )
    {
        mappedSize = d->mapFile( d->procnoPath + "/background", & d->allBackground, QFile::ReadOnly, & d->error );
        if ( mappedSize == 0 )
            return;

        expectedSize = static_cast<qint64>(d->numFrequencies)*d->numChannels*d->numBgPositions*sizeof(complex);
        if ( expectedSize != mappedSize )
        {
            d->error = tr ("Background file has wrong file size (%1 bytes instead of expected %2 bytes).").arg(mappedSize).arg(expectedSize);
            return;
        }
        // Automatically initialized to 0.0, will be filled on demand to save time on loading
        d->backgroundNoise_.resize(d->numFrequencies*d->numChannels);
    }

    // Load previous modification table
    if ( mode==Editor )
    {
        QFile modificationTable ( d->procnoPath + "/modificationTable.bin" );
        if ( modificationTable.exists() )
        {
            if ( ! modificationTable.open ( QIODevice::ReadOnly ) )
            {
                d->error = tr ( "Previous modification table exists, but cannot be opened.");
                return;
            }
            QDataStream ds ( &modificationTable );
            ds.setByteOrder( QDataStream::LittleEndian );
            unsigned int version;
            ds >> version;
            switch ( version )
            {
                case 0x01:
                    d->importChangeTableV1(ds);
                    break;
                case changeTableVersion:
                    ds >> d->changeList;
                    break;
                default:
                    d->error = tr ( "Cannot read modification table version %1.").arg(version);
                    break;
            }
        }
    }

    int div[3];
    for ( unsigned int i=0; i<3; i++ )
        div[i] = d->acqpParameters->value<int>("ACQ_MPI_div",i);
    for ( unsigned int i=0; i<3; i++ )
        d->baseFrequencyIndex[i]=lcm(3,div)/div[i];
    
    QSettings settings;
    d->maxMixingOrder=settings.value("maxMixingOrder",50).toInt();

    for ( int i=-d->maxMixingOrder; i<=d->maxMixingOrder; i++ )
        for ( int j=-d->maxMixingOrder+abs(i); j<=d->maxMixingOrder-abs(i); j++ )
            for ( int k=-d->maxMixingOrder+abs(i)+abs(j); k<=d->maxMixingOrder-abs(i)-abs(j); k++ )
            {
                int index=d->baseFrequencyIndex[0]*i+d->baseFrequencyIndex[1]*j+d->baseFrequencyIndex[2]*k;
                if ( index<0 || index>=d->numFrequencies )
                    continue;
                d->mixTable.insertMulti(index,QVector3D(i,j,k));
            }
}

SystemMatrix::~SystemMatrix() {
    d->unmapAll();
    delete d;
}

bool SystemMatrix::isModified() const
{
    return ! d->changeList.empty();
}

bool SystemMatrix::isValid ( QString * errorMsg ) const {
    if ( d->error.isEmpty() )
        return true;
    if ( errorMsg!=0 )
        *errorMsg = d->error;
    return false;
}

QString SystemMatrix::path() const
{
    return d->procnoPath;
}

void SystemMatrix::setInstitution(const QString & institution)
{
    d->institution = institution;
}

QString SystemMatrix::institution() const
{
    return d->institution;
}

void SystemMatrix::setSystemName(const QString & systemName)
{
    d->systemName = systemName;
}

QString SystemMatrix::systemName() const
{
    return d->systemName;
}

void SystemMatrix::setSystemId(const QString & systemId)
{
    d->systemId = systemId;
}

QString SystemMatrix::systemId() const
{
    return d->systemId;
}

void SystemMatrix::setManufacturer(const QString &manufacturer)
{
    d->manufacturer=manufacturer;
}

QString SystemMatrix::manufacturer() const
{
    return d->manufacturer;
}

void SystemMatrix::setExperimentName(const QString & experimentName)
{
    d->experimentName=experimentName;
}

QString SystemMatrix::experimentName() const
{
    return d->experimentName;
}

void SystemMatrix::setExperimentDate(const QDateTime & experimentDate)
{
    d->experimentDate = experimentDate;
}

QDateTime SystemMatrix::experimentDate() const
{
    return d->experimentDate;
}

int SystemMatrix::dimension ( Qt::Axis direction ) const {
    return d->grid[static_cast<unsigned int >(direction)];
}

double SystemMatrix::spatialExtent( Qt::Axis direction ) const
{
    return d->fov[static_cast<unsigned int >(direction)];
}

int SystemMatrix::globalIndex(int receiver, int index) const
{
    if ( receiver<0 || receiver >= d->numChannels )
        return -1;
    if ( index<0 || index >= d->numFrequencies )
        return -1;
    return receiver*d->numFrequencies+index;
}

int SystemMatrix::globalIndex(int snrRank) const
{
    if ( snrRank<0 || snrRank>= d->snrIndexTable.count() )
        return -1;
    return d->snrIndexTable.at( snrRank );
}

int SystemMatrix::globalIndex(int receiver, int mixingTerms[3]) const
{
    int index=0;
    for ( unsigned int i=0; i<3; i++ )
        index+=d->baseFrequencyIndex[i]*mixingTerms[i];
    return globalIndex(receiver,index);
}

int SystemMatrix::receiver(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1;
    return globalIndex/d->numFrequencies;
}


int SystemMatrix::frequencyIndex(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1;
    return globalIndex%d->numFrequencies;
}

int SystemMatrix::snrIndex(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1;
    return d->snrIndexTable.indexOf ( globalIndex );
}

double SystemMatrix::snr(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1.0;
    return d->snrValueTable[globalIndex];
}

double SystemMatrix::frequency(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1.0;
    return d->bandwidth * frequencyIndex(globalIndex) / (d->numFrequencies-1);
}

int SystemMatrix::mixingOrder(int globalIndex, int mixingTerms[3]) const
{
    if ( globalIndex<0 || globalIndex>=d->numChannels*d->numFrequencies )
        return -1;
    int order=d->maxMixingOrder+1;
    QVector3D res(0,0,0);
    int index = frequencyIndex(globalIndex);
    if ( d->mixTable.contains( index ) )
    {
        QList<QVector3D> values=d->mixTable.values(index);
        res=values.first();
        foreach(QVector3D v,values)
        {
            int o = abs(v.x())+abs(v.y())+abs(v.z());
            if ( o<=order )
            {
                order=o;
                res=v;
            }
        }
    }
    if ( order==d->maxMixingOrder+1 )
        order=-1;
    if ( mixingTerms )
    {
        mixingTerms[0] = res.x();
        mixingTerms[1] = res.y();
        mixingTerms[2] = res.z();
    }
    return order;

}

static bool compareVectors(const QVector3D & a, const QVector3D & b)
{
    int absA = abs(a.x()) + abs(a.y()) + abs(a.z());
    int absB = abs(b.x()) + abs(b.y()) + abs(b.z());
    return absA<absB;
}

QList<QVector3D> SystemMatrix::mixingTerms(int globalIndex) const
{
    QList<QVector3D> res;
    if ( globalIndex>=0 && globalIndex<d->numChannels*d->numFrequencies )
    {
        int index = frequencyIndex(globalIndex);
        if ( d->mixTable.contains( index ) )
        {
            res = d->mixTable.values(index);
            qSort(res.begin(),res.end(),compareVectors);
        }
    }
    return res;
}

int SystemMatrix::numSlices(Qt::Axis sliceDirection) const
{
    return dimension(sliceDirection);
}

QSize SystemMatrix::sliceMatrix(Qt::Axis horizontal, Qt::Axis vertical) const
{
    return QSize(dimension(horizontal),dimension(vertical));
}

QSizeF SystemMatrix::sliceFov(Qt::Axis horizontal, Qt::Axis vertical) const
{
    return QSizeF(spatialExtent(horizontal),spatialExtent(vertical));
}


double SystemMatrix::slicePosition ( Qt::Axis sliceDirection, int index ) const {
    double fov=spatialExtent(sliceDirection);
    if ( 0.0==fov )
        return 0.0;
    int dim=dimension(sliceDirection);
    if ( dim<=0 )
        return 0.0;
    double step=fov/dim;
    return step*index-0.5*(fov-step);
}

const SystemMatrix::complex* SystemMatrix::rawData(int globalIndex, bool backgroundCorrection) const
{
    if ( globalIndex<0 || globalIndex>= d->numChannels*d->numFrequencies )
        return 0;
    int cacheKey=backgroundCorrection?-globalIndex:globalIndex;
    int channel = receiver(globalIndex);
    TransferFunction * tf = d->transferFunction[channel];
    if ( tf )
    {
        if ( d->dataCache.contains(cacheKey) )
            return reinterpret_cast<const complex*>(d->dataCache.object(cacheKey)->constData());
    }
    size_t block=d->grid[0]*d->grid[1]*d->grid[2];
    complex * p = backgroundCorrection?d->rawDataCorrected:d->rawDataUncorrected;
    if ( tf )
    {
        QByteArray * data=new QByteArray(reinterpret_cast<char*>(p+block*globalIndex),static_cast<int>(block*sizeof(complex)));
        p = reinterpret_cast<complex*>(data->data());
        complex corr = tf->correctionFactor(frequency(globalIndex));
        if ( correctPhaseOnly )
            corr = std::polar(1.0,arg(corr));
        for ( size_t i=0; i<block; i++, p++)
        {
            *p *= corr;
        }
        d->dataCache.insert(cacheKey,data);
        return reinterpret_cast<const complex*>(data->constData());
    }
    else
        return p+block*globalIndex;
}

SystemMatrix::complex SystemMatrix::background(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>= d->numChannels*d->numFrequencies )
        return complex(0.0);
    return d->backgroundReference[globalIndex];
}

double SystemMatrix::backgroundVariance(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>= d->numChannels*d->numFrequencies )
        return 0.0;
    return d->backgroundVariance[globalIndex];
}

double SystemMatrix::backgroundNoise(int globalIndex) const
{
    if ( globalIndex<0 || globalIndex>= d->numChannels*d->numFrequencies )
        return 0.0;
    if ( globalIndex>=d->backgroundNoise_.size() )
        return 0.0;
    double & res=d->backgroundNoise_[ globalIndex ];
    if ( 0.0==res )
    {
        const complex * p=d->allBackground + d->numBgPositions*globalIndex;

        complex mean = 0.0;
        QList<complex> values;
        values.reserve(d->numBgPositions);
        for ( int i=0; i<d->numBgPositions; ++i, ++p )
            mean += *p;

        mean /= d->numBgPositions;
        double noise=0.0;
        p=d->allBackground + d->numBgPositions*globalIndex;
        for ( int i=0; i<d->numBgPositions; ++i, ++p )
        {
            noise += abs(*p-mean);
        }
        noise /= d->numBgPositions;
        res = noise;
    }
    return res;
}

bool SystemMatrix::validPosition(const MatrixPosition &pos) const
{
    for ( unsigned int i=0; i<3; i++ )
        if ( static_cast<int>(pos.index(i))>=d->grid[i] )
            return false;
    return true;
}

SystemMatrix::complex SystemMatrix::dataPoint(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const
{
    int offset=(pos.z()*d->grid[1]+pos.y())*d->grid[0]+pos.x();
    const complex * p = rawData(globalIndex,backgroundCorrection);
    p += offset;
    return *p;
}

SystemMatrix::complex SystemMatrix::interpolated(int globalIndex, const MatrixPosition & pos, bool backgroundCorrection) const
{
    complex corr=1.0;
    int channel = receiver(globalIndex);
    TransferFunction * tf = d->transferFunction[channel];
    if ( tf )
    {
        corr = tf->correctionFactor(frequency(globalIndex));
        if ( correctPhaseOnly )
            corr = std::polar(1.0,arg(corr));
    }
    return corr*d->interpolated(globalIndex,pos,backgroundCorrection);
}

bool SystemMatrix::interpolateDatapoint(int globalIndex, const MatrixPosition &pos, double threshold)
{
    bool changed=false;
    if ( validPosition(pos) && d->mode==Editor )
    {
        complex corr=1.0;
        int channel = receiver(globalIndex);
        TransferFunction * tf = d->transferFunction[channel];
        if ( tf )
        {
            corr = tf->correctionFactor(frequency(globalIndex));
            if ( correctPhaseOnly )
                corr = std::polar(1.0,arg(corr));
        }

        complex value [4];
        for ( int b=0; b<2; b++ )
        {
            bool backgroundCorrection = (b!=0);
            complex oldValue=value[2*b]=value[2*b+1]=dataPoint(globalIndex,pos,backgroundCorrection);
            complex newValue=interpolated(globalIndex,pos,backgroundCorrection);
            if ( abs(newValue)<abs(oldValue) &&
                 abs(oldValue-newValue)/abs(oldValue)>threshold ) // perform change if the reduction exceeds the threshold
            {
                value[2*b+1]=newValue;
                // Reverse calibration
                for ( int i=0; i<4; i++ )
                    value[i]/=corr;
                d->setDataPoint(globalIndex,pos,backgroundCorrection,newValue);
                changed=true;
            }
        }
        if ( changed )
        {
            d->changeList.append(ChangeListEntry(pos,globalIndex,value));
            d->recalcSNR(globalIndex);
            d->rebuildSNRIndex();
            QString error = d->writeModificationTable();
            if ( !error.isEmpty() )
                QMessageBox::warning(0,tr("File error"), error);
            emit dataChange();
        }
    }
    return changed;
}

int SystemMatrix::interpolateDatapoints(const QList<int> & indices, const MatrixPosition &pos, double threshold, QObject * progressReceiver, const char * progressSlot)
{
    if ( !validPosition(pos) || d->mode==Viewer )
        return 0;
    QList<ChangeListItem> changes;
    foreach(int globalIndex, indices)
    {
        complex corr=1.0;
        int channel = receiver(globalIndex);
        TransferFunction * tf = d->transferFunction[channel];
        if ( tf )
        {
            corr = tf->correctionFactor(frequency(globalIndex));
            if ( correctPhaseOnly )
                corr = std::polar(1.0,arg(corr));
        }

        complex value[4];
        bool changed=false;
        for ( int b=0; b<2; b++ )
        {
            bool backgroundCorrection=(b!=0);
            complex oldValue=value[2*b]=value[2*b+1]=dataPoint(globalIndex,pos,backgroundCorrection);
            complex newValue=interpolated(globalIndex,pos,backgroundCorrection);
            if ( abs(newValue)<abs(oldValue) &&
                 abs(oldValue-newValue)/abs(oldValue)>threshold ) // perform change if the reduction exceeds the threshold
            {
                value[2*b+1]=newValue;
                // Reverse calibration
                for ( int i=0; i<4; i++ )
                    value[i]/=corr;
                d->setDataPoint(globalIndex,pos,backgroundCorrection,newValue);
                changed=true;
            }
        }
        if (changed)
        {
            changes.append(ChangeListItem(globalIndex,value));
            d->recalcSNR(globalIndex);
            if ( progressReceiver!=0 && progressSlot!=0 )
            {
                QMetaObject::invokeMethod(progressReceiver,progressSlot,Q_ARG(int,changes.count()));
            }
            emit dataChange();
        }
    }
    if ( !changes.empty() )
    {
        d->changeList.append(ChangeListEntry(pos,changes));
        d->rebuildSNRIndex();
        QString error = d->writeModificationTable();
        if ( !error.isEmpty() )
            QMessageBox::warning(0,tr("File error"), error);
    }
    return changes.count();
}

int SystemMatrix::numberOfFrequencies() const
{
    return d->numFrequencies;
}

int SystemMatrix::numberOfReceivers() const
{
    return d->numChannels;
}

int SystemMatrix::maxGlobalIndex() const
{
    return d->numChannels*d->numFrequencies-1;
}

void SystemMatrix::setTracerName(const QString & tracerName)
{
    d->tracerName=tracerName;
}

QString SystemMatrix::tracerName() const
{
    return d->tracerName;
}

void SystemMatrix::setTracerConcentration(double concentration)
{
    d->tracerConcentration=concentration;
}

double SystemMatrix::tracerConcentration() const
{
    return d->tracerConcentration;
}

void SystemMatrix::setTracerVolume(double volume)
{
    d->tracerVolume=volume;
}

double SystemMatrix::tracerVolume() const
{
    return d->tracerVolume;
}

double SystemMatrix::selectionField() const
{
    return d->selectionField();
}

double SystemMatrix::driveField(unsigned int channel) const
{
    return d->driveField(channel);
}

void SystemMatrix::setAverages(int averages)
{
    d->averages=averages;
}

int SystemMatrix::averages() const
{
    return d->averages;
}

double SystemMatrix::bandwidth() const
{
    return d->bandwidth;
}

QString SystemMatrix::lastChangeDescription() const
{
    QString res;
    if ( ! d->changeList.isEmpty() )
    {
        QTextStream ts(&res);
        ChangeListEntry last=d->changeList.back();
        if ( last.changeItems.count()>0 )
        {
            ts << "[" << last.position << "] / ";
            ts << "Receiver " << 1+receiver(last.changeItems.first().globalIndex_) << " / ";
            if ( last.changeItems.count()>1 )
            {
                ts << last.changeItems.count() << " Frequencies";
            }
            else
            {
                ts << 1e-3*frequency(last.changeItems.first().globalIndex_) << "kHz";
            }
        }
    }
    return res;
}

void SystemMatrix::undoLastChange()
{
    if ( d->mode!=Editor )
        return;
    ChangeListEntry last=d->changeList.takeLast();
    foreach(ChangeListItem i,last.changeItems)
    {
        // No calibration correction here, since the change list contains the raw data.
        d->setDataPoint(i.globalIndex_,last.position,false,i.values_[0]);
        d->setDataPoint(i.globalIndex_,last.position,true,i.values_[2]);
        d->recalcSNR(i.globalIndex_);
    }
    d->rebuildSNRIndex();
    d->writeModificationTable();
    emit dataChange();
}

void SystemMatrix::undoAllChanges(QObject * progressReceiver, const char * progressSlot)
{
    if ( d->mode!=Editor )
        return;
    int total=d->changeList.count();
    int c=0;
    foreach(ChangeListEntry e, d->changeList)
    {
        foreach(ChangeListItem i,e.changeItems)
        {
            // No calibration correction here, since the change list contains the raw data.
            d->setDataPoint(i.globalIndex_,e.position,false,i.values_[0]);
            d->setDataPoint(i.globalIndex_,e.position,true,i.values_[2]);
            d->recalcSNR(i.globalIndex_);
        }
        if ( progressReceiver && progressSlot )
            QMetaObject::invokeMethod(progressReceiver,progressSlot,Q_ARG(int,100*c/total));
    }
    d->changeList.clear();
    d->rebuildSNRIndex();
    d->writeModificationTable();
    emit dataChange();
}

// Local static helper functions

static int gcd(int a, int b)
{
    if ( b==0 ) return a;
    int d=a%b;
    while ( 0!=d )
    {
        a=b;
        b=d;
        d=a%b;
    }
    return b;
}

static int lcm(int a, int b)
{
    int g=gcd(a,b);
    if ( g==0 )
        return a?a:b;
    return b*(a/g);
}

static int lcm(int n, int * a)
{
    int res=1;
    switch (n)
    {
        case 0:
            res=0;
            break;
        case 1:
            res=a[0];
            break;
        default:
            for ( int i=0; i<n; i++ )
                res=lcm(res,a[i]);
    }
    return res;
}
