#include <v3d_interface.h>
#include "v3d_message.h"
#include "tracing_func.h"
#include "../../../hackathon/zhi/APP2_large_scale/readRawfile_func.h"
#include "../istitch/y_imglib.h"
#include "../neurontracing_vn2/app2/my_surf_objs.h"
#include "../neurontracing_vn2/vn_app2.h"
#include "../neurontracing_vn2/vn_app1.h"

#include "../sort_neuron_swc/sort_swc.h"
#include "../istitch/y_imglib.h"
#include "../resample_swc/resampling.h"
#include "../neuron_image_profiling/profile_swc.h"
#include "../../../v3d_main/jba/c++/convert_type2uint8.h"
//#include "../../../hackathon/zhi/AllenNeuron_postprocessing/sort_swc_IVSCC.h"
#include "../mean_shift_center/mean_shift_fun.h"
#include "../neurontracing_vn2/app1/v3dneuron_gd_tracing.h"
#include "../../../hackathon/zhi/branch_point_detection/branch_pt_detection_func.h"
#include "../neuron_reliability_score/src/topology_analysis.h"

#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))
#define NTDIS2(a,b) (sqrt(0.25*((a).x-(b).x)*((a).x-(b).x)+0.25*((a).y-(b).y)*((a).y-(b).y)+0.25*((a).z-(b).z)*((a).z-(b).z)))
#define NTDOT(a,b) ((a).x*(b).x+(a).y*(b).y+(a).z*(b).z)
#define angle(a,b,c) (acos((((b).x-(a).x)*((c).x-(a).x)+((b).y-(a).y)*((c).y-(a).y)+((b).z-(a).z)*((c).z-(a).z))/(NTDIS(a,b)*NTDIS(a,c)))*180.0/3.14159265359)
#define getParent(n,nt) ((nt).listNeuron.at(n).pn<0)?(1000000000):((nt).hashNeuron.value((nt).listNeuron.at(n).pn))

#if  defined(Q_OS_LINUX)
    #include <omp.h>
#endif

template <class T>
void BinaryProcess(T *apsInput, T * aspOutput, V3DLONG iImageWidth, V3DLONG iImageHeight, V3DLONG iImageLayer, V3DLONG h, V3DLONG d)
{
    V3DLONG i, j,k,n,count;
    double t, temp;

    V3DLONG mCount = iImageHeight * iImageWidth;
    for (i=0; i<iImageLayer; i++)
    {
        for (j=0; j<iImageHeight; j++)
        {
            for (k=0; k<iImageWidth; k++)
            {
                V3DLONG curpos = i * mCount + j*iImageWidth + k;
                V3DLONG curpos1 = i* mCount + j*iImageWidth;
                V3DLONG curpos2 = j* iImageWidth + k;
                temp = 0;
                count = 0;
                for(n =1 ; n <= d  ;n++)
                {
                    if (k>h*n) {temp += apsInput[curpos1 + k-(h*n)]; count++;}
                    if (k+(h*n)< iImageWidth) { temp += apsInput[curpos1 + k+(h*n)]; count++;}
                    if (j>h*n) {temp += apsInput[i* mCount + (j-(h*n))*iImageWidth + k]; count++;}//
                    if (j+(h*n)<iImageHeight) {temp += apsInput[i* mCount + (j+(h*n))*iImageWidth + k]; count++;}//
                    if (i>(h*n)) {temp += apsInput[(i-(h*n))* mCount + curpos2]; count++;}//
                    if (i+(h*n)< iImageLayer) {temp += apsInput[(i+(h*n))* mCount + j* iImageWidth + k ]; count++;}
                }
                t =  apsInput[curpos]-temp/(count);
                aspOutput[curpos]= (t > 0)? t : 0;
            }
        }
    }
}

#include <boost/lexical_cast.hpp>
template <class T> T pow2(T a)
{
    return a*a;

}

QString getAppPath();

using namespace std;


#define getParent(n,nt) ((nt).listNeuron.at(n).pn<0)?(1000000000):((nt).hashNeuron.value((nt).listNeuron.at(n).pn))
bool export_list2file(vector<MyMarker*> & outmarkers, QString fileSaveName, QString fileOpenName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;
    QTextStream myfile(&file);

    QFile qf(fileOpenName);
    if (! qf.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }
    QString info;
    while (! qf.atEnd())
    {
        char _buf[1000], *buf;
        qf.readLine(_buf, sizeof(_buf));
        for (buf=_buf; (*buf && *buf==' '); buf++); //skip space

        if (buf[0]=='\0')	continue;
        if (buf[0]=='#')
        {
           info = buf;
           myfile<< info.remove('\n') <<endl;
        }

    }

    map<MyMarker*, int> ind;
    for(int i = 0; i < outmarkers.size(); i++) ind[outmarkers[i]] = i+1;

    for(V3DLONG i = 0; i < outmarkers.size(); i++)
    {
        MyMarker * marker = outmarkers[i];
        int parent_id;
        if(marker->parent == 0) parent_id = -1;
        else parent_id = ind[marker->parent];
        myfile<<i+1<<" "<<marker->type<<" "<<marker->x<<" "<<marker->y<<" "<<marker->z<<" "<<marker->radius<<" "<<parent_id<<"\n";
    }

    file.close();
    cout<<"swc file "<<fileSaveName.toStdString()<<" has been generated, size: "<<outmarkers.size()<<endl;
    return true;
};

// group images blending function
template <class SDATATYPE>
int region_groupfusing(SDATATYPE *pVImg, Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim, unsigned char *relative1d,
                       V3DLONG vx, V3DLONG vy, V3DLONG vz, V3DLONG vc, V3DLONG rx, V3DLONG ry, V3DLONG rz, V3DLONG rc,
                       V3DLONG tile2vi_zs, V3DLONG tile2vi_ys, V3DLONG tile2vi_xs,
                       V3DLONG z_start, V3DLONG z_end, V3DLONG y_start, V3DLONG y_end, V3DLONG x_start, V3DLONG x_end, V3DLONG *start)
{

    SDATATYPE *prelative = (SDATATYPE *)relative1d;

    if(x_end<x_start || y_end<y_start || z_end<z_start)
        return false;

    // update virtual image pVImg
    V3DLONG offset_volume_v = vx*vy*vz;
    V3DLONG offset_volume_r = rx*ry*rz;

    V3DLONG offset_pagesz_v = vx*vy;
    V3DLONG offset_pagesz_r = rx*ry;

    for(V3DLONG c=0; c<rc; c++)
    {
        V3DLONG o_c = c*offset_volume_v;
        V3DLONG o_r_c = c*offset_volume_r;
        for(V3DLONG k=z_start; k<z_end; k++)
        {
            V3DLONG o_k = o_c + (k-start[2])*offset_pagesz_v;
            V3DLONG o_r_k = o_r_c + (k-tile2vi_zs)*offset_pagesz_r;

            for(V3DLONG j=y_start; j<y_end; j++)
            {
                V3DLONG o_j = o_k + (j-start[1])*vx;
                V3DLONG o_r_j = o_r_k + (j-tile2vi_ys)*rx;

                for(V3DLONG i=x_start; i<x_end; i++)
                {
                    V3DLONG idx = o_j + i-start[0];
                    V3DLONG idx_r = o_r_j + (i-tile2vi_xs);

                    if(pVImg[idx])
                    {
                        pVImg[idx] = 0.5*(pVImg[idx] + prelative[idx_r]); // Avg. Intensity
                    }
                    else
                    {
                        pVImg[idx] = prelative[idx_r];
                    }
                }
            }
        }
    }

    return true;
}



bool crawler_raw_app(V3DPluginCallback2 &callback, QWidget *parent,TRACE_LS_PARA &P,bool bmenu)
{
    QElapsedTimer timer1;
    timer1.start();

    TRACE_LS_PARA P_all;
    P_all.listLandmarks.clear();
    P.listLandmarks.clear();

    QString fileOpenName = P.inimg_file;
    if(P.image)
    {
        P.in_sz[0] = P.image->getXDim();
        P.in_sz[1] = P.image->getYDim();
        P.in_sz[2] = P.image->getZDim();
    }else
    {
        if(fileOpenName.endsWith(".tc",Qt::CaseSensitive))
        {
            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            P.in_sz[0] = vim.sz[0];
            P.in_sz[1] = vim.sz[1];
            P.in_sz[2] = vim.sz[2];

        }else if (fileOpenName.endsWith(".raw",Qt::CaseSensitive) || fileOpenName.endsWith(".v3draw",Qt::CaseSensitive))
        {
            unsigned char * datald = 0;
            V3DLONG *in_zz = 0;
            V3DLONG *in_sz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), datald, in_zz, in_sz,datatype,0,0,0,1,1,1))
            {
                return false;
            }
            if(datald) {delete []datald; datald = 0;}
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }else
        {
            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(fileOpenName.toStdString(),in_zz))
            {
                return false;
            }
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }

        LocationSimple t;
        if(P.markerfilename.endsWith(".marker",Qt::CaseSensitive))
        {
            vector<MyMarker> file_inmarkers;
            file_inmarkers = readMarker_file(string(qPrintable(P.markerfilename)));
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x + 1;
                t.y = file_inmarkers[i].y + 1;
                t.z = file_inmarkers[i].z + 1;
                P.listLandmarks.push_back(t);

                //add by yongzhang
                cout<<"P.block_size = "<<P.block_size<<"     file_inmarkers[0].radius = "<<file_inmarkers[0].radius<<endl;
                if(file_inmarkers[0].radius > P.block_size)
                    P.block_size = P.block_size;
                else
                    P.block_size = file_inmarkers[0].radius;
                //endl;
            }
        }else
        {
            QList<CellAPO> file_inmarkers;
            file_inmarkers = readAPO_file(P.markerfilename);
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x;
                t.y = file_inmarkers[i].y;
                t.z = file_inmarkers[i].z;
                P.listLandmarks.push_back(t);
            }
        }

    }
    //add by yongzhang -- load all soma
    LocationSimple t_all;
    if(P.allmarkerfilename.endsWith(".marker",Qt::CaseSensitive))
    {
        vector<MyMarker> file_allinmarkers;
        file_allinmarkers = readMarker_file(string(qPrintable(P.allmarkerfilename)));
        for(int i = 0; i < file_allinmarkers.size(); i++)
        {
            t_all.x = file_allinmarkers[i].x + 1;
            t_all.y = file_allinmarkers[i].y + 1;
            t_all.z = file_allinmarkers[i].z + 1;
            P_all.listLandmarks.push_back(t_all);
        }
    }
    //end



    LandmarkList allTargetList;
    QList<LandmarkList> allTipsList;

    LocationSimple tileLocation;
    tileLocation.x = P.listLandmarks[0].x;
    tileLocation.y = P.listLandmarks[0].y;
    tileLocation.z = P.listLandmarks[0].z;

    LandmarkList inputRootList;
    if(P.method != gd )inputRootList.push_back(tileLocation);

    allTipsList.push_back(inputRootList);

    tileLocation.x = tileLocation.x -int(P.block_size/2);
    tileLocation.y = tileLocation.y -int(P.block_size/2);
    if(P.tracing_3D)
        tileLocation.z = tileLocation.z -int(P.block_size/8);
    else
        tileLocation.z = 0;

    tileLocation.ev_pc1 = P.block_size;
    tileLocation.ev_pc2 = P.block_size;
    tileLocation.ev_pc3 = P.block_size/4;

//    P.block_size = P.block_size/2; //original

    //change by yongzhang
    if(P.block_size == 128)
        P.block_size = P.block_size;
    else
        P.block_size = P.block_size/2;
    //end

    tileLocation.category = 1;
    allTargetList.push_back(tileLocation);


    P.output_folder = QFileInfo(P.markerfilename).path();
    QString tmpfolder;
    if(P.tracing_comb)
        tmpfolder= P.output_folder+QString("/x_%1_y_%2_z_%3_tmp_COMBINED").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
    else
    {
        if(P.method == app1)
            tmpfolder= P.output_folder+("/tmp_APP1");
        else if (P.method == app2)
        {
            tmpfolder= P.markerfilename+QString("_tmp_APP2");

        }
        else
            tmpfolder= P.output_folder+QString("/x_%1_y_%2_z_%3_tmp_GD_Curveline").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
    }

    if(!tmpfolder.isEmpty())
       system(qPrintable(QString("rm -rf %1").arg(tmpfolder.toStdString().c_str())));

    system(qPrintable(QString("mkdir %1").arg(tmpfolder.toStdString().c_str())));
    if(tmpfolder.isEmpty())
    {
        printf("Can not create a tmp folder!\n");
        return false;
    }

    QString finaloutputswc;
    if(P.global_name)
        finaloutputswc = P.markerfilename+QString("_nc_APP2_GD.swc");
    if(QFileInfo(finaloutputswc).exists())
        system(qPrintable(QString("rm %1").arg(finaloutputswc.toStdString().c_str())));

    LandmarkList newTargetList;
    QList<LandmarkList> newTipsList;
    bool flag = true;
    while(allTargetList.size()>0)
    {
        newTargetList.clear();
        newTipsList.clear();
        if(P.tracing_comb)
        {
            P.seed_win = 5;
            P.slip_win = 5;
            if(P.tracing_3D)
                combo_tracing_ada_win_3D(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
            else
                combo_tracing_ada_win(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
        }
        else
        {
            if(P.adap_win)
            {
                if(P.tracing_3D)
                    app_tracing_ada_win_3D(callback,P,P_all,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
                else
                    app_tracing_ada_win(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
            }
            else
            {
                app_tracing(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
            }
        }
        allTipsList.removeAt(0);
        allTargetList.removeAt(0);
        if(newTipsList.size()>0)
        {
            for(int i = 0; i < newTipsList.size(); i++)
            {
                allTargetList.push_back(newTargetList.at(i));
                allTipsList.push_back(newTipsList.at(i));
            }

            //add by yongzhang
//            cout<<"allTargetList size : "<<allTargetList.size()<<endl;
//            double dist,dist1;
//            for(int i = 0;i < allTargetList.size();i++)
//            {
//                cout<<"allTargetList_x = "<<allTargetList.at(i).x<<"    allTargetList_y = "<<allTargetList.at(i).y<<"    allTargetList_z = "<<allTargetList.at(i).z<<endl;
//                cout<<"P.listLandmarks size: "<<P.listLandmarks.size()<<endl;

//                dist = NTDIS2(allTargetList.at(i),P.listLandmarks[0]);
//                cout<<"dist = "<<dist<<endl;
//                double min_dist = 1000000000;
//                int k = -1;
//                for(int j = 0;j < P_all.listLandmarks.size();j++)
//                {
//                    if(P_all.listLandmarks.at(j).x == P.listLandmarks.at(0).x && P_all.listLandmarks.at(j).y == P.listLandmarks.at(0).y && P_all.listLandmarks.at(j).z == P.listLandmarks.at(0).z)
//                        continue;
//                    dist1 = NTDIS2(allTargetList.at(i),P_all.listLandmarks[j]);
//                    cout<<"dist1 = "<<dist1<<endl;
//                    if(dist1 < min_dist)
//                    {
//                        min_dist = dist1;
//                        k = j;
//                    }
//                }
//                cout<<"k_x = "<<P_all.listLandmarks.at(k).x<<"   k_y = "<<P_all.listLandmarks.at(k).y<<"    k_z = "<<P_all.listLandmarks.at(k).z<<endl;
//                cout<<"min_dist = "<<min_dist<<endl;
//                cout<<" dist thres = "<<1.73*P.block_size<<endl;

//                if(min_dist < 1.73*P.block_size)
//                {
//                    cout<<"tip_x = "<<allTargetList.at(i).x<<"   tip_y = "<<allTargetList.at(i).y<<"    tip_z = "<<allTargetList.at(i).z<<endl;
//                    //allTipsList.removeAt(i);
//                    allTargetList.removeAt(i);
//                }
//            }
            cout<<"allTargetList result size : "<<allTargetList.size()<<endl;
            //v3d_msg("stop");
            //end

            for(int i = 0; i < allTargetList.size();i++)
            {
                for(int j = 0; j < allTargetList.size();j++)
                {
                    //cout<<"radius "<<i<<" = "<<allTargetList.at(i).radius<<"    radius "<<j<<" = "<<allTargetList.at(j).radius<<endl;
                    //cout<<"value "<<i<<" = "<<allTargetList.at(i).value<<"    value "<<j<<" = "<<allTargetList.at(j).value<<endl;
                    if(allTargetList.at(i).radius > allTargetList.at(j).radius)
                    {
                        allTargetList.swap(i,j);
                        allTipsList.swap(i,j);
                        //v3d_msg("checkout!");
                    }
                }
            }
        }
    }

//    qint64 etime1 = timer1.elapsed();

//    list<string> infostring;
//    string tmpstr; QString qtstr;
//    if(P.method==app1)
//    {
//        tmpstr =  qPrintable( qtstr.prepend("## UT_APP1")); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.channel).prepend("#channel = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.bkg_thresh).prepend("#bkg_thresh = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.b_256cube).prepend("#b_256cube = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.visible_thresh).prepend("#visible_thresh = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.block_size).prepend("#block_size = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.adap_win).prepend("#adaptive_window = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(etime1).prepend("#neuron preprocessing time (milliseconds) = ") ); infostring.push_back(tmpstr);

//    }
//    else if(P.method==app2)
//    {
//        tmpstr =  qPrintable( qtstr.prepend("## UT_APP2")); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.channel).prepend("#channel = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.bkg_thresh).prepend("#bkg_thresh = ") ); infostring.push_back(tmpstr);

//        tmpstr =  qPrintable( qtstr.setNum(P.length_thresh).prepend("#length_thresh = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.SR_ratio).prepend("#SR_ratio = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.is_gsdt).prepend("#is_gsdt = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.is_break_accept).prepend("#is_gap = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.cnn_type).prepend("#cnn_type = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.b_256cube).prepend("#b_256cube = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.b_RadiusFrom2D).prepend("#b_radiusFrom2D = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.block_size).prepend("#block_size = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(P.adap_win).prepend("#adaptive_window = ") ); infostring.push_back(tmpstr);
//        tmpstr =  qPrintable( qtstr.setNum(etime1).prepend("#neuron preprocessing time (milliseconds) = ") ); infostring.push_back(tmpstr);
//    }

//    if(P.method != gd)
//    {
//        if(P.tracing_3D)
//            processSmartScan_3D(callback,infostring,tmpfolder +"/scanData.txt");
//        else
//            processSmartScan(callback,infostring,tmpfolder +"/scanData.txt");
//    }else
//        processSmartScan_3D_wofuison(callback,infostring,tmpfolder +"/scanData.txt");




//    v3d_msg(QString("The tracing uses %1 for tracing. Now you can drag and drop the generated swc fle [%2] into Vaa3D."
//                    ).arg(etime1).arg(tmpfolder +"/scanData.txt.swc"), bmenu);

    return true;
}

bool app_tracing(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString;
    if(P.method == app1)
        saveDirString = P.output_folder.append("/tmp_APP1");
    else
       // saveDirString = P.output_folder.append("/tmp_COMBINED");
        saveDirString = P.output_folder+ QString("/x_%1_y_%2_z_%3_tmp_APP2").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,end_x,end_y;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;

    end_x = tileLocation.x+P.block_size;
    end_y = tileLocation.y+P.block_size;
    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || end_x <= 0 || end_y <= 0 )
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = P.in_sz[2];
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,tileLocation.z,end_x-1,end_y-1,tileLocation.z + P.in_sz[2]-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }

        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,tileLocation.z,
                               end_x,end_y,tileLocation.z + P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }
            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,0,P.in_sz[2]);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(tileLocation.z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");
    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".swc");

    PARA_APP1 p1;
    PARA_APP2 p2;
    QString versionStr = "v0.001";

    if(P.method == app1)
    {
        p1.bkg_thresh = P.bkg_thresh;
        p1.channel = P.channel-1;
        p1.b_256cube = P.b_256cube;
        p1.visible_thresh = P.visible_thresh;

        p1.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p1.p4dImage = total4DImage;
        p1.xc0 = p1.yc0 = p1.zc0 = 0;
        p1.xc1 = p1.p4dImage->getXDim()-1;
        p1.yc1 = p1.p4dImage->getYDim()-1;
        p1.zc1 = p1.p4dImage->getZDim()-1;
    }
    else
    {
        p2.is_gsdt = P.is_gsdt;
        p2.is_coverage_prune = true;
        p2.is_break_accept = P.is_break_accept;
        p2.bkg_thresh = -1;//P.bkg_thresh;
        p2.length_thresh = P.length_thresh;
        p2.cnn_type = 2;
        p2.channel = 0;
        p2.SR_ratio = 3.0/9.9;
        p2.b_256cube = P.b_256cube;
        p2.b_RadiusFrom2D = P.b_RadiusFrom2D;
        p2.b_resample = 1;
        p2.b_intensity = 0;
        p2.b_brightfiled = 0;
        p2.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p2.p4dImage = total4DImage;
        p2.xc0 = p2.yc0 = p2.zc0 = 0;
        p2.xc1 = p2.p4dImage->getXDim()-1;
        p2.yc1 = p2.p4dImage->getYDim()-1;
        p2.zc1 = p2.p4dImage->getZDim()-1;
    }

    NeuronTree nt;

    ifstream ifs_image(imageSaveString.toStdString().c_str());
    if(!ifs_image)
    {
        qDebug()<<scanDataFileString;
        QFile saveTextFile;
        saveTextFile.setFileName(scanDataFileString);// add currentScanFile
        if (!saveTextFile.isOpen()){
            if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
                qDebug()<<"unable to save file!";
                return false;}     }
        QTextStream outputStream;
        outputStream.setDevice(&saveTextFile);
        outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<"\n";
        saveTextFile.close();

        simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

        if(P.method == app1)
            qDebug()<<"starting app1";
        else
            qDebug()<<"starting app2";
        qDebug()<<"rootlist size "<<QString::number(inputRootList.size());

        if(inputRootList.size() <1)
        {
            if(P.method == app1)
            {
                p1.outswc_file =swcString;
                proc_app1(callback, p1, versionStr);
            }
            else
            {
                p2.outswc_file =swcString;
                proc_app2_wp(callback, p2, versionStr);
            }
        }
        else
        {
            vector<MyMarker*> tileswc_file;
            for(int i = 0; i < inputRootList.size(); i++)
            {
                QString poutswc_file = swcString + (QString::number(i)) + (".swc");
                if(P.method == app1)
                    p1.outswc_file =poutswc_file;
                else
                    p2.outswc_file =poutswc_file;

                LocationSimple RootNewLocation;
                RootNewLocation.x = inputRootList.at(i).x - total4DImage->getOriginX();
                RootNewLocation.y = inputRootList.at(i).y - total4DImage->getOriginY();
                RootNewLocation.z = inputRootList.at(i).z - total4DImage->getOriginZ();

                bool flag = false;
                if(tileswc_file.size()>0)
                {
                    for(V3DLONG dd = 0; dd < tileswc_file.size();dd++)
                    {
                        double dis = sqrt(pow2(RootNewLocation.x - tileswc_file.at(dd)->x) + pow2(RootNewLocation.y - tileswc_file.at(dd)->y) + pow2(RootNewLocation.z - tileswc_file.at(dd)->z));
                        if(dis < 10.0)
                        {
                           flag = true;
                           break;
                        }
                    }
                }

                if(!flag)
                {
                    if(P.method == app1)
                    {
                        p1.landmarks.push_back(RootNewLocation);
                        proc_app1(callback, p1, versionStr);
                        p1.landmarks.clear();
                    }else
                    {
                        p2.landmarks.push_back(RootNewLocation);
                        proc_app2_wp(callback, p2, versionStr);
                        p2.landmarks.clear();
                    }

                    vector<MyMarker*> inputswc = readSWC_file(poutswc_file.toStdString());
                    for(V3DLONG d = 0; d < inputswc.size(); d++)
                    {
                        tileswc_file.push_back(inputswc[d]);
                    }
                }
            }
            saveSWC_file(swcString.toStdString().c_str(), tileswc_file);
            nt = readSWC_file(swcString);
        }

    }else
    {
        NeuronTree nt_tile = readSWC_file(swcString);
        LandmarkList inputRootList_pruned = eliminate_seed(nt_tile,inputRootList,total4DImage);
        if(inputRootList_pruned.size()<1)
            return true;
        else
        {
            vector<MyMarker*> tileswc_file;
            QString swcString_2nd = swcString + ("_2.swc");
            for(int i = 0; i < inputRootList_pruned.size(); i++)
            {
                QString poutswc_file = swcString + (QString::number(i)) + ("_2.swc");
                if(P.method == app1)
                    p1.outswc_file = poutswc_file;
                else
                    p2.outswc_file = poutswc_file;

                LocationSimple RootNewLocation;
                RootNewLocation.x = inputRootList_pruned.at(i).x - total4DImage->getOriginX();
                RootNewLocation.y = inputRootList_pruned.at(i).y - total4DImage->getOriginY();
                RootNewLocation.z = inputRootList_pruned.at(i).z - total4DImage->getOriginZ();

                bool flag = false;
                if(tileswc_file.size()>0)
                {
                    for(V3DLONG dd = 0; dd < tileswc_file.size();dd++)
                    {
                        double dis = sqrt(pow2(RootNewLocation.x - tileswc_file.at(dd)->x) + pow2(RootNewLocation.y - tileswc_file.at(dd)->y) + pow2(RootNewLocation.z - tileswc_file.at(dd)->z));
                        if(dis < 10.0)
                        {
                           flag = true;
                           break;
                        }
                    }
                }

                if(!flag)
                {
                    if(P.method == app1)
                    {
                        p1.landmarks.push_back(RootNewLocation);
                        proc_app1(callback, p1, versionStr);
                        p1.landmarks.clear();
                    }else
                    {
                        p2.landmarks.push_back(RootNewLocation);
                        proc_app2_wp(callback, p2, versionStr);
                        p2.landmarks.clear();
                    }
                    vector<MyMarker*> inputswc = readSWC_file(poutswc_file.toStdString());
                    qDebug()<<"ran app2";
                    for(V3DLONG d = 0; d < inputswc.size(); d++)
                    {
                        tileswc_file.push_back(inputswc[d]);
                    }
                }
            }
            vector<MyMarker*> tileswc_file_total = readSWC_file(swcString.toStdString());
            for(V3DLONG d = 0; d < tileswc_file.size(); d++)
            {
                tileswc_file_total.push_back(tileswc_file[d]);
            }

            saveSWC_file(swcString.toStdString().c_str(), tileswc_file_total);
            saveSWC_file(swcString_2nd.toStdString().c_str(), tileswc_file);
            nt = readSWC_file(swcString_2nd);
        }

    }

    QVector<QVector<V3DLONG> > childs;
    V3DLONG neuronNum = nt.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    QList<NeuronSWC> list = nt.listNeuron;
    for (V3DLONG i=0;i<list.size();i++)
    {
        if (childs[i].size()==0)
        {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                V3DLONG node_pn = getParent(i,nt);
                V3DLONG node_pn_2nd;
                if( list.at(node_pn).pn < 0)
                {
                    node_pn_2nd = node_pn;
                }
                else
                {
                    node_pn_2nd = getParent(node_pn,nt);
                }

                newTip.x = list.at(node_pn_2nd).x + total4DImage->getOriginX();
                newTip.y = list.at(node_pn_2nd).y + total4DImage->getOriginY();
                newTip.z = list.at(node_pn_2nd).z + total4DImage->getOriginZ();
            }
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }
        }
    }
    double overlap = 0.1;
    LocationSimple newTarget;
    if(tip_left.size()>0)
    {
        newTipsList->push_back(tip_left);
        newTarget.x = -floor(P.block_size*(1.0-overlap)) + tileLocation.x;
        newTarget.y = tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTarget.category = tileLocation.category + 1;
        newTargetList->push_back(newTarget);
    }
    if(tip_right.size()>0)
    {
        newTipsList->push_back(tip_right);
        newTarget.x = floor(P.block_size*(1.0-overlap)) + tileLocation.x;
        newTarget.y = tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTarget.category = tileLocation.category + 1;
        newTargetList->push_back(newTarget);
    }
    if(tip_up.size()>0)
    {
        newTipsList->push_back(tip_up);
        newTarget.x = tileLocation.x;
        newTarget.y = -floor(P.block_size*(1.0-overlap)) + tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTarget.category = tileLocation.category + 1;
        newTargetList->push_back(newTarget);
    }
    if(tip_down.size()>0)
    {
        newTipsList->push_back(tip_down);
        newTarget.x = tileLocation.x;
        newTarget.y = floor(P.block_size*(1.0-overlap)) + tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTarget.category = tileLocation.category + 1;
        newTargetList->push_back(newTarget);
    }
    total4DImage->deleteRawDataAndSetPointerToNull();

    return true;
}

bool app_tracing_ada_win(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{
    QString saveDirString;
    QString finaloutputswc;

    if(P.method == app1)
    {
        saveDirString = P.output_folder.append("/tmp_APP1");
        finaloutputswc = P.inimg_file + ("_nc_app1_adp.swc");
    }
    else
    {

        //   saveDirString = P.output_folder.append("/tmp_COMBINED");
        saveDirString = P.output_folder+ QString("/x_%1_y_%2_z_%3_tmp_APP2").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
        finaloutputswc = P.inimg_file + ("_nc_app2_adp.swc");
    }


    if(P.global_name)
        finaloutputswc = P.inimg_file + QString("_nc_APP2_GD.swc");

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,end_x,end_y;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;

    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || end_x <= 0 || end_y <= 0 )
    {
        printf("hit the boundary");
        return true;
    }

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));
//    ifstream ifs(scanDataFileString.toLatin1());
//    int offsetX, offsetY,sizeX,sizeY;
//    string swcfilepath;
//    string info_swc;

//    bool scanned = false;
//    V3DLONG start_x_updated,end_x_updated,start_y_updated,end_y_updated;
//    double overlap = 0.1;

//    if(tileLocation.ev_pc3 == 1)
//    {
//        start_x_updated = start_x;
//        end_x_updated = start_x +  floor(tileLocation.ev_pc1*(1.0-overlap) - 1);
//        start_y_updated =  start_y;
//        end_y_updated = end_y - 1;
//    }else if(tileLocation.ev_pc3 == 2)
//    {
//        start_x_updated = start_x +  floor(tileLocation.ev_pc1*(1.0-overlap));
//        end_x_updated = end_x - 1;
//        start_y_updated =  start_y;
//        end_y_updated = end_y - 1;

//    }else if(tileLocation.ev_pc3 == 3)
//    {
//        start_x_updated = start_x;
//        end_x_updated = end_x - 1;
//        start_y_updated =  start_y;
//        end_y_updated = start_y +  floor(tileLocation.ev_pc2*(1.0-overlap) - 1);

//    }else if(tileLocation.ev_pc3 == 4)
//    {
//        start_x_updated = start_x;
//        end_x_updated = end_x - 1;
//        start_y_updated =  start_y +  floor(tileLocation.ev_pc2*(1.0-overlap));
//        end_y_updated = end_y - 1;
//    }

//    int check_lu = 0,check_ru = 0,check_ld = 0,check_rd = 0;
//    while(ifs && getline(ifs, info_swc))
//    {
//        std::istringstream iss(info_swc);
//        iss >> offsetX >> offsetY >> swcfilepath >>sizeX >> sizeY;
//        int check1 = (start_x_updated >= offsetX && start_x_updated <= offsetX+sizeX -1)?  1 : 0;
//        int check2 = (end_x_updated >= offsetX && end_x_updated <= offsetX+sizeX - 1)?  1 : 0;
//        int check3 = (start_y_updated >= offsetY && start_y_updated <= offsetY+sizeY - 1)?  1 : 0;
//        int check4 = (end_y_updated >= offsetY && end_y_updated <= offsetY+sizeY- 1)?  1 : 0;

//        if(!check_lu && check1*check3) check_lu = 1;
//        if(!check_ru && check2*check3) check_ru = 1;
//        if(!check_ld && check1*check4) check_ld = 1;
//        if(!check_rd && check2*check4) check_rd = 1;
//    }
//    if(check_lu*check_ru*check_ld*check_rd)
//    {
//        printf("skip the scanned area");
//        return true;
//    }


    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = P.in_sz[2];
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,tileLocation.z,end_x-1,end_y-1,tileLocation.z + P.in_sz[2]-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,tileLocation.z,
                               end_x,end_y,tileLocation.z + P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }
            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;

            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,0,P.in_sz[2]);
            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }

    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(tileLocation.z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));
    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".swc");

    PARA_APP1 p1;
    PARA_APP2 p2;
    QString versionStr = "v0.001";

    if(P.method == app1)
    {
        p1.bkg_thresh = P.bkg_thresh;
        p1.channel = P.channel-1;
        p1.b_256cube = P.b_256cube;
        p1.visible_thresh = P.visible_thresh;

        p1.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p1.p4dImage = total4DImage;
        p1.xc0 = p1.yc0 = p1.zc0 = 0;
        p1.xc1 = p1.p4dImage->getXDim()-1;
        p1.yc1 = p1.p4dImage->getYDim()-1;
        p1.zc1 = p1.p4dImage->getZDim()-1;
    }
    else
    {
        p2.is_gsdt = P.is_gsdt;
        p2.is_coverage_prune = true;
        p2.is_break_accept = P.is_break_accept;
        p2.bkg_thresh = P.bkg_thresh;
        p2.length_thresh = P.length_thresh;
        p2.cnn_type = 2;
        p2.channel = 0;
        p2.SR_ratio = 3.0/9.9;
        p2.b_256cube = P.b_256cube;
        p2.b_RadiusFrom2D = P.b_RadiusFrom2D;
        p2.b_resample = 1;
        p2.b_intensity = 0;
        p2.b_brightfiled = 0;
        p2.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p2.p4dImage = total4DImage;
        p2.p4dImage->setFileName(imageSaveString.toStdString().c_str());
        p2.xc0 = p2.yc0 = p2.zc0 = 0;
        p2.xc1 = p2.p4dImage->getXDim()-1;
        p2.yc1 = p2.p4dImage->getYDim()-1;
        p2.zc1 = p2.p4dImage->getZDim()-1;
    }

    NeuronTree nt;
    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

    //v3d_msg(QString("%1,%2,%3,%4,%5").arg(start_x_updated).arg(end_x_updated).arg(start_y_updated).arg(end_y_updated).arg(tileLocation.ev_pc3));

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    vector<MyMarker*> finalswc;

    if(ifs_swc)
       finalswc = readSWC_file(finaloutputswc.toStdString());

    if(P.method == app1)
        qDebug()<<"starting app1";
    else
        qDebug()<<"starting app2";
    qDebug()<<"rootlist size "<<QString::number(inputRootList.size());

    vector<MyMarker*> tileswc_file;

    if(inputRootList.size() <1)
    {
        if(P.method == app1)
        {
            p1.outswc_file =swcString;
            proc_app1(callback, p1, versionStr);
        }
        else
        {
            p2.outswc_file =swcString;
            proc_app2_wp(callback, p2, versionStr);
        }
    }
    else
    {
        for(int i = 0; i < inputRootList.size(); i++)
        {
            QString poutswc_file = swcString + (QString::number(i)) + (".swc");
            if(P.method == app1)
                p1.outswc_file =poutswc_file;
            else
                p2.outswc_file =poutswc_file;

            bool flag = false;
            LocationSimple RootNewLocation;
            RootNewLocation.x = inputRootList.at(i).x - total4DImage->getOriginX();
            RootNewLocation.y = inputRootList.at(i).y - total4DImage->getOriginY();
            RootNewLocation.z = inputRootList.at(i).z - total4DImage->getOriginZ();

            const float dd = 0.5;

            if(P.method == app1 && (RootNewLocation.x<p1.xc0-dd || RootNewLocation.x>p1.xc1+dd || RootNewLocation.y<p1.yc0-dd || RootNewLocation.y>p1.yc1+dd || RootNewLocation.z<p1.zc0-dd || RootNewLocation.z>p1.zc1+dd))
                continue;

            if(tileswc_file.size()>0)
            {
                for(V3DLONG dd = 0; dd < tileswc_file.size();dd++)
                {
                    double dis = sqrt(pow2(RootNewLocation.x - tileswc_file.at(dd)->x) + pow2(RootNewLocation.y - tileswc_file.at(dd)->y) + pow2(RootNewLocation.z - tileswc_file.at(dd)->z));
                    if(dis < 10.0)
                    {
                        flag = true;
                        break;
                    }
                }
            }

            if(!flag)
            {
                if(P.method == app1)
                {

                    p1.landmarks.push_back(RootNewLocation);
                    proc_app1(callback, p1, versionStr);
                    p1.landmarks.clear();
                }else
                {
                    p2.landmarks.push_back(RootNewLocation);
                    proc_app2_wp(callback, p2, versionStr);
                    p2.landmarks.clear();
                }

                vector<MyMarker*> inputswc = readSWC_file(poutswc_file.toStdString());

                for(V3DLONG d = 0; d < inputswc.size(); d++)
                {
                    tileswc_file.push_back(inputswc[d]);
                }
            }
        }
        saveSWC_file(swcString.toStdString().c_str(), tileswc_file);
        nt = readSWC_file(swcString);
    }

    QVector<QVector<V3DLONG> > childs;
    V3DLONG neuronNum = nt.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    QList<NeuronSWC> list = nt.listNeuron;

    for (V3DLONG i=0;i<list.size();i++)
    {
        if (childs[i].size()==0)
        {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            bool check_tip = false;
            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                V3DLONG node_pn = getParent(i,nt);
                V3DLONG node_pn_2nd;
                if( list.at(node_pn).pn < 0)
                {
                    node_pn_2nd = node_pn;
                }
                else
                {
                    node_pn_2nd = getParent(node_pn,nt);
                }

                newTip.x = list.at(node_pn_2nd).x + total4DImage->getOriginX();
                newTip.y = list.at(node_pn_2nd).y + total4DImage->getOriginY();
                newTip.z = list.at(node_pn_2nd).z + total4DImage->getOriginZ();
                newTip.radius = list.at(node_pn_2nd).r;

                for(V3DLONG j = 0; j < finalswc.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.at(j)->x) + pow2(newTip.y - finalswc.at(j)->y) + pow2(newTip.z - finalswc.at(j)->z));
                   // if(dis < 2*finalswc.at(j)->radius || dis < 20)
                    if(dis < 10)
                    {
                        check_tip = true;
                        break;
                    }
                }
            }
            if(check_tip) continue;
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }
        }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,256,1);
        for(int i = 0; i < group_tips_left.size();i++)
            ada_win_finding(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,256,2);
        for(int i = 0; i < group_tips_right.size();i++)
            ada_win_finding(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
    }
    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,256,3);
        for(int i = 0; i < group_tips_up.size();i++)
            ada_win_finding(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);

    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,256,4);
        for(int i = 0; i < group_tips_down.size();i++)
            ada_win_finding(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
    }

    if(ifs_swc)
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }

    total4DImage->deleteRawDataAndSetPointerToNull();

    return true;
}

bool app_tracing_ada_win_3D(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,TRACE_LS_PARA &P_all,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{
    QString saveDirString;
    QString finaloutputswc;

    if(P.method == app1)
    {
        saveDirString = P.output_folder.append("/tmp_APP1");
        finaloutputswc = P.markerfilename + ("_nc_app1_adp.swc");
    }
    else if(P.method == app2)
    {
       // saveDirString = P.output_folder.append("/tmp_COMBINED");
   //     saveDirString = P.output_folder+ QString("/x_%1_y_%2_z_%3_tmp_APP2").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
          saveDirString = P.markerfilename+ QString("_tmp_APP2");
        finaloutputswc = P.markerfilename + QString("x_%1_y_%2_z_%3_nc_app2_adp_3D.swc").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
    }
    else if(P.method == gd)
    {
        saveDirString = P.output_folder+ QString("/x_%1_y_%2_z_%3_tmp_GD_Curveline").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
        finaloutputswc = P.markerfilename + QString("x_%1_y_%2_z_%3_nc_GD_Curveline_adp_3D.swc").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
    }

    if(P.global_name)
        finaloutputswc = P.markerfilename+QString("_nc_APP2_GD.swc");

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,start_z,end_x,end_y,end_z;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;
    start_z = (tileLocation.z < 0)?  0 : tileLocation.z;


    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    end_z = tileLocation.z+tileLocation.ev_pc3;

    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];
    if(end_z > P.in_sz[2]) end_z = P.in_sz[2];

//    cout<<"start_x = "<<start_x<<"    start_y = "<<start_y<<"    start_z = "<<start_z<<endl;
//    cout<<"end_x = "<<end_x<<"    end_y = "<<end_y<<"    end_z = "<<end_z<<endl;
//    cout<<"tileLocation.x = "<<tileLocation.x<<"   tileLocation.y = "<<tileLocation.y<<"    tileLocation.z = "<<tileLocation.z<<endl;
//    cout<<"tileLocation.ev_pc1 = "<<tileLocation.ev_pc1<<"   tileLocation.ev_pc2 = "<<tileLocation.ev_pc2<<"    tileLocation.ev_pc3 = "<<tileLocation.ev_pc3<<endl;

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    //add by yongzhang
    double  thres = 0;
    double  n_dist = 0.6;
    if(ifs_swc)
    {
        cout<<"Determine whether to remove the tip point"<<endl;
        for(int i = 0;i < P_all.listLandmarks.size();i++)
        {
            if(P_all.listLandmarks.at(i).x == P.listLandmarks.at(0).x && P_all.listLandmarks.at(i).y == P.listLandmarks.at(0).y && P_all.listLandmarks.at(i).z == P.listLandmarks.at(0).z)
                continue;
            //if(fabs(P_all.listLandmarks.at(i).x-start_x) < fabs(end_x-start_x)+thres && fabs(P_all.listLandmarks.at(i).y-start_y) < fabs(end_y-start_y)+thres && fabs(P_all.listLandmarks.at(i).z- start_z) < fabs(end_z-start_z)+thres)
            if(fabs(P_all.listLandmarks.at(i).x-0.5*(end_x + start_x)) < n_dist*fabs(end_x-start_x)+thres && fabs(P_all.listLandmarks.at(i).y-0.5*(end_y +start_y)) < n_dist*fabs(end_y-start_y)+thres && fabs(P_all.listLandmarks.at(i).z- 0.5*(end_z+start_z)) < n_dist*fabs(end_z-start_z)+thres)
            //if(P_all.listLandmarks.at(i).x>start_x && P_all.listLandmarks.at(i).x<end_x && P_all.listLandmarks.at(i).y>start_y && P_all.listLandmarks.at(i).y<end_y && P_all.listLandmarks.at(i).z>start_z && P_all.listLandmarks.at(i).z<end_z)
            {
                cout<<"remove tip point"<<endl;
                cout<<"P_all.listLandmarks.at(i).x = "<<P_all.listLandmarks.at(i).x<<"   P_all.listLandmarks.at(i).y = "<<P_all.listLandmarks.at(i).y<<"    P_all.listLandmarks.at(i).z = "<<P_all.listLandmarks.at(i).z<<endl;
                cout<<"start_x = "<<start_x<<"    start_y = "<<start_y<<"    start_z = "<<start_z<<endl;
                cout<<"end_x = "<<end_x<<"    end_y = "<<end_y<<"    end_z = "<<end_z<<endl;
                return false;
            }

        }
    }
    cout<<"cannot remove tip point"<<endl;
    //v3d_msg("checkout");
    //end

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || tileLocation.z >= P.in_sz[2] - 1 || end_x <= 0 || end_y <= 0 || end_z <= 0)
    {
        printf("hit the boundary");
        return true;
    }

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));
    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    //add by yongzhang
    QString scanDataFileString2 = saveDirString;
    scanDataFileString2.append("/").append("signal.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString2).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));

    QString scanDataFileString3 = saveDirString;
    scanDataFileString3.append("/").append("tip.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString3).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = end_z - start_z;
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = start_z; iz < end_z; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,start_z,end_x-1,end_y-1,end_z-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }

        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,start_z,
                               end_x,end_y,end_z))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }

            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,start_z,end_z);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    V3DLONG pagesz_vim = in_sz[0]*in_sz[1]*in_sz[2];
 //   unsigned char* total1dData_apa = 0;

    if(P.global_name)
    {
        double min,max;
        unsigned char * total1dData_scaled = 0;
        total1dData_scaled = new unsigned char [pagesz_vim];
        rescale_to_0_255_and_copy(total1dData,pagesz_vim,min,max,total1dData_scaled);
        total4DImage->setData((unsigned char*)total1dData_scaled, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    }else
    {
//        total1dData_apa = new unsigned char [pagesz_vim];
//        BinaryProcess(total1dData, total1dData_apa,in_sz[0],in_sz[1], in_sz[2], 5, 3);


        total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    }

    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(start_z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    total4DImage->setRezZ(3.0);//set the flg for 3d crawler

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".tif");

    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".swc");

//    qDebug()<<scanDataFileString;
//    QFile saveTextFile;
//    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
//    if (!saveTextFile.isOpen()){
//        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
//            qDebug()<<"unable to save file!";
//            return false;}     }
//    QTextStream outputStream;
//    outputStream.setDevice(&saveTextFile);
//    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<< (int) total4DImage->getOriginZ()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<" "<< (int) in_sz[2]<<"\n";
//    saveTextFile.close();

    //by yongzhang
    QString swcString_result = swcString + "_result.swc";
    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    //outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<< (int) total4DImage->getOriginZ()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<" "<< (int) in_sz[2]<<"\n";
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<< (int) total4DImage->getOriginZ()<<" "<<swcString_result<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<" "<< (int) in_sz[2]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

    //add by zhangyong
    Image4DSimple* p4DImage = new Image4DSimple;
    p4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

    cout<<"**********choose threshold.**********"<<endl;
    in_sz[0]=p4DImage->getXDim();
    in_sz[1]=p4DImage->getYDim();
    in_sz[2]=p4DImage->getZDim();
    in_sz[3]=p4DImage->getCDim();

    double imgAve, imgStd, bkg_thresh;
    mean_and_std(p4DImage->getRawData(), p4DImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);

    double td= (imgStd<10)? 10: imgStd;
    bkg_thresh = imgAve +0.5*td;
    cout<<"bkg_thresh = "<<bkg_thresh<<endl;
    //v3d_msg("checkout1");

//    ifstream ifs_swc(finaloutputswc.toStdString().c_str());

    // call unet segmentation
//    if(!ifs_swc && P.soma)
//    {
//        P.length_thresh = 5;
//        QString imageUnetString = imageSaveString + "unet.v3draw";
////        QString imageUnetString = imageSaveString;//change by wp

/////*change by wp
//#if  defined(Q_OS_LINUX)
//    QString cmd_predict = QString("%1/start_vaa3d.sh -x prediction_caffe -f Segmentation_3D_combine -i %2 -o %3 -p %1/unet_files/deploy.prototxt %1/unet_files/one_HVSMR_iter_42000.caffemodel %1/unet_files/two_HVSMR_iter_138000.caffemodel")
//            .arg(getAppPath().toStdString().c_str()).arg(imageSaveString.toStdString().c_str()).arg(imageUnetString.toStdString().c_str());
//    system(qPrintable(cmd_predict));
//#endif
////*/


//        int datatype;
//        if(!simple_loadimage_wrapper(callback, imageUnetString.toStdString().c_str(), total1dData, in_sz, datatype))
//        {
//            cerr<<"load image "<<imageUnetString.toStdString()<<" error!"<<endl;
//            return false;
//        }
//        total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
//    }else
//        P.length_thresh = 5;

     //begin -- python predictBigImage3DUnetPP.py input.tif result.tif trained_model.h5 256 256 128
        cout<<"********************** unet segmentation******************************"<<endl;

        int count = 0;
        double tif_ratio, unet_ratio, final_ratio;

        QString fileSaveName = saveDirString + "/signal.txt";
        QFile file(fileSaveName);
        if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
            return false;
        QTextStream myfile(&file);
        myfile<<"n\tsignal_tif\tsignal_unet\tsignal_final"<<endl;

        if(1)
        {
            P.length_thresh = 5;
            QString imageUnetString = imageSaveString+ "_unet.tif";

#if defined(Q_OS_LINUX)
    QString cmd_predict = QString("/home/zhang/tensorflow/bin/python3.6 /home/zhang/Desktop/pythoncode/UNetpp2/UltraTracer/predictBigImage3DUnetPP.py %1 %2 /home/zhang/Desktop/pythoncode/UNetpp2/UltraTracer/multi-trained_model.h5 %3 %4 %5 %6 %7 %8")
            .arg(imageSaveString.toStdString().c_str()).arg(imageUnetString.toStdString().c_str()).arg(mysz[0]).arg(mysz[1]).arg(mysz[2]);
    system(qPrintable(cmd_predict));
#endif

            cout<<"********************** combine image and predict image******************************"<<endl;
            unsigned char * unet_total1dData = 0;
            int datatype;
            if(!simple_loadimage_wrapper(callback, imageUnetString.toStdString().c_str(), unet_total1dData, in_sz, datatype))
            {
                cerr<<"load image "<<imageUnetString.toStdString()<<" error!"<<endl;
                return false;
            }

            V3DLONG N = in_sz[0];
            V3DLONG M = in_sz[1];
            V3DLONG P = in_sz[2];
            V3DLONG C = in_sz[3];
            V3DLONG pagesz = N*M;
            V3DLONG tol_sz = pagesz * P;

            cout<<"tol_sz = "<<tol_sz<<endl;

            unsigned char * final_total1dData = 0;
            try {final_total1dData = new unsigned char [tol_sz];}
             catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

            //初始化
            for(V3DLONG i = 0; i < tol_sz;i++)
                final_total1dData[i] = 0;

            double a = 0.4;
            for(V3DLONG iz = 0; iz < P; iz++)
            {
                V3DLONG offsetk = iz*M*N;
                for(V3DLONG iy = 0; iy < M; iy++)
                {
                    V3DLONG offsetj = iy*N;
                    for(V3DLONG ix = 0; ix < N; ix++)
                    {
                        if(total1dData[offsetk + offsetj + ix]-unet_total1dData[offsetk + offsetj + ix]>bkg_thresh)
                            final_total1dData[offsetk + offsetj + ix] = unet_total1dData[offsetk + offsetj + ix];
                        else
                            final_total1dData[offsetk + offsetj + ix] = a*(total1dData[offsetk + offsetj + ix])+(1-a)*(unet_total1dData[offsetk + offsetj + ix]);

                    }

                 }
            }

            QString outimg = imageSaveString + "_final.tif";

            if(!simple_saveimage_wrapper(callback,outimg.toStdString().c_str(),(unsigned char *)final_total1dData,in_sz,1))
            {
                cerr<<"save image "<<outimg.toStdString()<<" error!"<<endl;
                return false;
            }

            total4DImage->setData((unsigned char*)final_total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

            double count1=0, count2 = 0, count3 = 0;
            for(V3DLONG j=0;j<tol_sz;j++)
            {
                if(double(total1dData[j]) > bkg_thresh)
                    count1++;
                if(double(unet_total1dData[j]) > 0)
                    count2++;
                if(double(final_total1dData[j]) > bkg_thresh)
                    count3++;
            }

            //cout<<"count1 = "<<count1<<"   count2 = "<<count2<<endl;
            //double tif_ratio, unet_ratio, final_ratio;
            tif_ratio = count1/tol_sz;
            unet_ratio = count2/tol_sz;
            final_ratio = count3/tol_sz;
            //cout<<"tif_ratio = "<<tif_ratio<<"    unet_ratio = "<<unet_ratio<<"   final_ratio = "<<final_ratio<<endl;

            qDebug()<<scanDataFileString2;
            QFile saveTextFile2;
            saveTextFile2.setFileName(scanDataFileString2);// add currentScanFile
            if (!saveTextFile2.isOpen()){
                if (!saveTextFile2.open(QIODevice::Text|QIODevice::Append  )){
                    qDebug()<<"unable to save file!";
                    return false;}     }
            QTextStream outputStream2;
            //outputStream2<<"n\tsignal_tif\tsignal_unet\tsignal_final"<<endl;
            outputStream2.setDevice(&saveTextFile2);
            outputStream2<<count<<"\t"<<tif_ratio<<"\t"<< unet_ratio<<"\t"<< final_ratio<<"\t"<<"\n";
            saveTextFile2.close();
            count++;
        }
        else
            P.length_thresh = 5;

//        myfile <<count<<"\t"<<tif_ratio<<"\t"<< unet_ratio<<"\t"<< final_ratio<<"\t"<<endl;
//        file.close();

        // branch_angle
//        if(!ifs_swc && P.soma)
//        {
//            P.length_thresh = 5;
//            int datatype;
//            if(!simple_loadimage_wrapper(callback, imageSaveString.toStdString().c_str(), total1dData, in_sz, datatype))
//            {
//                cerr<<"load image "<<imageSaveString.toStdString()<<" error!"<<endl;
//                return false;
//            }
//            total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

//        }else
//            P.length_thresh = 5;


        //end

    PARA_APP1 p1;
    PARA_APP2 p2;
    QString versionStr = "v0.001";

    if(P.method == app1)
    {
        p1.bkg_thresh = P.bkg_thresh;
        p1.channel = P.channel-1;
        p1.b_256cube = P.b_256cube;
        p1.visible_thresh = P.visible_thresh;

        p1.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p1.p4dImage = total4DImage;
        p1.xc0 = p1.yc0 = p1.zc0 = 0;
        p1.xc1 = p1.p4dImage->getXDim()-1;
        p1.yc1 = p1.p4dImage->getYDim()-1;
        p1.zc1 = p1.p4dImage->getZDim()-1;
    }
    else
    {
        p2.is_gsdt = P.is_gsdt;
        p2.is_coverage_prune = true;
        p2.is_break_accept = P.is_break_accept;
        p2.bkg_thresh = -1;//P.bkg_thresh;
        p2.length_thresh = P.length_thresh;
        p2.cnn_type = 2;
        p2.channel = 0;
        p2.SR_ratio = 3.0/9.9;
        p2.b_256cube = P.b_256cube;
        p2.b_RadiusFrom2D = P.b_RadiusFrom2D;
        p2.b_resample = 0;
        p2.b_intensity = 0;
        p2.b_brightfiled = 0;
        p2.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p2.p4dImage = total4DImage;
        p2.p4dImage->setFileName(imageSaveString.toStdString().c_str());
        p2.xc0 = p2.yc0 = p2.zc0 = 0;
        p2.xc1 = p2.p4dImage->getXDim()-1;
        p2.yc1 = p2.p4dImage->getYDim()-1;
        p2.zc1 = p2.p4dImage->getZDim()-1;
    }

    NeuronTree nt;
    vector<MyMarker*> finalswc;

    if(ifs_swc)
       finalswc = readSWC_file(finaloutputswc.toStdString());

    //meanshift first before tracing

    mean_shift_fun fun_obj;
    fun_obj.pushNewData<unsigned char>((unsigned char*)total1dData, mysz);

    vector<MyMarker*> tileswc_file;
    if(P.method == app1 || P.method == app2)
    {
        if(P.method == app1)
            qDebug()<<"starting app1";
        else
            qDebug()<<"starting app2";
        qDebug()<<"rootlist size "<<QString::number(inputRootList.size());
        cout << "wp_rootlist size1" << inputRootList.size() << endl;

        if(inputRootList.size() <1)
        {
            if(P.method == app1)
            {
                p1.outswc_file =swcString;
                proc_app1(callback, p1, versionStr);
            }
            else
            {
                p2.outswc_file =swcString;
                proc_app2_wp(callback, p2, versionStr);
            }
        }
        else
        {   cout << "wp_rootlist size2" << inputRootList.size() << endl;
	
            for(int i = 0; i < inputRootList.size(); i++)
            {
                QString poutswc_file = swcString + (QString::number(i)) + (".swc");
                if(P.method == app1)
                    p1.outswc_file =poutswc_file;
                else
                    p2.outswc_file =poutswc_file;

                bool flag = false;
                LocationSimple RootNewLocation;
                RootNewLocation.x = inputRootList.at(i).x - total4DImage->getOriginX();
                RootNewLocation.y = inputRootList.at(i).y - total4DImage->getOriginY();
                RootNewLocation.z = inputRootList.at(i).z - total4DImage->getOriginZ();

                if(!ifs_swc && P.soma)
                {
                    LandmarkList marklist_tmp;
                    marklist_tmp.push_back(RootNewLocation);

                    ImageMarker outputMarker;
                    QList<ImageMarker> seedsToSave;
                    outputMarker.x = RootNewLocation.x;
                    outputMarker.y = RootNewLocation.y;
                    outputMarker.z = RootNewLocation.z;
                    seedsToSave.append(outputMarker);

                    vector<V3DLONG> poss_landmark;
                    double windowradius = 30;

                    poss_landmark=landMarkList2poss(marklist_tmp, mysz[0], mysz[0]*mysz[1]);
                    marklist_tmp.clear();
                    vector<float> mass_center=fun_obj.mean_shift_center_mass(poss_landmark[0],windowradius);
                    RootNewLocation.x = mass_center[0]+1;
                    RootNewLocation.y = mass_center[1]+1;
                    RootNewLocation.z = mass_center[2]+1;

                    outputMarker.x = RootNewLocation.x;
                    outputMarker.y = RootNewLocation.y;
                    outputMarker.z = RootNewLocation.z;
                    seedsToSave.append(outputMarker);

                    QString marker_name = imageSaveString + ".marker";
                    writeMarker_file(marker_name, seedsToSave);

                }

                const float dd = 0.5;

                if(RootNewLocation.x<-dd || RootNewLocation.x>total4DImage->getXDim()-1+dd || RootNewLocation.y<-dd || RootNewLocation.y>total4DImage->getYDim()-1+dd || RootNewLocation.z<-dd || RootNewLocation.z>total4DImage->getZDim()-1+dd)
                    continue;

                if(tileswc_file.size()>0)
                {
                    for(V3DLONG dd = 0; dd < tileswc_file.size();dd++)
                    {
                        double dis = sqrt(pow2(RootNewLocation.x - tileswc_file.at(dd)->x) + pow2(RootNewLocation.y - tileswc_file.at(dd)->y) + pow2(RootNewLocation.z - tileswc_file.at(dd)->z));
                        if(dis < 10.0)
                        {
                            flag = true;
                            break;
                        }
                    }
                }

                if(!flag)
                {
                    vector<MyMarker*> inputswc;
                    bool soma_tile;
                    if(P.method == app1)
                    {

                        p1.landmarks.push_back(RootNewLocation);
                        proc_app1(callback, p1, versionStr);
                        p1.landmarks.clear();
                    }else
                    {

                        v3d_msg(QString("root is (%1,%2,%3").arg(RootNewLocation.x).arg(RootNewLocation.y).arg(RootNewLocation.z),0);
                        V3DLONG num_tips = 100;
                        double tips_th;
                        if(P.global_name)
                        {
                            tips_th = 10;
                            p2.is_break_accept = false;
                        }
                        else
                            tips_th = p2.p4dImage->getXDim()*100/512;
                       // p2.bkg_thresh = -1;//P.bkg_thresh;
                        double imgAve, imgStd;
                        mean_and_std(p2.p4dImage->getRawDataAtChannel(0), p2.p4dImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);
                        double td= (imgStd<10)? 10: imgStd;
                        p2.bkg_thresh = imgAve +0.7*td ;

                        p2.landmarks.push_back(RootNewLocation);
			if(!ifs_swc && P.soma){
				cout << "in first" << endl;
				for(int wp_i=1;wp_i<P.listLandmarks.size();wp_i++){
					LocationSimple wpNewLocation;
					wpNewLocation.x=P.listLandmarks[wp_i].x-total4DImage->getOriginX();
					wpNewLocation.y=P.listLandmarks[wp_i].y-total4DImage->getOriginY();
					wpNewLocation.z=P.listLandmarks[wp_i].z-total4DImage->getOriginZ();
                    cout << wpNewLocation.x << " " << wpNewLocation.y << " " << wpNewLocation.z << endl;
                    if(wpNewLocation.x>=0&&wpNewLocation.y>=0&&wpNewLocation.z>=0&&wpNewLocation.x<total4DImage->getOriginX()&&wpNewLocation.y<total4DImage->getOriginY()&&wpNewLocation.z<total4DImage->getOriginZ()){
                        p2.landmarks.push_back(wpNewLocation);
                    }

				}
				
				//LocationSimple wpNewLocation;
				//wpNewLocation.x=RootNewLocation.x-96;
				//wpNewLocation.y=RootNewLocation.y+87;
				//wpNewLocation.z=RootNewLocation.z+8;
				
				//p2.landmarks.push_back(wpNewLocation);
			}
			cout << "P.listLandmarks.size()" << P.listLandmarks.size() << endl;
			

                       // if(P.global_name && ifs_swc)
                        if(0)
                        {
                            double imgAve, imgStd;
                            mean_and_std(p2.p4dImage->getRawDataAtChannel(0), p2.p4dImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);
                            double td= (imgStd<10)? 10: imgStd;
                            p2.bkg_thresh = imgAve +0.5*td ;
                            bool flag_high = false;
                            bool flag_low = false;
                            do
                            {
                                double fore_count = 0;
                                for(V3DLONG i = 0 ; i < p2.p4dImage->getTotalUnitNumberPerChannel(); i++)
                                {
                                    if(p2.p4dImage->getRawDataAtChannel(0)[i] > p2.bkg_thresh)
                                        fore_count++;
                                }

                                double fore_ratio = fore_count/p2.p4dImage->getTotalUnitNumberPerChannel();
                                if(fore_ratio > 0.05 && !flag_low)
                                {
                                    p2.bkg_thresh++;
                                    flag_high = true;
                                }else if (fore_ratio < 0.01 && !flag_high)
                                {
                                    p2.bkg_thresh--;
                                    flag_low = true;
                                }else
                                    break;
                            } while(1);
                        }

                        int count=0;
                        do
                        {
                            if(count>4)
                            {
                                inputswc.clear();
                                break;
                            }

                            count++;
                            soma_tile = false;
                            num_tips = 0;
			    cout << "in_do_while p2.landmarks.size()" << p2.landmarks.size() << endl; 
                            proc_app2_wp(callback, p2, versionStr);
    //                        if(ifs_swc)
    //                        {
    //                            NeuronTree nt_app2 = readSWC_file(poutswc_file);
    //                            NeuronTree nt_app2_pruned = pruning_cross_swc(nt_app2);
    //                            writeSWC_file(poutswc_file,nt_app2_pruned);
    //                        }

                            inputswc = readSWC_file(poutswc_file.toStdString());
                            NeuronTree inputswc_nt = readSWC_file(poutswc_file);
                            QVector<QVector<V3DLONG> > childs;
                            V3DLONG neuronNum = inputswc_nt.listNeuron.size();
                            childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
                            for (V3DLONG i=0;i<neuronNum;i++)
                            {
                                V3DLONG par = inputswc_nt.listNeuron[i].pn;
                                if (par<0) continue;
                                childs[inputswc_nt.hashNeuron.value(par)].push_back(i);
                            }

                            for(V3DLONG d = 0; d < inputswc_nt.listNeuron.size(); d++)
                            {
                                if(childs[d].size() == 0)
                                    num_tips++;
                                if(ifs_swc && inputswc[d]->radius >= 8) //was 8
                                {
                                    soma_tile = true;
                                }

                            }
                            if (num_tips>=tips_th  && ifs_swc) //add <=20 and #tips>=100 constraints by PHC 20170801
                            {
                                unsigned char* total1dData_apa = 0;
                                total1dData_apa = new unsigned char [pagesz_vim];
                                BinaryProcess(total1dData, total1dData_apa,in_sz[0],in_sz[1], in_sz[2], 5, 3);
                                p2.p4dImage->setData((unsigned char*)total1dData_apa, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
                                p2.bkg_thresh =-1;
                            }
                            else
                                break;
                        } while (1);

                        while(0 && inputswc.size()<=0) //by PHC 20170523
                        {
                            soma_tile = false;
                            num_tips = 0;
                            p2.bkg_thresh -=2;
                            proc_app2_wp(callback, p2, versionStr);
                            inputswc = readSWC_file(poutswc_file.toStdString());
                            for(V3DLONG d = 0; d < inputswc.size(); d++)
                            {
                                if(ifs_swc && inputswc[d]->radius >= 20) //was 8
                                {
                                    soma_tile = true;
                                    break;
                                }
                            }
                        }

                        p2.landmarks.clear();
                    }

                    for(V3DLONG d = 0; d < inputswc.size(); d++)
                    {
                        if(soma_tile)   inputswc[d]->type = 0;
                        tileswc_file.push_back(inputswc[d]);
                    }
                }
            }
//            saveSWC_file(swcString.toStdString().c_str(), tileswc_file);
//            nt = readSWC_file(swcString);
        }
    }
    else if (P.method == gd )
    {
        QString marker_name = imageSaveString + ".marker";
        QString inputswc_name = marker_name + ".swc";
        QList<ImageMarker> seedsToSave;
        ImageMarker outputMarker;
        if(inputRootList.size() <1)
        {
            outputMarker.x = int(total4DImage->getXDim()/2) + 1;
            outputMarker.y = int(total4DImage->getYDim()/2) + 1;
            outputMarker.z = int(total4DImage->getZDim()/2) + 1;
            seedsToSave.append(outputMarker);
            if(QFileInfo(P.swcfilename).exists())
            {
                NeuronTree input_nt = readSWC_file(P.swcfilename);
                for(V3DLONG i = 0; i <input_nt.listNeuron.size();i++)
                {
                    input_nt.listNeuron[i].x = input_nt.listNeuron[i].x - total4DImage->getOriginX() + 1;
                    input_nt.listNeuron[i].y = input_nt.listNeuron[i].y - total4DImage->getOriginY() + 1;
                    input_nt.listNeuron[i].z = input_nt.listNeuron[i].z - total4DImage->getOriginZ() + 1;
                }
                writeSWC_file(inputswc_name,input_nt);
            }

        }
        else
        {
            for(V3DLONG i = 0; i<inputRootList.size();i++)
            {
                ImageMarker outputMarker;
                outputMarker.x = inputRootList.at(i).x - total4DImage->getOriginX() + 1;
                outputMarker.y = inputRootList.at(i).y - total4DImage->getOriginY() + 1;
                outputMarker.z = inputRootList.at(i).z - total4DImage->getOriginZ() + 1;
                seedsToSave.append(outputMarker);
            }
        }
        writeMarker_file(marker_name, seedsToSave);

        V3DPluginArgItem arg;
        V3DPluginArgList input;
        V3DPluginArgList output;

        QString full_plugin_name;
        QString func_name;

        arg.type = "random";std::vector<char*> arg_input;
        std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
        arg_input.push_back(fileName_string);
        arg.p = (void *) & arg_input; input<< arg;

        char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
        arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

        arg.type = "random";
        std::vector<char*> arg_para;
        char* char_marker =  new char[marker_name.length() + 1];strcpy(char_marker, marker_name.toStdString().c_str());
        arg_para.push_back(char_marker);

        string S_seed_win = boost::lexical_cast<string>(P.seed_win);
        char* C_seed_win = new char[S_seed_win.length() + 1];
        strcpy(C_seed_win,S_seed_win.c_str());
        arg_para.push_back(C_seed_win);
        arg_para.push_back("10");
        arg_para.push_back("1");

        if(inputRootList.size() <1 && QFileInfo(inputswc_name).exists())
        {
            char* char_inputswc =  new char[inputswc_name.length() + 1];strcpy(char_inputswc, inputswc_name.toStdString().c_str());
            arg_para.push_back(char_inputswc);
        }
        full_plugin_name = "line_detector";
        func_name =  "GD_Curveline_v2";

        arg.p = (void *) & arg_para; input << arg;
        if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
        {
            printf("Can not find the tracing plugin!\n");
            return false;
        }

        nt = readSWC_file(swcString);
//        if(nt.listNeuron.size() < 50)
//        {
//            ImageMarker additionalMarker;
//            additionalMarker.x = seedsToSave.at(0).x+1;
//            additionalMarker.y = seedsToSave.at(0).y;
//            additionalMarker.z = seedsToSave.at(0).z;

//            seedsToSave.append(additionalMarker);
//            writeMarker_file(marker_name, seedsToSave);
//            if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
//            {
//                printf("Can not find the tracing plugin!\n");
//                return false;
//            }

//            nt = readSWC_file(swcString);
//        }

    }

    if(in_sz) {delete []in_sz; in_sz =0;}

    if(ifs_swc)
    {

        map<MyMarker*, double> score_map;
    //    vector<MyMarker *> neuronTree = readSWC_file(swcString.toStdString());
        topology_analysis_perturb_intense(total4DImage->getRawData(), tileswc_file, score_map, 1, p2.p4dImage->getXDim(), p2.p4dImage->getYDim(), p2.p4dImage->getZDim(), 1);

        for(V3DLONG i = 0; i<tileswc_file.size(); i++){
            MyMarker * marker = tileswc_file[i];
            double tmp = score_map[marker] * 120 +19;
            marker->type = (tmp > 255 || marker->type ==0) ? 255 : tmp;
        }
    }

    else
    {
        for(V3DLONG i = 0; i<tileswc_file.size(); i++){
            MyMarker * marker = tileswc_file[i];
            marker->type = 3;
        }
    }

    saveSWC_file(swcString.toStdString(),tileswc_file);
    //nt = readSWC_file(swcString);

    //call ImageProcessing ----branch_angle function     ----add by yongzhang 20190902
    if(!ifs_swc && P.soma)
    {
        nt = readSWC_file(swcString);
        tileswc_file = readSWC_file(swcString.toStdString());
        //v3d_msg("checkout1");
    }
    else
    {
        QString swcString_remove = swcString + "_remove.swc";
        //QString swcString_result = swcString + "_result.swc";
        #if  defined(Q_OS_LINUX)
        QString cmd_ImageProcessing = QString("%1/vaa3d -x ImageProcessing -f branch_angle -i %2 -o %3 %4")
        .arg(getAppPath().toStdString().c_str()).arg(swcString.toStdString().c_str()).arg(swcString_remove.toStdString().c_str()).arg(swcString_result.toStdString().c_str());
         system(qPrintable(cmd_ImageProcessing));
        #endif

        ifstream IFS_SWC(swcString_result.toStdString().c_str());
        if (!IFS_SWC)
        {
            nt= readSWC_file(swcString);
            tileswc_file = readSWC_file(swcString.toStdString());
        }
        else
        {
            nt = readSWC_file(swcString_result);
            tileswc_file = readSWC_file(swcString_result.toStdString());
        }

        //v3d_msg("checkout2");
    }

    saveSWC_file(swcString_result.toStdString(),tileswc_file);
    //end

    QVector<QVector<V3DLONG> > childs;
    V3DLONG neuronNum = nt.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }

    //tip points--addd by yongzhang
    int childNum;
    QList<NeuronSWC> tipList;
    tipList.clear();
    for (V3DLONG i=0; i<nt.listNeuron.size();i++)
    {
        if (nt.listNeuron[i].pn<0) continue;
        //childNum = childs[nt.listNeuron[i].pn].size();
        childNum = childs[i].size();
        if(childNum == 0)
            tipList.push_back(nt.listNeuron[i]);

    }
    cout<<"tipList = "<<tipList.size()<<endl;

    qDebug()<<scanDataFileString3;
    QFile saveTextFile3;
    saveTextFile3.setFileName(scanDataFileString3);// add currentScanFile
    if (!saveTextFile3.isOpen()){
        if (!saveTextFile3.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream3;
    outputStream3.setDevice(&saveTextFile3);
    outputStream3<<tipList.size()<<"\n";
    saveTextFile3.close();

    //end

    //assign all sub_trees
    QVector<int> visit(nt.listNeuron.size(),0);

    if(!ifs_swc)
    {
        for(int i=1; i<nt.listNeuron.size();i++)
        {
            if(NTDIS(nt.listNeuron.at(i),nt.listNeuron.at(0))<=40)
                visit[i]=1;

        }
        for(int i=0; i<nt.listNeuron.size();i++)
        {
            if(nt.listNeuron[i].radius>=8 && visit[i]==0)
            {
                QQueue<int> q;
                visit[i]=1;
                q.push_back(i);
                while(!q.empty())
                {
                    int current = q.front(); q.pop_front();
                    nt.listNeuron[current].type = 255;
                    for(int j=0; j<childs[current].size();j++)
                    {
                        if(visit[childs[current].at(j)]==0)
                        {
                            visit[childs[current].at(j)]=1;
                            q.push_back(childs[current].at(j));
                        }
                    }
                }
                q.clear();
            }
        }
    }
    else{
        for(int i=0; i<nt.listNeuron.size();i++)
        {
            if(nt.listNeuron[i].type>130 && nt.listNeuron[i].pn ==-1 && visit[i]==0)
            {
                QQueue<int> q;
                visit[i]=1;
                q.push_back(i);
                while(!q.empty())
                {
                    int current = q.front(); q.pop_front();
                    for(int j=0; j<childs[current].size();j++)
                    {
                        int current_child = childs[current].at(j);
                        if(visit[current_child]==0)
                        {
                            visit[current_child]=1;
                            if(childs[current_child].size()<2)
                                q.push_back(current_child);
                        }
                    }
                }
                q.clear();
            }

            int diff_radius;
            if(nt.listNeuron[i].type<=130 && nt.listNeuron[i].radius<2 && visit[i]==0)
            {
                int count=0;
                int pre_sum=0;
                int next_sum=0;
                int pre=i, next=i;
                while(count<15 && childs[next].size()>0 && nt.listNeuron[pre].pn>0)
                {
                    pre_sum += nt.listNeuron[pre].radius;
                    next_sum += nt.listNeuron[next].radius;
                    pre = nt.hashNeuron.value(nt.listNeuron[pre].pn);
                    next = childs[next].at(0);
                    count++;
                }

                if(count==15)
                {
                    diff_radius = (next_sum-pre_sum);
                    double node_angle = angle(nt.listNeuron[i], nt.listNeuron[pre], nt.listNeuron[next]);

                    if(diff_radius>15 && node_angle<120) nt.listNeuron[i].type=255;
                }
            }

            if(nt.listNeuron[i].type>130 && visit[i]==0 && childs[i].size()==1)
            {
                QQueue<int> q;
                visit[i]=1;
                q.push_back(i);
                while(!q.empty())
                {
                    int current = q.front(); q.pop_front();
                    nt.listNeuron[current].type = 255;
                    for(int j=0; j<childs[current].size();j++)
                    {
                        if(visit[childs[current].at(j)]==0)
                        {
                            visit[childs[current].at(j)]=1;
                            q.push_back(childs[current].at(j));
                        }
                    }
                }
                q.clear();
            }
        }
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    LandmarkList tip_out;
    LandmarkList tip_in;


    double overlap_ratio;
    if(P.method == gd)
        overlap_ratio = 0.5;
    else
        overlap_ratio = 0.05;

    QList<NeuronSWC> list = nt.listNeuron;
    if(list.size()<3) return true;
    for (V3DLONG i=0;i<list.size();i++)
    {
        if (childs[i].size()==0 || P.method != gd)
        {
            NeuronSWC curr = list.at(i);
            if(curr.type >130) continue;
            LocationSimple newTip;
            bool check_tip = false;
            if( curr.x < overlap_ratio*  total4DImage->getXDim() || curr.x > (1-overlap_ratio) *  total4DImage->getXDim() || curr.y < overlap_ratio * total4DImage->getYDim() || curr.y > (1-overlap_ratio)* total4DImage->getYDim()
                    || curr.z < overlap_ratio*  total4DImage->getZDim() || curr.z > (1-overlap_ratio) *  total4DImage->getZDim())
            {
//                V3DLONG node_pn = getParent(i,nt);
//                V3DLONG node_pn_2nd;
//                if( list.at(node_pn).pn < 0)
//                {
//                    node_pn_2nd = node_pn;
//                }
//                else
//                {
//                    node_pn_2nd = getParent(node_pn,nt);
//                }

//                newTip.x = list.at(node_pn_2nd).x + total4DImage->getOriginX();
//                newTip.y = list.at(node_pn_2nd).y + total4DImage->getOriginY();
//                newTip.z = list.at(node_pn_2nd).z + total4DImage->getOriginZ();

//                newTip.radius = list.at(node_pn_2nd).r;

                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
                newTip.radius = curr.r;

                //add by yongzhang
//                cout<<"add by yongzhang"<<endl;
//                unsigned char* data1d = p4DImage->getRawData();
//                cout<<curr.x<<"   "<<curr.y<<"   "<<curr.z<<endl;
//                cout<<newTip.x<<"   "<<newTip.y<<"   "<<newTip.z<<endl;

//                QList<NeuronSWC> swclist_parents;
//                vector<double>  valuelist;
//                swclist_parents.clear();
//                V3DLONG par = list.at(i).n - 1;
//                for(int k = 0;k < 5;k++)
//                {
//                    V3DLONG par1 = getParent(par,nt);
//                    //cout<<"par1 = "<<par1<<endl;
//                    swclist_parents.push_back(nt.listNeuron[par]);
//                    int list_postion = (nt.listNeuron[par].x)*(nt.listNeuron[par].y)*(nt.listNeuron[par].z);
//                    valuelist.push_back(data1d[list_postion]);
//                    par = par1;
//                    if(childs[par1].size() == 2 || childs[par1].size() == 0)
//                        break;
//                }
//                cout<<"swclist_parents = "<<swclist_parents.size()<<endl;
//                cout<<"valuelist = "<<valuelist.size()<<endl;

//                double sum_value = 0;
//                for(int l = 0;l < valuelist.size();l++)
//                {
//                    sum_value = sum_value + valuelist[i];
//                }
//                cout<<"sum_value = "<<sum_value<<endl;
//                double avg_value = sum_value/valuelist.size();
//                newTip.value = avg_value;
//                cout<<"newTip.value = "<<newTip.value<<endl;
//                //v3d_msg("checkout");

//                //end

                for(V3DLONG j = 0; j < finalswc.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.at(j)->x) + pow2(newTip.y - finalswc.at(j)->y) + pow2(newTip.z - finalswc.at(j)->z));
                    if(dis < 2*finalswc.at(j)->radius || dis < 10 || curr.type ==0)  //was 5
                   // if(dis < 10)
                    {
                        check_tip = true;
                        break;
                    }
                }
            }
            if(check_tip) continue;

            if(P.method == gd)
            {
                double x_diff = curr.x - list.at(0).x;
                double y_diff = curr.y - list.at(0).y;
                double z_diff = curr.z - list.at(0).z;

                if(fabs(x_diff) >= fabs(y_diff) && fabs(x_diff) >= fabs(z_diff))
                {
                    if(x_diff < 0)
                        tip_left.push_back(newTip);
                    else
                        tip_right.push_back(newTip);

                }else if(fabs(y_diff) >= fabs(x_diff) && fabs(y_diff) >= fabs(z_diff))
                {
                    if(y_diff < 0)
                        tip_up.push_back(newTip);
                    else
                        tip_down.push_back(newTip);

                }else if(fabs(z_diff) >= fabs(x_diff) && fabs(z_diff) >= fabs(y_diff))
                {
                    if(z_diff < 0)
                        tip_in.push_back(newTip);
                    else
                        tip_out.push_back(newTip);
                }

            }else
            {

                if( curr.x < overlap_ratio* total4DImage->getXDim())
                {
                    tip_left.push_back(newTip);


                }else if (curr.x > (1-overlap_ratio) * total4DImage->getXDim())
                {
                    tip_right.push_back(newTip);
                }else if (curr.y < overlap_ratio * total4DImage->getYDim())
                {
                    tip_up.push_back(newTip);
                }else if (curr.y > (1-overlap_ratio)*total4DImage->getYDim())
                {
                    tip_down.push_back(newTip);
                }else if (curr.z < overlap_ratio * total4DImage->getZDim())
                {
                    tip_out.push_back(newTip);
                }else if (curr.z > (1-overlap_ratio)*total4DImage->getZDim())
                {
                    tip_in.push_back(newTip);
                }
            }
        }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,512,1);
        for(int i = 0; i < group_tips_left.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
            else
                ada_win_finding_3D(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
        }
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,512,2);
        for(int i = 0; i < group_tips_right.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
            else
                ada_win_finding_3D(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
        }
    }
    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,512,3);
        for(int i = 0; i < group_tips_up.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);
            else
                ada_win_finding_3D(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);
        }
    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,512,4);
        for(int i = 0; i < group_tips_down.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
            else
                ada_win_finding_3D(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
        }
    }

    if(tip_out.size()>0)
    {
        QList<LandmarkList> group_tips_out = group_tips(tip_out,512,5);
        for(int i = 0; i < group_tips_out.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_out.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,5);
            else
                ada_win_finding_3D(group_tips_out.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,5);
        }
    }

    if(tip_in.size()>0)
    {
        QList<LandmarkList> group_tips_in = group_tips(tip_in,512,6);
        for(int i = 0; i < group_tips_in.size();i++)
        {
            if(P.method == gd)
                ada_win_finding_3D_GD(group_tips_in.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,6);
            else
                ada_win_finding_3D(group_tips_in.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,6);
        }
    }

    if(P.method == gd)
        tileswc_file = readSWC_file(swcString.toStdString());

    if(ifs_swc)
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }

    total4DImage->deleteRawDataAndSetPointerToNull();

    return true;
}

void processSmartScan(V3DPluginCallback2 &callback, list<string> & infostring, QString fileWithData)
{
    ifstream ifs(fileWithData.toLatin1());
    string info_swc;
    int offsetX, offsetY,sizeX, sizeY;
    string swcfilepath;
    vector<MyMarker*> outswc,inputswc;
    int node_type = 1;
    int offsetX_min = 10000000,offsetY_min = 10000000,offsetX_max = -10000000,offsetY_max =-10000000;
    int origin_x,origin_y;

    QString folderpath = QFileInfo(fileWithData).absolutePath();
    V3DLONG in_sz[4];
    QString fileSaveName = fileWithData + ".swc";


    while(ifs && getline(ifs, info_swc))
    {
        std::istringstream iss(info_swc);
        iss >> offsetX >> offsetY >> swcfilepath >> sizeX >> sizeY;
        if(offsetX < offsetX_min) offsetX_min = offsetX;
        if(offsetY < offsetY_min) offsetY_min = offsetY;
        if(offsetX > offsetX_max) offsetX_max = offsetX;
        if(offsetY > offsetY_max) offsetY_max = offsetY;

        if(node_type == 1)
        {
            origin_x = offsetX;
            origin_y = offsetY;

            QString firstImagepath = folderpath + "/" +   QFileInfo(QString::fromStdString(swcfilepath)).baseName().append(".v3draw");
            unsigned char * data1d = 0;
            int datatype;
            if(!simple_loadimage_wrapper(callback, firstImagepath.toStdString().c_str(), data1d, in_sz, datatype))
            {
                cerr<<"load image "<<firstImagepath.toStdString()<<" error!"<<endl;
                return;
            }
            if(data1d) {delete []data1d; data1d=0;}

            inputswc = readSWC_file(swcfilepath);;
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->x = inputswc[d]->x + offsetX;
                inputswc[d]->y = inputswc[d]->y + offsetY;
                inputswc[d]->type = node_type;
                outswc.push_back(inputswc[d]);
            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
        }else
        {
            inputswc = readSWC_file(swcfilepath);
            NeuronTree nt = readSWC_file(QString(swcfilepath.c_str()));
            QVector<QVector<V3DLONG> > childs;
            V3DLONG neuronNum = nt.listNeuron.size();
            childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
            for (V3DLONG i=0;i<neuronNum;i++)
            {
                V3DLONG par = nt.listNeuron[i].pn;
                if (par<0) continue;
                childs[nt.hashNeuron.value(par)].push_back(i);
            }
            outswc = readSWC_file(fileSaveName.toStdString());
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->x = inputswc[d]->x + offsetX;
                inputswc[d]->y = inputswc[d]->y + offsetY;
                inputswc[d]->type = node_type;
                int flag_prune = 0;
                for(int dd = 0; dd < outswc.size();dd++)
                {
                    int dis_prun = sqrt(pow2(inputswc[d]->x - outswc[dd]->x) + pow2(inputswc[d]->y - outswc[dd]->y) + pow2(inputswc[d]->z - outswc[dd]->z));
                    if( (inputswc[d]->radius + outswc[dd]->radius - dis_prun)/dis_prun > 0.2)
                    {
                        if(childs[d].size() > 0) inputswc[childs[d].at(0)]->parent = outswc[dd];
                        flag_prune = 1;
                        break;
                    }

                }
                if(flag_prune == 0)
                {
                   outswc.push_back(inputswc[d]);
                }

            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);

        }
        node_type++;
    }
    ifs.close();


    for(V3DLONG i = 0; i < outswc.size(); i++)
    {
        outswc[i]->x = outswc[i]->x - offsetX_min;
        outswc[i]->y = outswc[i]->y - offsetY_min;
    }

    saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
    NeuronTree nt_final = readSWC_file(fileSaveName);
    QList<NeuronSWC> neuron_final_sorted;

    if (!SortSWC(nt_final.listNeuron, neuron_final_sorted,VOID, 10))
    {
        v3d_msg("fail to call swc sorting function.",0);
    }

    export_list2file(neuron_final_sorted, fileSaveName,fileSaveName);

    //write tc file

    QString tc_name = fileWithData + ".tc";
    ofstream myfile;
    myfile.open(tc_name.toStdString().c_str(), ios::in);
    if (myfile.is_open()==true)
    {
        qDebug()<<"initial file can be opened for reading and will be removed";

        myfile.close();
        remove(tc_name.toStdString().c_str());
    }
    myfile.open (tc_name.toStdString().c_str(),ios::out | ios::app );
    if (!myfile.is_open()){
        qDebug()<<"file's not really open...";
        return;
    }

    myfile << "# thumbnail file \n";
    myfile << "NULL \n\n";
    myfile << "# tiles \n";
    myfile << node_type-1 << " \n\n";
    myfile << "# dimensions (XYZC) \n";
    myfile << in_sz[0] + offsetX_max - offsetX_min << " " << in_sz[1] + offsetY_max - offsetY_min << " " << in_sz[2] << " " << 1 << " ";
    myfile << "\n\n";
    myfile << "# origin (XYZ) \n";
    myfile << "0.000000 0.000000 0.000000 \n\n";
    myfile << "# resolution (XYZ) \n";
    myfile << "1.000000 1.000000 1.000000 \n\n";
    myfile << "# image coordinates look up table \n";

    ifstream ifs_2nd(fileWithData.toLatin1());
    while(ifs_2nd && getline(ifs_2nd, info_swc))
    {
        std::istringstream iss(info_swc);
        iss >> offsetX >> offsetY >> swcfilepath >> sizeX >> sizeY;
        QString imagename= QFileInfo(QString::fromStdString(swcfilepath)).completeBaseName() + ".v3draw";
        imagename.append(QString("   ( %1, %2, 0) ( %3, %4, %5)").arg(offsetX - origin_x).arg(offsetY- origin_y).arg(sizeX-1 + offsetX - origin_x).arg(sizeY-1 + offsetY - origin_y).arg(in_sz[2]-1));
        myfile << imagename.toStdString();
        myfile << "\n";
    }
    myfile.flush();
    myfile.close();
    ifs_2nd.close();
}

void processSmartScan_3D(V3DPluginCallback2 &callback, list<string> & infostring, QString fileWithData)
{
    ifstream ifs(fileWithData.toLatin1());
    string info_swc;
    int offsetX, offsetY,offsetZ,sizeX, sizeY, sizeZ;
    string swcfilepath;
    vector<MyMarker*> outswc,inputswc;
    int node_type = 1;
    int offsetX_min = 10000000,offsetY_min = 10000000,offsetZ_min = 10000000,offsetX_max = -10000000,offsetY_max =-10000000,offsetZ_max =-10000000;
    int origin_x,origin_y,origin_z;

    QString folderpath = QFileInfo(fileWithData).absolutePath();
    V3DLONG in_sz[4];
    QString fileSaveName = fileWithData + "wfusion.swc";

    QDir imagefolderDir = QDir(QFileInfo(fileWithData).absoluteDir());
    imagefolderDir.cdUp();
    //QString fileSaveName = imagefolderDir.absolutePath()+"/nc_APP2_GD.swc";

    while(ifs && getline(ifs, info_swc))
    {
        std::istringstream iss(info_swc);
        iss >> offsetX >> offsetY >> offsetZ >> swcfilepath >> sizeX >> sizeY >> sizeZ;
        if(offsetX < offsetX_min) offsetX_min = offsetX;
        if(offsetY < offsetY_min) offsetY_min = offsetY;
        if(offsetZ < offsetZ_min) offsetZ_min = offsetZ;

        if(offsetX > offsetX_max) offsetX_max = offsetX;
        if(offsetY > offsetY_max) offsetY_max = offsetY;
        if(offsetZ > offsetZ_max) offsetZ_max = offsetZ;

        if(!QFileInfo(QString(swcfilepath.c_str())).exists())
            continue;
        if(node_type == 1)
        {
            origin_x = offsetX;
            origin_y = offsetY;
            origin_z = offsetZ;

//            QString firstImagepath = folderpath + "/" +   QFileInfo(QString::fromStdString(swcfilepath)).baseName().append(".v3draw");
            QString firstImagepath = folderpath + "/" +   QFileInfo(QString::fromStdString(swcfilepath)).baseName().append(".tif");
            unsigned char * data1d = 0;
            int datatype;
            if(!simple_loadimage_wrapper(callback, firstImagepath.toStdString().c_str(), data1d, in_sz, datatype))
            {
                cerr<<"load image "<<firstImagepath.toStdString()<<" error!"<<endl;
                return;
            }
            if(data1d) {delete []data1d; data1d=0;}

            inputswc = readSWC_file(swcfilepath);;
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->x = inputswc[d]->x + offsetX;
                inputswc[d]->y = inputswc[d]->y + offsetY;
                inputswc[d]->z = inputswc[d]->z + offsetZ;
                inputswc[d]->type = node_type;
                outswc.push_back(inputswc[d]);
            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
        }else
        {
            inputswc = readSWC_file(swcfilepath);
            NeuronTree nt = readSWC_file(QString(swcfilepath.c_str()));
            QVector<QVector<V3DLONG> > childs;
            V3DLONG neuronNum = nt.listNeuron.size();
            childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
            for (V3DLONG i=0;i<neuronNum;i++)
            {
                V3DLONG par = nt.listNeuron[i].pn;
                if (par<0) continue;
                childs[nt.hashNeuron.value(par)].push_back(i);
            }
            if(inputswc.size()>0 && childs[0].size()==1)
            {
                int count=1;
                int current = childs[0].at(0);
                while(childs[current].size()==1)
                {
                    current=childs[current].at(0);
                    count++;
                }
                if(count<10)
                {
                    inputswc[current]->parent = inputswc[0]->parent;
                    inputswc.erase(inputswc.begin(),inputswc.begin()+count);
                }
            }

            outswc = readSWC_file(fileSaveName.toStdString());
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->x = inputswc[d]->x + offsetX;
                inputswc[d]->y = inputswc[d]->y + offsetY;
                inputswc[d]->z = inputswc[d]->z + offsetZ;
                inputswc[d]->type = node_type;
                int flag_prune = 0;
                for(int dd = 0; dd < outswc.size();dd++)
                {
                    int dis_prun = sqrt(pow2(inputswc[d]->x - outswc[dd]->x) + pow2(inputswc[d]->y - outswc[dd]->y) + pow2(inputswc[d]->z - outswc[dd]->z));
                    if( (inputswc[d]->radius + outswc[dd]->radius - dis_prun)/dis_prun > 0.2)
                    {
                        if(childs[d].size() > 0) inputswc[childs[d].at(0)]->parent = outswc[dd];
                        flag_prune = 1;
                        break;
                    }

                }
                if(flag_prune == 0)
                {
                   outswc.push_back(inputswc[d]);
                }

            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
        }
        node_type++;
    }
    ifs.close();

//    for(V3DLONG i = 0; i < outswc.size(); i++)
//    {
//        outswc[i]->x = outswc[i]->x - offsetX_min;
//        outswc[i]->y = outswc[i]->y - offsetY_min;
//        outswc[i]->z = outswc[i]->z - offsetZ_min;
//    }


    saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
    NeuronTree nt_final = readSWC_file(fileSaveName);
    QList<NeuronSWC> neuron_final_sorted;

    if (!SortSWC(nt_final.listNeuron, neuron_final_sorted,VOID, 10))
    {
        v3d_msg("fail to call swc sorting function.",0);
    }

    export_list2file(neuron_final_sorted, fileSaveName,fileSaveName);
    export_list2file(nt_final.listNeuron, fileSaveName,fileSaveName);
    NeuronTree nt_b4_prune = readSWC_file(fileSaveName);
    NeuronTree nt_pruned = smartPrune(nt_b4_prune,10);
    NeuronTree nt_pruned_2nd = smartPrune(nt_pruned,10);

    writeSWC_file(fileSaveName,nt_pruned_2nd);


    //write tc file

    QString tc_name = fileWithData + ".tc";
    ofstream myfile;
    myfile.open(tc_name.toStdString().c_str(), ios::in);
    if (myfile.is_open()==true)
    {
        qDebug()<<"initial file can be opened for reading and will be removed";

        myfile.close();
        remove(tc_name.toStdString().c_str());
    }
    myfile.open (tc_name.toStdString().c_str(),ios::out | ios::app );
    if (!myfile.is_open()){
        qDebug()<<"file's not really open...";
        return;
    }

    myfile << "# thumbnail file \n";
    myfile << "NULL \n\n";
    myfile << "# tiles \n";
    myfile << node_type-1 << " \n\n";
    myfile << "# dimensions (XYZC) \n";
    myfile << in_sz[0] + offsetX_max - offsetX_min << " " << in_sz[1] + offsetY_max - offsetY_min << " " << in_sz[2] + offsetZ_max - offsetZ_min<< " " << 1 << " ";
    myfile << "\n\n";
    myfile << "# origin (XYZ) \n";
    myfile << "0.000000 0.000000 0.000000 \n\n";
    myfile << "# resolution (XYZ) \n";
    myfile << "1.000000 1.000000 1.000000 \n\n";
    myfile << "# image coordinates look up table \n";

    ifstream ifs_2nd(fileWithData.toLatin1());
    while(ifs_2nd && getline(ifs_2nd, info_swc))
    {
        std::istringstream iss(info_swc);
        iss >> offsetX >> offsetY >> offsetZ >> swcfilepath >> sizeX >> sizeY >> sizeZ;
//        QString imagename= QFileInfo(QString::fromStdString(swcfilepath)).completeBaseName() + ".v3draw";
        QString imagename= QFileInfo(QString::fromStdString(swcfilepath)).completeBaseName() + ".tif";
        imagename.append(QString("   ( %1, %2, %3) ( %4, %5, %6)").arg(offsetX - origin_x).arg(offsetY- origin_y).arg(offsetZ- origin_z).arg(sizeX-1 + offsetX - origin_x).arg(sizeY-1 + offsetY - origin_y).arg(sizeZ-1 + offsetZ - origin_z));
        myfile << imagename.toStdString();
        myfile << "\n";
    }
    myfile.flush();
    myfile.close();
    ifs_2nd.close();
}
void processSmartScan_3D_wofuison(V3DPluginCallback2 &callback, list<string> & infostring, QString fileWithData)
{
    ifstream ifs(fileWithData.toLatin1());
    string info_swc;
    string swcfilepath;
    vector<MyMarker*> outswc,inputswc;

    QString fileSaveName = fileWithData + "wofusion.swc";
    int offsetX, offsetY,offsetZ,sizeX, sizeY, sizeZ;
    while(ifs && getline(ifs, info_swc))
    {
        std::istringstream iss(info_swc);
        iss >> offsetX >> offsetY >> offsetZ >> swcfilepath >> sizeX >> sizeY >> sizeZ;

            inputswc = readSWC_file(swcfilepath);;
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->x = inputswc[d]->x + offsetX;
                inputswc[d]->y = inputswc[d]->y + offsetY;
                inputswc[d]->z = inputswc[d]->z + offsetZ;
                outswc.push_back(inputswc[d]);
            }
    }
    ifs.close();
    saveSWC_file(fileSaveName.toStdString().c_str(), outswc,infostring);
    NeuronTree nt_final = readSWC_file(fileSaveName);
    QList<NeuronSWC> neuron_final_sorted;

    if (!SortSWC(nt_final.listNeuron, neuron_final_sorted,VOID, VOID))
    {
        v3d_msg("fail to call swc sorting function.",0);
    }

    export_list2file(neuron_final_sorted, fileSaveName,fileSaveName);
}

void smartFuse(V3DPluginCallback2 &callback, QString inputFolder, QString fileSaveName)
{
    QDir dir(inputFolder);
    if (!dir.exists())
    {
        qWarning("Cannot find the directory");
        return;
    }
    QStringList imgSuffix;
    imgSuffix<<"*.swc"<<"*.eswc"<<"*.SWC"<<"*.ESWC";

    string swcfilepath;
    vector<MyMarker*> outswc,inputswc;
    int node_type = 3;
    foreach (QString file, dir.entryList(imgSuffix, QDir::Files, QDir::Name))
    {
        swcfilepath = QFileInfo(dir, file).absoluteFilePath().toStdString();
        if(node_type == 3)
        {
            inputswc = readSWC_file(swcfilepath);;
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->type = node_type;
                outswc.push_back(inputswc[d]);
            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc);
        }
        else
        {
            inputswc = readSWC_file(swcfilepath);
            NeuronTree nt = readSWC_file(QString(swcfilepath.c_str()));
            QVector<QVector<V3DLONG> > childs;
            V3DLONG neuronNum = nt.listNeuron.size();
            childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
            for (V3DLONG i=0;i<neuronNum;i++)
            {
                V3DLONG par = nt.listNeuron[i].pn;
                if (par<0) continue;
                childs[nt.hashNeuron.value(par)].push_back(i);
            }
            if(inputswc.size()>0 && childs[0].size()==1)
            {
                int count=1;
                int current = childs[0].at(0);
                while(childs[current].size()==1)
                {
                    current=childs[current].at(0);
                    count++;
                }
                if(count<10)
                {
                    inputswc[current]->parent = inputswc[0]->parent;
                    inputswc.erase(inputswc.begin(),inputswc.begin()+count);
                }
            }

            outswc = readSWC_file(fileSaveName.toStdString());
            for(V3DLONG d = 0; d < inputswc.size(); d++)
            {
                inputswc[d]->type = node_type;
                int flag_prune = 0;
                for(int dd = 0; dd < outswc.size();dd++)
                {
                    int dis_prun = sqrt(pow2(inputswc[d]->x - outswc[dd]->x) + pow2(inputswc[d]->y - outswc[dd]->y) + pow2(inputswc[d]->z - outswc[dd]->z));
                    if( (inputswc[d]->radius + outswc[dd]->radius - dis_prun)/dis_prun > 0.2)
                    {
                        if(childs[d].size() > 0) inputswc[childs[d].at(0)]->parent = outswc[dd];
                        flag_prune = 1;
                        break;
                    }

                }
                if(flag_prune == 0)
                {
                   outswc.push_back(inputswc[d]);
                }

            }
            saveSWC_file(fileSaveName.toStdString().c_str(), outswc);
        }
        node_type++;
    }
    saveSWC_file(fileSaveName.toStdString().c_str(), outswc);
    NeuronTree nt_final = readSWC_file(fileSaveName);
    QList<NeuronSWC> neuron_final_sorted;
    if (!SortSWC(nt_final.listNeuron, neuron_final_sorted,VOID, 10))
    {
        v3d_msg("fail to call swc sorting function.",0);
    }

    export_list2file(neuron_final_sorted, fileSaveName,fileSaveName);
    NeuronTree nt_b4_prune = readSWC_file(fileSaveName);
    NeuronTree nt_pruned = smartPrune(nt_b4_prune,10);
    NeuronTree nt_pruned_2nd = smartPrune(nt_pruned,10);

    writeSWC_file(fileSaveName,nt_pruned_2nd);
    return;
}

bool crawler_raw_all(V3DPluginCallback2 &callback, QWidget *parent,TRACE_LS_PARA &P,bool bmenu)
{
    QElapsedTimer timer1;
    timer1.start();

    QString fileOpenName = P.inimg_file;

    if(P.image)
    {
        P.in_sz[0] = P.image->getXDim();
        P.in_sz[1] = P.image->getYDim();
        P.in_sz[2] = P.image->getZDim();
    }else
    {
        if(fileOpenName.endsWith(".tc",Qt::CaseSensitive))
        {
            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            P.in_sz[0] = vim.sz[0];
            P.in_sz[1] = vim.sz[1];
            P.in_sz[2] = vim.sz[2];

        }else if (fileOpenName.endsWith(".raw",Qt::CaseSensitive) || fileOpenName.endsWith(".v3draw",Qt::CaseSensitive))
        {
            unsigned char * datald = 0;
            V3DLONG *in_zz = 0;
            V3DLONG *in_sz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), datald, in_zz, in_sz,datatype,0,0,0,1,1,1))
            {
                return false;
            }
            if(datald) {delete []datald; datald = 0;}
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }else
        {
            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(fileOpenName.toStdString(),in_zz))
            {
                return false;
            }
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }

        LocationSimple t;
        if(P.markerfilename.endsWith(".marker",Qt::CaseSensitive))
        {
            vector<MyMarker> file_inmarkers;
            file_inmarkers = readMarker_file(string(qPrintable(P.markerfilename)));
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x + 1;
                t.y = file_inmarkers[i].y + 1;
                t.z = file_inmarkers[i].z + 1;
                P.listLandmarks.push_back(t);
            }
        }else
        {
            QList<CellAPO> file_inmarkers;
            file_inmarkers = readAPO_file(P.markerfilename);
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x;
                t.y = file_inmarkers[i].y;
                t.z = file_inmarkers[i].z;
                P.listLandmarks.push_back(t);
            }
        }
    }

    LandmarkList allTargetList;
    QList<LandmarkList> allTipsList;

    LocationSimple tileLocation;
    tileLocation.x = P.listLandmarks[0].x;
    tileLocation.y = P.listLandmarks[0].y;
    tileLocation.z = P.listLandmarks[0].z;

    LandmarkList inputRootList;
    inputRootList.push_back(tileLocation);
    allTipsList.push_back(inputRootList);

    tileLocation.x = tileLocation.x -int(P.block_size/2);
    tileLocation.y = tileLocation.y -int(P.block_size/2);
    if(P.tracing_3D)
        tileLocation.z = tileLocation.z -int(P.block_size/2);
    else
        tileLocation.z = 0;
    tileLocation.ev_pc1 = P.block_size;
    tileLocation.ev_pc2 = P.block_size;
    tileLocation.ev_pc3 = P.block_size;

    allTargetList.push_back(tileLocation);

    P.output_folder = QFileInfo(P.markerfilename).path();

    QString tmpfolder;
    if(P.method == neutube)
       tmpfolder = P.output_folder+("/tmp_NEUTUBE");
    else if(P.method == snake)
       tmpfolder = P.output_folder+("/tmp_SNAKE");
    else if(P.method == most)
       tmpfolder = P.output_folder+("/tmp_MOST");
    else if(P.method == neurogpstree)
       tmpfolder = P.output_folder+("/tmp_NeuroGPSTree");
    else if(P.method == advantra)
       tmpfolder = P.output_folder+("/tmp_Advantra");
    else if(P.method == tremap)
       tmpfolder = P.output_folder+("/tmp_TReMAP");
    else if(P.method == mst)
       tmpfolder = P.output_folder+("/tmp_MST");
    else if(P.method == neuronchaser)
       tmpfolder = P.output_folder+("/tmp_NeuronChaser");
    else if(P.method == rivulet2)
       tmpfolder = P.output_folder+("/tmp_Rivulet2");

    if(!tmpfolder.isEmpty())
       system(qPrintable(QString("rm -rf %1").arg(tmpfolder.toStdString().c_str())));

    system(qPrintable(QString("mkdir %1").arg(tmpfolder.toStdString().c_str())));
    if(tmpfolder.isEmpty())
    {
        printf("Can not create a tmp folder!\n");
        return false;
    }

    LandmarkList newTargetList;
    QList<LandmarkList> newTipsList;
    int iii = 0;
//    while(allTargetList.size()>0)
//    {
        iii++;
        newTargetList.clear();
        newTipsList.clear();
        if(P.adap_win)
        {
             if(P.tracing_3D)
                 all_tracing_ada_win_3D(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
             else
                 all_tracing_ada_win(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
        }
        else
            all_tracing(callback,P,allTipsList.at(0),allTargetList.at(0),&newTargetList,&newTipsList);
        allTipsList.removeAt(0);
        allTargetList.removeAt(0);
        if(newTipsList.size()>0)
        {
            for(int i = 0; i < newTipsList.size(); i++)
            {
                allTargetList.push_back(newTargetList.at(i));
                allTipsList.push_back(newTipsList.at(i));
            }

            for(int i = 0; i < allTargetList.size();i++)
            {
                for(int j = 0; j < allTargetList.size();j++)
                {
                    if(allTargetList.at(i).radius > allTargetList.at(j).radius)
                    {
                        allTargetList.swap(i,j);
                        allTipsList.swap(i,j);
                    }
                }
            }
        }

//    }
    qint64 etime1 = timer1.elapsed();

    list<string> infostring;
    string tmpstr; QString qtstr;
    if(P.method == neutube)
    {
        tmpstr =  qPrintable( qtstr.prepend("## UT_NEUTUBE")); infostring.push_back(tmpstr);
    }else if(P.method == snake)
    {
        tmpstr =  qPrintable( qtstr.prepend("## UT_SNAKE")); infostring.push_back(tmpstr);
    }else if(P.method == most)
    {
        tmpstr =  qPrintable( qtstr.prepend("## UT_MOST")); infostring.push_back(tmpstr);
        tmpstr =  qPrintable( qtstr.setNum(P.channel).prepend("#channel = ") ); infostring.push_back(tmpstr);
        tmpstr =  qPrintable( qtstr.setNum(P.bkg_thresh).prepend("#bkg_thresh = ") ); infostring.push_back(tmpstr);

        tmpstr =  qPrintable( qtstr.setNum(P.seed_win).prepend("#seed_win = ") ); infostring.push_back(tmpstr);
        tmpstr =  qPrintable( qtstr.setNum(P.slip_win).prepend("#slip_win = ") ); infostring.push_back(tmpstr);
    }else if(P.method == neurogpstree)
    {
        tmpstr =  qPrintable( qtstr.prepend("## UT_NeuroGPSTree")); infostring.push_back(tmpstr);
    }

    tmpstr =  qPrintable( qtstr.setNum(P.block_size).prepend("#block_size = ") ); infostring.push_back(tmpstr);
    tmpstr =  qPrintable( qtstr.setNum(P.adap_win).prepend("#adaptive_window = ") ); infostring.push_back(tmpstr);
    tmpstr =  qPrintable( qtstr.setNum(etime1).prepend("#neuron preprocessing time (milliseconds) = ") ); infostring.push_back(tmpstr);


    if(P.tracing_3D)
        processSmartScan_3D(callback,infostring,tmpfolder +"/scanData.txt");
    else
        processSmartScan(callback,infostring,tmpfolder +"/scanData.txt");


    v3d_msg(QString("The tracing uses %1 for tracing. Now you can drag and drop the generated swc fle [%2] into Vaa3D."
                    ).arg(etime1).arg(tmpfolder +"/scanData.txt.swc"), bmenu);

    return true;
}

bool all_tracing(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString;
    QString finaloutputswc;
    QString finaloutputswc_left;

    if(P.method == neutube)
    {
        saveDirString = P.output_folder.append("/tmp_NEUTUBE");
        finaloutputswc = P.markerfilename + ("_nc_neutube_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_neutube_adp_left.swc");
    }
    else if (P.method == snake)
    {
        saveDirString = P.output_folder.append("/tmp_SNAKE");
        finaloutputswc = P.markerfilename + ("_nc_snake_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_snake_adp_left.swc");

    }
    else if (P.method == most)
    {
        saveDirString = P.output_folder.append("/tmp_MOST");
        finaloutputswc = P.markerfilename + ("_nc_most_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_most_adp_left.swc");

    }
    else if (P.method == app2)
    {
        saveDirString = P.output_folder.append("/tmp_COMBINED");
        finaloutputswc = P.markerfilename + ("_nc_app2_combined.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_app2_combined_left.swc");

    }
    else if (P.method == neurogpstree)
    {
        saveDirString = P.output_folder.append("/tmp_NeuroGPSTree");
        finaloutputswc = P.markerfilename + ("_nc_NeuroGPSTree_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_NeuroGPSTree_adp_left.swc");
    }
    else if (P.method == advantra)
    {
        saveDirString = P.output_folder.append("/tmp_Advantra");
        finaloutputswc = P.markerfilename + ("_nc_Advantra_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_Advantra_adp_left.swc");
    }
    else if (P.method == tremap)
    {
        saveDirString = P.output_folder.append("/tmp_TReMAP");
        finaloutputswc = P.markerfilename + ("_nc_TReMAP_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_TReMAP_adp_left.swc");
    }
    else if (P.method == mst)
    {
        saveDirString = P.output_folder.append("/tmp_MST");
        finaloutputswc = P.markerfilename + ("_nc_MST_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_MST_adp_left.swc");
    }
    else if (P.method == neuronchaser)
    {
        saveDirString = P.output_folder.append("/tmp_NeuronChaser");
        finaloutputswc = P.markerfilename + ("_nc_NeuronChaser_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_NeuronChaser_adp_left.swc");
    }
    else if (P.method == rivulet2)
    {
        saveDirString = P.output_folder.append("/tmp_Rivulet2");
        finaloutputswc = P.markerfilename + ("_nc_Rivulet2_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_Rivulet2_adp_left.swc");
    }
    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,end_x,end_y;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;

    end_x = tileLocation.x+P.block_size;
    end_y = tileLocation.y+P.block_size;
    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];

    QString imagechecking = saveDirString;

    imagechecking.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));
    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || end_x <= 0 || end_y <= 0 )
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = P.in_sz[2];
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,tileLocation.z,end_x-1,end_y-1,tileLocation.z + P.in_sz[2]-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }

        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,0,
                               end_x,end_y,P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,tileLocation.z,
                               end_x,end_y,tileLocation.z + P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }
            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,0,P.in_sz[2]);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(tileLocation.z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");

    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists())
    {
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc_left.toStdString().c_str())));

    }

    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".swc");

    ifstream ifs_image(imageSaveString.toStdString().c_str());
    if(!ifs_image)
    {
        qDebug()<<scanDataFileString;
        QFile saveTextFile;
        saveTextFile.setFileName(scanDataFileString);// add currentScanFile
        if (!saveTextFile.isOpen()){
            if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
                qDebug()<<"unable to save file!";
                return false;}     }
        QTextStream outputStream;
        outputStream.setDevice(&saveTextFile);
        outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<"\n";
        saveTextFile.close();

        simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

        V3DPluginArgItem arg;
        V3DPluginArgList input;
        V3DPluginArgList output;

        QString full_plugin_name;
        QString func_name;

        arg.type = "random";std::vector<char*> arg_input;
        std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
        arg_input.push_back(fileName_string);
        arg.p = (void *) & arg_input; input<< arg;

//        char* char_swcout =  new char[swcNEUTUBE.length() + 1];strcpy(char_swcout, swcNEUTUBE.toStdString().c_str());
//        arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

        arg.type = "random";
        std::vector<char*> arg_para;

        if(P.method == neutube || P.method == app2)
        {
            arg_para.push_back("1");
            arg_para.push_back("1");
            full_plugin_name = "neuTube";
            func_name =  "neutube_trace";
        }else if(P.method == snake)
        {
            arg_para.push_back("1");
            arg_para.push_back("1");
            full_plugin_name = "snake";
            func_name =  "snake_trace";
        }else if(P.method == most)
        {
            string S_channel = boost::lexical_cast<string>(P.channel);
            char* C_channel = new char[S_channel.length() + 1];
            strcpy(C_channel,S_channel.c_str());
            arg_para.push_back(C_channel);

            string S_background_th = boost::lexical_cast<string>(P.bkg_thresh);
            char* C_background_th = new char[S_background_th.length() + 1];
            strcpy(C_background_th,S_background_th.c_str());
            arg_para.push_back(C_background_th);

            string S_seed_win = boost::lexical_cast<string>(P.seed_win);
            char* C_seed_win = new char[S_seed_win.length() + 1];
            strcpy(C_seed_win,S_seed_win.c_str());
            arg_para.push_back(C_seed_win);

            string S_slip_win = boost::lexical_cast<string>(P.slip_win);
            char* C_slip_win = new char[S_slip_win.length() + 1];
            strcpy(C_slip_win,S_slip_win.c_str());
            arg_para.push_back(C_slip_win);

            full_plugin_name = "mostVesselTracer";
            func_name =  "MOST_trace";
        }else if(P.method == neurogpstree)
        {
            arg_para.push_back("1");
            arg_para.push_back("1");
            arg_para.push_back("1");
            arg_para.push_back("30");
            full_plugin_name = "NeuroGPSTreeOld";
            func_name =  "tracing_func";
        }else if(P.method == advantra)
        {
            full_plugin_name = "region_neuron2";
            func_name =  "trace_advantra";
        }else if(P.method == tremap)
        {
            arg_para.push_back("0");
            arg_para.push_back("1");
            arg_para.push_back("20");
            full_plugin_name = "TReMap";
            func_name =  "trace_mip";
        }else if(P.method == mst)
        {
            full_plugin_name = "MST_tracing";
            func_name =  "trace_mst";
        }else if(P.method == neuronchaser)
        {
            full_plugin_name = "region_neuron2";
            func_name =  "trace_neuronchaser";
        }else if(P.method == rivulet2)
        {
            arg_para.push_back("1");
            arg_para.push_back("30");
            full_plugin_name = "Rivulet";
            func_name =  "tracing_func";
        }

        if(P.method == tremap)
        {
        #if  defined(Q_OS_LINUX)
            QString cmd_tremap = QString("%1/vaa3d -x TReMap -f trace_mip -i %2 -p 0 1 20").arg(getAppPath().toStdString().c_str()).arg(imageSaveString.toStdString().c_str());
            system(qPrintable(cmd_tremap));
        #elif defined(Q_OS_MAC)
            QString cmd_tremap = QString("%1/vaa3d64.app/Contents/MacOS/vaa3d64 -x TReMap -f trace_mip -i %2 -p 0 1 20").arg(getAppPath().toStdString().c_str()).arg(imageSaveString.toStdString().c_str());
            system(qPrintable(cmd_tremap));
        #else
            v3d_msg("The OS is not Linux or Mac. Do nothing.");
            return;
        #endif

        }else
        {
            arg.p = (void *) & arg_para; input << arg;

            if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
            {

                printf("Can not find the tracing plugin!\n");
                return false;
            }
        }
    }

    NeuronTree nt_neutube;
    QString swcNEUTUBE = saveDirString;
    if(P.method == neutube || P.method == app2)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_neutube.swc");
    else if (P.method == snake)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_snake.swc");
    else if (P.method == most)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_MOST.swc");
    else if (P.method == neurogpstree)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_NeuroGPSTree.swc");
    else if (P.method == advantra)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_region_Advantra.swc");
    else if (P.method == tremap)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_XY_3D_TreMap.swc");
    else if (P.method == mst)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_MST_Tracing.swc");
    else if (P.method == neuronchaser)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_region_NeuronChaser.swc");
    else if (P.method == rivulet2)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw.r2.swc");

    nt_neutube = readSWC_file(swcNEUTUBE);



//#if  defined(Q_OS_LINUX)
//    QString cmd_DL = QString("%1/vaa3d -x prediction_caffe -f Quality_Assess -i %2 -p %3 /local4/Data/IVSCC_test/comparison/Caffe_testing_3rd/train_package_4th/deploy.prototxt /local4/Data/IVSCC_test/comparison/Caffe_testing_3rd/train_package_4th/caffenet_train_iter_390000.caffemodel /local4/Data/IVSCC_test/comparison/Caffe_testing_3rd/train_package_4th/imagenet_mean.binaryproto").
//            arg(getAppPath().toStdString().c_str()).arg(imageSaveString.toStdString().c_str()).arg(swcNEUTUBE.toStdString().c_str());
//    system(qPrintable(cmd_DL));
//#else
//    v3d_msg("The OS is not Linux or Mac. Do nothing.");
//    return;
//#endif

//    QString fp_marker = imageSaveString + (".swc_fp.marker");
//    QList <ImageMarker> fp_marklist =  readMarker_file(fp_marker);
//    NeuronTree nt_neutube_DL = DL_eliminate_swc(nt_neutube,fp_marklist);
//    QString swcDL = imageSaveString + ("_DL.swc");
//    QList<NeuronSWC> nt_neutube_DL_sorted;
//    if (!SortSWC(nt_neutube_DL.listNeuron, nt_neutube_DL_sorted,VOID, 0))
//    {
//        v3d_msg("fail to call swc sorting function.",0);
//        return false;
//    }

//    export_list2file(nt_neutube_DL_sorted, swcDL,swcDL);
//    nt_neutube_DL = readSWC_file(swcDL);


//    QVector<QVector<V3DLONG> > children;
//    V3DLONG neuronNum = nt_neutube_DL.listNeuron.size();
//    children = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
//    for (V3DLONG i=0;i<neuronNum;i++)
//    {
//        V3DLONG par = nt_neutube_DL.listNeuron[i].pn;
//        if (par<0) continue;
//        children[nt_neutube_DL.hashNeuron.value(par)].push_back(i);
//    }

//    for (V3DLONG i=nt_neutube_DL.listNeuron.size()-1;i>=0;i--)
//    {
//        if(nt_neutube_DL.listNeuron[i].pn < 0 && children[i].size()==0)
//            nt_neutube_DL.listNeuron.removeAt(i);
//    }

    NeuronTree nt;
    //nt = nt_neutube;
    //combine_list2file(nt.listNeuron, swcString);
    ifstream ifs_swcString(swcString.toStdString().c_str());
    if(!ifs_swcString)
    {
      //  nt = sort_eliminate_swc(nt_neutube_DL,inputRootList,total4DImage);
        nt = sort_eliminate_swc(nt_neutube,inputRootList,total4DImage);
        export_list2file(nt.listNeuron, swcString,swcNEUTUBE);

    }else
    {
        NeuronTree nt_tile = readSWC_file(swcString);
        LandmarkList inputRootList_pruned = eliminate_seed(nt_tile,inputRootList,total4DImage);
        if(inputRootList_pruned.size()<1)
            return true;
        else
        {
           // nt = sort_eliminate_swc(nt_neutube_DL,inputRootList_pruned,total4DImage);
            nt = sort_eliminate_swc(nt_neutube,inputRootList_pruned,total4DImage);
            combine_list2file(nt.listNeuron, swcString);

        }
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    QList<NeuronSWC> list = nt.listNeuron;
    for (V3DLONG i=0;i<list.size();i++)
    {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
            }
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }
    }

    double overlap = 0.1;
    LocationSimple newTarget;
    if(tip_left.size()>0)
    {
        newTipsList->push_back(tip_left);
        newTarget.x = -floor(P.block_size*(1.0-overlap)) + tileLocation.x;
        newTarget.y = tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTargetList->push_back(newTarget);

    }
    if(tip_right.size()>0)
    {
        newTipsList->push_back(tip_right);
        newTarget.x = floor(P.block_size*(1.0-overlap)) + tileLocation.x;
        newTarget.y = tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTargetList->push_back(newTarget);

    }
    if(tip_up.size()>0)
    {
        newTipsList->push_back(tip_up);
        newTarget.x = tileLocation.x;
        newTarget.y = -floor(P.block_size*(1.0-overlap)) + tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTargetList->push_back(newTarget);

    }
    if(tip_down.size()>0)
    {
        newTipsList->push_back(tip_down);
        newTarget.x = tileLocation.x;
        newTarget.y = floor(P.block_size*(1.0-overlap)) + tileLocation.y;
        newTarget.z = total4DImage->getOriginZ();
        newTargetList->push_back(newTarget);

    }

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    vector<MyMarker*> tileswc_file = readSWC_file(swcString.toStdString());
    vector<MyMarker*> finalswc;

    if(ifs_swc)
    {
        finalswc = readSWC_file(finaloutputswc.toStdString());
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }


//    NeuronTree finalswc;
//    NeuronTree finalswc_left;
//    vector<QList<NeuronSWC> > nt_list;
//    vector<QList<NeuronSWC> > nt_list_left;

//    if(ifs_swc)
//    {
//       finalswc = readSWC_file(finaloutputswc);
//       nt_list.push_back(finalswc.listNeuron);
//       finalswc_left = readSWC_file(finaloutputswc_left);
//       nt_list_left.push_back(finalswc_left.listNeuron);
//    }


//    NeuronTree nt_traced = readSWC_file(swcString);
//    NeuronTree nt_left = neuron_sub(nt_neutube, nt_traced);

//    if(ifs_swc)
//    {
//        for(V3DLONG i = 0; i < nt.listNeuron.size(); i++)
//        {
//            nt.listNeuron[i].x = nt.listNeuron[i].x + total4DImage->getOriginX();
//            nt.listNeuron[i].y = nt.listNeuron[i].y + total4DImage->getOriginY();
//            nt.listNeuron[i].z = nt.listNeuron[i].z + total4DImage->getOriginZ();
//            nt.listNeuron[i].type = 3;
//        }
//        nt_list.push_back(nt.listNeuron);
//        QList<NeuronSWC> finalswc_updated;
//        if (combine_linker(nt_list, finalswc_updated))
//        {
//            export_list2file(finalswc_updated, finaloutputswc,finaloutputswc);
//        }

//        for(V3DLONG i = 0; i < nt_left.listNeuron.size(); i++)
//        {
//            NeuronSWC curr = nt_left.listNeuron.at(i);
//            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
//            {
//                nt_left.listNeuron[i].type = 1;
//            }else
//                nt_left.listNeuron[i].type = 2;

//            nt_left.listNeuron[i].x = curr.x + total4DImage->getOriginX();
//            nt_left.listNeuron[i].y = curr.y + total4DImage->getOriginY();
//            nt_left.listNeuron[i].z = curr.z + total4DImage->getOriginZ();
//        }

//        if(!ifs_image)
//        {
//            QList<NeuronSWC> nt_left_sorted;
//            if(SortSWC(nt_left.listNeuron, nt_left_sorted,VOID, 0))
//                nt_list_left.push_back(nt_left_sorted);

//            QList<NeuronSWC> finalswc_left_updated_added;
//            if (combine_linker(nt_list_left, finalswc_left_updated_added))
//            {
//                export_list2file(finalswc_left_updated_added, finaloutputswc_left,finaloutputswc_left);
//            }
//        }else
//        {
//            NeuronTree finalswc_left_updated_minus = neuron_sub(finalswc_left, nt);
//            export_list2file(finalswc_left_updated_minus.listNeuron, finaloutputswc_left,finaloutputswc_left);
//        }
//    }
//    else
//    {
//        for(V3DLONG i = 0; i < nt.listNeuron.size(); i++)
//        {
//            nt.listNeuron[i].x = nt.listNeuron[i].x + total4DImage->getOriginX();
//            nt.listNeuron[i].y = nt.listNeuron[i].y + total4DImage->getOriginY();
//            nt.listNeuron[i].z = nt.listNeuron[i].z + total4DImage->getOriginZ();
//            nt.listNeuron[i].type = 3;
//        }
//        export_list2file(nt.listNeuron, finaloutputswc,finaloutputswc);

//        for(V3DLONG i = 0; i < nt_left.listNeuron.size(); i++)
//        {
//            NeuronSWC curr = nt_left.listNeuron.at(i);
//            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
//            {
//                nt_left.listNeuron[i].type = 1;
//            }else
//                nt_left.listNeuron[i].type = 2;

//            nt_left.listNeuron[i].x = curr.x + total4DImage->getOriginX();
//            nt_left.listNeuron[i].y = curr.y + total4DImage->getOriginY();
//            nt_left.listNeuron[i].z = curr.z + total4DImage->getOriginZ();
//        }
//        QList<NeuronSWC> nt_left_sorted;
//        if(SortSWC(nt_left.listNeuron, nt_left_sorted,VOID, 0))
//            export_list2file(nt_left_sorted, finaloutputswc_left,finaloutputswc_left);
//    }


    total4DImage->deleteRawDataAndSetPointerToNull();
    return true;
}

bool all_tracing_ada_win(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString;
    QString finaloutputswc;
    QString finaloutputswc_left;

    if(P.method == neutube)
    {
        saveDirString = P.output_folder.append("/tmp_NEUTUBE");
        finaloutputswc = P.markerfilename + ("_nc_neutube_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_neutube_adp_left.swc");
    }
    else if (P.method == snake)
    {
        saveDirString = P.output_folder.append("/tmp_SNAKE");
        finaloutputswc = P.markerfilename + ("_nc_snake_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_snake_adp_left.swc");
    }
    else if (P.method == most)
    {
        saveDirString = P.output_folder.append("/tmp_MOST");
        finaloutputswc = P.markerfilename + ("_nc_most_adp.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_most_adp_left.swc");

    }
    else if (P.method == app2)
    {
        saveDirString = P.output_folder.append("/tmp_COMBINED");
        finaloutputswc = P.markerfilename + ("_nc_app2_combined.swc");
        finaloutputswc_left = P.markerfilename + ("_nc_app2_combined_left.swc");

    }

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,end_x,end_y;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;

    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || end_x <= 0 || end_y <= 0 )
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = P.in_sz[2];
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,tileLocation.z,end_x-1,end_y-1,tileLocation.z + P.in_sz[2]-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }

        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,tileLocation.z,
                               end_x,end_y,tileLocation.z + P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }
            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,0,P.in_sz[2]);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(tileLocation.z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");

    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists())
    {
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc_left.toStdString().c_str())));
    }

    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".swc");

    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

    V3DPluginArgItem arg;
    V3DPluginArgList input;
    V3DPluginArgList output;

    QString full_plugin_name;
    QString func_name;

    arg.type = "random";std::vector<char*> arg_input;
    std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
    arg_input.push_back(fileName_string);
    arg.p = (void *) & arg_input; input<< arg;

    char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
    arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

    arg.type = "random";
    std::vector<char*> arg_para;

    if(P.method == neutube || P.method == app2)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "neuTube";
        func_name =  "neutube_trace";
    }else if(P.method == snake)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "snake";
        func_name =  "snake_trace";
    }else if(P.method == most )
    {
        string S_channel = boost::lexical_cast<string>(P.channel);
        char* C_channel = new char[S_channel.length() + 1];
        strcpy(C_channel,S_channel.c_str());
        arg_para.push_back(C_channel);

        string S_background_th = boost::lexical_cast<string>(P.bkg_thresh);
        char* C_background_th = new char[S_background_th.length() + 1];
        strcpy(C_background_th,S_background_th.c_str());
        arg_para.push_back(C_background_th);

        string S_seed_win = boost::lexical_cast<string>(P.seed_win);
        char* C_seed_win = new char[S_seed_win.length() + 1];
        strcpy(C_seed_win,S_seed_win.c_str());
        arg_para.push_back(C_seed_win);

        string S_slip_win = boost::lexical_cast<string>(P.slip_win);
        char* C_slip_win = new char[S_slip_win.length() + 1];
        strcpy(C_slip_win,S_slip_win.c_str());
        arg_para.push_back(C_slip_win);

        full_plugin_name = "mostVesselTracer";
        func_name =  "MOST_trace";
    }

    arg.p = (void *) & arg_para; input << arg;

    if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
    {

        printf("Can not find the tracing plugin!\n");
        return false;
    }


    NeuronTree nt_neutube;
    QString swcNEUTUBE = saveDirString;
    if(P.method == neutube || P.method == app2)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_neutube.swc");
    else if (P.method == snake )
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_snake.swc");
    else if (P.method == most )
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_MOST.swc");

    nt_neutube = readSWC_file(swcNEUTUBE);
    if(nt_neutube.listNeuron.size() ==0)
        return true;

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    NeuronTree finalswc;
    NeuronTree finalswc_left;
    vector<QList<NeuronSWC> > nt_list;
    vector<QList<NeuronSWC> > nt_list_left;

    if(ifs_swc)
    {
       finalswc = readSWC_file(finaloutputswc);
       finalswc_left = readSWC_file(finaloutputswc_left);
    }


    NeuronTree nt;
    ifstream ifs_swcString(swcString.toStdString().c_str());
    if(!ifs_swcString)
    {
        nt = sort_eliminate_swc(nt_neutube,inputRootList,total4DImage);
        export_list2file(nt.listNeuron, swcString,swcNEUTUBE);

    }else
    {
        NeuronTree nt_tile = readSWC_file(swcString);
        LandmarkList inputRootList_pruned = eliminate_seed(nt_tile,inputRootList,total4DImage);
        if(inputRootList_pruned.size()<1)
            return true;
        else
        {
            nt = sort_eliminate_swc(nt_neutube,inputRootList_pruned,total4DImage);
            combine_list2file(nt.listNeuron, swcString);

        }
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    QList<NeuronSWC> list = nt.listNeuron;
    LandmarkList tip_visited;
    for (V3DLONG i=0;i<list.size();i++)
    {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            bool check_tip = false, check_visited= false;

            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
                newTip.radius = curr.r;
                for(V3DLONG j = 0; j < finalswc.listNeuron.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.listNeuron.at(j).x) + pow2(newTip.y - finalswc.listNeuron.at(j).y) + pow2(newTip.z - finalswc.listNeuron.at(j).z));
                    if(dis < 10)
                    {
                        check_tip = true;
                        break;
                    }
                }

                if(!check_tip)
                {
                    for(V3DLONG j = 0; j < finalswc_left.listNeuron.size(); j++ )
                    {
                        double dis = sqrt(pow2(newTip.x - finalswc_left.listNeuron.at(j).x) + pow2(newTip.y - finalswc_left.listNeuron.at(j).y) + pow2(newTip.z - finalswc_left.listNeuron.at(j).z));
                        if(dis < 10)
                        {
                            check_visited = true;
                            tip_visited.push_back(newTip);
                            break;
                        }
                    }
                }
            }
            if(check_tip || check_visited) continue;
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,256,1);
        for(int i = 0; i < group_tips_left.size();i++)
            ada_win_finding(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,256,2);
        for(int i = 0; i < group_tips_right.size();i++)
            ada_win_finding(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
    }

    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,256,3);
        for(int i = 0; i < group_tips_up.size();i++)
            ada_win_finding(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);

    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,256,4);
        for(int i = 0; i < group_tips_down.size();i++)
            ada_win_finding(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
    }



  //  vector<MyMarker*> tileswc_file = readSWC_file(swcString.toStdString());
    NeuronTree nt_traced = readSWC_file(swcString);
    NeuronTree nt_left = neuron_sub(nt_neutube, nt_traced);

    if(ifs_swc)
    {
        nt_list.push_back(finalswc.listNeuron);
        NeuronTree nt_visited;
        NeuronTree finalswc_left_nonvisited;
        if(tip_visited.size()>0)
        {
            nt_visited = sort_eliminate_swc(finalswc_left,tip_visited,total4DImage);
        }
        if(nt_visited.listNeuron.size()>0)
        {
            finalswc_left_nonvisited = neuron_sub(finalswc_left,nt_visited);
            nt_list_left.push_back(finalswc_left_nonvisited.listNeuron);
        }else
            nt_list_left.push_back(finalswc_left.listNeuron);

        for(V3DLONG i = 0; i < nt.listNeuron.size(); i++)
        {
            nt.listNeuron[i].x = nt.listNeuron[i].x + total4DImage->getOriginX();
            nt.listNeuron[i].y = nt.listNeuron[i].y + total4DImage->getOriginY();
            nt.listNeuron[i].z = nt.listNeuron[i].z + total4DImage->getOriginZ();
            nt.listNeuron[i].type = 3;
        }
        nt_list.push_back(nt.listNeuron);
        QList<NeuronSWC> finalswc_updated;
        if (combine_linker(nt_list, finalswc_updated))
        {
            export_list2file(finalswc_updated, finaloutputswc,finaloutputswc);
        }

        if(nt_left.listNeuron.size()>0)
        {
            v3d_msg("check1",0);
            for(V3DLONG i = 0; i < nt_left.listNeuron.size(); i++)
            {
                NeuronSWC curr = nt_left.listNeuron.at(i);
                if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
                {
                    nt_left.listNeuron[i].type = 1;
                }else
                    nt_left.listNeuron[i].type = 2;

                nt_left.listNeuron[i].x = curr.x + total4DImage->getOriginX();
                nt_left.listNeuron[i].y = curr.y + total4DImage->getOriginY();
                nt_left.listNeuron[i].z = curr.z + total4DImage->getOriginZ();
            }

            NeuronTree nt_left_left = neuron_sub(nt_left, finalswc);
            v3d_msg("check2",0);

            if(nt_left_left.listNeuron.size()>0)
            {
                QList<NeuronSWC> nt_left_sorted;
                if(SortSWC(nt_left_left.listNeuron, nt_left_sorted,VOID, 0))
                    nt_list_left.push_back(nt_left_sorted);
                v3d_msg("check3",0);


                QList<NeuronSWC> finalswc_left_updated_added;
                if (combine_linker(nt_list_left, finalswc_left_updated_added))
                {
                    QList<NeuronSWC> finalswc_left_updated_added_sorted;
                    v3d_msg("check4",0);

                    if(SortSWC(finalswc_left_updated_added, finalswc_left_updated_added_sorted,VOID, 10))
                        export_list2file(finalswc_left_updated_added_sorted, finaloutputswc_left,finaloutputswc_left);
                }
            }
        }else if(nt_visited.listNeuron.size()>0)
        {
            export_list2file(finalswc_left_nonvisited.listNeuron, finaloutputswc_left,finaloutputswc_left);
        }

    }
    else
    {
        for(V3DLONG i = 0; i < nt.listNeuron.size(); i++)
        {
            nt.listNeuron[i].x = nt.listNeuron[i].x + total4DImage->getOriginX();
            nt.listNeuron[i].y = nt.listNeuron[i].y + total4DImage->getOriginY();
            nt.listNeuron[i].z = nt.listNeuron[i].z + total4DImage->getOriginZ();
            nt.listNeuron[i].type = 3;
        }
        export_list2file(nt.listNeuron, finaloutputswc,finaloutputswc);

        for(V3DLONG i = 0; i < nt_left.listNeuron.size(); i++)
        {
            NeuronSWC curr = nt_left.listNeuron.at(i);
            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                nt_left.listNeuron[i].type = 1;
            }else
                nt_left.listNeuron[i].type = 2;

            nt_left.listNeuron[i].x = curr.x + total4DImage->getOriginX();
            nt_left.listNeuron[i].y = curr.y + total4DImage->getOriginY();
            nt_left.listNeuron[i].z = curr.z + total4DImage->getOriginZ();
        }

        QList<NeuronSWC> nt_left_sorted;
        if(SortSWC(nt_left.listNeuron, nt_left_sorted,VOID, 0))
            export_list2file(nt_left_sorted, finaloutputswc_left,finaloutputswc_left);
    }

//    QString marker_name = imageSaveString + ".marker";
//    QList<ImageMarker> seedsToSave;
//    for(V3DLONG i = 0; i<inputRootList.size();i++)
//    {
//        ImageMarker outputMarker;
//        outputMarker.x = inputRootList.at(i).x;
//        outputMarker.y = inputRootList.at(i).y;
//        outputMarker.z = inputRootList.at(i).z;
//        seedsToSave.append(outputMarker);
//    }

//    writeMarker_file(marker_name, seedsToSave);
    total4DImage->deleteRawDataAndSetPointerToNull();
    return true;
}

bool all_tracing_ada_win_3D(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString;
    QString finaloutputswc;
    if(P.method == neutube)
    {
        saveDirString = P.output_folder.append("/tmp_NEUTUBE");
        finaloutputswc = P.markerfilename + ("_nc_neutube_adp_3D.swc");
    }
    else if (P.method == snake)
    {
        saveDirString = P.output_folder.append("/tmp_SNAKE");
        finaloutputswc = P.markerfilename + ("_nc_snake_adp_3D.swc");
    }
    else if (P.method == most)
    {
        saveDirString = P.output_folder.append("/tmp_MOST");
        finaloutputswc = P.markerfilename + ("_nc_most_adp_3D.swc");
    }
    else if (P.method == app2)
    {
        saveDirString = P.output_folder.append("/tmp_COMBINED");
        finaloutputswc = P.markerfilename + ("_nc_app2_adp_3D.swc");
    }

    if(P.global_name)
        finaloutputswc = P.output_folder.append("/nc_APP2_GD.swc");

    QString imageSaveString = saveDirString;
    V3DLONG start_x,start_y,start_z,end_x,end_y,end_z;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;
    start_z = (tileLocation.z < 0)?  0 : tileLocation.z;

    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    end_z = tileLocation.z+tileLocation.ev_pc3;

    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];
    if(end_z > P.in_sz[2]) end_z = P.in_sz[2];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || tileLocation.z >= P.in_sz[2] - 1 || end_x <= 0 || end_y <= 0 || end_z <= 0)
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = end_z - start_z;
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,start_z,end_x-1,end_y-1,end_z-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,start_z,
                               end_x,end_y,end_z))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }
            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3] || P.channel ==0)
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,start_z,end_z);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(start_z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    total4DImage->setRezZ(3.0);//set the flg for 3d crawler

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw");

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");

    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));

    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".swc");

    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<< (int) total4DImage->getOriginZ()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<" "<< (int) in_sz[2]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    vector<MyMarker*> finalswc;

    if(ifs_swc)
       finalswc = readSWC_file(finaloutputswc.toStdString());

    V3DPluginArgItem arg;
    V3DPluginArgList input;
    V3DPluginArgList output;

    QString full_plugin_name;
    QString func_name;

    arg.type = "random";std::vector<char*> arg_input;
    std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
    arg_input.push_back(fileName_string);
    arg.p = (void *) & arg_input; input<< arg;

    char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
    arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

    arg.type = "random";
    std::vector<char*> arg_para;

    if(P.method == neutube || P.method == app2)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "neuTube";
        func_name =  "neutube_trace";
    }else if(P.method == snake)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "snake";
        func_name =  "snake_trace";
    }else if(P.method == most )
    {
        string S_channel = boost::lexical_cast<string>(P.channel);
        char* C_channel = new char[S_channel.length() + 1];
        strcpy(C_channel,S_channel.c_str());
        arg_para.push_back(C_channel);

        string S_background_th = boost::lexical_cast<string>(P.bkg_thresh);
        char* C_background_th = new char[S_background_th.length() + 1];
        strcpy(C_background_th,S_background_th.c_str());
        arg_para.push_back(C_background_th);

        string S_seed_win = boost::lexical_cast<string>(P.seed_win);
        char* C_seed_win = new char[S_seed_win.length() + 1];
        strcpy(C_seed_win,S_seed_win.c_str());
        arg_para.push_back(C_seed_win);

        string S_slip_win = boost::lexical_cast<string>(P.slip_win);
        char* C_slip_win = new char[S_slip_win.length() + 1];
        strcpy(C_slip_win,S_slip_win.c_str());
        arg_para.push_back(C_slip_win);

        full_plugin_name = "mostVesselTracer";
        func_name =  "MOST_trace";
    }

    arg.p = (void *) & arg_para; input << arg;

    if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
    {

        printf("Can not find the tracing plugin!\n");
        return false;
    }

    NeuronTree nt_neutube;
    QString swcNEUTUBE = saveDirString;
    if(P.method == neutube || P.method == app2)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_neutube.swc");
    else if (P.method == snake )
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_snake.swc");
    else if (P.method == most )
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_MOST.swc");

    nt_neutube = readSWC_file(swcNEUTUBE);
    if(nt_neutube.listNeuron.size() ==0)
        return true;

//    NeuronTree nt_pruned = pruneswc(nt_neutube, 2);
//    NeuronTree nt_pruned_rs = resample(nt_pruned, 10);
    QString outfilename = swcNEUTUBE;// + "_connected.swc";
//    QList<NeuronSWC> newNeuron;
//    connect_swc(nt_pruned_rs,newNeuron,120,120);
//    export_list2file(newNeuron, outfilename, swcNEUTUBE);


    nt_neutube = readSWC_file(outfilename);
    NeuronTree nt;
    ifstream ifs_swcString(swcString.toStdString().c_str());
    if(!ifs_swcString)
    {
        export_list2file(nt_neutube.listNeuron, swcString,swcNEUTUBE);
        return true;

        nt = sort_eliminate_swc(nt_neutube,inputRootList,total4DImage);
        export_list2file(nt.listNeuron, swcString,swcNEUTUBE);

    }else
    {
        NeuronTree nt_tile = readSWC_file(swcString);
        LandmarkList inputRootList_pruned = eliminate_seed(nt_tile,inputRootList,total4DImage);
        if(inputRootList_pruned.size()<1)
            return true;
        else
        {
            nt = sort_eliminate_swc(nt_neutube,inputRootList_pruned,total4DImage);
            combine_list2file(nt.listNeuron, swcString);

        }
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    LandmarkList tip_out;
    LandmarkList tip_in;

    double ratio = 0.1;
    QList<NeuronSWC> list = nt.listNeuron;
    for (V3DLONG i=0;i<list.size();i++)
    {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            bool check_tip = false;

            if( curr.x < ratio*  total4DImage->getXDim() || curr.x > (1 - ratio) *  total4DImage->getXDim() || curr.y < ratio * total4DImage->getYDim() || curr.y > (1 - ratio)* total4DImage->getYDim()
                   || curr.z < ratio*  total4DImage->getZDim() || curr.z > (1 - ratio) *  total4DImage->getZDim())
            {
                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
                newTip.radius = curr.r;
                for(V3DLONG j = 0; j < finalswc.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.at(j)->x) + pow2(newTip.y - finalswc.at(j)->y) + pow2(newTip.z - finalswc.at(j)->z));
                    if(dis < 20)
                    {
                        check_tip = true;
                        break;
                    }
                }
            }
            if(check_tip) continue;
            if( curr.x < ratio* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > (1 - ratio) * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < ratio * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > (1 - ratio)*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }else if (curr.z < ratio * total4DImage->getZDim())
            {
                tip_out.push_back(newTip);
            }else if (curr.z > (1 - ratio)*total4DImage->getZDim())
            {
                tip_in.push_back(newTip);
            }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,256,1);
        for(int i = 0; i < group_tips_left.size();i++)
            ada_win_finding_3D(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,256,2);
        for(int i = 0; i < group_tips_right.size();i++)
            ada_win_finding_3D(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
    }
    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,256,3);
        for(int i = 0; i < group_tips_up.size();i++)
            ada_win_finding_3D(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);
    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,256,4);
        for(int i = 0; i < group_tips_down.size();i++)
            ada_win_finding_3D(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
    }

    if(tip_out.size()>0)
    {
        QList<LandmarkList> group_tips_out = group_tips(tip_out,256,5);
        for(int i = 0; i < group_tips_out.size();i++)
            ada_win_finding_3D(group_tips_out.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,5);
    }

    if(tip_in.size()>0)
    {
        QList<LandmarkList> group_tips_in = group_tips(tip_in,256,6);
        for(int i = 0; i < group_tips_in.size();i++)
            ada_win_finding_3D(group_tips_in.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,6);
    }

    vector<MyMarker*> tileswc_file = readSWC_file(swcString.toStdString());

    if(ifs_swc)
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }

    total4DImage->deleteRawDataAndSetPointerToNull();
    return true;
}


NeuronTree sort_eliminate_swc(NeuronTree nt,LandmarkList inputRootList,Image4DSimple* total4DImage)
{
    NeuronTree nt_result;
    NeuronTree nt_resampled = nt;//resample(nt, 10);
    QList<NeuronSWC> neuron_sorted;

    if (!SortSWC(nt_resampled.listNeuron, neuron_sorted,VOID, 10))  //was 10
    {
        v3d_msg("fail to call swc sorting function.",0);
        return nt_result;
    }

    V3DLONG neuronNum = neuron_sorted.size();
    V3DLONG *flag = new V3DLONG[neuronNum];
//    if(inputRootList.size() == 1 && int(inputRootList.at(0).x - total4DImage->getOriginX()) == int(inputRootList.at(0).y - total4DImage->getOriginY()))
//    {
//        for (V3DLONG i=0;i<neuronNum;i++)
//        {
//            flag[i] = 1;
//        }
//    }
//    else
//    {
        for (V3DLONG i=0;i<neuronNum;i++)
        {
            flag[i] = 0;
        }

        int root_index = -1;
        for(V3DLONG i = 0; i<inputRootList.size();i++)
        {
            int marker_x = inputRootList.at(i).x - total4DImage->getOriginX();
            int marker_y = inputRootList.at(i).y - total4DImage->getOriginY();
            int marker_z = inputRootList.at(i).z - total4DImage->getOriginZ();
            for(V3DLONG j = 0; j<neuronNum;j++)
            {
                NeuronSWC curr = neuron_sorted.at(j);
                if(curr.pn < 0) root_index = j;
                double dis = sqrt(pow2(marker_x - curr.x) + pow2(marker_y - curr.y) + pow2(marker_z - curr.z));

                if(dis < 10 && flag[j] ==0)
                {
                    flag[root_index] = 1;
                    V3DLONG d;
                    for( d = root_index+1; d < neuronNum; d++)
                    {
                        if(neuron_sorted.at(d).pn < 0)
                            break;
                        else
                            flag[d] = 1;

                    }
                    break;
                }

            }
        }


//        V3DLONG counter = 0;
//        V3DLONG root_ID = 0;
//        for (V3DLONG i=1;i<neuronNum;i++)
//        {
//            if(neuron_sorted.at(i).parent > 0)
//            {
//                counter++;
//            }else
//            {
//                bool flag_soma = false;
//                for(V3DLONG j=root_ID; j<=root_ID+counter;j++)
//                {
//                    NeuronSWC curr = neuron_sorted.at(j);
//                    if(curr.radius >=10)
//                    {
//                        flag_soma = true;
//                        break;
//                    }
//                }

//                if(flag_soma)
//                {
//                    for(V3DLONG j=root_ID; j<=root_ID+counter;j++)
//                    {
//                        flag[j] = 1;
//                        if(flag_soma)
//                        {
//                            neuron_sorted[j].type = 0;
//                        }
//                    }
//                }

//                counter = 0;
//                root_ID = i;
//            }
//        }
//    }

    QList <NeuronSWC> listNeuron;
    QHash <int, int>  hashNeuron;
    listNeuron.clear();
    hashNeuron.clear();
    //set node
    NeuronSWC S;
    for (V3DLONG i=0;i<neuron_sorted.size();i++)
    {
        if(flag[i] == 1)
        {
            NeuronSWC curr = neuron_sorted.at(i);
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
    nt_result.n = -1;
    nt_result.on = true;
    nt_result.listNeuron = listNeuron;
    nt_result.hashNeuron = hashNeuron;
    if(flag) {delete[] flag; flag = 0;}

    return nt_result;
}

LandmarkList eliminate_seed(NeuronTree nt,LandmarkList inputRootList,Image4DSimple* total4DImage)
{
    LandmarkList inputRootList_pruned;
    V3DLONG neuronNum = nt.listNeuron.size();


    for(V3DLONG i = 0; i<inputRootList.size();i++)
    {
        int marker_x = inputRootList.at(i).x - total4DImage->getOriginX();
        int marker_y = inputRootList.at(i).y - total4DImage->getOriginY();
        int marker_z = inputRootList.at(i).z - total4DImage->getOriginZ();

        bool flag = false;
        for(V3DLONG j = 0; j<neuronNum;j++)
        {
            NeuronSWC curr = nt.listNeuron.at(j);
            double dis = sqrt(pow2(marker_x - curr.x) + pow2(marker_y - curr.y) + pow2(marker_z - curr.z));

            if(dis < 20)
            {
                flag = true;
                break;
            }

        }
        if(!flag)
            inputRootList_pruned.push_back(inputRootList.at(i));

    }
    return inputRootList_pruned;
}

bool combine_list2file(QList<NeuronSWC> & lN, QString fileSaveName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::Text|QIODevice::Append))
        return false;
    QTextStream myfile(&file);
    for (V3DLONG i=0;i<lN.size();i++)
        myfile << lN.at(i).n <<" " << lN.at(i).type << " "<< lN.at(i).x <<" "<<lN.at(i).y << " "<< lN.at(i).z << " "<< lN.at(i).r << " " <<lN.at(i).pn << "\n";

    file.close();
    cout<<"swc file "<<fileSaveName.toStdString()<<" has been generated, size: "<<lN.size()<<endl;
    return true;
};

bool ada_win_finding(LandmarkList tips,LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList,Image4DSimple* total4DImage,int block_size,int direction)
{
    newTipsList->push_back(tips);
    double overlap = 0.1;

    float min_y = INF, max_y = -INF;
    float min_x = INF, max_x = -INF;

    double adaptive_size;
    double max_r = -INF;

   // double window_size[6] = {1023.999814,698.1053667,490.1445791,351.6972784,256.7910738,194.147554};  //

  //  double window_size[6] = {1024.000304,585.8401591,352.995627,233.5811987,168.0319363,128.6192806};  //mouse

  //  double distance_soma = 0;

    if(direction == 1 || direction == 2)
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).y <= min_y) min_y = tips.at(i).y;
            if(tips.at(i).y >= max_y) max_y = tips.at(i).y;
            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }
        adaptive_size = (max_y - min_y)*1.2;

    }else
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).x <= min_x) min_x = tips.at(i).x;
            if(tips.at(i).x >= max_x) max_x = tips.at(i).x;
            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }
        adaptive_size = (max_x - min_x)*1.2;
    }

//    for(int i = 0; i<tips.size();i++)
//    {
//        double x1 = tileLocation.x + tips.at(i).x;
//        double y1 = tileLocation.y + tips.at(i).y;
//        double z1 = tips.at(i).z;
//      //  distance_soma += sqrt(pow2(0.24*(x1 -1038.555)) + pow2(0.24*(y1 - 1237.994)) + pow2(0.42*(z1 - 23.044))); //
//        distance_soma += sqrt(pow2(0.143*(x1 -1745.392)) + pow2(0.143*(y1 - 1607.064)) + pow2(0.28*(z1 - 61.541)));
//    }

//    double avarge_distance = distance_soma/tips.size();
//    int index_dis = floor (avarge_distance/50);

//    if(index_dis>5) index_dis = 5;
//    if(adaptive_size <= window_size[index_dis])
//        adaptive_size = window_size[index_dis];


    if(adaptive_size <= 256) adaptive_size = 256;
    if(adaptive_size >= block_size) adaptive_size = block_size;

    LocationSimple newTarget;

    if(direction == 1)
    {
        newTarget.x = -floor(adaptive_size*(1.0-overlap)) + tileLocation.x;
        newTarget.y = floor((min_y + max_y - adaptive_size)/2);
    }else if(direction == 2)
    {
        newTarget.x = tileLocation.x + tileLocation.ev_pc1 - floor(adaptive_size*overlap);
        newTarget.y = floor((min_y + max_y - adaptive_size)/2);

    }else if(direction == 3)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size)/2);
        newTarget.y = -floor(adaptive_size*(1.0-overlap)) + tileLocation.y;
    }else if(direction == 4)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size)/2);
        newTarget.y = tileLocation.y + tileLocation.ev_pc2 - floor(adaptive_size*overlap);
    }
    newTarget.z = total4DImage->getOriginZ();
    newTarget.ev_pc1 = adaptive_size;
    newTarget.ev_pc2 = adaptive_size;
    newTarget.ev_pc3 = adaptive_size;

    newTarget.radius = max_r;
    newTargetList->push_back(newTarget);
    return true;
}


bool combo_tracing_ada_win(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString = P.output_folder.append("/tmp_COMBINED");
    QString finaloutputswc = P.markerfilename + ("_nc_combo.swc");

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,end_x,end_y;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;

    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || end_x <= 0 || end_y <= 0 )
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = P.in_sz[2];
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = P.in_sz[2];

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,tileLocation.z,end_x-1,end_y-1,tileLocation.z + P.in_sz[2]-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,tileLocation.z,
                               end_x,end_y,tileLocation.z + P.in_sz[2]))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    total4DImage->setData((unsigned char*)total1dData, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(tileLocation.z);

    Image4DSimple* total4DImage_mip = new Image4DSimple;
    total4DImage_mip->createBlankImage(in_sz[0], in_sz[1], 1, 1, V3D_UINT8);
    if (!total4DImage_mip->valid())
        return false;

    V3DLONG pagesz = in_sz[0]*in_sz[1];

    for (V3DLONG i=0; i<in_sz[2]; i++)
    {
        unsigned char *dst = total4DImage_mip->getRawDataAtChannel(0);
        unsigned char *src = total4DImage->getRawDataAtChannel(0) + i*pagesz;
        if (i==0)
        {
            memcpy(dst, src, pagesz);
        }
        else
        {
            for (V3DLONG j=0; j<pagesz; j++)
                if (dst[j]<src[j]) dst[j] = src[j];
        }
    }

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y).append(".v3draw"));

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists())
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));
    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".swc");

    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, total4DImage->getDatatype());

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    vector<MyMarker*> finalswc;

    if(ifs_swc)
       finalswc = readSWC_file(finaloutputswc.toStdString());

    V3DPluginArgItem arg;
    V3DPluginArgList input_neutube;
    V3DPluginArgList input_most;

    V3DPluginArgList output;

    QString full_plugin_name;
    QString func_name;

    arg.type = "random";std::vector<char*> arg_input;
    std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
    arg_input.push_back(fileName_string);
    arg.p = (void *) & arg_input; input_neutube<< arg;input_most<< arg;

    char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
    arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

    arg.type = "random";
    std::vector<char*> arg_para;

    //neutube tracing
    arg_para.push_back("1");
    arg_para.push_back("1");
    full_plugin_name = "neuTube";
    func_name =  "neutube_trace";
    arg.p = (void *) & arg_para; input_neutube << arg;
    if(!callback.callPluginFunc(full_plugin_name,func_name,input_neutube,output))
    {
        printf("Can not find the tracing plugin!\n");
        return false;
    }

    //MOST tracing
//    arg_para.clear();
//    string S_channel = boost::lexical_cast<string>(P.channel);
//    char* C_channel = new char[S_channel.length() + 1];
//    strcpy(C_channel,S_channel.c_str());
//    arg_para.push_back(C_channel);

//    string S_background_th = boost::lexical_cast<string>(P.bkg_thresh);
//    char* C_background_th = new char[S_background_th.length() + 1];
//    strcpy(C_background_th,S_background_th.c_str());
//    arg_para.push_back(C_background_th);

//    string S_seed_win = boost::lexical_cast<string>(P.seed_win);
//    char* C_seed_win = new char[S_seed_win.length() + 1];
//    strcpy(C_seed_win,S_seed_win.c_str());
//    arg_para.push_back(C_seed_win);

//    string S_slip_win = boost::lexical_cast<string>(P.slip_win);
//    char* C_slip_win = new char[S_slip_win.length() + 1];
//    strcpy(C_slip_win,S_slip_win.c_str());
//    arg_para.push_back(C_slip_win);

//    full_plugin_name = "mostVesselTracer";
//    func_name =  "MOST_trace";
//    arg.p = (void *) & arg_para; input_most << arg;
//    if(!callback.callPluginFunc(full_plugin_name,func_name,input_most,output))
//    {
//        printf("Can not find the tracing plugin!\n");
//        return false;
//    }

    //APP2
    PARA_APP2 p2;
    QString versionStr = "v0.001";
    p2.is_gsdt = P.is_gsdt;
    p2.is_coverage_prune = true;
    p2.is_break_accept = P.is_break_accept;
    p2.bkg_thresh = P.bkg_thresh;
    p2.length_thresh = P.length_thresh;
    p2.cnn_type = 2;
    p2.channel = 0;
    p2.SR_ratio = 3.0/9.9;
    p2.b_256cube = P.b_256cube;
    p2.b_RadiusFrom2D = P.b_RadiusFrom2D;
    p2.b_resample = 1;
    p2.b_intensity = 0;
    p2.b_brightfiled = 0;
    p2.b_menu = 0; //if set to be "true", v3d_msg window will show up.

    p2.p4dImage = total4DImage;
    p2.p4dImage->setFileName(imageSaveString.toStdString().c_str());
    p2.xc0 = p2.yc0 = p2.zc0 = 0;
    p2.xc1 = p2.p4dImage->getXDim()-1;
    p2.yc1 = p2.p4dImage->getYDim()-1;
    p2.zc1 = p2.p4dImage->getZDim()-1;

    QString swcAPP2 = saveDirString;
    swcAPP2.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_app2.swc");

    vector<MyMarker*> tileswc_app2;
    if(inputRootList.size() <1)
    {
        p2.outswc_file =swcString;
        proc_app2_wp(callback, p2, versionStr);
    }
    else
    {
        for(int i = 0; i < inputRootList.size(); i++)
        {
            QString poutswc_file = swcString + (QString::number(i)) + (".swc");
            p2.outswc_file =poutswc_file;

            bool flag = false;
            LocationSimple RootNewLocation;
            RootNewLocation.x = inputRootList.at(i).x - total4DImage->getOriginX();
            RootNewLocation.y = inputRootList.at(i).y - total4DImage->getOriginY();
            RootNewLocation.z = inputRootList.at(i).z - total4DImage->getOriginZ();

            const float dd = 0.5;
            if(tileswc_app2.size()>0)
            {
                for(V3DLONG dd = 0; dd < tileswc_app2.size();dd++)
                {
                    double dis = sqrt(pow2(RootNewLocation.x - tileswc_app2.at(dd)->x) + pow2(RootNewLocation.y - tileswc_app2.at(dd)->y) + pow2(RootNewLocation.z - tileswc_app2.at(dd)->z));
                    if(dis < 10.0)
                    {
                        flag = true;
                        break;
                    }
                }
            }

            if(!flag)
            {
                p2.landmarks.push_back(RootNewLocation);
                proc_app2_wp(callback, p2, versionStr);
                p2.landmarks.clear();
                vector<MyMarker*> inputswc = readSWC_file(poutswc_file.toStdString());

                for(V3DLONG d = 0; d < inputswc.size(); d++)
                {
                    tileswc_app2.push_back(inputswc[d]);
                }
            }
        }
        saveSWC_file(swcAPP2.toStdString().c_str(), tileswc_app2);
    }

    NeuronTree nt;
//    NeuronTree nt_app2_final = readSWC_file(swcAPP2);

    QString swcNEUTUBE = saveDirString;
    swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_neutube.swc");
    NeuronTree nt_neutube = readSWC_file(swcNEUTUBE);
    if(nt_neutube.listNeuron.size()<=0)
    {
        nt = readSWC_file(swcAPP2);
        //nt = nt_app2_final;
        combine_list2file(nt.listNeuron, swcString);
        return true;
    }
    NeuronTree nt_neutube_final = sort_eliminate_swc(nt_neutube,inputRootList,total4DImage);
    system(qPrintable(QString("rm %1").arg(swcNEUTUBE.toStdString().c_str())));

    combine_list2file(nt_neutube_final.listNeuron, swcNEUTUBE);

    vector<MyMarker*> swc_neutube = readSWC_file(swcNEUTUBE.toStdString());
    vector<MyMarker*> swc_app2= readSWC_file(swcAPP2.toStdString());

    QVector<QVector<V3DLONG> > childs;
    V3DLONG neuronNum = nt_neutube_final.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        V3DLONG par = nt_neutube_final.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt_neutube_final.hashNeuron.value(par)].push_back(i);
    }
    for(V3DLONG d = 0; d < swc_neutube.size(); d++)
    {
        int flag_prune = 0;
        for(int dd = 0; dd < swc_app2.size();dd++)
        {
            int dis_prun = sqrt(pow2(swc_neutube[d]->x - swc_app2[dd]->x) + pow2(swc_neutube[d]->y - swc_app2[dd]->y) + pow2(swc_neutube[d]->z - swc_app2[dd]->z));
            if( (swc_neutube[d]->radius + swc_app2[dd]->radius - dis_prun)/dis_prun > 0.05 || dis_prun < 20)
            {
                if(childs[d].size() > 0) swc_neutube[childs[d].at(0)]->parent = swc_app2[dd];
                flag_prune = 1;
                break;
            }

        }
        if(flag_prune == 0)
        {
           swc_app2.push_back(swc_neutube[d]);
        }

    }
    saveSWC_file(swcString.toStdString().c_str(), swc_app2);
    nt = readSWC_file(swcAPP2);


//    if(nt_app2_final.listNeuron.size() == 0 && nt_neutube_final.listNeuron.size() == 0)
//    {
//        combine_list2file(nt.listNeuron, swcString);
//        return true;
//    }else if(nt_app2_final.listNeuron.size() > nt_neutube_final.listNeuron.size())
//        nt = nt_app2_final;
//    else
//        nt =  nt_neutube_final;

//    if(nt_app2_final.listNeuron.size() > 0 && nt_neutube_final.listNeuron.size() > 0)
//    {
//        QList<IMAGE_METRICS> result_metrics_app2 = intensity_profile(nt_app2_final, total4DImage_mip, 3,0,0,0,callback);
//        QList<IMAGE_METRICS> result_metrics_neutube = intensity_profile(nt_neutube_final, total4DImage_mip, 3,0,0,0,callback);
//        if(result_metrics_app2[0].cnr > result_metrics_neutube[0].cnr)
//            nt = nt_app2_final;
//        else
//            nt = nt_neutube_final;
//    }else if (nt_app2_final.listNeuron.size() <= 0 && nt_neutube_final.listNeuron.size() > 0)
//        nt = nt_neutube_final;
//    else if (nt_app2_final.listNeuron.size() > 0 && nt_neutube_final.listNeuron.size() <= 0)
//        nt = nt_app2_final;
//    else
//    {
//        combine_list2file(nt.listNeuron, swcString);
//        return true;
//    }

 //   combine_list2file(nt.listNeuron, swcString);

//    QString swcMOST = saveDirString;
//    swcMOST.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append(".v3draw_MOST.swc");
//    NeuronTree nt_most = readSWC_file(swcMOST);
//    NeuronTree nt_most_final = sort_eliminate_swc(nt_most,inputRootList,total4DImage);
//    QList<IMAGE_METRICS> result_metrics_most = intensity_profile(nt_most_final, total4DImage_mip, 3,0,0,0,callback);

//    v3d_msg(QString("APP2 is %1, neutube is %2, most is %3").arg(result_metrics_app2[0].cnr).arg(result_metrics_neutube[0].cnr).arg(result_metrics_most[0].cnr));


    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    QList<NeuronSWC> list = nt.listNeuron;
    for (V3DLONG i=0;i<list.size();i++)
    {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            bool check_tip = false;

            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim())
            {
                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
                newTip.radius = curr.r;
                for(V3DLONG j = 0; j < finalswc.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.at(j)->x) + pow2(newTip.y - finalswc.at(j)->y) + pow2(newTip.z - finalswc.at(j)->z));
                    if(dis < 2*finalswc.at(j)->radius || dis < 20)
                    {
                        check_tip = true;
                        break;
                    }
                }
            }
            if(check_tip) continue;
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,P.block_size,1);
        for(int i = 0; i < group_tips_left.size();i++)
            ada_win_finding(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,P.block_size,2);
        for(int i = 0; i < group_tips_right.size();i++)
            ada_win_finding(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
    }

    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,P.block_size,3);
        for(int i = 0; i < group_tips_up.size();i++)
            ada_win_finding(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);

    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,P.block_size,4);
        for(int i = 0; i < group_tips_down.size();i++)
            ada_win_finding(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
    }



    vector<MyMarker*> tileswc_file = readSWC_file(swcString.toStdString());

    if(ifs_swc)
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }

    total4DImage->deleteRawDataAndSetPointerToNull();
    return true;
}


bool combo_tracing_ada_win_3D(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,LandmarkList inputRootList, LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList)
{

    QString saveDirString = P.output_folder+ QString("/x_%1_y_%2_z_%3_tmp_COMBINED").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
    QString finaloutputswc;
    if(P.global_name)
        finaloutputswc = P.markerfilename + QString("_nc_APP2_GD.swc");
    else
        finaloutputswc = P.markerfilename + QString("x_%1_y_%2_z_%3_nc_combo_adp_3D.swc").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,start_z,end_x,end_y,end_z;
    start_x = (tileLocation.x < 0)?  0 : tileLocation.x;
    start_y = (tileLocation.y < 0)?  0 : tileLocation.y;
    start_z = (tileLocation.z < 0)?  0 : tileLocation.z;


    end_x = tileLocation.x+tileLocation.ev_pc1;
    end_y = tileLocation.y+tileLocation.ev_pc2;
    end_z = tileLocation.z+tileLocation.ev_pc3;

    if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    if(end_y > P.in_sz[1]) end_y = P.in_sz[1];
    if(end_z > P.in_sz[2]) end_z = P.in_sz[2];

    if(tileLocation.x >= P.in_sz[0] - 1 || tileLocation.y >= P.in_sz[1] - 1 || tileLocation.z >= P.in_sz[2] - 1 || end_x <= 0 || end_y <= 0 || end_z <= 0)
    {
        printf("hit the boundary");
        return true;
    }

    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;

    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = end_z - start_z;
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = start_z; iz < end_z; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,start_z,end_x-1,end_y-1,end_z-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }

        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,start_z,
                               end_x,end_y,end_z))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }

            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
                P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,start_z, end_z);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }
    }

    Image4DSimple* total4DImage = new Image4DSimple;
    double min,max;
    V3DLONG pagesz_vim = in_sz[0]*in_sz[1]*in_sz[2];
    unsigned char * total1dData_scaled = 0;
    total1dData_scaled = new unsigned char [pagesz_vim];
    rescale_to_0_255_and_copy(total1dData,pagesz_vim,min,max,total1dData_scaled);


    total4DImage->setData((unsigned char*)total1dData_scaled, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);
    total4DImage->setOriginX(start_x);
    total4DImage->setOriginY(start_y);
    total4DImage->setOriginZ(start_z);

    V3DLONG mysz[4];
    mysz[0] = total4DImage->getXDim();
    mysz[1] = total4DImage->getYDim();
    mysz[2] = total4DImage->getZDim();
    mysz[3] = total4DImage->getCDim();

    total4DImage->setRezZ(3.0);//set the flg for 3d crawler

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw");

    QString scanDataFileString = saveDirString;
    scanDataFileString.append("/").append("scanData.txt");
    if(QFileInfo(finaloutputswc).exists() && !QFileInfo(scanDataFileString).exists() && !P.global_name)
        system(qPrintable(QString("rm -rf %1").arg(finaloutputswc.toStdString().c_str())));


    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".swc");

    qDebug()<<scanDataFileString;
    QFile saveTextFile;
    saveTextFile.setFileName(scanDataFileString);// add currentScanFile
    if (!saveTextFile.isOpen()){
        if (!saveTextFile.open(QIODevice::Text|QIODevice::Append  )){
            qDebug()<<"unable to save file!";
            return false;}     }
    QTextStream outputStream;
    outputStream.setDevice(&saveTextFile);
    outputStream<< (int) total4DImage->getOriginX()<<" "<< (int) total4DImage->getOriginY()<<" "<< (int) total4DImage->getOriginZ()<<" "<<swcString<<" "<< (int) in_sz[0]<<" "<< (int) in_sz[1]<<" "<< (int) in_sz[2]<<"\n";
    saveTextFile.close();

    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData_scaled, mysz, total4DImage->getDatatype());
    if(in_sz) {delete []in_sz; in_sz =0;}

    ifstream ifs_swc(finaloutputswc.toStdString().c_str());
    vector<MyMarker*> finalswc;
    NeuronTree nt;

    if(!ifs_swc)
    {
        //APP2 for the first tile
        PARA_APP2 p2;
        QString versionStr = "v0.001";
        p2.is_gsdt = P.is_gsdt;
        p2.is_coverage_prune = true;
        p2.is_break_accept = P.is_break_accept;
        p2.bkg_thresh = P.bkg_thresh;
        p2.length_thresh = P.length_thresh;
        p2.cnn_type = 2;
        p2.channel = 0;
        p2.SR_ratio = 3.0/9.9;
        p2.b_256cube = P.b_256cube;
        p2.b_RadiusFrom2D = P.b_RadiusFrom2D;
        p2.b_resample = 1;
        p2.b_intensity = 0;
        p2.b_brightfiled = 0;
        p2.b_menu = 0; //if set to be "true", v3d_msg window will show up.

        p2.p4dImage = total4DImage;
        p2.p4dImage->setFileName(imageSaveString.toStdString().c_str());
        p2.xc0 = p2.yc0 = p2.zc0 = 0;
        p2.xc1 = p2.p4dImage->getXDim()-1;
        p2.yc1 = p2.p4dImage->getYDim()-1;
        p2.zc1 = p2.p4dImage->getZDim()-1;

        p2.outswc_file =swcString;
        proc_app2_wp(callback, p2, versionStr);
        nt = readSWC_file(swcString);
    }
    else
    {
        finalswc = readSWC_file(finaloutputswc.toStdString());
        V3DPluginArgItem arg;
        V3DPluginArgList input_neutube;
        V3DPluginArgList input_most;

        V3DPluginArgList output;

        QString full_plugin_name;
        QString func_name;

        arg.type = "random";std::vector<char*> arg_input;
        std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
        arg_input.push_back(fileName_string);
        arg.p = (void *) & arg_input; input_neutube<< arg;input_most<< arg;

        char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
        arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

        arg.type = "random";
        std::vector<char*> arg_para;

        //neutube tracing
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "neuTube";
        func_name =  "neutube_trace";
        arg.p = (void *) & arg_para; input_neutube << arg;
        if(!callback.callPluginFunc(full_plugin_name,func_name,input_neutube,output))
        {
            printf("Can not find the tracing plugin!\n");
            return false;
        }

        NeuronTree nt_neutube;
        QString swcNEUTUBE = saveDirString;
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_neutube.swc");
        nt_neutube = readSWC_file(swcNEUTUBE);
        if(nt_neutube.listNeuron.size() ==0)
            return true;

        ifstream ifs_swcString(swcString.toStdString().c_str());
        if(!ifs_swcString)
        {
            nt = sort_eliminate_swc(nt_neutube,inputRootList,total4DImage);
            export_list2file(nt.listNeuron, swcString,swcNEUTUBE);

        }else
        {
            NeuronTree nt_tile = readSWC_file(swcString);
            LandmarkList inputRootList_pruned = eliminate_seed(nt_tile,inputRootList,total4DImage);
            if(inputRootList_pruned.size()<1)
                return true;
            else
            {
                nt = sort_eliminate_swc(nt_neutube,inputRootList_pruned,total4DImage);
                combine_list2file(nt.listNeuron, swcString);
            }
        }
    }

    LandmarkList tip_left;
    LandmarkList tip_right;
    LandmarkList tip_up ;
    LandmarkList tip_down;
    LandmarkList tip_out;
    LandmarkList tip_in;

    QList<NeuronSWC> list = nt.listNeuron;
    for (V3DLONG i=0;i<list.size();i++)
    {
            NeuronSWC curr = list.at(i);
            LocationSimple newTip;
            bool check_tip = false;
            if( curr.x < 0.05*  total4DImage->getXDim() || curr.x > 0.95 *  total4DImage->getXDim() || curr.y < 0.05 * total4DImage->getYDim() || curr.y > 0.95* total4DImage->getYDim()
                   || curr.z < 0.05*  total4DImage->getZDim() || curr.z > 0.95 *  total4DImage->getZDim())
            {
                newTip.x = curr.x + total4DImage->getOriginX();
                newTip.y = curr.y + total4DImage->getOriginY();
                newTip.z = curr.z + total4DImage->getOriginZ();
                newTip.radius = curr.r;

                for(V3DLONG j = 0; j < finalswc.size(); j++ )
                {
                    double dis = sqrt(pow2(newTip.x - finalswc.at(j)->x) + pow2(newTip.y - finalswc.at(j)->y) + pow2(newTip.z - finalswc.at(j)->z));
                    if(dis < 2*finalswc.at(j)->radius || dis < 20) //can be changed
                    {
                        check_tip = true;
                        break;
                    }
                }
            }
            if(check_tip) continue;
            if( curr.x < 0.05* total4DImage->getXDim())
            {
                tip_left.push_back(newTip);
            }else if (curr.x > 0.95 * total4DImage->getXDim())
            {
                tip_right.push_back(newTip);
            }else if (curr.y < 0.05 * total4DImage->getYDim())
            {
                tip_up.push_back(newTip);
            }else if (curr.y > 0.95*total4DImage->getYDim())
            {
                tip_down.push_back(newTip);
            }else if (curr.z < 0.05 * total4DImage->getZDim())
            {
                tip_out.push_back(newTip);
            }else if (curr.z > 0.95*total4DImage->getZDim())
            {
                tip_in.push_back(newTip);
            }
    }

    if(tip_left.size()>0)
    {
        QList<LandmarkList> group_tips_left = group_tips(tip_left,512,1);
        for(int i = 0; i < group_tips_left.size();i++)
            ada_win_finding_3D(group_tips_left.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,1);
    }
    if(tip_right.size()>0)
    {
        QList<LandmarkList> group_tips_right = group_tips(tip_right,512,2);
        for(int i = 0; i < group_tips_right.size();i++)
            ada_win_finding_3D(group_tips_right.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,2);
    }
    if(tip_up.size()>0)
    {
        QList<LandmarkList> group_tips_up = group_tips(tip_up,512,3);
        for(int i = 0; i < group_tips_up.size();i++)
            ada_win_finding_3D(group_tips_up.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,3);
    }
    if(tip_down.size()>0)
    {
        QList<LandmarkList> group_tips_down = group_tips(tip_down,512,4);
        for(int i = 0; i < group_tips_down.size();i++)
            ada_win_finding_3D(group_tips_down.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,4);
    }

    if(tip_out.size()>0)
    {
        QList<LandmarkList> group_tips_out = group_tips(tip_out,512,5);
        for(int i = 0; i < group_tips_out.size();i++)
            ada_win_finding_3D(group_tips_out.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,5);
    }

    if(tip_in.size()>0)
    {
        QList<LandmarkList> group_tips_in = group_tips(tip_in,512,6);
        for(int i = 0; i < group_tips_in.size();i++)
            ada_win_finding_3D(group_tips_in.at(i),tileLocation,newTargetList,newTipsList,total4DImage,P.block_size,6);
    }

    vector<MyMarker*> tileswc_file = readSWC_file(swcString.toStdString());
    if(ifs_swc)
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();

            finalswc.push_back(tileswc_file[i]);
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), finalswc);
    }
    else
    {
        for(V3DLONG i = 0; i < tileswc_file.size(); i++)
        {
            tileswc_file[i]->x = tileswc_file[i]->x + total4DImage->getOriginX();
            tileswc_file[i]->y = tileswc_file[i]->y + total4DImage->getOriginY();
            tileswc_file[i]->z = tileswc_file[i]->z + total4DImage->getOriginZ();
        }
        saveSWC_file(finaloutputswc.toStdString().c_str(), tileswc_file);
    }

    total4DImage->deleteRawDataAndSetPointerToNull();

    return true;
}


bool ada_win_finding_3D_GD(LandmarkList tips,LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList,Image4DSimple* total4DImage,int block_size,int direction)
{
    newTipsList->push_back(tips);
    double overlap = 0.05;

    float min_x = INF, max_x = -INF;
    float min_y = INF, max_y = -INF;
    float min_z = INF, max_z = -INF;


    double adaptive_size_x,adaptive_size_y,adaptive_size_z;
    double max_r = -INF;

    if(direction == 1 || direction == 2)
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).y <= min_y) min_y = tips.at(i).y;
            if(tips.at(i).y >= max_y) max_y = tips.at(i).y;

            if(tips.at(i).z <= min_z) min_z = tips.at(i).z;
            if(tips.at(i).z >= max_z) max_z = tips.at(i).z;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }

        adaptive_size_y = (max_y - min_y)*1.2;
        adaptive_size_z = (max_z - min_z)*1.2;
        adaptive_size_x = adaptive_size_y;

    }else if(direction == 3 || direction == 4)
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).x <= min_x) min_x = tips.at(i).x;
            if(tips.at(i).x >= max_x) max_x = tips.at(i).x;

            if(tips.at(i).z <= min_z) min_z = tips.at(i).z;
            if(tips.at(i).z >= max_z) max_z = tips.at(i).z;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }

        adaptive_size_x = (max_x - min_x)*1.2;
        adaptive_size_z = (max_z - min_z)*1.2;
        adaptive_size_y = adaptive_size_x;
    }else
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).x <= min_x) min_x = tips.at(i).x;
            if(tips.at(i).x >= max_x) max_x = tips.at(i).x;

            if(tips.at(i).y <= min_y) min_y = tips.at(i).y;
            if(tips.at(i).y >= max_y) max_y = tips.at(i).y;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }

        adaptive_size_x = (max_x - min_x)*1.2;
        adaptive_size_y = (max_y - min_y)*1.2;
        adaptive_size_z = adaptive_size_x;
    }

    adaptive_size_x = (adaptive_size_x <= 256) ? 256 : adaptive_size_x;
    adaptive_size_y = (adaptive_size_y <= 256) ? 256 : adaptive_size_y;
    adaptive_size_z = (adaptive_size_z <= 256) ? 256 : adaptive_size_z;


    adaptive_size_x = (adaptive_size_x >= block_size) ? block_size : adaptive_size_x;
    adaptive_size_y = (adaptive_size_y >= block_size) ? block_size : adaptive_size_y;
    adaptive_size_z = (adaptive_size_z >= block_size) ? block_size : adaptive_size_z;

    LocationSimple newTarget;
    if(direction == 1)
    {
        newTarget.x = -floor(adaptive_size_x*(1.0-overlap)) + tips.at(0).x;
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);
    }else if(direction == 2)
    {
        //newTarget.x = tips.at(0).x + tileLocation.ev_pc1 - floor(adaptive_size_x*overlap);
        newTarget.x = tips.at(0).x - floor(adaptive_size_x*overlap);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);

    }else if(direction == 3)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = -floor(adaptive_size_y*(1.0-overlap)) + tips.at(0).y;
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);
    }else if(direction == 4)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        //newTarget.y = tips.at(0).y - tileLocation.ev_pc2 - floor(adaptive_size_y*overlap);
        newTarget.y = tips.at(0).y - floor(adaptive_size_y*overlap);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);


    }else if(direction == 5)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = -floor(adaptive_size_z*(1.0-overlap)) + tips.at(0).z;
    }else if(direction == 6)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        //newTarget.z = tips.at(0).z + tileLocation.ev_pc3 - floor(adaptive_size_z*overlap);
        newTarget.z = tips.at(0).z - floor(adaptive_size_z*overlap);
    }


   // v3d_msg(QString("zmin is %1, zmax is %2, z is %3, z_winsize is %4").arg(min_z).arg(max_z).arg(tileLocation.z).arg(adaptive_size_z));


    newTarget.ev_pc1 = adaptive_size_x;
    newTarget.ev_pc2 = adaptive_size_y;
    newTarget.ev_pc3 = adaptive_size_z;

    newTarget.radius = max_r;

    newTargetList->push_back(newTarget);
    return true;
}
bool ada_win_finding_3D(LandmarkList tips,LocationSimple tileLocation,LandmarkList *newTargetList,QList<LandmarkList> *newTipsList,Image4DSimple* total4DImage,int block_size,int direction)
{
    newTipsList->push_back(tips);
    double overlap = 0.1;

    float min_x = INF, max_x = -INF;
    float min_y = INF, max_y = -INF;
    float min_z = INF, max_z = -INF;


    double adaptive_size_x,adaptive_size_y,adaptive_size_z;
    double max_r = -INF;
    double max_v = -INF;  //add by yongzhang

    if(direction == 1 || direction == 2)
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).y <= min_y) min_y = tips.at(i).y;
            if(tips.at(i).y >= max_y) max_y = tips.at(i).y;

            if(tips.at(i).z <= min_z) min_z = tips.at(i).z;
            if(tips.at(i).z >= max_z) max_z = tips.at(i).z;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
            if(tips.at(i).value >= max_v) max_v = tips.at(i).value;  //add by yongzhang
        }

        adaptive_size_y = (max_y - min_y)*1.2;
        adaptive_size_z = (max_z - min_z)*1.2;
        adaptive_size_x = adaptive_size_y;

    }else if(direction == 3 || direction == 4)
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).x <= min_x) min_x = tips.at(i).x;
            if(tips.at(i).x >= max_x) max_x = tips.at(i).x;

            if(tips.at(i).z <= min_z) min_z = tips.at(i).z;
            if(tips.at(i).z >= max_z) max_z = tips.at(i).z;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }

        adaptive_size_x = (max_x - min_x)*1.2;
        adaptive_size_z = (max_z - min_z)*1.2;
        adaptive_size_y = adaptive_size_x;
    }else
    {
        for(int i = 0; i<tips.size();i++)
        {
            if(tips.at(i).x <= min_x) min_x = tips.at(i).x;
            if(tips.at(i).x >= max_x) max_x = tips.at(i).x;

            if(tips.at(i).y <= min_y) min_y = tips.at(i).y;
            if(tips.at(i).y >= max_y) max_y = tips.at(i).y;

            if(tips.at(i).radius >= max_r) max_r = tips.at(i).radius;
        }

        adaptive_size_x = (max_x - min_x)*1.2;
        adaptive_size_y = (max_y - min_y)*1.2;
        adaptive_size_z = adaptive_size_x;
    }

    adaptive_size_x = (adaptive_size_x <= 128) ? 128 : adaptive_size_x;
    adaptive_size_y = (adaptive_size_y <= 128) ? 128 : adaptive_size_y;
    adaptive_size_z = (adaptive_size_z <= 128) ? 128 : adaptive_size_z;


    adaptive_size_x = (adaptive_size_x >= block_size) ? block_size : adaptive_size_x;
    adaptive_size_y = (adaptive_size_y >= block_size) ? block_size : adaptive_size_y;
    adaptive_size_z = (adaptive_size_z >= block_size) ? block_size : adaptive_size_z;

    LocationSimple newTarget;
    if(direction == 1)
    {
        newTarget.x = -floor(adaptive_size_x*(1.0-overlap)) + tileLocation.x;
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);
    }else if(direction == 2)
    {
        newTarget.x = tileLocation.x + tileLocation.ev_pc1 - floor(adaptive_size_x*overlap);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);

    }else if(direction == 3)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = -floor(adaptive_size_y*(1.0-overlap)) + tileLocation.y;
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);
    }else if(direction == 4)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = tileLocation.y + tileLocation.ev_pc2 - floor(adaptive_size_y*overlap);
        newTarget.z = floor((min_z + max_z - adaptive_size_z)/2);
    }else if(direction == 5)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = -floor(adaptive_size_z*(1.0-overlap)) + tileLocation.z;
    }else if(direction == 6)
    {
        newTarget.x = floor((min_x + max_x - adaptive_size_x)/2);
        newTarget.y = floor((min_y + max_y - adaptive_size_y)/2);
        newTarget.z = tileLocation.z + tileLocation.ev_pc3 - floor(adaptive_size_z*overlap);
    }


   // v3d_msg(QString("zmin is %1, zmax is %2, z is %3, z_winsize is %4").arg(min_z).arg(max_z).arg(tileLocation.z).arg(adaptive_size_z));


    newTarget.ev_pc1 = adaptive_size_x;
    newTarget.ev_pc2 = adaptive_size_y;
    newTarget.ev_pc3 = adaptive_size_z;

    newTarget.radius = max_r;
    newTarget.value = max_v;  //add by yongzhang

    newTargetList->push_back(newTarget);
    return true;
}


QList<LandmarkList> group_tips(LandmarkList tips,int block_size, int direction)
{
    QList<LandmarkList> groupTips;

   //bubble sort
   if(direction == 1 || direction == 2 || direction == 5 || direction == 6)
   {
       for(int i = 0; i < tips.size();i++)
       {
           for(int j = 0; j < tips.size();j++)
           {
               if(tips.at(i).y < tips.at(j).y)
                   tips.swap(i,j);
           }
       }

       LandmarkList eachGroupList;
       eachGroupList.push_back(tips.at(0));
       for(int d = 0; d < tips.size()-1; d++)
       {
           if(tips.at(d+1).y - tips.at(d).y < block_size)
           {
               eachGroupList.push_back(tips.at(d+1));
           }
           else
           {
               groupTips.push_back(eachGroupList);
               eachGroupList.erase(eachGroupList.begin(),eachGroupList.end());
               eachGroupList.push_back(tips.at(d+1));
           }
       }
       groupTips.push_back(eachGroupList);
   }else
   {
       for(int i = 0; i < tips.size();i++)
       {
           for(int j = 0; j < tips.size();j++)
           {
               if(tips.at(i).x < tips.at(j).x)
                   tips.swap(i,j);
           }
       }

       LandmarkList eachGroupList;
       eachGroupList.push_back(tips.at(0));
       for(int d = 0; d < tips.size()-1; d++)
       {
           if(tips.at(d+1).x - tips.at(d).x < block_size)
           {
               eachGroupList.push_back(tips.at(d+1));
           }
           else
           {
               groupTips.push_back(eachGroupList);
               eachGroupList.erase(eachGroupList.begin(),eachGroupList.end());
               eachGroupList.push_back(tips.at(d+1));

           }
       }
       groupTips.push_back(eachGroupList);
   }
   return groupTips;
}


bool load_region_tc(V3DPluginCallback2 &callback,QString &tcfile, Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim,unsigned char * & pVImg_TC,V3DLONG startx, V3DLONG starty, V3DLONG startz,
                     V3DLONG endx, V3DLONG endy, V3DLONG endz)
{

    //virtual image
    V3DLONG vx, vy, vz, vc;

    vx = endx - startx + 1;
    vy = endy - starty + 1;
    vz = endz - startz + 1;
    vc = vim.sz[3];

    V3DLONG pagesz_vim = vx*vy*vz*vc;

    // flu bird algorithm
    bitset<3> lut_ss, lut_se, lut_es, lut_ee;

    //
    V3DLONG x_s = startx + vim.min_vim[0];
    V3DLONG y_s = starty + vim.min_vim[1];
    V3DLONG z_s = startz + vim.min_vim[2];

    V3DLONG x_e = endx + vim.min_vim[0];
    V3DLONG y_e = endy + vim.min_vim[1];
    V3DLONG z_e = endz + vim.min_vim[2];
    printf("%d, %d, ,%d, %d, %d, %d\n\n\n\n\n",x_s, y_s, z_s, x_e,y_e,z_e);

    ImagePixelType datatype;
    bool flag_init = true;

    unsigned char *pVImg_UINT8 = NULL;
    unsigned short *pVImg_UINT16 = NULL;
    float *pVImg_FLOAT32 = NULL;

    QString curFilePath = QFileInfo(tcfile).path();
    curFilePath.append("/");

    for(V3DLONG ii=0; ii<vim.number_tiles; ii++)
    {
        int check_lu = 0,check_ru = 0,check_ld = 0,check_rd = 0;
        int check_lu2 = 0,check_ru2 = 0,check_ld2 = 0,check_rd2 = 0;


        int check1 = (x_s >=  vim.lut[ii].start_pos[0] && x_s <= vim.lut[ii].end_pos[0])?  1 : 0;
        int check2 = (x_e >=  vim.lut[ii].start_pos[0] && x_e <= vim.lut[ii].end_pos[0])?  1 : 0;
        int check3 = (y_s >=  vim.lut[ii].start_pos[1] && y_s <= vim.lut[ii].end_pos[1])?  1 : 0;
        int check4 = (y_e >=  vim.lut[ii].start_pos[1] && y_e <= vim.lut[ii].end_pos[1])?  1 : 0;

        if(check1*check3) check_lu = 1;
        if(check2*check3) check_ru = 1;
        if(check1*check4) check_ld = 1;
        if(check2*check4) check_rd = 1;

        int check5 = (vim.lut[ii].start_pos[0] >= x_s && vim.lut[ii].start_pos[0] <= x_e)?  1 : 0;
        int check6 = (vim.lut[ii].end_pos[0] >= x_s && vim.lut[ii].end_pos[0] <= x_e)?  1 : 0;
        int check7 = (vim.lut[ii].start_pos[1] >= y_s && vim.lut[ii].start_pos[1] <= y_e)?  1 : 0;
        int check8 = (vim.lut[ii].end_pos[1] >= y_s && vim.lut[ii].end_pos[1] <= y_e)?  1 : 0;

        if(check1*check3) check_lu = 1;
        if(check2*check3) check_ru = 1;
        if(check1*check4) check_ld = 1;
        if(check2*check4) check_rd = 1;

        if(check5*check7) check_lu2 = 1;
        if(check6*check7) check_ru2 = 1;
        if(check5*check8) check_ld2 = 1;
        if(check6*check8) check_rd2 = 1;

        if(check_lu || check_ru || check_ld || check_rd || check_lu2 || check_ru2 || check_ld2 || check_rd2)
        {
            //
            cout << "satisfied image: "<< vim.lut[ii].fn_img << endl;

            // loading relative image files
            V3DLONG sz_relative[4];
            int datatype_relative = 0;
            unsigned char* relative1d = 0;

            QString curPath = curFilePath;

            string fn = curPath.append( QString(vim.lut[ii].fn_img.c_str()) ).toStdString();

            qDebug()<<"testing..."<<curFilePath<< fn.c_str();

            if(!simple_loadimage_wrapper(callback, fn.c_str(), relative1d, sz_relative, datatype_relative))
            {
                fprintf (stderr, "Error happens in reading the subject file [%s]. Exit. \n",vim.tilesList.at(ii).fn_image.c_str());
                continue;
            }
            V3DLONG rx=sz_relative[0], ry=sz_relative[1], rz=sz_relative[2], rc=sz_relative[3];


            if(flag_init)
            {
                if(datatype_relative == V3D_UINT8)
                {
                    datatype = V3D_UINT8;

                    try
                    {
                        pVImg_UINT8 = new unsigned char [pagesz_vim];
                    }
                    catch (...)
                    {
                        printf("Fail to allocate memory.\n");
                        return false;
                    }

                    // init
                    memset(pVImg_UINT8, 0, pagesz_vim*sizeof(unsigned char));

                    flag_init = false;

                }
                else if(datatype_relative == V3D_UINT16)
                {
                    datatype = V3D_UINT16;

                    try
                    {
                        pVImg_UINT16 = new unsigned short [pagesz_vim];
                    }
                    catch (...)
                    {
                        printf("Fail to allocate memory.\n");
                        return false;
                    }

                    // init
                    memset(pVImg_UINT16, 0, pagesz_vim*sizeof(unsigned short));

                    flag_init = false;
                }
                else if(datatype_relative == V3D_FLOAT32)
                {
                    datatype = V3D_FLOAT32;

                    try
                    {
                        pVImg_FLOAT32 = new float [pagesz_vim];
                    }
                    catch (...)
                    {
                        printf("Fail to allocate memory.\n");
                        return false;
                    }

                    // init
                    memset(pVImg_FLOAT32, 0, pagesz_vim*sizeof(float));

                    flag_init = false;
                }
                else
                {
                    printf("Currently this program only support UINT8, UINT16, and FLOAT32 datatype.\n");
                    return false;
                }
            }

            //
            V3DLONG tile2vi_xs = vim.lut[ii].start_pos[0]-vim.min_vim[0];
            V3DLONG tile2vi_xe = vim.lut[ii].end_pos[0]-vim.min_vim[0];
            V3DLONG tile2vi_ys = vim.lut[ii].start_pos[1]-vim.min_vim[1];
            V3DLONG tile2vi_ye = vim.lut[ii].end_pos[1]-vim.min_vim[1];
            V3DLONG tile2vi_zs = vim.lut[ii].start_pos[2]-vim.min_vim[2];
            V3DLONG tile2vi_ze = vim.lut[ii].end_pos[2]-vim.min_vim[2];

            V3DLONG x_start = (startx > tile2vi_xs) ? startx : tile2vi_xs;
            V3DLONG x_end = (endx < tile2vi_xe) ? endx : tile2vi_xe;
            V3DLONG y_start = (starty > tile2vi_ys) ? starty : tile2vi_ys;
            V3DLONG y_end = (endy < tile2vi_ye) ? endy : tile2vi_ye;
            V3DLONG z_start = (startz > tile2vi_zs) ? startz : tile2vi_zs;
            V3DLONG z_end = (endz < tile2vi_ze) ? endz : tile2vi_ze;

            x_end++;
            y_end++;
            z_end++;

            V3DLONG start[3];
            start[0] = startx;
            start[1] = starty;
            start[2] = startz;

            //
            cout << x_start << " " << x_end << " " << y_start << " " << y_end << " " << z_start << " " << z_end << endl;
            try
            {
                pVImg_TC = new unsigned char [pagesz_vim];
            }
            catch (...)
            {
                printf("Fail to allocate memory.\n");
                return false;
            }
            double min,max;
            if(datatype == V3D_UINT8)
            {
                region_groupfusing<unsigned char>(pVImg_UINT8, vim, relative1d,
                                                  vx, vy, vz, vc, rx, ry, rz, rc,
                                                  tile2vi_zs, tile2vi_ys, tile2vi_xs,
                                                  z_start, z_end, y_start, y_end, x_start, x_end, start);            }
            else if(datatype == V3D_UINT16)
            {
                region_groupfusing<unsigned short>(pVImg_UINT16, vim, relative1d,
                                                   vx, vy, vz, vc, rx, ry, rz, rc,
                                                   tile2vi_zs, tile2vi_ys, tile2vi_xs,
                                                   z_start, z_end, y_start, y_end, x_start, x_end, start);
            }
            else if(datatype == V3D_FLOAT32)
            {
                region_groupfusing<float>(pVImg_FLOAT32, vim, relative1d,
                                          vx, vy, vz, vc, rx, ry, rz, rc,
                                          tile2vi_zs, tile2vi_ys, tile2vi_xs,
                                          z_start, z_end, y_start, y_end, x_start, x_end, start);            }
            else
            {
                printf("Currently this program only support UINT8, UINT16, and FLOAT32 datatype.\n");
                return false;
            }


            //de-alloc
            if(relative1d) {delete []relative1d; relative1d=0;}
        }

    }

    double min,max;
    if(datatype == V3D_UINT8)
    {
        memcpy(pVImg_TC, pVImg_UINT8, pagesz_vim);
    }else if(datatype == V3D_UINT16)
    {
        rescale_to_0_255_and_copy(pVImg_UINT16,pagesz_vim,min,max,pVImg_TC);
    }else if(datatype == V3D_FLOAT32)
    {
        rescale_to_0_255_and_copy(pVImg_FLOAT32,pagesz_vim,min,max,pVImg_TC);
    }

    if(pVImg_UINT8) {delete []pVImg_UINT8; pVImg_UINT8=0;}
    if(pVImg_UINT16) {delete []pVImg_UINT16; pVImg_UINT16 =0;}
    if(pVImg_FLOAT32) {delete []pVImg_FLOAT32; pVImg_FLOAT32 =0;}

    return true;
}

bool grid_raw_all(V3DPluginCallback2 &callback, QWidget *parent,TRACE_LS_PARA &P,bool bmenu)
{
    QElapsedTimer timer1;
    timer1.start();

    QString fileOpenName = P.inimg_file;

    if(P.image)
    {
        P.in_sz[0] = P.image->getXDim();
        P.in_sz[1] = P.image->getYDim();
        P.in_sz[2] = P.image->getZDim();
    }else
    {
        if(fileOpenName.endsWith(".tc",Qt::CaseSensitive))
        {
            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            P.in_sz[0] = vim.sz[0];
            P.in_sz[1] = vim.sz[1];
            P.in_sz[2] = vim.sz[2];

        }else if (fileOpenName.endsWith(".raw",Qt::CaseSensitive) || fileOpenName.endsWith(".v3draw",Qt::CaseSensitive))
        {
            unsigned char * datald = 0;
            V3DLONG *in_zz = 0;
            V3DLONG *in_sz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), datald, in_zz, in_sz,datatype,0,0,0,1,1,1))
            {
                return false;
            }
            if(datald) {delete []datald; datald = 0;}
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }else
        {
            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(fileOpenName.toStdString(),in_zz))
            {
                return false;
            }
            P.in_sz[0] = in_zz[0];
            P.in_sz[1] = in_zz[1];
            P.in_sz[2] = in_zz[2];
        }

        LocationSimple t;
        if(P.markerfilename.endsWith(".marker",Qt::CaseSensitive))
        {
            vector<MyMarker> file_inmarkers;
            file_inmarkers = readMarker_file(string(qPrintable(P.markerfilename)));
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x + 1;
                t.y = file_inmarkers[i].y + 1;
                t.z = file_inmarkers[i].z + 1;
                P.listLandmarks.push_back(t);
            }
        }else
        {
            QList<CellAPO> file_inmarkers;
            file_inmarkers = readAPO_file(P.markerfilename);
            for(int i = 0; i < file_inmarkers.size(); i++)
            {
                t.x = file_inmarkers[i].x;
                t.y = file_inmarkers[i].y;
                t.z = file_inmarkers[i].z;
                P.listLandmarks.push_back(t);
            }
        }
    }

    P.output_folder = QFileInfo(P.markerfilename).path();

    QString tmpfolder;
    if(P.method == neutube)
       tmpfolder = P.output_folder+("/tmp_NEUTUBE");
    else if(P.method == snake)
       tmpfolder = P.output_folder+("/tmp_SNAKE");
    else if(P.method == most)
       tmpfolder = P.output_folder+("/tmp_MOST");


    system(qPrintable(QString("mkdir %1").arg(tmpfolder.toStdString().c_str())));
    if(tmpfolder.isEmpty())
    {
        printf("Can not create a tmp folder!\n");
        return false;
    }

    unsigned int numOfThreads = 16; // default value for number of theads

#if  defined(Q_OS_LINUX)

    omp_set_num_threads(numOfThreads);

#pragma omp parallel for

#endif

    //for(V3DLONG ix = (int)P.listLandmarks[0].x; ix<= (int)P.listLandmarks[1].x; ix += P.block_size)
    for(V3DLONG ix = 0; ix < P.in_sz[0]; ix += P.block_size)
    {
#if  defined(Q_OS_LINUX)

        printf("number of threads for iy = %d\n", omp_get_num_threads());

#pragma omp parallel for
#endif

       // for(V3DLONG iy = (int)P.listLandmarks[0].y; iy<= (int)P.listLandmarks[1].y; iy += P.block_size)
        for(V3DLONG iy = 0; iy < P.in_sz[1]; iy += P.block_size)
        {
#if  defined(Q_OS_LINUX)

            printf("number of threads for iz = %d\n", omp_get_num_threads());

#pragma omp parallel for
#endif

         //   for(V3DLONG iz = (int)P.listLandmarks[0].z; iz<= (int)P.listLandmarks[1].z; iz += P.block_size)
            for(V3DLONG iz = 0; iz< P.in_sz[2]; iz += P.block_size)
            {

                    all_tracing_grid(callback,P,ix,iy,iz);
            }
        }
    }

    return true;
}


bool all_tracing_grid(V3DPluginCallback2 &callback,TRACE_LS_PARA &P,V3DLONG ix, V3DLONG iy, V3DLONG iz)
{
    QString saveDirString;
    if(P.method == neutube)
        saveDirString = P.output_folder.append("/tmp_NEUTUBE");
    else if (P.method == snake)
        saveDirString = P.output_folder.append("/tmp_SNAKE");
    else if (P.method == most)
        saveDirString = P.output_folder.append("/tmp_MOST");
    else if (P.method == app2)
        saveDirString = P.output_folder.append("/tmp_COMBINED");

    QString imageSaveString = saveDirString;

    V3DLONG start_x,start_y,start_z,end_x,end_y,end_z;
    start_x = ix;
    start_y = iy;
    start_z = iz;
    end_x = ix + P.block_size; if(end_x > P.in_sz[0]) end_x = P.in_sz[0];
    end_y = iy + P.block_size; if(end_y > P.in_sz[1]) end_y = P.in_sz[1];
    end_z = iz + P.block_size; if(end_z > P.in_sz[2]) end_z = P.in_sz[2];


    unsigned char * total1dData = 0;
    V3DLONG *in_sz = 0;
    if(P.image)
    {
        in_sz = new V3DLONG[4];
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = end_z - start_z;
        V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
        try {total1dData = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
        V3DLONG i = 0;
        for(V3DLONG iz = 0; iz < P.in_sz[2]; iz++)
        {
            V3DLONG offsetk = iz*P.in_sz[1]*P.in_sz[0];
            for(V3DLONG iy = start_y; iy < end_y; iy++)
            {
                V3DLONG offsetj = iy*P.in_sz[0];
                for(V3DLONG ix = start_x; ix < end_x; ix++)
                {
                    total1dData[i] = P.image->getRawData()[offsetk + offsetj + ix];
                    i++;
                }
            }
        }
    }else
    {
        if(QFileInfo(P.inimg_file).completeSuffix() == "tc")
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

            if( !vim.y_load( P.inimg_file.toStdString()) )
            {
                printf("Wrong stitching configuration file to be load!\n");
                return false;
            }

            if (!load_region_tc(callback,P.inimg_file,vim,total1dData,start_x,start_y,end_z,end_x-1,end_y-1,end_z-1))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else if ((QFileInfo(P.inimg_file).completeSuffix() == "raw") || (QFileInfo(P.inimg_file).completeSuffix() == "v3draw"))
        {
            V3DLONG *in_zz = 0;
            int datatype;
            if (!loadRawRegion(const_cast<char *>(P.inimg_file.toStdString().c_str()), total1dData, in_zz, in_sz,datatype,start_x,start_y,start_x,
                               end_x,end_y,end_z))
            {
                printf("can not load the region");
                if(total1dData) {delete []total1dData; total1dData = 0;}
                return false;
            }
        }else
        {
            in_sz = new V3DLONG[4];
            in_sz[0] = end_x - start_x;
            in_sz[1] = end_y - start_y;
            in_sz[2] = end_z - start_z;

            V3DLONG *in_zz = 0;
            if(!callback.getDimTeraFly(P.inimg_file.toStdString(),in_zz))
            {
                return false;
            }

            V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2];
            try {total1dData = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for loading the region.",0); return false;}
            if(P.channel > in_zz[3])
               P.channel = 1;
            unsigned char * total1dDataTerafly = 0;
            total1dDataTerafly = callback.getSubVolumeTeraFly(P.inimg_file.toStdString(),start_x,end_x,
                                                              start_y,end_y,start_z,end_z);

            for(V3DLONG i=0; i<pagesz; i++)
            {
                total1dData[i] = total1dDataTerafly[pagesz*(P.channel-1)+i];
            }
            if(total1dDataTerafly) {delete []total1dDataTerafly; total1dDataTerafly = 0;}
        }

    }

    V3DLONG mysz[4];
    mysz[0] = in_sz[0];
    mysz[1] = in_sz[1];
    mysz[2] = in_sz[2];
    mysz[3] = 1;

    double PixelSum = 0;
    for(V3DLONG i = 0; i < in_sz[0]*in_sz[1]*in_sz[2]; i++)
    {
        double PixelVaule = total1dData[i];
        PixelSum = PixelSum + PixelVaule;
    }

    double PixelMean = PixelSum/(in_sz[0]*in_sz[1]*in_sz[2]);
    if(PixelMean < 10)
    {
        if(total1dData) {delete []total1dData; total1dData = 0;}
        if(in_sz) {delete []in_sz; in_sz =0;}
        return true;
    }

    imageSaveString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z).append(".v3draw"));
    simple_saveimage_wrapper(callback, imageSaveString.toLatin1().data(),(unsigned char *)total1dData, mysz, 1);

    if(total1dData) {delete []total1dData; total1dData = 0;}
    if(in_sz) {delete []in_sz; in_sz =0;}

    QString swcString = saveDirString;
    swcString.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".swc");

    V3DPluginArgItem arg;
    V3DPluginArgList input;
    V3DPluginArgList output;

    QString full_plugin_name;
    QString func_name;

    arg.type = "random";std::vector<char*> arg_input;
    std:: string fileName_Qstring(imageSaveString.toStdString());char* fileName_string =  new char[fileName_Qstring.length() + 1]; strcpy(fileName_string, fileName_Qstring.c_str());
    arg_input.push_back(fileName_string);
    arg.p = (void *) & arg_input; input<< arg;

    char* char_swcout =  new char[swcString.length() + 1];strcpy(char_swcout, swcString.toStdString().c_str());
    arg.type = "random";std::vector<char*> arg_output;arg_output.push_back(char_swcout); arg.p = (void *) & arg_output; output<< arg;

    arg.type = "random";
    std::vector<char*> arg_para;

    if(P.method == neutube || P.method == app2)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "neuTube";
        func_name =  "neutube_trace";
    }else if(P.method == snake)
    {
        arg_para.push_back("1");
        arg_para.push_back("1");
        full_plugin_name = "snake";
        func_name =  "snake_trace";
    }else if(P.method == most)
    {
        string S_channel = boost::lexical_cast<string>(P.channel);
        char* C_channel = new char[S_channel.length() + 1];
        strcpy(C_channel,S_channel.c_str());
        arg_para.push_back(C_channel);

        string S_background_th = boost::lexical_cast<string>(P.bkg_thresh);
        char* C_background_th = new char[S_background_th.length() + 1];
        strcpy(C_background_th,S_background_th.c_str());
        arg_para.push_back(C_background_th);

        string S_seed_win = boost::lexical_cast<string>(P.seed_win);
        char* C_seed_win = new char[S_seed_win.length() + 1];
        strcpy(C_seed_win,S_seed_win.c_str());
        arg_para.push_back(C_seed_win);

        string S_slip_win = boost::lexical_cast<string>(P.slip_win);
        char* C_slip_win = new char[S_slip_win.length() + 1];
        strcpy(C_slip_win,S_slip_win.c_str());
        arg_para.push_back(C_slip_win);

        full_plugin_name = "mostVesselTracer";
        func_name =  "MOST_trace";
    }

    arg.p = (void *) & arg_para; input << arg;

    if(!callback.callPluginFunc(full_plugin_name,func_name,input,output))
    {
        printf("Can not find the tracing plugin!\n");
        return false;
    }

    QString swcNEUTUBE = saveDirString;
    if(P.method == neutube || P.method == app2)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_neutube.swc");
    else if (P.method == snake)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_snake.swc");
    else if (P.method == most)
        swcNEUTUBE.append("/x_").append(QString::number(start_x)).append("_y_").append(QString::number(start_y)).append("_z_").append(QString::number(start_z)).append(".v3draw_MOST.swc");

    vector<MyMarker*> inputswc;
    inputswc = readSWC_file(swcNEUTUBE.toStdString());;
    for(V3DLONG d = 0; d < inputswc.size(); d++)
    {
        inputswc[d]->x = inputswc[d]->x + start_x;
        inputswc[d]->y = inputswc[d]->y + start_y;
        inputswc[d]->z = inputswc[d]->z + start_z;

    }
    saveSWC_file(swcString.toStdString().c_str(), inputswc);
    system(qPrintable(QString("rm %1").arg(swcNEUTUBE.toStdString().c_str())));
    system(qPrintable(QString("rm %1").arg(imageSaveString.toStdString().c_str())));
}


NeuronTree neuron_sub(NeuronTree nt_total, NeuronTree nt)
{
    V3DLONG neuronNum = nt_total.listNeuron.size();
    NeuronTree nt_left;
    QList <NeuronSWC> listNeuron;
    QHash <int, int>  hashNeuron;
    listNeuron.clear();
    hashNeuron.clear();
    //set node
    NeuronSWC S;
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        NeuronSWC curr = nt_total.listNeuron.at(i);
        S.n 	= curr.n;
        S.type 	= curr.type;
        S.x 	= curr.x;
        S.y 	= curr.y;
        S.z 	= curr.z;
        S.r 	= curr.r;
        S.pn 	= curr.pn;
        bool flag = false;
        for(V3DLONG j=0;j<nt.listNeuron.size();j++)
        {
            double dis = sqrt(pow2(nt.listNeuron[j].x - curr.x) + pow2(nt.listNeuron[j].y - curr.y) + pow2(nt.listNeuron[j].z - curr.z));
            if(dis < 5)
            {
                flag = true;
                break;
            }
        }
        if(!flag)
        {
            listNeuron.append(S);
            hashNeuron.insert(S.n, listNeuron.size()-1);
        }
    }
    nt_left.n = -1;
    nt_left.on = true;
    nt_left.listNeuron = listNeuron;
    nt_left.hashNeuron = hashNeuron;
    return nt_left;
}


QString getAppPath()
{
    QString v3dAppPath("~/Work/v3d_external/v3d");
    QDir testPluginsDir = QDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    if (testPluginsDir.dirName().toLower() == "debug" || testPluginsDir.dirName().toLower() == "release")
        testPluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (testPluginsDir.dirName() == "MacOS") {
        QDir testUpperPluginsDir = testPluginsDir;
        testUpperPluginsDir.cdUp();
        testUpperPluginsDir.cdUp();
        testUpperPluginsDir.cdUp(); // like foo/plugins next to foo/v3d.app
        if (testUpperPluginsDir.cd("plugins")) testPluginsDir = testUpperPluginsDir;
        testPluginsDir.cdUp();
    }
#endif

    v3dAppPath = testPluginsDir.absolutePath();
    return v3dAppPath;
}



NeuronTree DL_eliminate_swc(NeuronTree nt,QList <ImageMarker> marklist)
{
    NeuronTree nt_prunned;
    QList <NeuronSWC> listNeuron;
    QHash <int, int>  hashNeuron;
    listNeuron.clear();
    hashNeuron.clear();
    NeuronSWC S;

    for (V3DLONG i=0;i<nt.listNeuron.size();i++)
    {
        bool flag = false;
        for(V3DLONG j=0; j<marklist.size();j++)
        {
            double dis = sqrt(pow2(nt.listNeuron.at(i).x - marklist.at(j).x) + pow2(nt.listNeuron.at(i).y - marklist.at(j).y) + pow2(nt.listNeuron.at(i).z - marklist.at(j).z));
            if(dis < 1.0)
            {
                flag = true;
                break;
            }
        }
        if(!flag)
        {
            NeuronSWC curr = nt.listNeuron.at(i);
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

    return nt_prunned;
}


bool extract_tips(V3DPluginCallback2 &callback, QWidget *parent,TRACE_LS_PARA &P)
{
    P.output_folder = QFileInfo(P.markerfilename).path();
    QString finaloutputswc = P.output_folder.append("/nc_APP2_GD.swc");
    NeuronTree nt_APP2 = readSWC_file(finaloutputswc);
    QList<NeuronSWC> neuronAPP2_sorted;
    if (!SortSWC(nt_APP2.listNeuron, neuronAPP2_sorted,VOID, 10))
    {
        v3d_msg("fail to call swc sorting function.",0);
    }
    export_list2file(neuronAPP2_sorted, finaloutputswc,finaloutputswc);
    NeuronTree nt = readSWC_file(finaloutputswc);

    QVector<QVector<V3DLONG> > childs;
    V3DLONG neuronNum = nt.listNeuron.size();
    childs = QVector< QVector<V3DLONG> >(neuronNum, QVector<V3DLONG>() );
    for (V3DLONG i=0;i<neuronNum;i++)
    {
        V3DLONG par = nt.listNeuron[i].pn;
        if (par<0) continue;
        childs[nt.hashNeuron.value(par)].push_back(i);
    }
    QList<NeuronSWC> list = nt.listNeuron;
    QString tips_folder = P.output_folder+ QString("/tips");
    system(qPrintable(QString("mkdir %1").arg(tips_folder.toStdString().c_str())));
    QList<ImageMarker> tipMarkerList;

    for (V3DLONG i=0;i<list.size();i++)
    {
        if (childs[i].size()==0)
        {
            tipMarkerList.clear();
            NeuronSWC curr = list.at(i);
            QString tip_marker_name =  tips_folder+QString("/x_%1_y_%2_z_%3.marker").arg(curr.x).arg(curr.y).arg(curr.z);
            QString tip_swc_name =  tips_folder+QString("/x_%1_y_%2_z_%3.swc").arg(curr.x).arg(curr.y).arg(curr.z);;
            ImageMarker tipMarker;
            tipMarker.x = curr.x + 1;
            tipMarker.y = curr.y + 1;
            tipMarker.z = curr.z + 1;
            tipMarkerList.append(tipMarker);
            writeMarker_file(tip_marker_name, tipMarkerList);

            NeuronTree nt_tps;
            QList <NeuronSWC> & listNeuron = nt_tps.listNeuron;
            listNeuron << curr;
            int count = 0;
            int parent_tip = getParent(i,nt);
            while(list.at(parent_tip).pn > 0 && count < 50)
            {
                listNeuron << list.at(parent_tip);
                parent_tip = getParent(parent_tip,nt);
                count++;
            }
            writeSWC_file(tip_swc_name,nt_tps);
            listNeuron.clear();

            //call GD
            P.markerfilename = tip_marker_name;
            P.block_size = 64;
            P.seed_win = 32;
            P.swcfilename = tip_swc_name;
            P.method = gd;
            crawler_raw_app(callback,parent,P,0);

            //detect branch points and call GD
            QString GD_folder = P.output_folder+QString("/x_%1_y_%2_z_%3_tmp_GD_Curveline").arg(P.listLandmarks[0].x).arg(P.listLandmarks[0].y).arg(P.listLandmarks[0].z);
            QString GD_result_name = GD_folder + QString("/scanData.txtwofusion.swc");
            QString branches_folder = GD_folder + QString("/branches");
            system(qPrintable(QString("mkdir %1").arg(branches_folder.toStdString().c_str())));

            NeuronTree GD_result = readSWC_file(GD_result_name);
            if(GD_result.listNeuron.size() > 50)
            {
                vector<MyMarker> branchMarker = extract_branch_pts(callback,P.inimg_file,GD_result);
                QString all_branch_marker_name =  branches_folder+QString("/branches.marker");
                saveMarker_file(all_branch_marker_name.toStdString(),branchMarker);

                vector<MyMarker> branch_list_temp;
                for(V3DLONG d = 0; d < branchMarker.size(); d++)
                {
                    branch_list_temp.clear();
                    MyMarker each_branch = branchMarker.at(d);
                    QString branch_marker_name =  branches_folder+QString("/x_%1_y_%2_z_%3.marker").arg(each_branch.x).arg(each_branch.y).arg(each_branch.z);
                    branch_list_temp.push_back(each_branch);
                    saveMarker_file(branch_marker_name.toStdString(), branch_list_temp);

                    QString branch_swc_name =  branches_folder+QString("/x_%1_y_%2_z_%3.swc").arg(each_branch.x).arg(each_branch.y).arg(each_branch.z);;
                    NeuronTree nt_branches;
                    QList <NeuronSWC> & listNeuron = nt_branches.listNeuron;
                    int start_node =  (each_branch.type - 25)<0 ? 0 : each_branch.type - 25;
                    int end_node   =  (each_branch.type + 25)>GD_result.listNeuron.size()-1 ? GD_result.listNeuron.size()-1 : each_branch.type + 25;
                    for(int dd = start_node; dd <= end_node; dd++)
                    {
                        listNeuron << GD_result.listNeuron.at(dd);
                    }
                    writeSWC_file(branch_swc_name,nt_branches);
                    listNeuron.clear();

                    P.markerfilename = branch_marker_name;
                    P.block_size = 64;
                    P.seed_win = 32;
                    P.swcfilename = branch_swc_name;
                    P.method = gd;
                    crawler_raw_app(callback,parent,P,0);
                }
            }
        }
    }
}


bool tracing_pair_app(V3DPluginCallback2 &callback, QWidget *parent,TRACE_LS_PARA &P,bool bmenu)
{
    QString fileOpenName = P.inimg_file;
    vector<MyMarker> file_inmarkers;
    file_inmarkers = readMarker_file(string(qPrintable(P.markerfilename)));
    unsigned char * total1dData = 0;
    V3DLONG *in_zz = 0;
    if(!callback.getDimTeraFly(fileOpenName.toStdString(),in_zz))
    {
        return false;
    }
    P.in_sz[0] = in_zz[0];
    P.in_sz[1] = in_zz[1];
    P.in_sz[2] = in_zz[2];

    V3DLONG start_x,end_x,start_y,end_y,start_z,end_z;
    if(file_inmarkers.size()>=4)
    {
        start_x = (file_inmarkers[2].x - 50)<0 ? 0 :file_inmarkers[2].x - 50;
        start_y = (file_inmarkers[2].y - 50)<0 ? 0 :file_inmarkers[2].y - 50;
        start_z = (file_inmarkers[2].z - 50)<0 ? 0 :file_inmarkers[2].z - 50;

        end_x = (file_inmarkers[3].x + 50)> P.in_sz[0]? P.in_sz[0] :file_inmarkers[3].x + 50;
        end_y = (file_inmarkers[3].y + 50)> P.in_sz[1]? P.in_sz[1] :file_inmarkers[3].y + 50;
        end_z = (file_inmarkers[3].z + 50)> P.in_sz[2]? P.in_sz[2] :file_inmarkers[3].z + 50;
    }
    else if(file_inmarkers.size()>=2)
    {
        if(file_inmarkers.at(0).x >= file_inmarkers.at(1).x)
        {
            start_x = file_inmarkers.at(1).x;
            end_x = file_inmarkers.at(0).x+1;
        }else
        {
            end_x = file_inmarkers.at(1).x+1;
            start_x = file_inmarkers.at(0).x;
        }

        if(file_inmarkers.at(0).y >= file_inmarkers.at(1).y)
        {
            start_y = file_inmarkers.at(1).y;
            end_y = file_inmarkers.at(0).y+1;
        }else
        {
            end_y = file_inmarkers.at(1).y+1;
            start_y = file_inmarkers.at(0).y;
        }

        if(file_inmarkers.at(0).z >= file_inmarkers.at(1).z)
        {
            start_z = file_inmarkers.at(1).z;
            end_z = file_inmarkers.at(0).z+1;
        }else
        {
            end_z = file_inmarkers.at(1).z+1;
            start_z = file_inmarkers.at(0).z;
        }

        int win_size = 100;
        start_x = (start_x - win_size)<0 ? 0 :start_x - win_size;
        start_y = (start_y - win_size)<0 ? 0 :start_y - win_size;
        start_z = (start_z - win_size)<0 ? 0 :start_z - win_size;

        end_x = (end_x + win_size)> P.in_sz[0]? P.in_sz[0] :end_x + win_size;
        end_y = (end_y + win_size)> P.in_sz[1]? P.in_sz[1] :end_y + win_size;
        end_z = (end_z + win_size)> P.in_sz[2]? P.in_sz[2] :end_z + win_size;
    }

    /*unsigned char * total1dData_apa = 0;
    total1dData_apa = new unsigned char [pagesz];
    BinaryProcess(total1dData, total1dData_apa,in_sz[0],in_sz[1], in_sz[2], 5, 3);*/

    V3DLONG *in_sz = 0;
    in_sz = new V3DLONG[4];
    in_sz[0] = end_x - start_x;
    in_sz[1] = end_y - start_y;
    in_sz[2] = end_z - start_z;
    in_sz[3] = 1;
    V3DLONG pagesz = in_sz[0]*in_sz[1]*in_sz[2]*in_sz[3];
    if(pagesz >= 8589934592) //use the 2nd highest resolution folder
    {
        fileOpenName = P.inimg_file_2nd;
        start_x /= 2; start_y /= 2; start_z /= 2;
        end_x /= 2;   end_y /= 2;   end_z /= 2;
        in_sz[0] = end_x - start_x;
        in_sz[1] = end_y - start_y;
        in_sz[2] = end_z - start_z;

        for(V3DLONG i = 0; i <file_inmarkers.size(); i++)
        {
            file_inmarkers[i].x /= 2;
            file_inmarkers[i].y /= 2;
            file_inmarkers[i].z /= 2;
        }
     }

    total1dData = callback.getSubVolumeTeraFly(fileOpenName.toStdString(),start_x,end_x,
                                                      start_y,end_y,start_z,end_z);

    unsigned char ****p4d = 0;
    if (!new4dpointer(p4d, in_sz[0], in_sz[1], in_sz[2], 1, total1dData))
    {
        fprintf (stderr, "Fail to create a 4D pointer for the image data. Exit. \n");
        if(p4d) {delete []p4d; p4d = 0;}
        return false;
    }
    LocationSimple p0;
    p0.x = file_inmarkers[1].x - start_x;
    p0.y = file_inmarkers[1].y - start_y;
    p0.z = file_inmarkers[1].z - start_z;

    vector<LocationSimple> pp;
    LocationSimple p1;
    p1.x = file_inmarkers[0].x - start_x;
    p1.y = file_inmarkers[0].y - start_y;
    p1.z = file_inmarkers[0].z - start_z;
    pp.push_back(p1);

    double weight_xy_z=1.0;
    CurveTracePara trace_para;
    trace_para.channo = 0;
    trace_para.sp_graph_background = 0;
    trace_para.b_postMergeClosebyBranches = false;
    trace_para.sp_graph_resolution_step=4;
    trace_para.b_3dcurve_width_from_xyonly = true;
    trace_para.b_pruneArtifactBranches = false;
    trace_para.sp_num_end_nodes = 2;
    trace_para.b_deformcurve = false;
    trace_para.sp_graph_resolution_step = 1;
    trace_para.b_estRadii = false;
    trace_para.sp_downsample_method = 1;
    trace_para.sp_downsample_step = 4;
    trace_para.sp_graph_resolution_step = 4;

    NeuronTree nt = v3dneuron_GD_tracing(p4d, in_sz,
                                         p0, pp,
                                         trace_para, weight_xy_z);

    for(V3DLONG i = 0; i < nt.listNeuron.size();i++)
    {
        nt.listNeuron[i].x += start_x;
        nt.listNeuron[i].y += start_y;
        nt.listNeuron[i].z += start_z;

        if(pagesz >= 8589934592)
        {
            nt.listNeuron[i].x *= 2;
            nt.listNeuron[i].y *= 2;
            nt.listNeuron[i].z *= 2;
        }
    }

    for(V3DLONG i = 0; i <file_inmarkers.size(); i++)
    {
        file_inmarkers[i].x -= start_x;
        file_inmarkers[i].y -= start_y;
        file_inmarkers[i].z -= start_z;
    }

    QString swc_name = P.markerfilename+"_gd.swc";
    QString marker_name = P.markerfilename+"_shifted.marker";
    QString image_name = P.markerfilename+"_gd.v3draw";


    export_list2file(nt.listNeuron, swc_name,swc_name);
    saveMarker_file(marker_name.toStdString().c_str(),file_inmarkers);
    simple_saveimage_wrapper(callback,image_name.toStdString().c_str(),total1dData,in_sz,1);
    if(total1dData) {delete [] total1dData; total1dData =0;}
}


NeuronTree pruning_cross_swc(NeuronTree nt)
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
    QList<int> branch_IDs;
    for (int i=0;i<list.size();i++)
    {
        if (childs[i].size()>1)
        {
            branch_IDs.push_back(i);
        }
    }

    for(int i = 0; i< branch_IDs.size(); i++)
    {
        double x_i = list.at(branch_IDs.at(i)).x;
        double y_i = list.at(branch_IDs.at(i)).y;
        double z_i = list.at(branch_IDs.at(i)).z;

        for(int j = i+1; j< branch_IDs.size(); j++)
        {
            double x_j = list.at(branch_IDs.at(j)).x;
            double y_j = list.at(branch_IDs.at(j)).y;
            double z_j = list.at(branch_IDs.at(j)).z;
            double dis = sqrt(pow2(x_i -x_j ) + pow2(y_i - y_j) + pow2(z_i - z_j));
            if(dis<=10)
            {
                QList<int> iIDs;
                iIDs.push_back(branch_IDs.at(i));
                iIDs.push_back(branch_IDs.at(j));
                while(iIDs.size()>0)
                {
                    int pn = iIDs.at(0);
                    for(int d = 0; d < childs[pn].size();d++)
                    {
                        if(list.at(branch_IDs.at(i)).type != list.at(childs[pn].at(d)).type)
                        {
                            flag[childs[pn].at(d)] = 0;
                            iIDs.push_back(childs[pn].at(d));
                        }
                    }
                    iIDs.removeAt(0);

                }

            }
        }
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


vector<MyMarker> extract_branch_pts(V3DPluginCallback2 &callback, const QString& filename,NeuronTree nt)
{
    V3DLONG start_x = nt.listNeuron.at(0).x;
    V3DLONG end_x = nt.listNeuron.at(0).x;
    V3DLONG start_y = nt.listNeuron.at(0).y;
    V3DLONG end_y = nt.listNeuron.at(0).y;
    V3DLONG start_z = nt.listNeuron.at(0).z;
    V3DLONG end_z = nt.listNeuron.at(0).z;

    for(V3DLONG i = 1; i < nt.listNeuron.size();i++)
    {
        if(start_x>nt.listNeuron.at(i).x) start_x = nt.listNeuron.at(i).x;
        if(end_x<nt.listNeuron.at(i).x) end_x = nt.listNeuron.at(i).x;

        if(start_y>nt.listNeuron.at(i).y) start_y = nt.listNeuron.at(i).y;
        if(end_y<nt.listNeuron.at(i).y) end_y = nt.listNeuron.at(i).y;

        if(start_z>nt.listNeuron.at(i).z) start_z = nt.listNeuron.at(i).z;
        if(end_z<nt.listNeuron.at(i).z) end_z = nt.listNeuron.at(i).z;

    }

    unsigned char * total1dData = 0;
    total1dData = callback.getSubVolumeTeraFly(filename.toStdString(),start_x,end_x+1,
                                               start_y,end_y+1,start_z,end_z+1);
    V3DLONG mysz[4];
    mysz[0] = end_x -start_x +1;
    mysz[1] = end_y - start_y +1;
    mysz[2] = end_z - start_z +1;
    mysz[3] = 1;

    double seg_mean = 0;
    for(V3DLONG i =0; i <nt.listNeuron.size(); i++)
    {
        V3DLONG ix = nt.listNeuron.at(i).x - start_x;
        V3DLONG iy = nt.listNeuron.at(i).y - start_y;
        V3DLONG iz = nt.listNeuron.at(i).z - start_z;

        V3DLONG offsetk = iz*mysz[1]*mysz[0];
        V3DLONG offsetj = iy*mysz[0];

        seg_mean += total1dData[offsetk + offsetj + ix];
    }
    seg_mean /= nt.listNeuron.size();

    int c = mysz[3] - 1;
    double rs = 5;

    double score_each = 0, ave_v=0;
    vector<MyMarker> file_inmarkers;
    for(V3DLONG i =0; i <nt.listNeuron.size(); i++)
    {
        V3DLONG ix = nt.listNeuron.at(i).x - start_x;
        V3DLONG iy = nt.listNeuron.at(i).y - start_y;
        V3DLONG iz = nt.listNeuron.at(i).z - start_z;

        V3DLONG offsetk = iz*mysz[1]*mysz[0];
        V3DLONG offsetj = iy*mysz[0];

        V3DLONG PixelValue = total1dData[offsetk + offsetj + ix];
        compute_Anisotropy_sphere(total1dData, mysz[0], mysz[1], mysz[2], c, ix, iy, iz, rs, score_each, ave_v);
        if(PixelValue >= seg_mean && score_each >0.25)
        {
            MyMarker t;
            t.x = nt.listNeuron.at(i).x + 1;
            t.y = nt.listNeuron.at(i).y + 1;
            t.z = nt.listNeuron.at(i).z + 1;
            t.type = i;// give the index
            file_inmarkers.push_back(t);
        }
    }
    if(total1dData) {delete []total1dData; total1dData = 0;}
    return file_inmarkers;

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


