#include "get_expand_swc.h"
#include "blastneuron/sort_swc.h"
#include <v3d_interface.h>
#include <vector>
#include <iostream>

bool get_expand_swc(V3DPluginCallback2 &callback,QWidget *parent)
{
    QString fileOpenName;
    fileOpenName = QFileDialog::getOpenFileName(0, QObject::tr("Open SWC File"),
                                                "",

                                                QObject::tr("Supported file (*.swc *.SWC *.eswc)"

                                            ));
    NeuronTree nt=readSWC_file(fileOpenName);

    cout<<fileOpenName.toStdString()<<endl;

    //Get expand swc
    QList<NeuronSWC> neuron;
    QString filename = fileOpenName + "_expand.swc";
    for(V3DLONG i = 0;i<nt.listNeuron.size();i++)
    {
        NeuronSWC cur;
        cur.x = 2*(nt.listNeuron[i].x);
        cur.y = 2*(nt.listNeuron[i].y);
        cur.z = 2*(nt.listNeuron[i].z);
        cur.type = nt.listNeuron[i].type;
        cur.radius = nt.listNeuron[i].radius;
        cur.n = nt.listNeuron[i].n;
        cur.parent = nt.listNeuron[i].parent;

        neuron.push_back(cur);
    }
    if (!export_list2file(neuron, filename, fileOpenName))
    {
        printf("fail to write the output swc file.\n");
        return false;
    }

    //create .apo and .ano file
    //unsigned int Vsize=50;
//    Vsize = QInputDialog::getInteger(parent, "Volume size ",
//                                  "Enter volume size:",
//                                  50, 1, 1000, 1);
//    QList<CellAPO> neuron_inmarkers;
//    for(V3DLONG i = 0; i <neuron.size();i++)
//    {
//        CellAPO t;
//        t.x = neuron.at(i).x+1;
//        t.y = neuron.at(i).y+1;
//        t.z = neuron.at(i).z+1;
//        t.color.r=255;
//        t.color.g=0;
//        t.color.b=0;
//        t.volsize = Vsize;
//        neuron_inmarkers.push_back(t);
//    }

//    QString apo_name = filename + ".apo";
//    writeAPO_file(apo_name,neuron_inmarkers);

//    QString linker_name = filename + ".ano";
//    QFile qf_anofile(linker_name);
//    if(!qf_anofile.open(QIODevice::WriteOnly))
//    {   bool flag = false;
//        v3d_msg("Cannot open file for writing!");
//        return flag;
//    }

//    QTextStream out(&qf_anofile);
//    out << "SWCFILE=" << QFileInfo(filename).fileName()<<endl;
//    out << "APOFILE=" << QFileInfo(apo_name).fileName()<<endl;
//    v3d_msg(QString("Save the linker file to: %1 Complete!").arg(linker_name));

    return true;
}
