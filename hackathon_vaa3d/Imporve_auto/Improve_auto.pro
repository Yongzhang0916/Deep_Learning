
TEMPLATE	= lib
CONFIG	+= qt plugin warn_off
#CONFIG	+= x86_64
VAA3DPATH = ../../../../v3d_external
INCLUDEPATH	+= $$VAA3DPATH/v3d_main/basic_c_fun
INCLUDEPATH	+= $$VAA3DPATH/v3d_main/common_lib/include

HEADERS	+= Improve_auto_plugin.h \
    branch_angle.h
SOURCES	+= Improve_auto_plugin.cpp \
    branch_angle.cpp
SOURCES	+= $$VAA3DPATH/v3d_main/basic_c_fun/v3d_message.cpp
SOURCES	+= $$VAA3DPATH/v3d_main/basic_c_fun/basic_surf_objs.cpp

TARGET	= $$qtLibraryTarget(Improve_auto)
DESTDIR	= $$VAA3DPATH/bin/plugins/Improve_auto/

INCLUDEPATH += /usr/local/opencv3.1.0//include \
                /usr/local/opencv3.1.0/include/opencv \
                /usr/local/opencv3.1.0/include/opencv2

LIBS += /usr/local/opencv3.1.0/lib/libopencv_highgui.so \
        /usr/local/opencv3.1.0/lib/libopencv_core.so    \
        /usr/local/opencv3.1.0/lib/libopencv_imgproc.so

