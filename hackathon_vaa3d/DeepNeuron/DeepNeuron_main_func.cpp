#include "v3d_message.h"
#include <vector>
#include <iostream>
#include "string.h"
#include "basic_surf_objs.h"
#include "my_surf_objs.h"
//#include "SelectNeuronDlg.h"
#include "blastneuron/resampling.h"
#include "blastneuron/sort_swc.h"
#include "blastneuron/local_alignment.h"

#include "DeepNeuron_main_func.h"
#include "DeepNeuron_plugin.h"

#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))
#define MHDIS(a,b) ( (fabs((a).x-(b).x)) + (fabs((a).y-(b).y)) + (fabs((a).z-(b).z)) )
#define MHDIS_xy(a,b) ( (fabs((a).x-(b).x)) + (fabs((a).y-(b).y)) )
#define VOID 1000000000

bool DeepNeuron_main_func(input_PARA &PARA,V3DPluginCallback2 &callback,bool bmenu,QWidget *parent)
{
    QString fileOpenName1;
    fileOpenName1 = QFileDialog::getOpenFileName(0, QObject::tr("Open Auto_tracing_SWC File"),
                                                "",

                                                QObject::tr("Supported file (*.swc *.SWC *.eswc)"

                                            ));
    NeuronTree nt1=readSWC_file(fileOpenName1);
    //QList<NeuronSWC> neuron1=nt1.listNeuron;
    cout<<fileOpenName1.toStdString()<<endl;

//    NeuronTree nt_prune;
//    double prune_thre = 0.5;  //need checkout prune thres!
//    //result_prune=prune_long_alignment(neuron_prune,prune_thres);
//    prune_branch(nt1,nt_prune,prune_thre);
//    writeSWC_file(fileOpenName1+"_pruned.swc",nt_prune);
//    v3d_msg("stop");

    QString fileOpenName2;
    fileOpenName2 = QFileDialog::getOpenFileName(0, QObject::tr("Open Manual_tracing_SWC File"),
                                                "",

                                                QObject::tr("Supported file (*.swc *.SWC *.eswc)"

                                            ));
    NeuronTree nt2=readSWC_file(fileOpenName2);
    cout<<fileOpenName2.toStdString()<<endl;

    QString inimg_file = callback.getPathTeraFly();


    cout<<"************************this is pre_processing***********************"<<endl;

    QList<NeuronSWC> auto_bad_point1;  //auto_tracing too many and wrong
    QList<NeuronSWC> auto_bad_point2;  //auto_tracing too little
    QList<NeuronSWC> auto_good_point;  //auto_tracing corresponds to the manual_tracing

    cout<<"***Look for wrong auto_tracing points before pre_processing.***"<<endl;
    vector<vector<V3DLONG> > childs;
    NeuronTree nt1_temp;
    QList<NeuronSWC> wrong_point,other_point,other_point_temp;
    childs = vector< vector<V3DLONG> >(nt1.listNeuron.size(), vector<V3DLONG>() );
    getChildNum(nt1,childs);

    nt1_temp = get_right_area(nt1,childs,other_point_temp,wrong_point);

    other_point = match_point(other_point_temp,other_point_temp);

    cout<<"other_point.size = "<<other_point.size()<<endl;
    cout<<"wrong_point.size = "<<wrong_point.size()<<endl;
    cout<<"nt1_temp_point.size = "<<nt1_temp.listNeuron.size()<<endl;

    //step 1:Get auto_bad_point1 based on angle
    for(V3DLONG i=0;i<wrong_point.size();i++)
    {
        auto_bad_point1.push_back(wrong_point[i]);
    }

//    QString filename_other_point = fileOpenName1 + "_other_point.swc";
//    QString filename_wrong_point = fileOpenName1 + "_wrong_point.swc";
//    QString filename_right_point = fileOpenName1 + "_right_point.swc";

//    export_list2file(other_point,filename_other_point,fileOpenName1);
//    export_list2file(wrong_point,filename_wrong_point,fileOpenName1);
//    export_list2file(nt1_temp.listNeuron,filename_right_point,fileOpenName1);

    cout<<"***this is prune_branch***"<<endl;
    NeuronTree nt1_prune,nt2_prune,bound_neuron;
    double prune_thres = 0;  //need checkout prune thres!

    //result_prune=prune_long_alignment(neuron_prune,prune_thres);
    prune_branch(nt1,nt1_prune,prune_thres);
    cout<<"nt1.size = "<<nt1.listNeuron.size()<<endl;
    cout<<"nt1_prune.size = "<<nt1_prune.listNeuron.size()<<endl;

    prune_branch(nt2,nt2_prune,prune_thres);
    cout<<"nt2.size = "<<nt2.listNeuron.size()<<endl;
    cout<<"nt2_prune.size = "<<nt2_prune.listNeuron.size()<<endl;

    cout<<"***this is resample***"<<endl;
//    bool ok;
//    double resample_step;
//    resample_step = QInputDialog::getDouble(0, "Would you like to set a step for resample?","resample_step:(If you select 'cancel', the resample_step is 2)",0,0,2147483647,1,&ok);
//    if (!ok)
//        resample_step = 5;

    double resample_step = 5;
    NeuronTree nt1_resample=resample(nt1_prune,resample_step);
    NeuronTree nt2_resample=resample(nt2_prune,resample_step);
    cout<<"nt1_resample.size = "<<nt1_resample.listNeuron.size()<<endl;
    cout<<"nt2_resample.size = "<<nt2_resample.listNeuron.size()<<endl;

    cout<<"***this is sort***"<<endl;
    QList<NeuronSWC> result1,result1_temp,result2;
    QList<CellAPO> markers;
    V3DLONG rootid=VOID;
    V3DLONG thres=VOID;
    V3DLONG root_dist_thres=0;

    sort_with_standard(nt1_resample.listNeuron,nt2_resample.listNeuron,result1_temp);
    //sort_with_standard(nt2_resample.listNeuron,neuron2,result2);

//    if(!Sort_auto_SWC(nt1_resample.listNeuron,result1,rootid,thres))
//    {
//        cout<<"Error in sorting swc"<<endl;
//        return false;
//    }

    if (!Sort_manual_SWC(nt2_resample.listNeuron,result2,rootid,thres,root_dist_thres,markers))
    {
        cout<<"Error in sorting swc"<<endl;
        return false;
    }
    cout<<"result1_temp.size = "<<result1_temp.size()<<endl;
    cout<<"result2.size = "<<result2.size()<<endl;

    cout<<"***Matching automatic tracking bound based on manual tracking.***"<<endl;

    double maxx=-VOID;
    double maxy=-VOID;
    double maxz=-VOID;
    double minx= VOID;
    double miny= VOID;
    double minz= VOID;
    double add_num = 10;

    for(V3DLONG i=0;i<result2.size();i++)
    {
        if(result2[i].x>maxx)
        {
            maxx = result2[i].x+add_num;
        }
        if(result2[i].y>maxy)
        {
            maxy = result2[i].y+add_num;
        }
        if(result2[i].z>maxz)
        {
            maxz = result2[i].z+add_num;
        }

        if(result2[i].x<minx)
        {
            minx = result2[i].x-add_num;
        }
        if(result2[i].y<miny)
        {
            miny = result2[i].y-add_num;
        }
        if(result2[i].z<minz)
        {
            minz = result2[i].z-add_num;
        }
    }

    //step 2:Get auto_bad_point1 based on bound.
    for(V3DLONG i=0;i<result1_temp.size();i++)
    {
        if( (result1_temp[i].x<maxx)&&(result1_temp[i].y<maxy)&&(result1_temp[i].z<maxz)&&(result1_temp[i].x>minx)&&(result1_temp[i].y>miny)&&(result1_temp[i].z>minz) )
        {
            bound_neuron.listNeuron.push_back(result1_temp[i]);
        }
        else
        {
            auto_bad_point1.push_back(result1_temp[i]);
        }
    }

    result1 = bound_neuron.listNeuron;

    cout<<"bound_neuron.size = "<<bound_neuron.listNeuron.size()<<endl;
    cout<<"result1.size = "<<result1.size()<<endl;
    cout<<"result2.size = "<<result2.size()<<endl;

    //output pre_processing result swc.
//    QString fileSaveName1 = fileOpenName1 + "_result1.swc";
//    QString fileSaveName2 = fileOpenName2 + "_result2.swc";

//    if (!export_list2file(result1, fileSaveName1, fileOpenName1))
//    {
//        printf("fail to write the output swc file.\n");
//        return false;
//    }
//    if (!export_list2file(result2, fileSaveName2, fileOpenName2))
//    {
//        printf("fail to write the output swc file.\n");
//        return false;
//    }

    cout<<"******************this is choose point and get sub_block*********************"<<endl;

    bool ok;
    double method;
    method = QInputDialog::getDouble(0, "Would you like to set a method for choose point?","method class:(If you select 'cancel', the method is 1)",0,0,2147483647,1,&ok);
    if (!ok)
       method = 1;

    if(method == 1)
    {
        //method 1: Get closer points to the manual_tracing on auto_tracing.
        QList<NeuronSWC> auto_bad_point2_temp;
        auto_good_point = choose_point(result1,result2,0,10);
        auto_bad_point1 = choose_point(result1,result2,10,10000);
        auto_bad_point2_temp = choose_point(result2,result1,10,10000);

        for(V3DLONG i=0;i<auto_bad_point2_temp.size();i++)
        {
            if(auto_bad_point2_temp[i].type == 2)
                auto_bad_point2.push_back(auto_bad_point2_temp[i]);
        }

        cout<<"auto_good_point.size = "<<auto_good_point.size()<<endl;
        cout<<"auto_bad_point1.size = "<<auto_bad_point1.size()<<endl;
        cout<<"auto_bad_point2_temp.size = "<<auto_bad_point2_temp.size()<<endl;
        cout<<"auto_bad_point2.size = "<<auto_bad_point2.size()<<endl;

        QString filename1 ="auto_good.swc";
        QString filename2 ="auto_bad1.swc";
        QString filename3 ="auto_bad2.swc";

        export_list2file(auto_good_point,filename1, fileOpenName1);
        export_list2file(auto_bad_point1,filename2, fileOpenName1);
        export_list2file(auto_bad_point2,filename3, fileOpenName1);


        cout<<"***************this is get sub_block based on terafly image******************"<<endl;

        //QString inimg_file = callback.getPathTeraFly();
        bool ok;
        double sub_block;
        sub_block = QInputDialog::getDouble(0, "Would you like to set a class for sub_block?","sub_block class:(If you select 'cancel', the sub_block is 1)",0,0,2147483647,1,&ok);
        if (!ok)
           sub_block = 1;

        if(sub_block == 1)
        {
            cout<<"***************auto_tracing good*************************"<<endl;
            get_sub_block(callback,auto_good_point,filename1);
        }
        else if(sub_block == 2)
        {
            cout<<"***************auto_tracing too many ********************"<<endl;
            get_sub_block(callback,auto_bad_point1,filename2);
        }
        else if(sub_block == 3)
        {
            cout<<"***************auto_tracing not enough*******************"<<endl;
            get_sub_block(callback,auto_bad_point2,filename3);
        }
    }
    else if (method == 2)
    {
        //method 2: Get closer points to the manual_tracing on auto_tracing.
        QList<NeuronSWC> choose_auto1,choose_auto2,choose_manual1,choose_manual2;
        choose_auto1 = choose_point(result1,result2,0,10);
        choose_auto2 = choose_point(result1,result2,10,10000);
        cout<<"choose_auto1.size = "<<choose_auto1.size()<<endl;
        cout<<"choose_auto2.size = "<<choose_auto2.size()<<endl;

        choose_manual1 = choose_point(result2,result1,0,10);
        choose_manual2 = choose_point(result2,result1,10,10000);
        cout<<"choose_manual.size = "<<choose_manual1.size()<<endl;
        cout<<"choose_manual2.size = "<<choose_manual2.size()<<endl;

        QList<NeuronSWC> final_result;

        for(V3DLONG i=0;i<choose_auto1.size();i++)
        {
            choose_auto1[i].type = 3;
            final_result.push_back(choose_auto1[i]);
        }
        for(V3DLONG i=0;i<choose_auto2.size();i++)
        {
            choose_auto2[i].type = 2;
            final_result.push_back(choose_auto2[i]);
        }
        cout<<"final_result.szie = "<<final_result.size()<<endl;

        QString filename1 =fileOpenName1 + "_temp1.swc";
        QString filename2 =fileOpenName1 + "_temp2.swc";
        QString filename3 =fileOpenName1 + "_final_result.swc";

        export_list2file(choose_auto1, filename1, fileOpenName1);
        export_list2file(choose_auto2, filename2, fileOpenName1);
        export_list2file(final_result, filename3, fileOpenName1);

        vector<FourType> fourtype;
        vector<vector<Coordinate> > fourcoord;
        V3DLONG lens = 5;
        V3DLONG length = 2;  //step_for_get_sub_img
        get_subarea_in_nt(fourtype,length,choose_auto1,choose_auto2,choose_manual1,choose_manual2);
        cout<<"four_type_size = "<<fourtype.size()<<endl;
        make_coordinate(fourcoord,fourtype,lens);

        cout<<"******************this is terafly image *********************"<<endl;

        vector<string> vec,vec_auto;
        string img = inimg_file.toStdString();
        string name_temp_ = fileOpenName1.toStdString();

        SplitString(name_temp_,vec_auto,"/");
        cout<<"vec_auto.size = "<<vec_auto.size()<<endl;

        SplitString(img,vec,"/");
        cout<<"vec.size = "<<vec.size()<<endl;

        QString name_temp= QString::fromStdString(vec_auto[vec_auto.size()-1]);
        QString filename_little = "_"+name_temp+"_little_1_";
        QString filename_middle = "_"+name_temp+"_middle_2_";
        //QString filename_rec = "_"+name_temp+"_point_not_auto_3_";
        QString filename_manual = "_"+name_temp+"_point_not_manual_4_";

        QString image =QString::fromStdString(vec[vec.size()-1]);
        cout<<image.toStdString()<<endl;

        bool ok;
        double sub_terafly;
        sub_terafly = QInputDialog::getDouble(0, "Would you like to set a class for sub_terafly?","sub_terafly class:(If you select 'cancel', the sub_terafly is 1)",0,0,2147483647,1,&ok);
        if (!ok)
           sub_terafly = 1;

        if(sub_terafly == 1)
        {
            cout<<"***************class 1*************************"<<endl;
            get_subimg_terafly(inimg_file,filename_little,fourcoord[0],callback);
            processImage(callback,fourcoord[0]);
        }
        else if(sub_terafly == 2)
        {
            cout<<"***************class 2*************************"<<endl;
            get_subimg_terafly(inimg_file,filename_middle,fourcoord[1],callback);
            processImage(callback,fourcoord[1]);
        }
        else if(sub_terafly == 3)
        {
            cout<<"***************class 3*************************"<<endl;
            get_subimg_terafly(inimg_file,filename_manual,fourcoord[3],callback);
            processImage(callback,fourcoord[3]);
        }

    }

     return true;
}



void getChildNum(const NeuronTree &nt, vector<vector<V3DLONG> > &childs)
{
    V3DLONG nt_size=nt.listNeuron.size();
    for (V3DLONG i=0; i<nt_size;i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);

        //cout<<"i="<<i<<"   par="<<nt.hashNeuron.value(par)<<endl;
    }
}

NeuronTree get_right_area(NeuronTree &nt,vector<vector<V3DLONG> > &childs,QList<NeuronSWC> &other_point,QList<NeuronSWC> &wrong_point)
{
    NeuronTree right_point,right_point2;
    QList<NeuronSWC> right_point_temp,branch,branch_sort,except_other;
    //getChildNum(nt,childs);
    int child_num;
    int useless;
    NeuronSWC son1;
    NeuronSWC son2;
    NeuronSWC curr;
    double angle;
    double dis_a,dis_b,dis_c;
    for(V3DLONG i=0;i<nt.listNeuron.size();i++)
    {
        curr = nt.listNeuron[i];
        child_num = childs[nt.listNeuron[i].n].size();
        if(child_num==2)
        {
            son1 = nt.listNeuron[childs[nt.listNeuron[i].n][0]];
            son2 = nt.listNeuron[childs[nt.listNeuron[i].n][1]];

            dis_a = NTDIS(curr,son1);
            dis_b = NTDIS(curr,son2);
            dis_c = NTDIS(son1,son2);
            angle = ( (dis_a*dis_a) + (dis_b*dis_b) - (dis_c*dis_c ) )/(2*dis_a*dis_b);
            //cout<<"angle = "<<angle<<endl;
            if(angle<0)
            {
                other_point.push_back(curr);
                branch.push_back(curr);
                other_point.push_back(son1);
                branch.push_back(son1);
                other_point.push_back(son2);
                branch.push_back(son2);
                useless = down_child(nt,childs,son1.n,other_point);
                useless = down_child(nt,childs,son2.n,other_point);
            }
            else
            {
                right_point.listNeuron.push_back(curr);
            }

        }
        else if(child_num>2)
        {
            wrong_point.push_back(nt.listNeuron[i]);
        }
        else
        {
            right_point.listNeuron.push_back(curr);
        }

    }
    QString branch_name = "branch.swc";
 //   SortSWC(branch,branch_sort,branch[0].n,100000);
 //   export_list2file(branch_sort,branch_name,branch_name);
    right_point_temp = match_point_v2(right_point.listNeuron,other_point);
    //other_point = match_point_v2(nt.listNeuron,right_point.listNeuron);

  //  SortSWC(other_point,branch_sort,other_point[0].n,100000);
  //  export_list2file(branch_sort,branch_name,branch_name);
 //   cout<<"right1 = "<<right_point.listNeuron.size()<<endl;
 //   cout<<"right2 = "<<right_point_temp.size()<<endl;
    right_point2.listNeuron = right_point_temp;
    return right_point2;
}

QList<NeuronSWC> match_point(QList<NeuronSWC> &swc1,QList<NeuronSWC> &swc2)
{
    int ind1;
    QList<NeuronSWC> swc3;
    for(V3DLONG i=0;i<swc1.size();i++)
    {
        ind1=0;
        for(V3DLONG j=i+1;j<swc2.size();j++)
        {
            if(NTDIS(swc1[i],swc2[j])<0.00001)
            { ind1=1;//
            }
            continue;
        }
        if(ind1==0)
        {
            swc3.push_back(swc1[i]);
        }
    }
    //cout<<"swc3 size = "<<swc3.size()<<endl;
    return swc3;
}

QList<NeuronSWC> match_point_v2(QList<NeuronSWC> &swc1,QList<NeuronSWC> &swc2)
{
    int ind1;
    QList<NeuronSWC> swc3;
    for(V3DLONG i=0;i<swc1.size();i++)
    {
        ind1=0;
        for(V3DLONG j=0;j<swc2.size();j++)
        {
            if(NTDIS(swc1[i],swc2[j])<0.00001)
            { ind1=1;//
            }
            continue;
        }
        if(ind1==0)
        {
            swc3.push_back(swc1[i]);
        }

    }
    //cout<<"swc3 size = "<<swc3.size()<<endl;
    return swc3;
}

V3DLONG down_child(NeuronTree &nt,vector<vector<V3DLONG> > &childs,V3DLONG node,QList<NeuronSWC> &other_point)
{
    int n;
    //cout<<"node = "<<node<<endl;
    if(childs[node].size() == 1)
    {
        other_point.push_back(nt.listNeuron[childs[node][0]]);
        n = down_child(nt,childs,childs[node][0],other_point);
    }
    else if(childs[node].size() == 0)
    {
        //other_point.push_back(nt.listNeuron[childs[node][0]]);
        n = -2;
    }
    else
    {
        //cout<<"node = "<<node<<endl;
        return node;
    }
    return n;
}

QList<NeuronSWC>choose_point(QList<NeuronSWC> &neuron1,QList<NeuronSWC> &neuron2,int thre1,int thre2)
{
    //vector<V3DLONG> mark;
    V3DLONG ind;
    QList<NeuronSWC> choose;
    choose.clear();
    double min_dis;
    double dis;

    for(V3DLONG i=0;i<neuron1.size();i++)
    {
        min_dis = 10000000;
        for(V3DLONG j=0;j<neuron2.size();j++)
        {
            dis = MHDIS(neuron1[i],neuron2[j]);
            //dis = NTDIS(neuron1[i],neuron2[j]);
            //cout<<"dis = "<<dis<<endl;

            if(dis<min_dis)
            {
                min_dis = dis;
            }

        }

        if((thre1<=min_dis)&&(min_dis<thre2))
        {
            //mark.push_back(i);
            choose.push_back(neuron1[i]);
        }

    }
    return choose;
}


QList<NeuronSWC> choose_alignment(QList<NeuronSWC> &neuron,QList<NeuronSWC> &gold,double thres1,double thres2)
{
    QList<NeuronSWC> result;
    V3DLONG siz = neuron.size();
    double dist;
    for(V3DLONG i=0; i<siz-1;i=i+2)
    {
        dist = MHDIS(neuron[i],neuron[i+1]);
//        dist = sqrt((neuron[i].x-neuron[i+1].x)*(neuron[i].x-neuron[i+1].x)
//                +(neuron[i].y-neuron[i+1].y)*(neuron[i].y-neuron[i+1].y)
//                +(neuron[i].z-neuron[i+1].z)*(neuron[i].z-neuron[i+1].z));
        cout<<"dist = "<<dist<<endl;
        if(dist >= thres1 && dist < thres2)
        {
            result.push_back(neuron[i]);
            gold.push_back(neuron[i+1]);
        }
    }
    return result;
}

bool get_sub_block(V3DPluginCallback2 &callback,QList<NeuronSWC> & neuron,QString filename)
{
    QString inimg_file = callback.getPathTeraFly();

    V3DLONG im_cropped_sz[4];
    NeuronTree nt;
    nt.listNeuron = neuron;

    for(V3DLONG i=0;i<nt.listNeuron.size();)
    {
        NeuronSWC S;
        S.x = nt.listNeuron[i].x;
        S.y = nt.listNeuron[i].y;
        S.z = nt.listNeuron[i].z;

        double l_x = 128;
        double l_y = 128;
        double l_z = 64;
        V3DLONG xb = S.x-l_x;
        V3DLONG xe = S.x+l_x-1;
        V3DLONG yb = S.y-l_y;
        V3DLONG ye = S.y+l_y-1;
        V3DLONG zb = S.z-l_z;
        V3DLONG ze = S.z+l_z-1;


        unsigned char * im_cropped = 0;
        V3DLONG pagesz;

        pagesz = (xe-xb+1)*(ye-yb+1)*(ze-zb+1);
        im_cropped_sz[0] = xe-xb+1;
        im_cropped_sz[1] = ye-yb+1;
        im_cropped_sz[2] = ze-zb+1;
        im_cropped_sz[3] = 1;

        try {im_cropped = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}


        //cout<<"begin = "<<xb<<"  "<<yb<<"  "<<"  "<<zb<<endl;
        //cout<<"end = "<<xe<<"  "<<ye<<"  "<<"  "<<ze<<endl;

        QList<NeuronSWC> outswc;
        for(V3DLONG j=0;j<nt.listNeuron.size();j++)
        {
            NeuronSWC p;
            if(nt.listNeuron[j].x<xe&&nt.listNeuron[j].x>xb&&nt.listNeuron[j].y<ye&&nt.listNeuron[j].y>yb&&nt.listNeuron[j].z<ze&&nt.listNeuron[j].z>zb)
            {
                p.n = nt.listNeuron[j].n;
                p.x = nt.listNeuron[j].x-xb;
                p.y = nt.listNeuron[j].y-yb;
                p.z = nt.listNeuron[j].z-zb;
                p.type = nt.listNeuron[j].type;
                p.r = nt.listNeuron[j].r;
                p.pn = nt.listNeuron[j].pn;
                outswc.push_back(p);
            }
        }

        im_cropped = callback.getSubVolumeTeraFly(inimg_file.toStdString(),xb,xe+1,
                                              yb,ye+1,zb,ze+1);


        QString outimg_file,outswc_file;
        outimg_file = filename + QString::number(i)+".tif";
        outswc_file = filename + QString::number(i)+".swc";
        i=i+50;
        export_list2file(outswc,outswc_file,filename);

        simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
        if(im_cropped) {delete []im_cropped; im_cropped = 0;}

    }
    return true;

}

bool get_subarea_in_nt(vector<FourType> &fourtype,V3DLONG length,QList<NeuronSWC> &little,QList<NeuronSWC> &middle,QList<NeuronSWC> &point_rec,QList<NeuronSWC> &point_gold)
{
    V3DLONG i=0;
    FourType type;
    while(i<little.size())
    {
        type.little.push_back(little[i]);
        i=i+length;
    }
    i=0;
    while(i<middle.size())
    {
        type.middle.push_back(middle[i]);
        i=i+length;
    }
    i=0;
    while(i<point_rec.size())
    {
        type.point_rec.push_back(point_rec[i]);
        i=i+length;
    }
    i=0;
    while(i<point_gold.size())
    {
        type.point_gold.push_back(point_gold[i]);
        i=i+length;
    }
    fourtype.push_back(type);
    return true;
}

bool make_coordinate(vector<vector<Coordinate> > &four_coord,vector<FourType> &four_type,V3DLONG lens)
{
    Coordinate coord;
    vector<Coordinate> vec_coord;
    for(V3DLONG i=0;i<four_type[0].little.size();i++)
    {
        coord.x = four_type[0].little[i].x;
        coord.y = four_type[0].little[i].y;
        coord.z = four_type[0].little[i].z;
        coord.lens = lens;
        vec_coord.push_back(coord);
    }
    four_coord.push_back(vec_coord);
    vec_coord.clear();

    for(V3DLONG i=0;i<four_type[0].middle.size();i++)
    {
        coord.x = four_type[0].middle[i].x;
        coord.y = four_type[0].middle[i].y;
        coord.z = four_type[0].middle[i].z;
        coord.lens = lens;
        vec_coord.push_back(coord);
    }
    four_coord.push_back(vec_coord);
    vec_coord.clear();

    for(V3DLONG i=0;i<four_type[0].point_rec.size();i++)
    {
        coord.x = four_type[0].point_rec[i].x;
        coord.y = four_type[0].point_rec[i].y;
        coord.z = four_type[0].point_rec[i].z;
        coord.lens = lens;
        vec_coord.push_back(coord);
    }
    four_coord.push_back(vec_coord);
    vec_coord.clear();

    for(V3DLONG i=0;i<four_type[0].point_gold.size();i++)
    {
        coord.x = four_type[0].point_gold[i].x;
        coord.y = four_type[0].point_gold[i].y;
        coord.z = four_type[0].point_gold[i].z;
        coord.lens = lens;
        vec_coord.push_back(coord);
    }
    four_coord.push_back(vec_coord);
    vec_coord.clear();
    return true;
}

bool get_subimg_terafly(QString inimg_file,QString name,vector<Coordinate> &mean,V3DPluginCallback2 &callback)
{
    cout<<"enter into get subimg for terafly"<<endl;
    V3DLONG im_cropped_sz[4];
    V3DLONG im_cropped_sz2[4];
    V3DLONG im_cropped_sz3[3];
    V3DLONG pagesz;
    V3DLONG pagesz_2d;
    //V3DLONG M = in_sz[0];
    //V3DLONG N = in_sz[1];
    //V3DLONG P = in_sz[2];
    //V3DLONG sc = in_sz[3];
    V3DLONG sc = 1;
    V3DLONG xe,xb,ye,yb,ze,zb;
    V3DLONG xee,xbb,yee,ybb,zee,zbb;
    unsigned char *im_cropped = 0;
    unsigned char *im_cropped2 = 0;
    unsigned char *im_cropped3 = 0;
    cout<<"img_size = "<<mean.size()<<endl;
    for(V3DLONG i =0;i<mean.size();i++)
    {
        xe=mean[i].x+mean[i].lens;
        xb=mean[i].x-mean[i].lens;
        ye=mean[i].y+mean[i].lens;
        yb=mean[i].y-mean[i].lens;
        ze=mean[i].z+mean[i].lens;
        zb=mean[i].z-mean[i].lens;
//        cout<<"M = "<<M<<endl;
//        if(xb<0) xb = 0;
//        if(xe>=N-1) xe = N-1;
//        if(yb<0) yb = 0;
//        if(ye>=M-1) ye = M-1;
//        if(zb<0) zb = 0;
//        if(ze>=N-1) ze = P-1;


        xee=mean[i].x+100;
        xbb=mean[i].x-100;
        yee=mean[i].y+100;
        ybb=mean[i].y-100;
        zee=mean[i].z+100;
        zbb=mean[i].z-100;


        im_cropped_sz[0] = xe - xb + 1;
        im_cropped_sz[1] = ye - yb + 1;
        im_cropped_sz[2] = ze - zb + 1;
        im_cropped_sz[3] = sc;


        im_cropped_sz2[0] = xee - xbb + 1;
        im_cropped_sz2[1] = yee - ybb + 1;
        im_cropped_sz2[2] = zee - zbb + 1;
        im_cropped_sz2[3] = sc;

        im_cropped_sz3[0]=36;
        im_cropped_sz3[1]=36;
        im_cropped_sz3[2]=sc;



        pagesz = im_cropped_sz[0]* im_cropped_sz[1]* im_cropped_sz[2];
        pagesz_2d = im_cropped_sz3[0]* im_cropped_sz3[1]* im_cropped_sz[2];

        try {im_cropped = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        try {im_cropped2 = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        try {im_cropped3 = new unsigned char [pagesz_2d];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}


        //V3DLONG mysz[4];
        //    mysz[0] = end_x - start_x +1;
        //    mysz[1] = end_y - start_y +1;
        //    mysz[2] = end_z - start_z +1;
        //    mysz[3] = 1;
        //    unsigned char* data1d_crop = 0;
        //V3DLONG pagesz = mysz[0]*mysz[1]*mysz[2];
        cout<<xb<<"   "<<xe<<"   "<<yb<<"   "<<ye<<"   "<<zb<<"   "<<ze<<endl;
        cout<<inimg_file.toStdString()<<endl;
        im_cropped = callback.getSubVolumeTeraFly(inimg_file.toStdString(),xb,xe+1,yb,ye+1,zb,ze+1);

        //im_cropped2 = callback.getSubVolumeTeraFly(inimg_file.toStdString(),xbb,xee+1,ybb,yee+1,zbb,zee+1);
        QString outimg_file;
        QString outimg_file2;
        QString outimg_file3;
        outimg_file = name+QString::number(i)+".tif";
        outimg_file2 = "big_"+QString::number(i)+".tif";

        outimg_file3 = name+QString::number(i)+".txt";

//        int new_lens = 36;
//        QVector<QVector<int> > img_vec;
//        QVector<int> tmp;
//        int l=0;
//        for(V3DLONG i=0;i<new_lens;i++)
//        {
//            for(V3DLONG j=0;j<new_lens;j++)
//            {
//                tmp.push_back(im_cropped[l]);
//                l++;
//            }
//            img_vec.push_back(tmp);
//            tmp.clear();
//        }

        //export_TXT(img_vec,outimg_file3);

       // export_2dtif(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz3,1);

         export_1dtxt(im_cropped,outimg_file3);

       simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
       //simple_saveimage_wrapper(callback, outimg_file2.toStdString().c_str(),(unsigned char *)im_cropped2,im_cropped_sz2,1);
       if(im_cropped) {delete []im_cropped; im_cropped = 0;}

    }
    return true;
}

bool get_subarea(QList<NeuronSWC> &nt,vector<Coordinate> &fourcoord,vector<QList<NeuronSWC> > &subarea)
{
   double min_x;
   double min_y;
   double min_z;
   double max_x;
   double max_y;
   double max_z;
  // QList<NeuronSWC> swc;
    for(V3DLONG i=0; i<nt.size(); i++)
    {
        NeuronSWC curr = nt[i];
        QList<NeuronSWC> sub;

        for(int j=0; j<fourcoord.size(); j++)
        {
            if(min_x<0) min_x = 0;
           // if(max_x>=N-1) max_x = N-1;
            if(min_y<0) min_y = 0;
           // if(max_y>=M-1) max_y= M-1;
            if(min_z<0) min_z = 0;
          //  if(max_z>=N-1) max_z = P-1;

            min_x = fourcoord[i].x-fourcoord[i].lens;
            max_x = fourcoord[i].x+fourcoord[i].lens;
            min_y = fourcoord[i].y-fourcoord[i].lens;
            max_y = fourcoord[i].y+fourcoord[i].lens;
            min_z = fourcoord[i].z-fourcoord[i].lens;
            max_z = fourcoord[i].z+fourcoord[i].lens;
            if(curr.x>min_x&& curr.y>min_y && curr.z>min_z &&curr.x<max_x && curr.y<max_y&&curr.z<max_z)
            {
                sub.push_back(curr);
            }
        }
        subarea.push_back(sub);
    }

    return true;
}

void SplitString(const string& s, vector<string>& v, const string& c)
{
    string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}

void processImage(V3DPluginCallback2 &callback,vector<Coordinate> &fourcoord_each)
{
    cout<<"processimage"<<endl;
    v3dhandle curwin = callback.currentImageWindow();
    if (!curwin)
    {
        QMessageBox::information(0, "", "You don't have any image open in the main window.");
        return;
    }

    Image4DSimple* p4DImage = callback.getImage(curwin);

    if (!p4DImage)
    {
        QMessageBox::information(0, "", "The image pointer is invalid. Ensure your data is valid and try again!");
        return;
    }

    unsigned char* data1d = p4DImage->getRawData();
    QString imgname = callback.getImageName(curwin);
    //V3DLONG totalpxls = p4DImage->getTotalBytes();
    V3DLONG pagesz = p4DImage->getTotalUnitNumberPerChannel();
    cout<<"pagesz = "<<pagesz<<endl;

    V3DLONG N = p4DImage->getXDim();
    V3DLONG M = p4DImage->getYDim();
    V3DLONG P = p4DImage->getZDim();
    V3DLONG sc = p4DImage->getCDim();

    V3DLONG in_sz[4];
    in_sz[0] = N; in_sz[1] = M; in_sz[2] = P; in_sz[3] = sc;

    int tmpx,tmpy,tmpz,x1,y1,z1;
    LandmarkList listLandmarks = callback.getLandmark(curwin);
    //LocationSimple tmpLocation(0,0,0);
    int marknum = fourcoord_each.size();
    if(marknum ==0)
    {
        v3d_msg("No markers in the current image, please double check.");
        return;
    }

    NeuronTree marker_windows;
    QList <NeuronSWC> listNeuron;
    QHash <int, int>  hashNeuron;
    listNeuron.clear();
    hashNeuron.clear();
    int index = 1;
    cout<<"fourcoord_each.size() = "<<fourcoord_each.size()<<endl;
    for (int i=0;i<fourcoord_each.size();i++)
    {
       tmpx= fourcoord_each[i].x;
       cout<<"tmpx = "<<tmpx<<endl;
       tmpy=fourcoord_each[i].y;
       cout<<"tmpy = "<<tmpy<<endl;
       tmpz=fourcoord_each[i].z;
       cout<<"tmpz = "<<tmpz<<endl;
        //tmpLocation = listLandmarks.at(i);
        //tmpLocation.getCoord(tmpx,tmpy,tmpz);
        V3DLONG ix = tmpx-1;
        V3DLONG iy = tmpy-1;
        V3DLONG iz = tmpz-1;
        V3DLONG offsetk = iz*M*N;
        V3DLONG offsetj = iy*N;
        V3DLONG PixelValue;


       PixelValue = data1d[offsetk + offsetj + ix];
       cout<<"PixelValue = "<<PixelValue<<endl;

       // int Ws = 2*(int)round((log(PixelValue)/log(2)));
        int Ws = fourcoord_each[0].lens;
        cout<<"ws = "<<Ws<<endl;
        //int Ws = 2*PixelValue;
        //printf("window size is %d %d (%d %d %d)\n", Ws,PixelValue,ix,iy,iz);
        NeuronSWC S;
        if(Ws>=0)
        {

            S.n     = index;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz-Ws;
            S.r 	= 1;
            S.pn 	= -1;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+1;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz-Ws;
            S.r 	= 1;
            S.pn 	= index;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+2;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz-Ws;
            S.r 	= 1;
            S.pn 	= index+1;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+3;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz-Ws;
            S.r 	= 1;
            S.pn 	= index+2;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+4;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+3;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+5;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+4;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+6;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+5;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+7;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+6;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+8;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+1;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+9;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+2;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+10;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+3;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);


            S.n     = index+11;
            S.type 	= 7;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz-Ws;
            S.r 	= 1;
            S.pn 	= index;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);

            S.n     = index+12;
            S.type 	= 7;
            S.x 	= tmpx+Ws;
            S.y 	= tmpy-Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index+4;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);


            S.n     = index+13;
            S.x 	= tmpx-Ws;
            S.y 	= tmpy+Ws;
            S.z 	= tmpz+Ws;
            S.r 	= 1;
            S.pn 	= index;
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);


             index += 14;

       }
        marker_windows.n = -1;
        marker_windows.on = true;
        marker_windows.listNeuron = listNeuron;
        marker_windows.hashNeuron = hashNeuron;

    }
    QString outfilename = imgname + "_marker.swc";
    if (outfilename.startsWith("http", Qt::CaseInsensitive))
    {
        QFileInfo ii(outfilename);
        outfilename = QDir::home().absolutePath() + "/" + ii.fileName();
    }
    //v3d_msg(QString("The anticipated output file is [%1]").arg(outfilename));
    writeSWC_file(outfilename,marker_windows);
    NeuronTree nt = readSWC_file(outfilename);
    callback.setSWC(curwin, nt);
    callback.open3DWindow(curwin);
    callback.getView3DControl(curwin)->updateWithTriView();
    v3d_msg(QString("You have totally [%1] markers for the file [%2] and the computed MST has been saved to the file [%3]").arg(marknum).arg(imgname).arg(outfilename));

    return;
}

bool export_1dtxt(unsigned char *im_cropped ,QString fileSaveName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;
    QTextStream myfile(&file);


    for (int i=0;i<64*64;i++)
    {
             myfile << im_cropped[i] <<"    ";

    }

    file.close();
    return true;
}

NeuronTree mm2nt(vector<MyMarker*> & inswc, QString fileSaveName)
{
    QString tempSaveName = fileSaveName + "temp.swc";
    saveSWC_file(tempSaveName.toStdString(), inswc);
    NeuronTree nt_out = readSWC_file(tempSaveName);
    //const char * tempRemoveName = tempSaveName.toLatin1().data();
    //const char * tempRemoveName = tempSaveName.toStdString().data();
    //if(remove(tempRemoveName))
    QFile f;
    if(!f.remove(tempSaveName))
    {
        cout << "nt_temp file didn't remove."<< endl;
        perror("remove");
    }
    return nt_out;
}


