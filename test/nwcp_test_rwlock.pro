######################################################################
# Automatically generated by qmake (2.01a) ?? ??? 2 23:39:39 2010
######################################################################

TEMPLATE = app
TARGET = nwcp_test_rwlock
CONFIG += nwcp

include(../nwcp_example.pri)

SOURCES += test_rwlock.cxx

test.files      = ./*.pro ./*.h ./*.cxx ./*.cpp
test.path       = $$NWCP_INSTALL_DIR/test
INSTALLS       +=  test

