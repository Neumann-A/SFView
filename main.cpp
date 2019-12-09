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
 * $Id: main.cpp 69 2017-02-26 16:15:45Z uhei $
 */

// Qt includes
#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
#include <QtCore/QTranslator>
#include <QtCore/QDebug>

// Local includes
#include "SFView.h"


int main ( int argc, char ** argv ) {
    QApplication app ( argc, argv );
    QStringList arguments = app.arguments();
    bool editor=(arguments.first().contains("SFEdit")) ||
                ((arguments.first().contains("SFView")) && arguments.contains("-edit"));
    app.setOrganizationName("HS_Pforzheim");
    app.setApplicationName("SFView");
    QTranslator translator;
    translator.load ( "SFView", ":/" );
    app.installTranslator ( &translator );

    QString file;
    for ( int i=1; i<arguments.count(); i++ )
    {
        if ( arguments.at(i).startsWith("-") )
            continue;
        file=arguments.at(i);
        break;
    }
    SFView sfview(editor?SFView::Editor:SFView::Viewer);
    if (editor)
        sfview.setWindowTitle("SFEdit");
    sfview.show();
    if ( !file.isEmpty() )
        sfview.loadSystemMatrix(file);
    return app.exec();
}
