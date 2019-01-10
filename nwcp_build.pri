
QT     -= core gui
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += warn_on
CONFIG += debug_and_release build_all
win32:CONFIG += dll
win32:DEFINES += _CRT_SECURE_NO_WARNINGS
contains(TEMPLATE, lib) {
    CONFIG += plugin
}

include(./nwcp_variables.pri)
NWCP_INSTALL_DIR = $$(NWCP_ROOT)
isEmpty(NWCP_INSTALL_DIR) {
    NWCP_INSTALL_DIR = $${PWD}/../nwcp_dev
    message("NWCP_INSTALL = $${NWCP_INSTALL_DIR} (export NWCP_ROOT to change it)" )
}

win32:LIBS += -lkernel32 -luser32 -ladvapi32 -lws2_32
unix:LIBS  += -lrt -lpthread -ldl
contains(DEFINES, _NWCP_SUNOS) {
    LIBS  += -lsocket
}

headers.path = $$NWCP_INSTALL_DIR/$${TARGET}
target.path  = $$NWCP_INSTALL_DIR/lib
INSTALLS     += target headers
TARGET  = $${TARGET}$${DEBUG_SUFFIX_STR}
VERSION = $${NWCP_VERSION}
DESTDIR = $${PWD}/lib

