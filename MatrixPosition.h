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
 * $Id: MatrixPosition.h 69 2017-02-26 16:15:45Z uhei $
 */

#ifndef MATRIXPOSITION_H
#define MATRIXPOSITION_H

// Forward declarations
class QDataStream;
class QTextStream;

class MatrixPosition
{
public:
    MatrixPosition(int x=-1, int y=-1, int z=-1)
    {
        setTo(x,y,z);
    }

    MatrixPosition(const int pos[])
    {
        setTo(pos);
    }

    MatrixPosition(const MatrixPosition & p)
    {
        setTo(p.pos());
    }

    MatrixPosition & operator = (const MatrixPosition & p)
    {
        setTo(p.pos());
        return *this;
    }

    bool isValid() const
    {
        for ( unsigned int i=0; i<3; i++ )
            if ( m_pos[i]<0 )
                return false;
        return true;
    }
    int x() const
    {
        return m_pos[0];
    }

    int y() const
    {
        return m_pos[1];
    }

    int z() const
    {
        return m_pos[2];
    }

    int index(unsigned int dir) const
    {
        if ( dir<3 )
            return m_pos[dir];
        return 0;
    }

    const int * pos() const
    {
        return m_pos;
    }

    void set(unsigned int dir, int index)
    {
        if ( dir<3 )
            m_pos[dir] = index;
    }

    void setX(int x)
    {
        m_pos[0]=x;
    }

    void setY(int y)
    {
        m_pos[1]=y;
    }

    void setZ(int z)
    {
        m_pos[2]=z;
    }

    void setTo( int x, int y, int z)
    {
        m_pos[0]=x;
        m_pos[1]=y;
        m_pos[2]=z;
    }

    void setTo(const int pos[])
    {
        for (unsigned int i=0; i<3; i++)
            m_pos[i]=pos[i];
    }

    bool operator < (const MatrixPosition & p) const
    {
        if ( z()<p.z() )
            return true;
        if ( z()>p.z() )
            return false;
        if ( y()<p.y() )
            return true;
        if ( y()>p.y() )
            return false;
        if ( x()<p.x() )
            return true;
        return false;
    }

    bool operator == (const MatrixPosition & p) const
    {
        for ( unsigned int i=0; i<3; i++)
            if ( m_pos[i]!=p.m_pos[i] )
                return false;
        return true;
    }

    bool operator != (const MatrixPosition & p) const
    {
        return !(*this==p);
    }
private:
    int m_pos[3];
};

QDataStream & operator<< (QDataStream & d, const MatrixPosition & p);
QDataStream & operator>> (QDataStream & d, MatrixPosition & p);
QTextStream & operator<< (QTextStream & d, const MatrixPosition & p);
#endif // MATRIXPOSITION_H
