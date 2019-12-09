#
# SF Viewer - A program to visualize MPI system matrices
# Copyright (C) 2014-2017  Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# $Id: SFView.pro 86 2017-03-18 21:44:25Z uhei $
#

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets svg

CONFIG += qt static debug_and_release

TARGET = SFView
TEMPLATE = app

unix {
!isEmpty(LINK_ROOT_PATH) {
    createlink.commands = ln -sf $${INSTALL_ROOT_PATH}/$${BINARY_TARGET} $${LINK_ROOT_PATH}/$${BINARY_TARGET}
    QMAKE_EXTRA_TARGETS += createlink
    PRE_TARGETDEPS += createlink
}
DATE='$$system(date +%d.%m.%Y)'

}

win32 {
DATE='$$system(date /t)'
}

win64 {
DATE='$$system(date /t)'
}

!isEmpty(RELEASE) {
    RELEASE = -$${RELEASE}
}

VERSION = 1.0$${RELEASE}

DEFINES += VERSION=$${VERSION} DATE=$${DATE}


FORMS += \
    SFViewForm.ui \
    CorrectionDialog.ui \
    SettingsDialog.ui


SOURCES += main.cpp PlotWidget.cpp PvParameterFile.cpp SFRenderer.cpp SFView.cpp SystemMatrix.cpp \
    MatrixPosition.cpp \
    ChangeListV1.cpp \
    Changelist.cpp \
    CorrectionDialog.cpp \
    SpectralPlot.cpp \
    PhaseView.cpp \
    ColorScale.cpp \
    ColorScaleManager.cpp \
    TransferFunction.cpp

HEADERS  += PlotWidget.h PvParameterFile.h SFRenderer.h SFView.h SystemMatrix.h \
    MatrixPosition.h \
    ChangeListV1.h \
    ChangeList.h \
    CorrectionDialog.h \
    SpectralPlot.h \
    PhaseView.h \
    ColorScale.h \
    ColorScaleManager.h \
    utility.h \
    TransferFunction.h

TRANSLATIONS = SFView_de.ts

RESOURCES = SFView.qrc

win32 {
RC_ICONS = SFView.ico
}

DISTFILES += COPYING README INSTALL \
    ChangeLog \
    doc/en/SFView.qhcp \
    doc/en/SFView.qhp \
    doc/en/doc.html \
    doc/en/SFView.qch \
    doc/en/SFView.qhc

OTHER_FILES += \
    title_and_license.txt \
    SFView.spec \
    AUTHORS \
    TODO \
    SFView.desktop \
    SFView-16x16.png \
    SFView-22x22.png \
    SFView-32x32.png \
    SFView-48x48.png \
    SFView-64x64.png \
    SFView-96x96.png \
    SFView-128x128.png \
    SFEdit.desktop \
    SFEdit-16x16.png \
    SFEdit-22x22.png \
    SFEdit-32x32.png \
    SFEdit-48x48.png \
    SFEdit-64x64.png \
    SFEdit-96x96.png \
    SFEdit-128x128.png \
    application-x-bruker-mpi-systemmatrix.xml
