#include "apo_to_markers.h"

bool apo_to_markers(V3DPluginCallback2 &callback,const V3DPluginArgList &input,V3DPluginArgList &output,QWidget *parent,input_PARA &PARA)
{
    vector<char*>* inlist = (vector<char*>*)(input.at(0).p);
    vector<char*>* outlist = (vector<char*>*)(output.at(0).p);
    vector<char*>* paralist = NULL;

    QString fileOpenName = QString(inlist->at(0));
    QString fileOutName = QString(outlist->at(0));

    QList<CellAPO> file_inmarkers;
    file_inmarkers = readAPO_file(fileOpenName);
    for(int i = 0; i < file_inmarkers.size(); i++)
    {
        QList <ImageMarker> listMarker;
        listMarker.clear();
        ImageMarker curr;
        curr.x = 0.5*file_inmarkers[i].x;
        curr.y = 0.5*file_inmarkers[i].y;
        curr.z = 0.5*file_inmarkers[i].z;
        listMarker.push_back(curr);
        curr.name = file_inmarkers[i].name;
        writeMarker_file(fileOutName + QString("/%1_x_%2_y_%3_z_%4.marker").arg(curr.name).arg(curr.x).arg(curr.y).arg(curr.z),listMarker);
    }

    return true;
}
