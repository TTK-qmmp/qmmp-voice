include(../../plugins.pri)
include(common/common.pri)

TARGET = $$PLUGINS_PREFIX/Visual/voice

HEADERS += voice.h \
           visualvoicefactory.h \
           visualpalette.h

SOURCES += voice.cpp \
           visualvoicefactory.cpp \
           visualpalette.cpp

unix{
    target.path = $$PLUGIN_DIR/Visual
    INSTALLS += target
}
