
TEMPLATE	= lib
CONFIG	+= qt plugin warn_off
#CONFIG	+= x86_64
VAA3DPATH = ../../../../v3d_external
INCLUDEPATH	+= $$VAA3DPATH/v3d_main/basic_c_fun
INCLUDEPATH	+= $$VAA3DPATH/v3d_main/common_lib/include

HEADERS	+= DeepNeuron_plugin.h \
    DeepNeuron_main_func.h \
    blastneuron/resampling.h \
    blastneuron/sort_swc.h \
    my_surf_objs.h \
    blastneuron/swc_utils.h \
    blastneuron/seq_weight.h \
    get_sub_terafly.h \
    blastneuron/local_alignment.h \
    get_sample_area.h \
    get_expand_swc.h
SOURCES	+= DeepNeuron_plugin.cpp \
    DeepNeuron_main_func.cpp \
    blastneuron/resampling.cpp \
    blastneuron/sort_swc.cpp \
    my_surf_objs.cpp \
    blastneuron/swc_utils.cpp \
    blastneuron/seq_weight.cpp \
    get_sub_terafly.cpp \
    blastneuron/local_alignment.cpp \
    get_sample_area.cpp \
    get_expand_swc.cpp
SOURCES	+= $$VAA3DPATH/v3d_main/basic_c_fun/v3d_message.cpp
SOURCES	+= $$VAA3DPATH/v3d_main/basic_c_fun/basic_surf_objs.cpp

TARGET	= $$qtLibraryTarget(DeepNeuron)
DESTDIR	= $$VAA3DPATH/bin/plugins/DeepNeuron/

