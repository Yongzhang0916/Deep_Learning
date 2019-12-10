#ifndef DEEPNEURON_FUNC_H
#define DEEPNEURON_FUNC_H
#include<v3d_interface.h>
#include <vector>
#include "my_surf_objs.h"

#include "DeepNeuron_plugin.h"

struct Coordinate
{
    V3DLONG x;
    V3DLONG y;
    V3DLONG z;
    V3DLONG lens;
};
struct FourType
{
    QList<NeuronSWC> little;
    QList<NeuronSWC> middle;
    QList<NeuronSWC> point_rec;
    QList<NeuronSWC> point_gold;
};


bool DeepNeuron_main_func(input_PARA &PARA,V3DPluginCallback2 &callback,bool bmenu,QWidget *parent);
void getChildNum(const NeuronTree &nt, vector<vector<V3DLONG> > &childs);
NeuronTree get_right_area(NeuronTree &nt,vector<vector<V3DLONG> > &childs,QList<NeuronSWC> &other_point,QList<NeuronSWC> &wrong_point);
QList<NeuronSWC> match_point(QList<NeuronSWC> &swc1,QList<NeuronSWC> &swc2);
QList<NeuronSWC> match_point_v2(QList<NeuronSWC> &swc1,QList<NeuronSWC> &swc2);
V3DLONG down_child(NeuronTree &nt,vector<vector<V3DLONG> > &childs,V3DLONG node,QList<NeuronSWC> &other_point);
QList<NeuronSWC>choose_point(QList<NeuronSWC> &neuron1,QList<NeuronSWC> &neuron2,int thre1,int thre2);
bool get_sub_block(V3DPluginCallback2 &callback,QList<NeuronSWC> & neuron,QString fileOpenName);
bool get_subarea_in_nt(vector<FourType> &fourtype,V3DLONG length,QList<NeuronSWC> &little,QList<NeuronSWC> &middle,QList<NeuronSWC> &point_rec,QList<NeuronSWC> &point_gold);
bool make_coordinate(vector<vector<Coordinate> > &four_coord,vector<FourType> &four_type,V3DLONG lens);
NeuronTree mm2nt(vector<MyMarker*> & inswc, QString fileSaveName);
QList<NeuronSWC> choose_alignment(QList<NeuronSWC> &neuron,QList<NeuronSWC> &gold,double thres1,double thres2);
bool get_subarea(QList<NeuronSWC> &nt,vector<Coordinate> &fourcood,vector<QList<NeuronSWC> > &subarea);
void SplitString(const string& s, vector<string>& v, const string& c);
void processImage(V3DPluginCallback2 &callback,vector<Coordinate> &fourcood_each);
bool get_subimg_terafly(QString inimg_file,QString name,vector<Coordinate> &mean,V3DPluginCallback2 &callback);
bool export_1dtxt(unsigned char *im_cropped ,QString fileSaveName);



#endif // DEEPNEURON_FUNC_H
