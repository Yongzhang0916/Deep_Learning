#include "projection_swc.h"
#include "branch_angle.h"
#include "../sort_neuron_swc/sort_swc.h"

#define getParent(n,nt) ((nt).listNeuron.at(n).pn<0)?(1000000000):((nt).hashNeuron.value((nt).listNeuron.at(n).pn))

bool projection_swc(V3DPluginCallback2 &callback,const V3DPluginArgList &input,V3DPluginArgList &output,QWidget *parent,input_PARA &PARA)
{
    vector<char*>* inlist = (vector<char*>*)(input.at(0).p);
    vector<char*>* outlist = NULL;
    vector<char*>* paralist = NULL;

    QString fileOpenName = QString(inlist->at(0));
    NeuronTree nt = readSWC_file(fileOpenName);
    cout<<fileOpenName.toStdString()<<endl;
    cout<<"nt.listNeuron = "<<nt.listNeuron.size()<<endl;
    QString fileSaveName;

    if (output.size()==0)
    {
        printf("No outputfile specified.\n");
        fileSaveName = fileOpenName;
    }
    else if (output.size()==1)
    {
        outlist = (vector<char*>*)(output.at(0).p);
        fileSaveName = QString(outlist->at(0));
    }
    else
    {
        printf("You have specified more than 1 output file.\n");
        return false;
    }

//    QList<NeuronSWC> neuron_sorted;
//    if (!SortSWC(nt.listNeuron, neuron_sorted,VOID, 10))
//    {
//        v3d_msg("fail to call swc sorting function.",0);
//    }

//    export_list2file(neuron_sorted, fileSaveName,fileSaveName);
//    NeuronTree nt_b4_prune = readSWC_file(fileSaveName);
//    double threLen = 10;
//    NeuronTree nt_pruned = smartPrune(nt_b4_prune,threLen);
//    NeuronTree nt_pruned_2nd = smartPrune(nt_pruned,threLen);
//    cout<<"nt_pruned_2nd size : "<<nt_pruned_2nd.listNeuron.size()<<endl;
//    writeSWC_file(fileSaveName + QString("pruned.swc"),nt_pruned_2nd);
//    v3d_msg("stop");

    vector<vector<V3DLONG> > childs;
    childs = vector< vector<V3DLONG> >(nt.listNeuron.size(), vector<V3DLONG>() );

    //getChildNum
    for (V3DLONG i=0; i<nt.listNeuron.size();i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (nt.listNeuron[i].pn<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }

    //get branchpoints and wrongpoints
    int childNum;
    QList<NeuronSWC> branch_swclist,wrong_swclist;
    for (V3DLONG i=0; i<nt.listNeuron.size();i++)
    {
        if (nt.listNeuron[i].pn<0) continue;
        //childNum = childs[nt.listNeuron[i].pn].size();
        childNum = childs[i].size();
        if(childNum == 2)
            branch_swclist.push_back(nt.listNeuron[i]);
        else if(childNum >= 3)
            wrong_swclist.push_back(nt.listNeuron[i]);
    }
    cout<<"branch_swclist = "<<branch_swclist.size()<<endl;
    cout<<"wrong_swclist = "<<wrong_swclist.size()<<endl;

    QList<ImageMarker> branch_markerlist,wrong_markerlist;
    NeuronTree branch_ntlist, wrong_ntlist;
    branch_markerlist = swc_to_marker(branch_swclist);
    wrong_markerlist = swc_to_marker(wrong_swclist);
    branch_ntlist.listNeuron = branch_swclist;
    wrong_ntlist.listNeuron = wrong_swclist;
    writeMarker_file(fileOpenName+QString("_branchpoints.marker"),branch_markerlist);
    writeMarker_file(fileOpenName+QString("_wrongpoints.marker"),wrong_markerlist);
    writeSWC_file(fileOpenName + QString("_branchpoints.swc"),branch_ntlist);
    writeSWC_file(fileOpenName + QString("_wrongpoints.swc"),wrong_ntlist);
    v3d_msg("11111111111111111");

    //remove 3 branch points
    QList<NeuronSWC> choose_swclist1, choose_swclist2, choose_swclist3, choose_swclist4,remove_swclist;
    NeuronTree choose_ntlist1, choose_ntlist2,choose_ntlist3,choose_ntlist4,remove_ntlist;
    QList<ImageMarker> choose_markerlist1, choose_markerlist2, choose_markerlist3, choose_markerlist4;
    for(int i = 0;i < wrong_swclist.size();i++)
    {
        V3DLONG par = wrong_swclist[i].n;
        cout<<"par = "<<par<<endl;
        V3DLONG par1 = getParent(par,nt);
        cout<<"par1 = "<<par1<<endl;
        par = par1;
        if(nt.listNeuron[par1].pn == -1)
            break;
        //cout<<"childs[par1].size() = "<<childs[par1].size()<<endl;
        if(childs[par1].size() == 3)
        {
            cout<<"The wrong point :"<<nt.listNeuron[par1].n<<endl;
            choose_swclist1.clear();
            choose_markerlist1.clear();
            choose_swclist2.clear();
            choose_markerlist2.clear();
            choose_swclist3.clear();
            choose_markerlist3.clear();
            choose_swclist4.clear();
            choose_markerlist4.clear();

            //get parents
            getAllParent(nt,childs,par1-1,choose_swclist1,5);
//            for(int j = 0;j < 5;j++)
//            {
//                V3DLONG parent = getParent(par1, nt);
//                choose_swclist1.push_back(nt.listNeuron[parent]);
//                par1 = parent;
//            }

            //get points of 3 branch
            V3DLONG par11 = childs[wrong_swclist[i].n-1][0];
            V3DLONG par12 = childs[wrong_swclist[i].n-1][1];
            V3DLONG par13 = childs[wrong_swclist[i].n-1][2];
            cout<<"par11 = "<<par11<<"      par12 = "<<par12<<"      par13 = "<<par13<<endl;

            choosePointOf3Poiont(nt,childs,par11,par12,par13,choose_swclist2,choose_swclist3,choose_swclist4,20);

            cout<<"wrongpoint "<<wrong_swclist[i].n<<": choose_swclist1 = "<<choose_swclist1.size()<<endl;
            cout<<"wrongpoint "<<wrong_swclist[i].n<<": choose_swclist2 = "<<choose_swclist2.size()<<endl;
            cout<<"wrongpoint "<<wrong_swclist[i].n<<": choose_swclist3 = "<<choose_swclist3.size()<<endl;
            cout<<"wrongpoint "<<wrong_swclist[i].n<<": choose_swclist4 = "<<choose_swclist4.size()<<endl;

            choose_ntlist1.listNeuron = choose_swclist1;
            choose_ntlist2.listNeuron = choose_swclist2;
            choose_ntlist3.listNeuron = choose_swclist3;
            choose_ntlist4.listNeuron = choose_swclist4;
            choose_markerlist1 = swc_to_marker(choose_swclist1);
            choose_markerlist2 = swc_to_marker(choose_swclist2);
            choose_markerlist3 = swc_to_marker(choose_swclist3);
            choose_markerlist4 = swc_to_marker(choose_swclist4);

            writeSWC_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints1.swc"),choose_ntlist1);
            writeSWC_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints2.swc"),choose_ntlist2);
            writeSWC_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints3.swc"),choose_ntlist3);
            writeSWC_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints4.swc"),choose_ntlist4);

            writeMarker_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints1.marker"),choose_markerlist1);
            writeMarker_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints2.marker"),choose_markerlist2);
            writeMarker_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints3.marker"),choose_markerlist3);
            writeMarker_file(fileOpenName+QString("wrongpoint")+QString::number(wrong_swclist[i].n)+QString("_choosepoints4.marker"),choose_markerlist4);

//            //fitline
//            double a1_2,b1_2,c1_2,angle1_2,a1_3,b1_3,c1_3,angle1_3,a1_4,b1_4,c1_4,angle1_4;
//            cv::Vec6f line_para1,line_para2,line_para3,line_para4;
//            if(choose_swclist1.size() < 3 || choose_swclist2.size() < 3 )
//                break;
//            getFitLine(choose_swclist1,choose_swclist2,line_para1,line_para2);

//            if(choose_swclist1.size() < 3 || choose_swclist3.size() < 3 )
//                break;
//            getFitLine(choose_swclist1,choose_swclist3,line_para1,line_para3);

//            if(choose_swclist1.size() < 3 || choose_swclist4.size() < 3 )
//                break;
//            getFitLine(choose_swclist1,choose_swclist4,line_para1,line_para4);

//            cout<<"line_para1 = "<<line_para1<<endl;
//            cout<<"line_para2 = "<<line_para2<<endl;
//            cout<<"line_para3 = "<<line_para3<<endl;
//            cout<<"line_para4 = "<<line_para4<<endl;

//            //calculate angle
//            PointCoordinate ProjPoint11,ProjPoint12,ProjPoint21,ProjPoint22,ProjPoint31,ProjPoint32,ProjPoint41,ProjPoint42;
//            GetProjPoint(choose_swclist1,line_para1,ProjPoint11,ProjPoint12);
//            GetProjPoint(choose_swclist2,line_para2,ProjPoint21,ProjPoint22);
//            GetProjPoint(choose_swclist3,line_para3,ProjPoint31,ProjPoint32);
//            GetProjPoint(choose_swclist4,line_para4,ProjPoint41,ProjPoint42);

//            a1_2 = ((ProjPoint12.x - ProjPoint11.x)*(ProjPoint22.x - ProjPoint21.x)+(ProjPoint12.y - ProjPoint11.y)*(ProjPoint22.y - ProjPoint21.y)+(ProjPoint12.z - ProjPoint11.z)*(ProjPoint22.z - ProjPoint21.z));
//            b1_2= sqrt((ProjPoint12.x - ProjPoint11.x)*(ProjPoint12.x - ProjPoint11.x) + (ProjPoint12.y - ProjPoint11.y)*(ProjPoint12.y - ProjPoint11.y) + (ProjPoint12.z - ProjPoint11.z)*(ProjPoint12.z - ProjPoint11.z));
//            c1_2= sqrt((ProjPoint22.x - ProjPoint21.x)*(ProjPoint22.x - ProjPoint21.x) + (ProjPoint22.y - ProjPoint21.y)*(ProjPoint22.y - ProjPoint21.y) + (ProjPoint22.z - ProjPoint21.z)*(ProjPoint22.z - ProjPoint21.z));
//            cout<<"a1_2 = "<<a1_2<<"   b1_2 = "<<b1_2<<"    c1_2 = "<<c1_2<<endl;

//            a1_3 = ((ProjPoint12.x - ProjPoint11.x)*(ProjPoint32.x - ProjPoint31.x)+(ProjPoint12.y - ProjPoint11.y)*(ProjPoint32.y - ProjPoint31.y)+(ProjPoint12.z - ProjPoint11.z)*(ProjPoint32.z - ProjPoint31.z));
//            b1_3= sqrt((ProjPoint12.x - ProjPoint11.x)*(ProjPoint12.x - ProjPoint11.x) + (ProjPoint12.y - ProjPoint11.y)*(ProjPoint12.y - ProjPoint11.y) + (ProjPoint12.z - ProjPoint11.z)*(ProjPoint12.z - ProjPoint11.z));
//            c1_3= sqrt((ProjPoint32.x - ProjPoint31.x)*(ProjPoint32.x - ProjPoint31.x) + (ProjPoint32.y - ProjPoint31.y)*(ProjPoint32.y - ProjPoint31.y) + (ProjPoint32.z - ProjPoint31.z)*(ProjPoint32.z - ProjPoint31.z));
//            cout<<"a1_3 = "<<a1_3<<"   b1_3 = "<<b1_3<<"    c1_3 = "<<c1_3<<endl;

//            a1_4 = ((ProjPoint12.x - ProjPoint11.x)*(ProjPoint42.x - ProjPoint41.x)+(ProjPoint12.y - ProjPoint11.y)*(ProjPoint42.y - ProjPoint41.y)+(ProjPoint12.z - ProjPoint11.z)*(ProjPoint42.z - ProjPoint41.z));
//            b1_4= sqrt((ProjPoint12.x - ProjPoint11.x)*(ProjPoint12.x - ProjPoint11.x) + (ProjPoint12.y - ProjPoint11.y)*(ProjPoint12.y - ProjPoint11.y) + (ProjPoint12.z - ProjPoint11.z)*(ProjPoint12.z - ProjPoint11.z));
//            c1_4= sqrt((ProjPoint42.x - ProjPoint41.x)*(ProjPoint42.x - ProjPoint41.x) + (ProjPoint42.y - ProjPoint41.y)*(ProjPoint42.y - ProjPoint41.y) + (ProjPoint42.z - ProjPoint41.z)*(ProjPoint42.z - ProjPoint41.z));
//            cout<<"a1_4 = "<<a1_4<<"   b1_4 = "<<b1_4<<"    c1_4 = "<<c1_4<<endl;

//            angle1_2 = a1_2/(b1_2*c1_2);
//            angle1_3 = a1_3/(b1_3*c1_3);
//            angle1_4 = a1_4/(b1_4*c1_4);
//            cout<<"angle1_2 = "<<angle1_2<<endl;
//            cout<<"angle1_3 = "<<angle1_3<<endl;
//            cout<<"angle1_4 = "<<angle1_4<<endl;

//            QList<NeuronSWC> chooseChild_swclist1,chooseChild_swclist2;
//            //remove all childs
//            chooseChild_swclist1.clear();
//            chooseChild_swclist2.clear();
//            getAllChild(nt,childs,childs[par][1],chooseChild_swclist1,VOID);
//            getAllChild(nt,childs,childs[par][2],chooseChild_swclist2,VOID);
//            remove_swclist.append(chooseChild_swclist1);
//            remove_swclist.append(chooseChild_swclist2);
//            cout<<"remove_swclist_temp = "<<remove_swclist.size()<<endl;

        }

    }

//    remove_ntlist.listNeuron = remove_swclist;
//    writeSWC_file(fileOpenName + QString("_remove.swc"),remove_ntlist);
//    for(int p = 0;p < nt.listNeuron.size();p++)
//    {
//        for(int q = 0;q < remove_swclist.size();q++)
//        {
//            if((nt.listNeuron[p].x == remove_swclist[q].x)&&(nt.listNeuron[p].y == remove_swclist[q].y)&&(nt.listNeuron[p].z == remove_swclist[q].z))
//                nt.listNeuron.removeAt(p);
//        }
//    }
//    cout<<"nt.listNeuron = "<<nt.listNeuron.size()<<endl;
//    writeSWC_file(fileOpenName + QString("_result.swc"),nt);

    v3d_msg("stop1");

//    //method:remove by parentpoints and angle
//    QList<NeuronSWC> choose_swclist5,choose_swclist6,choose_swclist7,choose_swclist8;
//    NeuronTree choose_ntlist5,choose_ntlist6,choose_ntlist7,choose_ntlist8;
//    QList<ImageMarker> choose_markerlist5,choose_markerlist6,choose_markerlist7,choose_markerlist8;
//    double dist_branch,dist_thres = 8;
//    int count =0;
//    for(int i = 0;i < branch_swclist.size();i++)
//    {
//        V3DLONG par = branch_swclist[i].n;
//        V3DLONG par1 = getParent(par,nt);
//        //cout<<"par1 = "<<par1<<endl;
//         par = par1;
//        if(nt.listNeuron[par1].pn == -1)
//            break;
//        //cout<<"childs[par1].size() = "<<childs[par1].size()<<endl;
//        if(childs[par1].size() == 2)
//        {
//            choose_swclist5.clear();
//            choose_markerlist5.clear();
//            choose_swclist6.clear();
//            choose_markerlist6.clear();
//            V3DLONG par11 = childs[branch_swclist[i].n-1][0];
//            V3DLONG par21 = childs[branch_swclist[i].n-1][1];
//            //cout<<"par11 = "<<par11<<"      par21 = "<<par21<<endl;

//            choosePoint(nt,childs,par11,par21,choose_swclist5,choose_swclist6,5);
//            cout<<"branchpoint "<<branch_swclist[i].n<<": choose_swclist5 = "<<choose_swclist5.size()<<endl;
//            cout<<"branchpoint "<<branch_swclist[i].n<<": choose_swclist6 = "<<choose_swclist6.size()<<endl;
//            choose_ntlist5.listNeuron = choose_swclist5;
//            choose_ntlist6.listNeuron = choose_swclist6;
//            choose_markerlist5 = swc_to_marker(choose_swclist5);
//            choose_markerlist6 = swc_to_marker(choose_swclist6);
//            writeSWC_file(fileOpenName+QString("_branchpoint")+QString::number(branch_swclist[i].n)+QString("_choosepoints5.swc"),choose_ntlist5);
//            writeSWC_file(fileOpenName+QString("_branchpoint")+QString::number(branch_swclist[i].n)+QString("_choosepoints6.swc"),choose_ntlist6);
//            writeMarker_file(fileOpenName+QString("_branchpoint")+QString::number(branch_swclist[i].n)+QString("_choosepoints5.marker"),choose_markerlist5);
//            writeMarker_file(fileOpenName+QString("_branchpoint")+QString::number(branch_swclist[i].n)+QString("_choosepoints6.marker"),choose_markerlist6);

//            //fitline
//            double a1,b1,c1,angle1;
//            cv::Vec6f line_para5,line_para6;
//            if(choose_swclist5.size() < 3 || choose_swclist6.size() < 3 )
//                continue;
//            getFitLine(choose_swclist5,choose_swclist6,line_para5,line_para6);
//            cout<<"line_para5 = "<<line_para5<<endl;
//            cout<<"line_para6 = "<<line_para6<<endl;

//            //calculate angle
//            PointCoordinate ProjPoint1,ProjPoint2,ProjPoint3,ProjPoint4;
//            GetProjPoint(choose_swclist5,line_para5,ProjPoint1,ProjPoint2);
//            GetProjPoint(choose_swclist6,line_para6,ProjPoint3,ProjPoint4);

//            a1 = ((ProjPoint2.x - ProjPoint1.x)*(ProjPoint4.x - ProjPoint3.x)+(ProjPoint2.y - ProjPoint1.y)*(ProjPoint4.y - ProjPoint3.y)+(ProjPoint2.z - ProjPoint1.z)*(ProjPoint4.z - ProjPoint3.z));
//            b1= sqrt((ProjPoint2.x - ProjPoint1.x)*(ProjPoint2.x - ProjPoint1.x) + (ProjPoint2.y - ProjPoint1.y)*(ProjPoint2.y - ProjPoint1.y) + (ProjPoint2.z - ProjPoint1.z)*(ProjPoint2.z - ProjPoint1.z));
//            c1= sqrt((ProjPoint4.x - ProjPoint3.x)*(ProjPoint4.x - ProjPoint3.x) + (ProjPoint4.y - ProjPoint3.y)*(ProjPoint4.y - ProjPoint3.y) + (ProjPoint4.z - ProjPoint3.z)*(ProjPoint4.z - ProjPoint3.z));
//            cout<<"a1 = "<<a1<<"   b1 = "<<b1<<"    c1 = "<<c1<<endl;
//            angle1 = a1/(b1*c1);
//            cout<<"angle1 = "<<angle1<<endl;
//        }

//    }

 //   v3d_msg("stop2");


//    //get subtree
//    double x1 = 12701.5, y1 = 40650, z1 = 3518.02;
//    double x2 = 12697.6, y2 = 4045.19, z2 = 3539.40;
//    double thres = 20;
//    NeuronTree sub_nt1, sub_nt2;
//    for(int i = 0;i < nt.listNeuron.size();i++)
//    {
//        //get sub1
//        if((nt.listNeuron[i].x > x1-thres && nt.listNeuron[i].x < x1 +thres) || (nt.listNeuron[i].y > y1-thres && nt.listNeuron[i].y < y1 +thres) || (nt.listNeuron[i].z > z1-thres && nt.listNeuron[i].z < z1 +thres))
//        {
//            NeuronSWC cur1;
//            cur1.x = nt.listNeuron[i].x;
//            cur1.y = nt.listNeuron[i].y;
//            cur1.z = nt.listNeuron[i].z;
//            cur1.parent = nt.listNeuron[i].parent;
//            cur1.radius = nt.listNeuron[i].radius;
//            cur1.type = nt.listNeuron[i].type;
//            cur1.n = nt.listNeuron[i].n;

//            sub_nt1.listNeuron.push_back(cur1);
//        }

//        //get sub2
//        if((nt.listNeuron[i].x > x2-thres && nt.listNeuron[i].x < x2 +thres) || (nt.listNeuron[i].y > y2-thres && nt.listNeuron[i].y < y2+thres) || (nt.listNeuron[i].z > z2-thres && nt.listNeuron[i].z < z2 +thres))
//        {
//            NeuronSWC cur2;
//            cur2.x = nt.listNeuron[i].x;
//            cur2.y = nt.listNeuron[i].y;
//            cur2.z = nt.listNeuron[i].z;
//            cur2.parent = nt.listNeuron[i].parent;
//            cur2.radius = nt.listNeuron[i].radius;
//            cur2.type = nt.listNeuron[i].type;
//            cur2.n = nt.listNeuron[i].n;

//            sub_nt2.listNeuron.push_back(cur2);
//        }
//    }

//    writeSWC_file(fileOpenName+"_"+QString::number(x1)+"_x"+QString::number(y1)+"_y"+QString::number(z1)+"_z"+QString("_subtree1.swc"),sub_nt1);
//    writeSWC_file(fileOpenName+"_"+QString::number(x2)+"_x"+QString::number(y2)+"_y"+QString::number(z2)+"_z"+QString("_subtree2.swc"),sub_nt2);


//    NeuronTree nt_xy,nt_xz,nt_yz;
//    for(int i = 0;i < nt.listNeuron.size();i++)
//    {
//        NeuronSWC cur_xy,cur_xz,cur_yz;
//        //xy
//        cur_xy.x = nt.listNeuron[i].x;
//        cur_xy.y = nt.listNeuron[i].y;
//        cur_xy.r = nt.listNeuron[i].r;
//        cur_xy.type = nt.listNeuron[i].type;
//        cur_xy.n = nt.listNeuron[i].n;
//        cur_xy.parent = nt.listNeuron[i].parent;
//        nt_xy.listNeuron.push_back(cur_xy);

//        //xz
//        cur_xz.x = nt.listNeuron[i].x;
//        cur_xz.z = nt.listNeuron[i].z;
//        cur_xz.r = nt.listNeuron[i].r;
//        cur_xz.type = nt.listNeuron[i].type;
//        cur_xz.n = nt.listNeuron[i].n;
//        cur_xz.parent = nt.listNeuron[i].parent;
//        nt_xz.listNeuron.push_back(cur_xz);

//        //yz
//        cur_yz.y = nt.listNeuron[i].y;
//        cur_yz.z = nt.listNeuron[i].z;
//        cur_yz.r = nt.listNeuron[i].r;
//        cur_yz.type = nt.listNeuron[i].type;
//        cur_yz.n = nt.listNeuron[i].n;
//        cur_yz.parent = nt.listNeuron[i].parent;
//        nt_yz.listNeuron.push_back(cur_yz);

//    }
//    cout<<"nt_xy.size = "<<nt_xy.listNeuron.size()<<"    nt_xz.size = "<<nt_xz.listNeuron.size()<<"    nt_yz.size = "<<nt_yz.listNeuron.size()<<endl;
//    writeSWC_file(fileOpenName+QString("_xy.swc"),nt_xy);
//    writeSWC_file(fileOpenName+QString("_xz.swc"),nt_xz);
//    writeSWC_file(fileOpenName+QString("_yz.swc"),nt_yz);



    return true;
}



NeuronTree smartPrune(NeuronTree nt, double length)
{
    QVector<QVector<V3DLONG> > childs;

    V3DLONG neuronNum = nt.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    V3DLONG *flag = new V3DLONG[neuronNum];

    for (V3DLONG i=0;i<neuronNum;i++)
    {
        flag[i] = 1;

        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }

    QList<NeuronSWC> list = nt.listNeuron;

    for (int i=0;i<list.size();i++)
    {
        if (childs[i].size()==0 && list.at(i).parent >=0)
        {
            int index_tip = 0;
            int parent_tip = getParent(i,nt);
            while(childs[parent_tip].size()<2)
            {
                parent_tip = getParent(parent_tip,nt);
                index_tip++;
                if(parent_tip == 1000000000)
                    break;
            }
            if(parent_tip != 1000000000 && index_tip < length && list.at(childs[parent_tip].at(0)).type != list.at(childs[parent_tip].at(1)).type)
            {
                flag[i] = -1;

                int parent_tip = getParent(i,nt);
                while(childs[parent_tip].size()<2)
               {
                    flag[parent_tip] = -1;
                    parent_tip = getParent(parent_tip,nt);
                    if(parent_tip == 1000000000)
                        break;
               }
            }

        }else if (childs[i].size()==0 && list.at(i).parent < 0)
            flag[i] = -1;

    }

   //NeutronTree structure
   NeuronTree nt_prunned;
   QList <NeuronSWC> listNeuron;
   QHash <int, int>  hashNeuron;
   listNeuron.clear();
   hashNeuron.clear();

   //set node

   NeuronSWC S;
   for (int i=0;i<list.size();i++)
   {
       if(flag[i] == 1)
       {
            NeuronSWC curr = list.at(i);
            S.n 	= curr.n;
            S.type 	= curr.type;
            S.x 	= curr.x;
            S.y 	= curr.y;
            S.z 	= curr.z;
            S.r 	= curr.r;
            S.pn 	= curr.pn;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);
       }

  }
   nt_prunned.n = -1;
   nt_prunned.on = true;
   nt_prunned.listNeuron = listNeuron;
   nt_prunned.hashNeuron = hashNeuron;

   if(flag) {delete[] flag; flag = 0;}
   return nt_prunned;
}

void choosePointOf3Poiont(NeuronTree &nt,vector<vector<V3DLONG> > &childs,V3DLONG &par11,V3DLONG &par12,V3DLONG &par13,QList<NeuronSWC> &choose_swclist1,QList<NeuronSWC> &choose_swclist2,QList<NeuronSWC> &choose_swclist3,int m)
{
    //QList<NeuronSWC> branchChildtoolittle_swclist;
    //NeuronTree branchChildtoolittle_ntlist;
    NeuronSWC child1,child2,child3;
    child1 = nt.listNeuron[par11];
    child2 = nt.listNeuron[par12];
    child3 = nt.listNeuron[par13];
    //choose_swclist1.push_back(child1);
    //choose_swclist2.push_back(child2);

    for(int n =0;n < m;n++)
    {
        if(childs[child1.n-1].size()==0 || childs[child1.n-1].size()>=2)
        {
            //branchChildtoolittle_swclist.push_back(nt.listNeuron[branch_swclist[i].n]);
            break;
        }
        if(childs[child2.n-1].size()==0 || childs[child2.n-1].size()>=2)
            break;
        if(childs[child3.n-1].size()==0 || childs[child3.n-1].size()>=2)
            break;
        V3DLONG par1 = childs[child1.n-1][0];
        //cout<<childs[child1.n-1].size()<<endl;
        V3DLONG par2 = childs[child2.n-1][0];
        V3DLONG par3 = childs[child3.n-1][0];
        cout<<"par1 = "<<par1<<"    par2 = "<<par2<<"    par3 = "<<par3<<endl;
        child1 = nt.listNeuron[par1];
        child2 = nt.listNeuron[par2];
        child3 = nt.listNeuron[par3];
        choose_swclist1.push_back(child1);
        choose_swclist2.push_back(child2);
        choose_swclist3.push_back(child3);
    }

    return;
}
