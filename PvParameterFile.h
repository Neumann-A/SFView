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
 * $Id: PvParameterFile.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef PVPARAMETERFILE_H
#define PVPARAMETERFILE_H

// Qt includes
#include <QtCore/QObject>
#include <QtCore/QVariant>

class PvParameterFile : public QObject {
        Q_OBJECT
    public:
        PvParameterFile ( const QString & path, QObject * parent = 0 );
        virtual ~PvParameterFile();
        template<typename T> T value ( const QString & name ) const;
        template<typename T> T value ( const QString & name, int index ) const;
        bool isArray ( const QString & name ) const;
        int dimension ( const QString & name ) const;
        bool isValid ( QString * error = 0 ) const;
    private:
        QVariant valueInternal( const QString & name ) const;
        struct Impl;
        Impl * d;
};

template<typename T>
T PvParameterFile::value ( const QString & name ) const {
    QVariant v = valueInternal ( name );
    
    if ( v.isNull() )
        return T();

    if ( ! v.canConvert<T>() )
        return T();

    return v.value<T>();
}


template<typename T>
T PvParameterFile::value ( const QString & name, int index ) const {
    QVariant v = valueInternal( name );
    
    if ( v.isNull() )
        return T();

    if ( ! v.canConvert ( QVariant::List ) )
        return T();

    QList<QVariant> array = v.toList();

    if ( array.length() <= index )
        return T();

    QVariant item = array.at ( index );

    if ( ! item.canConvert<T>() )
        return T();

    return item.value<T>();
}

#endif // PVPARAMETERFILE_H
