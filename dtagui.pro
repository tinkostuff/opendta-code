
#
# Qwt
#
contains(DEFINES,MXE) {
    # cross compile with MXE
    # usage: qmake DEFINES+=MXE
    QT += svg
    LIBS += -lqwt
} else {
    # normal settings
    include ( $(QWT_ROOT)/features/qwt.prf )
}

QT += network
QT += printsupport

HEADERS += \
    dtagui/mainwindow.h \
    dtafile/dtafile.h \
    dtagui/dtaplotframe.h \
    dtaplot/datetimescaleengine.h \
    dtaplot/dtaplot.h \
    dtagui/dtastatsframe.h \
    dtagui/dtacompstartsframe.h \
    statistics/dtafieldstatistics.h \
    statistics/dtacompstartsstatistics.h \
    dtafile/datafile.h \
    dtafile/dumpfile.h \
    dtagui/config.h \
    dtagui/downloaddta.h

SOURCES += \
    dtagui/mainwindow.cpp \
    dtagui/main.cpp \
    dtafile/dtafile.cpp \
    dtagui/dtaplotframe.cpp \
    dtaplot/datetimescaleengine.cpp \
    dtaplot/dtaplot.cpp \
    dtagui/dtastatsframe.cpp \
    dtagui/dtacompstartsframe.cpp \
    statistics/dtafieldstatistics.cpp \
    statistics/dtacompstartsstatistics.cpp \
    dtafile/datafile.cpp \
    dtafile/dumpfile.cpp \
    dtagui/downloaddta.cpp

FORMS += \
    dtagui/mainwindow.ui \
    dtagui/downloaddta.ui

RESOURCES += \
    dtagui/dtagui.qrc

OTHER_FILES += \
   dtagui/doc/hilfe.html \
   HISTORY.txt \
   INSTALL.txt \
   README.txt \
   doc/dta_format.txt \
   doc/dump_format.txt \
   doc/gpl-3.0.txt

### translations ###
TRANSLATIONS = \
   translations/dtagui_en.ts \
   translations/dtagui_cs.ts \
   translations/dtagui_pl.ts

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_BASE}.qm
lrelease.CONFIG       += no_link target_predeps

### application icon ###
# reference: http://qt-project.org/doc/qt-4.8/appicon.html
# windows
RC_FILE = dtagui/icon/dtagui.rc
# MacOSX
ICON = dtagui/icon/dtagui.icns
