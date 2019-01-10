TEMPLATE = lib
TARGET = nwcp

CONFIG +=

include(../nwcp_build.pri)

win32:DEFINES += CPDLDLL_DEF
win32:DEFINES -= UNICODE
unix:DEFINES += HAVE_SYS_STATVFS_H \
    HAVE_SYS_TIMEB_H

contains(DEFINES, _NWCP_LINUX) {
    DEFINES += HAVE_FSTAB_H
}

INCLUDEPATH += ..

# Input
HEADERS += \
    ../cpdltype.h \
    ../cpgetopt.h \
    ../cpdll.h \
    ../cpfloat.h \
    ../cpmisc.h \
    ../cpprocess.h \
    ../cpsocket.h \
    ../cpsyslog.h \
    ../cpthread.h \
    ../cprwlock.h \
    ../cptime.h \
    ../cptimer.h \
    ../cpshm.h \
    ../cpshmmq.h \
    ../cpprocsem.h \
    ../cpspibus.h \
    ../cpstring.h \
    ../nwcp.h

win32:SOURCES += getopt.c getopt_long.c
SOURCES += cpdll.c \
    cpmisc.c \
    cpprocess.c \
    cpshm.c \
    cpsyslog.c \
    cpthread.c \
    cprwlock.c \
    cptime.c \
    cptimer.c \
    cpshmmq.c \
    cpprocsem.c \
    cpstring.c \
    cputil.c \
    cpspibus.c

headers.files  = ./../*.h
