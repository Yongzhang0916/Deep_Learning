
TEMPLATE	= lib
CONFIG	+= qt plugin warn_off
#CONFIG	+= x86_64
VAA3DPATH = ../../../v3d_main
INCLUDEPATH	+= $$VAA3DPATH/basic_c_fun
INCLUDEPATH	+= $$VAA3DPATH/neuron_editing


HEADERS	+= neuron_dist_plugin.h \
    resampling.h \
    sort_swc.h
HEADERS	+= neuron_dist_func.h
HEADERS	+= neuron_dist_gui.h
HEADERS += $$VAA3DPATH/../../vaa3d_tools/released_plugins/v3d_plugins/swc_to_maskimage/filter_dialog.h


SOURCES	= neuron_dist_plugin.cpp \
    resampling.cpp
SOURCES	+= neuron_dist_func.cpp
SOURCES	+= neuron_dist_gui.cpp
SOURCES	+= $$VAA3DPATH/basic_c_fun/v3d_message.cpp
SOURCES	+= $$VAA3DPATH/basic_c_fun/basic_surf_objs.cpp
SOURCES	+= $$VAA3DPATH/neuron_editing/neuron_sim_scores.cpp
SOURCES	+= $$VAA3DPATH/neuron_editing/v_neuronswc.cpp
SOURCES += $$VAA3DPATH/../../vaa3d_tools/released_plugins/v3d_plugins/swc_to_maskimage/filter_dialog.cpp
SOURCES += $$VAA3DPATH/neuron_editing/global_feature_compute.cpp

TARGET	= $$qtLibraryTarget(neuron_dist)
DESTDIR	= $$VAA3DPATH/../bin/plugins/neuron_utilities/neuron_distance
