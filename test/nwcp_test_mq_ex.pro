
TEMPLATE = app
TARGET = nwcp_test_mq_ex
CONFIG += nwcp

include(../nwcp_example.pri)

SOURCES += test_mq_ex.cxx \
    mq_send_recv_ex.cxx \
    mq_send_recv.cxx

test.files      = ./*.pro ./*.h ./*.cxx ./*.cpp
test.path       = $$NWCP_INSTALL_DIR/test
INSTALLS       +=  test

