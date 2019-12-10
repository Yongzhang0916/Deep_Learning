#ifndef GET_SAMPLE_AREA_H
#define GET_SAMPLE_AREA_H
#include <v3d_interface.h>
#include "basic_surf_objs.h"
#include "my_surf_objs.h"


struct Center
{
    V3DLONG x;
    V3DLONG y;
    V3DLONG z;
};

template <class T> bool getHistogram(const T * pdata1d, V3DLONG datalen, double max_value, V3DLONG & histscale, QVector<int> &hist)
{
    // init hist
   // cout<<"datalen = "<<datalen<<endl;
    hist = QVector<int>(histscale, 0);
    //cout<<"max_valve = "<<max_value<<endl;
    //cout<<"histscale = "<<histscale<<endl;
    for (V3DLONG i=0;i<datalen;i++)
    {
        //std::cout<<"pdata1d[i] = "<<int(pdata1d[i])<<endl;
        V3DLONG ind = pdata1d[i]/max_value * histscale;
        //V3DLONG ind = pdata1d[i];
        //std::cout<<"ind = "<<ind<<endl;
        hist[ind] ++;
    }
    //std::cout<<"hist = "<<hist[0]<<endl;
    return true;

}

class Path
{
public:
    V3DLONG src_ind;
    V3DLONG dst_ind;
    double w;
public:
    Path(V3DLONG point1,V3DLONG point2,double w)
    {
        this->w = w;
        src_ind = point1 > point2 ? point2 : point1;
        dst_ind = point1 <= point2 ? point2 : point1;
    }
    V3DLONG getDst(V3DLONG ind)
    {
        return src_ind == ind ? dst_ind : dst_ind == ind ? src_ind : -1;
    }
};

template<class GraphNode>
class Graph
{
public:
    GraphNode node;
    QMap<V3DLONG,Path*> connect;//连接集合
public:
    Graph(GraphNode node)
    {
        this->node = node;
    }
    ~Graph()
    {
        qDeleteAll(connect);
    }
};

class Node
{
public:
    double x;
    double y;
    double z;
    double r;
    double cover;
    int type;
public:
    Node(double x,double y,double z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->r = 0;
        this->type = 0;
        this->cover = 0;
    }
};


bool get_feature(V3DPluginCallback2 & callback,unsigned char * inimg1d,V3DLONG sz[4],QVector<QVector<int> > &hist_vec,vector<double> &entrople);
bool get_subimg(unsigned char * data1d,V3DLONG in_sz[4],V3DPluginCallback2 &callback,vector<double> &entrople);
bool get_sample_area(V3DPluginCallback2 &callback,QWidget *parent);
bool choose_cube(unsigned char* img1d,double thre,V3DLONG in_sz[4],unsigned char bresh, QMap<V3DLONG,Graph<Node*>*> &nodeMap);
#endif // GET_SAMPLE_AREA_H
