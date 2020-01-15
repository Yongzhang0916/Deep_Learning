#ifndef SORT_SWC_H
#define SORT_SWC_H
#include <QtGlobal>
#include <math.h>
#include "basic_surf_objs.h"
#include <string.h>
#include <vector>
#include <iostream>

#ifndef VOID
#define VOID 1000000000
#endif

//#define PI 3.14159265359
#define getParent(n,nt) ((nt).listNeuron.at(n).pn<0)?(1000000000):((nt).hashNeuron.value((nt).listNeuron.at(n).pn))
#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))
#define NTDOT(a,b) ((a).x*(b).x+(a).y*(b).y+(a).z*(b).z)
#define angle(a,b,c) (acos((((b).x-(a).x)*((c).x-(a).x)+((b).y-(a).y)*((c).y-(a).y)+((b).z-(a).z)*((c).z-(a).z))/(NTDIS(a,b)*NTDIS(a,c)))*180.0/3.14159265359)

#ifndef MAX_DOUBLE
#define MAX_DOUBLE 1.79768e+308        //actual: 1.79769e+308
#endif

using namespace std;
QHash<V3DLONG, V3DLONG> ChildParent(QList<NeuronSWC> &neurons, const QList<V3DLONG> & idlist, const QHash<V3DLONG,V3DLONG> & LUT) ;
QHash<V3DLONG, V3DLONG> getUniqueLUT(QList<NeuronSWC> &neurons);
void DFS(bool** matrix, V3DLONG* neworder, V3DLONG node, V3DLONG* id, V3DLONG siz, int* numbered, int *group);
double computeDist2(const NeuronSWC & s1, const NeuronSWC & s2);
bool combine_linker(vector<QList<NeuronSWC> > & linker, QList<NeuronSWC> & combined);
bool Sort_auto_SWC(QList<NeuronSWC> & neurons, QList<NeuronSWC> & result, V3DLONG newrootid, double thres);
bool Sort_manual_SWC(QList<NeuronSWC> & neuron, QList<NeuronSWC> & result, V3DLONG newrootid, double thres, double root_dist_thres, QList<CellAPO> markers);
bool export_list2file(QList<NeuronSWC> & lN, QString fileSaveName, QString fileOpenName);

#endif // SORT_SWC_H
