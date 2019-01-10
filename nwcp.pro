TEMPLATE = subdirs

include(./nwcp_build.pri)
CONFIG   += ordered

SUBDIRS = src \
    test
    
headers.files  = *.h nwcp_variables.pri nwcp_example.pri
headers.path   = $$NWCP_INSTALL_DIR
INSTALLS       += headers
