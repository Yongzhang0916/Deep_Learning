#ifndef PROJECTION_SWC_H
#define PROJECTION_SWC_H
#include "ImageProcessing_plugin.h"

bool projection_swc(V3DPluginCallback2 &callback,const V3DPluginArgList &input,V3DPluginArgList &output,QWidget *parent,input_PARA &PARA);
NeuronTree smartPrune(NeuronTree nt, double length);
void choosePointOf3Poiont(NeuronTree &nt,vector<vector<V3DLONG> > &childs,V3DLONG &par11,V3DLONG &par12,V3DLONG &par13,QList<NeuronSWC> &choose_swclist1,QList<NeuronSWC> &choose_swclist2,QList<NeuronSWC> &choose_swclist3,int m);


#endif // PROJECTION_SWC_H
