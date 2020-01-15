#include "get_sub_terafly.h"
#include <iostream>
#include "blastneuron/sort_swc.h"
#include "blastneuron/resampling.h"
#include "DeepNeuron_main_func.h"

#define MHDIS(a,b) ( (fabs((a).x-(b).x)) + (fabs((a).y-(b).y)) + (fabs((a).z-(b).z)) )
#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))

bool get_sub_terafly(V3DPluginCallback2 &callback,QWidget *parent)
{
    QString inimg_file = callback.getPathTeraFly();

    QString fileOpenName;
    fileOpenName = QFileDialog::getOpenFileName(0, QObject::tr("Open SWC File"),
                                                "",

                                                QObject::tr("Supported file (*.swc *.SWC *.eswc)"

                                            ));
    NeuronTree nt=readSWC_file(fileOpenName);

    cout<<fileOpenName.toStdString()<<endl;

    bool ok0;
    double method;
    method = QInputDialog::getDouble(0, "Would you like to set a class for method?","method:(If you select 'cancel', the class is 1)",0,0,2147483647,1,&ok0);
    if (!ok0)
        method = 1;

    if(method == 1)
    {
        cout<<"************************this is pre_processing***********************"<<endl;

        cout<<"***this is prune_branch***"<<endl;
        NeuronTree nt_prune;
        double prune_thres = -1;  //need checkout prune thres!
        prune_branch(nt,nt_prune,prune_thres);
        cout<<"nt.size = "<<nt.listNeuron.size()<<endl;
        cout<<"nt_prune.size = "<<nt_prune.listNeuron.size()<<endl;

        cout<<"***this is resample***"<<endl;
        bool ok1;
        double resample_step;
        resample_step = QInputDialog::getDouble(0, "Would you like to set a step for resample?","resample_step:(If you select 'cancel', the resample_step is 3)",0,0,2147483647,1,&ok1);
        if (!ok1)
            resample_step = 3;

        NeuronTree nt_resample=resample(nt_prune,resample_step);

        cout<<"nt_resample.size = "<<nt_resample.listNeuron.size()<<endl;

        cout<<"***this is sort***"<<endl;

        QList<NeuronSWC> result;
        QList<CellAPO> markers;
        V3DLONG rootid=VOID;
        V3DLONG thres=VOID;
        V3DLONG root_dist_thres=0;

        bool ok2;
        double sort_class;
        sort_class = QInputDialog::getDouble(0, "Would you like to set a class for sort?","sort_class:(If you select 'cancel', the sort class is 1)",0,0,2147483647,1,&ok2);
        if (!ok2)
            sort_class = 1;

        if(sort_class == 1)
        {
            Sort_auto_SWC(nt_resample.listNeuron,result,nt_resample.listNeuron[0].n,thres);
        }
        else if(sort_class == 2)
        {
            Sort_manual_SWC(nt_resample.listNeuron,result,rootid,thres,root_dist_thres,markers);
        }

        cout<<"result.size = "<<result.size()<<endl;

        NeuronTree nt_result;
        nt_result.listNeuron = result;

        V3DLONG im_cropped_sz[4];

        for(V3DLONG i=0;i<nt_result.listNeuron.size();)
        {
            NeuronSWC S;
            S.x = nt_result.listNeuron[i].x;
            S.y = nt_result.listNeuron[i].y;
            S.z = nt_result.listNeuron[i].z;

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
            for(V3DLONG j=0;j<nt_result.listNeuron.size();j++)
            {
                NeuronSWC p;
                if(nt_result.listNeuron[j].x<xe&&nt_result.listNeuron[j].x>xb&&nt_result.listNeuron[j].y<ye&&nt_result.listNeuron[j].y>yb&&nt_result.listNeuron[j].z<ze&&nt_result.listNeuron[j].z>zb)
                {
                    p.n = nt_result.listNeuron[j].n;
                    p.x = nt_result.listNeuron[j].x-xb;
                    p.y = nt_result.listNeuron[j].y-yb;
                    p.z = nt_result.listNeuron[j].z-zb;
                    p.type = nt_result.listNeuron[j].type;
                    p.r = nt_result.listNeuron[j].r;
                    p.pn = nt_result.listNeuron[j].pn;
                    outswc.push_back(p);
                }
            }

            im_cropped = callback.getSubVolumeTeraFly(inimg_file.toStdString(),xb,xe+1,
                                                  yb,ye+1,zb,ze+1);


            QString outimg_file,outswc_file;
            outimg_file = fileOpenName + QString::number(i)+".tif";
            outswc_file = fileOpenName + QString::number(i)+".swc";
            export_list2file(outswc,outswc_file,fileOpenName);


            simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
            if(im_cropped) {delete []im_cropped; im_cropped = 0;}

    //        QList<ImageMarker> listmarker,listmarker_o;

    //        ImageMarker u,u_o;

    //        u_o.x = S.x;
    //        u_o.y = S.y;
    //        u_o.z = S.z;
    //        u.x = S.x-xb;
    //        u.y = S.y-yb;
    //        u.z = S.z-zb;
    //        //v3d_msg("this is marker info");
    //        cout<<"this is marker info"<<endl;
    //        cout<<u_o.x<<"  "<<u_o.y<<"  "<<u_o.z<<endl;
    //        cout<<u.x<<"  "<<u.y<<"  "<<u.z<<endl;
    //        listmarker.push_back(u);
    //        listmarker_o.push_back(u_o);
    //        writeMarker_file(QString(QString::number(i)+".marker"),listmarker);
    //        writeMarker_file(QString(QString::number(i)+"_o.marker"),listmarker_o);

            i=i+50;
        }

    }

    else if(method == 2)
    {
        V3DLONG im_cropped_sz[4];

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
            outimg_file = fileOpenName + QString::number(i)+".tif";
            outswc_file = fileOpenName + QString::number(i)+".swc";
            export_list2file(outswc,outswc_file,fileOpenName);


            simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
            if(im_cropped) {delete []im_cropped; im_cropped = 0;}

    //        QList<ImageMarker> listmarker,listmarker_o;

    //        ImageMarker u,u_o;

    //        u_o.x = S.x;
    //        u_o.y = S.y;
    //        u_o.z = S.z;
    //        u.x = S.x-xb;
    //        u.y = S.y-yb;
    //        u.z = S.z-zb;
    //        //v3d_msg("this is marker info");
    //        cout<<"this is marker info"<<endl;
    //        cout<<u_o.x<<"  "<<u_o.y<<"  "<<u_o.z<<endl;
    //        cout<<u.x<<"  "<<u.y<<"  "<<u.z<<endl;
    //        listmarker.push_back(u);
    //        listmarker_o.push_back(u_o);
    //        writeMarker_file(QString(QString::number(i)+".marker"),listmarker);
    //        writeMarker_file(QString(QString::number(i)+"_o.marker"),listmarker_o);

            i=i+50;
        }

    }

    return true;

}




