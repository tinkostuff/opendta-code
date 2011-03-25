win32:include(C:\\Qt\\Qwt-5.2.1\\qwt.pri)
unix:include(../qwt.pri)

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
    dtafile/dumpfile.h

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
    dtafile/dumpfile.cpp

FORMS += \
    dtagui/mainwindow.ui

RESOURCES += \
    dtagui/dtagui.qrc

OTHER_FILES +=
