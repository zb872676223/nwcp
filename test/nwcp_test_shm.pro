TEMPLATE = app
TARGET = nwcp_test_shm
CONFIG += nwcp

include(../nwcp_example.pri)

SOURCES += test_shm.cxx

test.files      = ./*.pro ./*.h ./*.cxx ./*.cpp
test.path       = $$NWCP_INSTALL_DIR/test
INSTALLS       +=  test
