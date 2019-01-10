
# VERSION = $${NWCP_VER_MAJ}.$${NWCP_VER_MIN}.$${NWCP_VER_PAT}
NWCP_VER_MAJ = 1
NWCP_VER_MIN = 1
NWCP_VER_PAT = 1
NWCP_VERSION = $${NWCP_VER_MAJ}.$${NWCP_VER_MIN}.$${NWCP_VER_PAT}

CONFIG += thread

win32 {
    #QMAKE_LFLAGS += /ignore:4819
    DEFINES += WIN32 _NWCP_WIN32
    DEFINES -= UNICODE _UNOCODE
    win32-msvc* {
        QMAKE_CXXFLAGS += /MP
    }
} else {
    UNAME = $$system(uname -s)
    contains( UNAME, Linux ) {
        DEFINES += _NWCP_LINUX
    }
    contains( UNAME, SunOS ) {
        DEFINES += _NWCP_SUNOS
    }
    contains( UNAME, AIX ) {
        DEFINES += _NWCP_IBMOS
    }
}

CONFIG(debug, debug|release) {
    DEBUG_SUFFIX_STR = d
}
else {
    DEBUG_SUFFIX_STR =
}

LIBS += -L$${PWD}/lib
INCLUDEPATH += $${PWD}

contains(CONFIG, nwcp) {
    INCLUDEPATH += $${PWD}/nwcp
    win32 {
        LIBS += -lnwcp$${DEBUG_SUFFIX_STR}$${NWCP_VER_MAJ}
    } else {
        unix::LIBS += -lnwcp$${DEBUG_SUFFIX_STR}
    }
}

DEPENDPATH  += $${INCLUDEPATH}
