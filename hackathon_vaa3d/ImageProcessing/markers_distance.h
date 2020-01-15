#ifndef PROJECTION_SWC_H
#define PROJECTION_SWC_H
#include "ImageProcessing_plugin.h"

bool markers_distance(V3DPluginCallback2 &callback,const V3DPluginArgList &input,V3DPluginArgList &output,QWidget *parent,input_PARA &PARA);
QStringList importFileList_addnumbersort(const QString & curFilePath);
int max3numbers(int x,int y,int z);
int min3numbers(int x,int y,int z);
#endif // PROJECTION_SWC_H
