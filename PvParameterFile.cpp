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
 * $Id: PvParameterFile.cpp 69 2017-02-26 16:15:45Z uhei $
 */

// Qt includes
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QStringList>

// Local includes
#include "PvParameterFile.h"

struct PvParameterFile::Impl {
    typedef QMap<QString, QVariant> ParameterTable;
    ParameterTable parameters;
    void interpretParameter ( QByteArray p );
    QString error;
};

PvParameterFile::PvParameterFile ( const QString & path, QObject * parent ) : QObject ( parent ), d ( new Impl ) {
    QFile file ( path );

    if ( !file.open ( QIODevice::ReadOnly ) ) {
        d->error = tr ( "Cannot open file %1." ).arg ( path );
        return;
    }

    QByteArray lineBuf, parBuf;

    while ( ! file.atEnd() ) {
        lineBuf = file.readLine();

        if ( lineBuf.indexOf ( "##$" ) == 0 ) {
            d->interpretParameter ( parBuf );
            parBuf = lineBuf;
        } else if ( lineBuf.indexOf ( "##" ) == 0 ||
                    lineBuf.indexOf ( "$$" ) == 0 ) {
            // Do nothing
        } else
            parBuf.append ( lineBuf );
    }

    if ( !parBuf.isEmpty() )
        d->interpretParameter ( parBuf );
}

PvParameterFile::~PvParameterFile() {
    delete d;
}

bool PvParameterFile::isValid ( QString * error ) const {
    if ( d->error.isEmpty() )
        return true;

    if ( error )
        *error = d->error;

    return false;
}

bool PvParameterFile::isArray(const QString &name) const
{
    QVariant v=valueInternal(name);
    return v.canConvert ( QVariant::List );
}

int PvParameterFile::dimension(const QString & name ) const
{
    QVariant v=valueInternal(name);

    if ( !v.canConvert( QVariant::List ) )
        return 0;

    return v.toList().count();
}

QVariant PvParameterFile::valueInternal ( const QString & name ) const {
    Impl::ParameterTable::const_iterator i = d->parameters.find ( name );

    if ( i == d->parameters.end() )
        return QVariant();
    return *i;
}

void PvParameterFile::Impl::interpretParameter ( QByteArray p ) {
    if ( p.isEmpty() )
        return;

    p.replace ( "##$", "" );
    int pos = p.indexOf ( "=" );

    if ( pos < 0 )
        return;

    QString parameter = QString::fromLatin1( p.left ( pos ) );
    p.remove ( 0, pos + 1 );

    if ( p.indexOf ( "(" ) == 0 ) {
        pos = p.indexOf ( ")" );

        if ( pos < 0 )
            return;

        QByteArray q = p.mid ( 1, pos - 1 ).simplified();
        int numItems = 0;

        if ( q.contains ( "," ) ) {
            numItems = 1;
            QStringList entries = QString::fromLatin1( q ).split ( "," );

            foreach ( QString s, entries ) {
                numItems *= s.simplified().toInt();
            }
        } else
            numItems = q.toInt();


        p.remove ( 0, pos + 1 );
        if ( p.contains('<') && p.contains('>') )
        {
            // String parameter
            pos = p.indexOf('<');
            p = p.remove( 0, pos+1);
            pos = p.indexOf('>');
            p = p.left(pos);
            parameters.insert( parameter, QString::fromLatin1( p ) );
        }
        else
        {
            QList<QVariant> entry;
            QStringList s = QString::fromLatin1 ( p.simplified() ).split ( QRegExp ( "\\s+" ) );

            foreach ( QString v, s )
                entry.append ( v.simplified() );

            if ( entry.count() != numItems )
            {
                // Parser cannot handle this parameter for now
                //qDebug() << parameter << " " << entry.count() << " " << numItems;
            }
            else
                parameters.insert ( parameter, entry );
        }
    }
    else if ( p.contains('<') && p.contains('>' ) )
    {
        // String parameter
        pos = p.indexOf('<');
        p = p.remove( 0, pos+1);
        pos = p.indexOf('>');
        p = p.left(pos);
        parameters.insert( parameter, p );
    } else {
        parameters.insert ( parameter, p.simplified() );
    }
}
