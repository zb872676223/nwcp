
TEMPLATE = app
TARGET = nwcp_test_mq
CONFIG += nwcp

include(../nwcp_example.pri)

SOURCES += test_mq.cxx \
    mq_send_recv.cxx

test.files      = ./*.pro ./*.h ./*.cxx ./*.cpp
test.path       = $$NWCP_INSTALL_DIR/test
INSTALLS       +=  test

