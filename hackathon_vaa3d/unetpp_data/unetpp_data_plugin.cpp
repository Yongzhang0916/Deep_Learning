/* unetpp_data_plugin.cpp
 * This is a test plugin, you can use it as a demo.
 * 2019-5-31 : by Yongzhang
 */
 
#include "v3d_message.h"
#include <vector>
#include "string"
#include "iostream"
#include "basic_surf_objs.h"
#include "resampling.h"
#include "sort_swc.h"
#include "../../../released_plugins/v3d_plugins/mean_shift_center/mean_shift_fun.cpp"
#include "../../../released_plugins/v3d_plugins/mean_shift_center/mean_shift_fun.h"

#include "unetpp_data_plugin.h"

#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))
Q_EXPORT_PLUGIN2(unetpp_data, unetpp_dataPlugin);


using namespace std;

bool export_TXT(vector<double> &vec,QString fileSaveName);
bool export_TXT2(vector<ForefroundCoordinate> &vec_coord,QString fileSaveName);

QStringList unetpp_dataPlugin::menulist() const
{
	return QStringList() 
        <<tr("binary_image")
		<<tr("about");
}

QStringList unetpp_dataPlugin::funclist() const
{
	return QStringList()
		<<tr("tracing_func")
		<<tr("help");
}

void unetpp_dataPlugin::domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent)
{
    if (menu_name == tr("get_samples_allswc"))
    {
        v3d_msg("vaa3d -x unetpp_data -f get_samples_allswc -i <inimg_file> -p <allswc_file> -o <out_imagefile> <out_swcfile>\n"
                "inimg_file       The input image\n"
                "allswc_file      The input swc\n"
                "out_imagefile         The output image block\n"
                "out_swcfile         The output swc block\n");
    }
    else if (menu_name == tr("processing_swc"))
    {
        v3d_msg("vaa3d -x unetpp_data -f processing_swc -i <swc_file> -o <out_swcfile>\n"
                "swc_file         The input swc\n"
                "out_swcfile         The output swc block\n");
    }

    else if (menu_name == tr("get_samples_someswc"))
    {
        v3d_msg("vaa3d -x unetpp_data -f get_samples_someswc -i <inimg_file> <allswc_file> -p <someswc_file> -o <out_imagefile> <out_swcfile>\n"
                "inimg_file       The input image\n"
                "swc_file         The input swc\n"
                "out_imagefile         The output image block\n"
                "out_swcfile         The output swc block\n");
    }

    else if (menu_name == tr("get_smaples_soma"))
    {
        v3d_msg("vaa3d -x unetpp_data -f get_samples_soma -i <inimg_file>  -p <swc_file> <soma_file> -o <out_file>\n"
                "inimg_file       The input image\n"
                "swc_file         The input swc\n"
                "soma_file         The input soma\n"
                "out_file         The output image and swc block\n");
    }

    else if (menu_name == tr("binary_image"))
    {
        input_PARA PA;
        binary_image(callback,parent,PA);
    }

    else if (menu_name == tr("combined_image_mask"))
    {
        v3d_msg("vaa3d -x unetpp_data -f combined_image_mask -i <inimg_file>  -p <mask_inimg> -o <out_file>\n"
                "inimg_file       The input image\n"
                "mask_inimg       The input inimg\n"
                "out_file         The output image and swc block\n");
    }

    else if (menu_name == tr("combined_image_mask_batch"))
    {
        v3d_msg("vaa3d -x unetpp_data -f combined_image_mask_batch -i <inimg_file>  -p <mask_inimg> -o <out_file>\n"
                "inimg_file       The input image\n"
                "mask_inimg       The input inimg\n"
                "out_file         The output image and swc block\n");
    }

    else if (menu_name == tr("combined_image_predict"))
    {
        v3d_msg("vaa3d -x unetpp_data -f combined_image_predict -i <inimg_file>  -p <predict_inimg> -o <out_file>\n"
                "inimg_file       The input image\n"
                "predict_inimg       The input inimg\n"
                "out_file         The output image and swc block\n");
    }

    else if (menu_name == tr("evaluate_predict_mask"))
    {
        v3d_msg("vaa3d -x unetpp_data -f evaluate_predict_mask -i <predict_inimg>  -p <mask_inimg> -o <out_file>\n"
                "predict_inimg    The input predict inimg\n"
                "mask_inimg       The input mask image\n"
                "out_file         The txt file\n");
    }

    else if (menu_name == tr("evaluate_binary_predict_mask"))
    {
        v3d_msg("vaa3d -x unetpp_data -f evaluate_binary_predict_mask -i <predict_inimg>  -p <mask_inimg> -o <out_file>\n"
                "predict_inimg    The input predict inimg\n"
                "mask_inimg       The input mask image\n"
                "out_file         The txt file\n");
    }

    else if (menu_name == tr("evaluate_predict_mask"))
    {
        v3d_msg("vaa3d -x unetpp_data -f evluate_whole_reconstruction -i <gold_standard_swc>  -p <Auto_trcing_swc> -o <out_file>\n"
                "gold_standard_swc     The input gs_swc\n"
                "Auto_trcing_swc      The input swc\n"
                "out_file               The txt file\n");
    }

    else if (menu_name == tr("get_smaples_soma"))
    {
        v3d_msg("vaa3d -x unetpp_data -f swc_processing -i <swc_file> \n"
                "inimg_file       The input image\n"
                "swc_file         The input swc\n");
    }

    else if (menu_name == tr("signal_ratio"))
    {
        v3d_msg("vaa3d -x unetpp_data -f signal_ratio -i image \n"
                "inimg_file       The input image\n");
    }
    else if (menu_name == tr("evaluate_whole_swc"))
    {
        v3d_msg("vaa3d -x unetpp_data -f evaluate_whole_swc -i <manual_swc>  -p <auto_swc> <unetpp_swc> -o <out_file>\n"
                "manual_swc    The input manual_swc\n"
                "auto_swc       The input auto_swc\n"
                "out_file         The txt file\n");
    }

	else
	{
		v3d_msg(tr("This is a test plugin, you can use it as a demo.. "
			"Developed by Yongzhang, 2019-5-31"));
	}
}

bool unetpp_dataPlugin::dofunc(const QString & func_name, const V3DPluginArgList & input, V3DPluginArgList & output, V3DPluginCallback2 & callback,  QWidget * parent)
{
    input_PARA P;
    vector<char*> * pinfiles = (input.size() >= 1) ? (vector<char*> *) input[0].p : 0;
    vector<char*> * pparas = (input.size() >= 2) ? (vector<char*> *) input[1].p : 0;
    vector<char*> infiles = (pinfiles != 0) ? * pinfiles : vector<char*>();
    vector<char*> paras = (pparas != 0) ? * pparas : vector<char*>();
//    vector<char*>* outlist = NULL;
    vector<char*>* outlist = (vector<char*>*)(output.at(0).p);

    P.inimg_file = infiles[0];

    cout<<"inimg : "<<P.inimg_file.toStdString()<<endl;

    if (func_name == tr("get_samples_allswc"))
    {
        int k=0;
        QString swc_file = paras.empty() ? "" : paras[k]; if(swc_file == "NULL") swc_file = ""; k++;
        if(swc_file.isEmpty())
        {
            cerr<<"Need a swc file"<<endl;
            return false;
        }
        else
            P.swc_file = swc_file;

        cout<<"swc : "<<P.swc_file.toStdString()<<endl;

//        if (output.size()==2)
//        {
            //outlist = (vector<char*>*)(output.at(0).p);
            P.out_imagefile = QString(outlist->at(0));
            P.out_swcfile = QString(outlist->at(1));
       // }

        get_samples_allswc(callback,parent,input,output,P);
    }

    else if (func_name == tr("processing_swc"))
    {
        processing_swc(callback,parent,input,output,P);
    }

    else if (func_name == tr("get_samples_soma"))
    {
        P.swc_file = paras[0];
        P.soma_file = paras[1];

        cout<<"swc_file :  "<<P.swc_file.toStdString()<<endl;
        cout<<"soma_file : "<<P.soma_file.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }

        get_samples_soma(callback,parent,input,output,P);
    }
    else if (func_name == tr("get_samples_someswc"))
    {
        P.swc_file = infiles[1];
        cout<<"Allswc : "<<P.swc_file.toStdString()<<endl;

        int k=0;
        QString some_swc = paras.empty() ? "" : paras[k]; if(some_swc == "NULL") some_swc = ""; k++;
        if(some_swc.isEmpty())
        {
            cerr<<"Need a some swc file"<<endl;
            return false;
        }
        else
            P.some_swc = some_swc;

        cout<<"swc : "<<P.some_swc.toStdString()<<endl;

//        if (output.size()==1)
//        {
//            outlist = (vector<char*>*)(output.at(0).p);
//            P.out_file = QString(outlist->at(0));
//        }

        P.out_imagefile = QString(outlist->at(0));
        P.out_swcfile = QString(outlist->at(1));


        get_samples_someswc(callback,parent,input,output,P);
    }

    else if (func_name == tr("binary_image"))
    {
        binary_image(callback,parent,input,P);
    }

    else if (func_name == tr("combined_image_mask"))
    {
        P.mask_inimg = paras[0];
        cout<<"mask_inimg : "<<P.mask_inimg.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }

        combined_image_mask(callback,parent,input,output,P);
    }

    else if (func_name == tr("combined_image_mask_batch"))
    {
        P.mask_inimg = paras[0];
        cout<<"mask_inimg : "<<P.mask_inimg.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }

        combined_image_mask_batch(callback,parent,input,output,P);
    }

    else if (func_name == tr("combined_image_predict"))
    {
        P.unet_inimg = paras[0];
        cout<<"unet_inimg : "<<P.unet_inimg.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }

        combined_image_predict(callback,parent,input,output,P);
    }

    else if (func_name == tr("evaluate_predict_mask"))
    {
        P.mask_inimg = paras[0];
        cout<<"mask_inimg : "<<P.mask_inimg.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }

        evaluate_predict_mask(callback,parent,input,output,P);
    }

    else if (func_name == tr("evaluate_binary_predict_mask"))
    {
        P.mask_inimg = paras[0];
        cout<<"mask_inimg : "<<P.mask_inimg.toStdString()<<endl;

        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }
        evaluate_binary_predict_mask(callback,parent,input,output,P);
    }

    else if (func_name == tr("evluate_whole_reconstruction"))
    {
        P.swc_file1 = paras[0];
        P.swc_file2 = paras[1];
        cout<<"swc_file : "<<P.swc_file.toStdString()<<endl;
//        if (output.size()==1)
//        {
//            outlist = (vector<char*>*)(output.at(0).p);
//            P.out_file = QString(outlist->at(0));
//        }
        P.out_file1 = QString(outlist->at(0));
        P.out_file2 = QString(outlist->at(1));
        P.out_file3 = QString(outlist->at(2));


        evluate_whole_reconstruction(callback,parent,input,output,P);
    }

    else if (func_name == tr("evaluate_whole_swc"))
    {
        P.swc_file1 = paras[0];
        P.swc_file2 = paras[1];
        cout<<"swc_file : "<<P.swc_file.toStdString()<<endl;
//        if (output.size()==1)
//        {
//            outlist = (vector<char*>*)(output.at(0).p);
//            P.out_file = QString(outlist->at(0));
//        }
        P.out_file1 = QString(outlist->at(0));
        P.out_file2 = QString(outlist->at(1));
        P.out_file3 = QString(outlist->at(2));

        evaluate_whole_swc(callback,parent,input,output,P);
    }

    else if (func_name == tr("swc_processing"))
    {
        input_PARA PARA;
        swc_processing(input,output);
    }
    else if (func_name == tr("signal_ratio"))
    {
        if (output.size()==1)
        {
            outlist = (vector<char*>*)(output.at(0).p);
            P.out_file = QString(outlist->at(0));
        }
        signal_ratio(callback,parent,input,output,P);
    }

    else if (func_name == tr("help"))
    {

        v3d_msg(tr("This is a test plugin, you can use it as a demo.. "
            "Developed by Yongzhang, 2019-5-31"));

	}
	else return false;

	return true;
}

bool processing_swc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    NeuronTree nt = readSWC_file(PA.inimg_file);
    cout<<"nt.size: "<<nt.listNeuron.size()<<endl;

    QList<NeuronSWC> swc_new;
    NeuronTree nt_new;
    double n = 2.0;
    for(int i = 0;i < nt.listNeuron.size();i++)
    {
        NeuronSWC cur;
        cur.x = n*nt.listNeuron[i].x;
        cur.y = n*nt.listNeuron[i].y;
        cur.z = n*nt.listNeuron[i].z;
        cur.n = nt.listNeuron[i].n;
        cur.parent = nt.listNeuron[i].parent;
        cur.radius = nt.listNeuron[i].radius;
        cur.type = nt.listNeuron[i].type;
        swc_new.push_back(cur);
    }
    nt_new.listNeuron = swc_new;

    //resample swc
//    double step=10.0;
//    NeuronTree nt_resample = resample(nt_new,step);
//    cout<<"nt_resample size : "<<nt_resample.listNeuron.size()<<endl;

//    V3DLONG rootid = VOID;
//    V3DLONG thres = 0;
//    QList<NeuronSWC> swc_sort;
//    NeuronTree nt_sort;
//    SortSWC(nt_resample.listNeuron, swc_sort ,rootid, thres);
//    nt_sort.listNeuron = swc_sort;

    QString fileSaveName = PA.inimg_file + "_pre.swc";
    writeSWC_file(fileSaveName,nt_new);

    return true;
}

bool get_samples_allswc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P)
{
    //load image
    QString imagepath = P.inimg_file;

    //load and processing swc
    QStringList swclist = importFileList_addnumbersort(P.swc_file);
    cout<<"swclist = "<<swclist.size()<<endl;
    double step1=1.0;
    double l_x = 32;
    double l_y = 32;
    double l_z = 16;
    double step2 = 100;
    V3DLONG nt_size = 60;
    QList<NeuronTree> All_nt;
    for(int i = 2;i < swclist.size();i++)
    {
        //cout<<"swclist : "<<swclist[i].toStdString()<<endl;
        NeuronTree nt = readSWC_file(swclist[i]);
        //resample swc
        NeuronTree nt_resample = resample(nt,step1);
        cout<<"nt_resample = "<<nt_resample.listNeuron.size()<<endl;

        All_nt.push_back(nt_resample);

    }
    cout<<"All_nt.size = "<<All_nt.size()<<endl;

    //get samples
    for(int i = 2;i < swclist.size();i++)
    {
        //cout<<"swclist : "<<swclist[i].toStdString()<<endl;
        NeuronTree nt = readSWC_file(swclist[i]);
        //resample swc
        NeuronTree nt_resample = resample(nt,step1);

        for(V3DLONG j =0;j < nt_resample.listNeuron.size();j=j+step2)
        {
            NeuronSWC cur;
            cur.x = nt_resample.listNeuron[j].x;
            cur.y = nt_resample.listNeuron[j].y;
            cur.z = nt_resample.listNeuron[j].z;

            V3DLONG xb = cur.x-l_x;
            V3DLONG xe = cur.x+l_x-1;
            V3DLONG yb = cur.y-l_y;
            V3DLONG ye = cur.y+l_y-1;
            V3DLONG zb = cur.z-l_z;
            V3DLONG ze = cur.z+l_z-1;

            V3DLONG im_cropped_sz[4];
            unsigned char * im_cropped = 0;
            V3DLONG pagesz;

            pagesz = (xe-xb+1)*(ye-yb+1)*(ze-zb+1);
            im_cropped_sz[0] = xe-xb+1;
            im_cropped_sz[1] = ye-yb+1;
            im_cropped_sz[2] = ze-zb+1;
            im_cropped_sz[3] = 1;

            try {im_cropped = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

            QList<NeuronSWC> outswc, outswc_sort;
            int lens=0;
            for(V3DLONG k = 0; k < All_nt.size();k++)
            {
                for(V3DLONG l= 0;l < All_nt[k].listNeuron.size();l++)
                {
                    NeuronSWC S;
                    if(All_nt[k].listNeuron[l].x<xe&&All_nt[k].listNeuron[l].x>xb&&All_nt[k].listNeuron[l].y<ye&&All_nt[k].listNeuron[l].y>yb&&All_nt[k].listNeuron[l].z<ze&&All_nt[k].listNeuron[l].z>zb)
                    {
                        S.x = All_nt[k].listNeuron[l].x-xb;
                        S.y = All_nt[k].listNeuron[l].y-yb;
                        S.z = All_nt[k].listNeuron[l].z-zb;
                        S.n = All_nt[k].listNeuron[l].n+lens;
                        S.pn = All_nt[k].listNeuron[l].pn+lens;
                        S.r = All_nt[k].listNeuron[l].r;
                        S.type = All_nt[k].listNeuron[l].type;

                        outswc.push_back(S);
                    }
                }
                lens=lens+All_nt[k].listNeuron.size();

            }

            V3DLONG rootid = VOID;
            V3DLONG thres = 0;
            SortSWC(outswc, outswc_sort ,rootid, thres);

            im_cropped = callback.getSubVolumeTeraFly(imagepath.toStdString(),xb,xe+1,
                                                  yb,ye+1,zb,ze+1);

            //mean shift center
            //QList<NeuronSWC> outswc_center;
            //mean_shift_center(outswc_sort,outswc_center,im_cropped, im_cropped_sz);

            cout<<"outswc_sort = "<<outswc_sort.size()<<endl;
            //cout<<"outswc_center = "<<outswc_center.size()<<endl;

            if(outswc_sort.size() < nt_size)
                continue;

            QString outimg_file,outswc_file,outswc_file2;
            outimg_file = P.out_imagefile+"/"+QString::number(i-1)+"_"+QString("x_%1_y_%2_z_%3").arg(xb).arg(yb).arg(zb)+".tif";
            outswc_file = P.out_swcfile+"/"+QString::number(i-1)+"_"+QString("x_%1_y_%2_z_%3").arg(xb).arg(yb).arg(zb)+".swc";
            //outswc_file2 = P.out_file+"/"+QString::number(i-1)+"_"+QString("x_%1_y_%2_z_%3").arg(xb).arg(yb).arg(zb)+"_meanshift.swc";
            cout<<"outimg_file : "<<outimg_file.toStdString()<<endl;
            cout<<"outswc_file : "<<outswc_file.toStdString()<<endl;
            export_list2file(outswc_sort,outswc_file,outswc_file);
            //export_list2file(outswc_center,outswc_file2,outswc_file2);

            simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
            //if(im_cropped) {delete []im_cropped; im_cropped = 0;}

        }

    }

    return true;
}


bool get_samples_someswc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P)
{
    //load image
    QString imagepath = P.inimg_file;

    //load and processing swc
    QStringList swclist = importFileList_addnumbersort(P.swc_file);
    cout<<"swclist = "<<swclist.size()<<endl;

    double step1=1.0;
    double l_x = 128;
    double l_y = 128;
    double l_z = 64;
    double step2 = 120;
    V3DLONG nt_size = 60;

    QList<NeuronTree> All_nt;
    for(int i = 2;i < swclist.size();i++)
    {
        //cout<<"swclist : "<<swclist[i].toStdString()<<endl;
        NeuronTree nt = readSWC_file(swclist[i]);
        //resample swc
        NeuronTree nt_resample = resample(nt,step1);
        cout<<"nt_resample = "<<nt_resample.listNeuron.size()<<endl;

        All_nt.push_back(nt_resample);

    }
    cout<<"All_nt.size = "<<All_nt.size()<<endl;

    //get samples
   QStringList someswclist = importFileList_addnumbersort(P.some_swc);
   cout<<"someswxlist = "<<someswclist.size()<<endl;

    for(int i = 2;i < someswclist.size();i++)
    {
        //cout<<"swclist : "<<swclist[i].toStdString()<<endl;
        NeuronTree nt = readSWC_file(someswclist[i]);
        //resample swc
        NeuronTree nt_resample = resample(nt,step1);

        for(V3DLONG j =0;j < nt_resample.listNeuron.size();j=j+step2)
        {
            NeuronSWC cur;
            cur.x = nt_resample.listNeuron[j].x;
            cur.y = nt_resample.listNeuron[j].y;
            cur.z = nt_resample.listNeuron[j].z;

            V3DLONG xb = cur.x-l_x;
            V3DLONG xe = cur.x+l_x-1;
            V3DLONG yb = cur.y-l_y;
            V3DLONG ye = cur.y+l_y-1;
            V3DLONG zb = cur.z-l_z;
            V3DLONG ze = cur.z+l_z-1;


            V3DLONG im_cropped_sz[4];
            unsigned char * im_cropped = 0;
            V3DLONG pagesz;

            pagesz = (xe-xb+1)*(ye-yb+1)*(ze-zb+1);
            im_cropped_sz[0] = xe-xb+1;
            im_cropped_sz[1] = ye-yb+1;
            im_cropped_sz[2] = ze-zb+1;
            im_cropped_sz[3] = 1;

            try {im_cropped = new unsigned char [pagesz];}
            catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

            QList<NeuronSWC> outswc, outswc_sort;
            int lens=0;
            for(V3DLONG k = 0; k < All_nt.size();k++)
            {
                for(V3DLONG l= 0;l < All_nt[k].listNeuron.size();l++)
                {
                    NeuronSWC S;
                    if(All_nt[k].listNeuron[l].x<xe&&All_nt[k].listNeuron[l].x>xb&&All_nt[k].listNeuron[l].y<ye&&All_nt[k].listNeuron[l].y>yb&&All_nt[k].listNeuron[l].z<ze&&All_nt[k].listNeuron[l].z>zb)
                    {
                        S.x = All_nt[k].listNeuron[l].x-xb;
                        S.y = All_nt[k].listNeuron[l].y-yb;
                        S.z = All_nt[k].listNeuron[l].z-zb;
                        S.n = All_nt[k].listNeuron[l].n+lens;
                        S.pn = All_nt[k].listNeuron[l].pn+lens;
                        S.r = All_nt[k].listNeuron[l].r;
                        S.type = All_nt[k].listNeuron[l].type;

                        outswc.push_back(S);
                    }
                }
                lens=lens+All_nt[k].listNeuron.size();

            }

            V3DLONG rootid = VOID;
            V3DLONG thres = 0;
            SortSWC(outswc, outswc_sort ,rootid, thres);

            cout<<"1111111111111111111111"<<endl;
            im_cropped = callback.getSubVolumeTeraFly(imagepath.toStdString(),xb,xe+1,
                                                  yb,ye+1,zb,ze+1);

            cout<<"2222222222222222222222222"<<endl;
            if(outswc_sort.size() < nt_size)
                continue;

            QString outimg_file,outswc_file;
            outimg_file = P.out_imagefile +"/"+QString("x_%1_y_%2_z_%3").arg(xb).arg(yb).arg(zb)+".tif";
            outswc_file = P.out_swcfile +"/"+QString("x_%1_y_%2_z_%3").arg(xb).arg(yb).arg(zb)+".swc";
            export_list2file(outswc_sort,outswc_file,outswc_file);

            simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
//            if(im_cropped) {delete []im_cropped; im_cropped = 0;}

        }

    }
    return true;

}


bool get_samples_soma(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P)
{
    //load image
    QString imagepath = P.inimg_file;

    //load and processing swc
    QStringList swclist = importFileList_addnumbersort(P.swc_file);
    cout<<"swclist = "<<swclist.size()<<endl;

    double step=1.0;

    double l_x = 32;
    double l_y = 32;
    double l_z = 16;

    QList<NeuronTree> All_nt;
    for(int i = 2;i < swclist.size();i++)
    {
        //cout<<"swclist : "<<swclist[i].toStdString()<<endl;
        NeuronTree nt = readSWC_file(swclist[i]);
        //resample swc
        NeuronTree nt_resample = resample(nt,step);

        All_nt.push_back(nt_resample);

    }
    cout<<"All_nt.size = "<<All_nt.size()<<endl;


    //load soma
    NeuronTree nt = readSWC_file(P.soma_file);
    LandmarkList somaMarkers;
    for(V3DLONG i = 0;i < nt.listNeuron.size();i++)
    {
        LocationSimple t;
        t.x = nt.listNeuron[i].x;
        t.y = nt.listNeuron[i].y;
        t.z = nt.listNeuron[i].z;
        //somaMarkers.push_back(t);

        V3DLONG im_cropped_sz[4];

        unsigned char * im_cropped = 0;
        V3DLONG pagesz;
        V3DLONG xb = t.x-l_x;
        V3DLONG xe = t.x+l_x-1;
        V3DLONG yb = t.y-l_y;
        V3DLONG ye = t.y+l_y-1;
        V3DLONG zb = t.z-l_z;
        V3DLONG ze = t.z+l_z-1;
        pagesz = (xe-xb+1)*(ye-yb+1)*(ze-zb+1);
        im_cropped_sz[0] = xe-xb+1;
        im_cropped_sz[1] = ye-yb+1;
        im_cropped_sz[2] = ze-zb+1;
        im_cropped_sz[3] = 1;

        try {im_cropped = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        QList<NeuronSWC> outswc,outswc_sort;
        int lens=0;
        for(V3DLONG k = 0; k < All_nt.size();k++)
        {
            for(V3DLONG l= 0;l < All_nt[k].listNeuron.size();l++)
            {
                NeuronSWC S;
                if(All_nt[k].listNeuron[l].x<xe&&All_nt[k].listNeuron[l].x>xb&&All_nt[k].listNeuron[l].y<ye&&All_nt[k].listNeuron[l].y>yb&&All_nt[k].listNeuron[l].z<ze&&All_nt[k].listNeuron[l].z>zb)
                {
                    S.x = All_nt[k].listNeuron[l].x-xb;
                    S.y = All_nt[k].listNeuron[l].y-yb;
                    S.z = All_nt[k].listNeuron[l].z-zb;
                    S.n = All_nt[k].listNeuron[l].n+lens;
                    S.pn = All_nt[k].listNeuron[l].pn+lens;
                    S.r = All_nt[k].listNeuron[l].r;
                    S.type = All_nt[k].listNeuron[l].type;

                    outswc.push_back(S);
                }
            }
            lens=lens+All_nt[k].listNeuron.size();

        }

        V3DLONG rootid = VOID;
        V3DLONG thres = 0;
        SortSWC(outswc, outswc_sort ,rootid, thres);


        im_cropped = callback.getSubVolumeTeraFly(imagepath.toStdString(),xb,xe+1,
                                                  yb,ye+1,zb,ze+1);


        QString outimg_file,outswc_file;
        outimg_file = P.out_file+"/"+QString("x_%1_y_%2_z_%3").arg(t.x).arg(t.y).arg(t.z)+".tif";
        outswc_file = P.out_file+"/"+QString("x_%1_y_%2_z_%3").arg(t.x).arg(t.y).arg(t.z)+".swc";
        export_list2file(outswc_sort,outswc_file,outswc_file);

        simple_saveimage_wrapper(callback, outimg_file.toStdString().c_str(),(unsigned char *)im_cropped,im_cropped_sz,1);
    }



    return true;
}

bool combined_image_predict(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA)
{
    V3DLONG c=1;
    V3DLONG in_sz[4];
    unsigned char * data1d = 0;
    unsigned char * unet_data1d = 0;
    int dataType;
    if(!simple_loadimage_wrapper(callback, PA.inimg_file.toStdString().c_str(), data1d, in_sz, dataType))
    {
        cerr<<"load image "<<PA.inimg_file.toStdString()<<" error!"<<endl;
        return false;
    }


    if(!simple_loadimage_wrapper(callback, PA.unet_inimg.toStdString().c_str(), unet_data1d, in_sz, dataType))
    {
        cerr<<"load image "<<PA.unet_inimg.toStdString()<<" error!"<<endl;
        return false;
    }

    Image4DSimple* p4DImage = new Image4DSimple;
    p4DImage->setData((unsigned char*)data1d, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

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

    cout<<"**********combine image and unetImage.**********"<<endl;
    V3DLONG N = in_sz[0];
    V3DLONG M = in_sz[1];
    V3DLONG P = in_sz[2];
    V3DLONG C = in_sz[3];
    V3DLONG pagesz = N*M;
    V3DLONG tol_sz = pagesz * P;

    cout<<"tol_sz = "<<tol_sz<<endl;

    unsigned char * final_data1d = 0;
    try {final_data1d = new unsigned char [tol_sz];}
     catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

    //初始化
    for(V3DLONG i = 0; i < tol_sz;i++)
        final_data1d[i] = 0;

    double a = 0.3;

    for(V3DLONG iz = 0; iz < P; iz++)
    {
        V3DLONG offsetk = iz*M*N;
        for(V3DLONG iy = 0; iy < M; iy++)
        {
            V3DLONG offsetj = iy*N;
            for(V3DLONG ix = 0; ix < N; ix++)
            {
                if(data1d[offsetk + offsetj + ix]-unet_data1d[offsetk + offsetj + ix]>bkg_thresh)
                    final_data1d[offsetk + offsetj + ix] = unet_data1d[offsetk + offsetj + ix];
                else
                    final_data1d[offsetk + offsetj + ix] = a*(data1d[offsetk + offsetj + ix])+(1-a)*(unet_data1d[offsetk + offsetj + ix]);

            }

         }
    }

    QString outimg = PA.inimg_file.split(".").first() + "_final.tif";

    if(!simple_saveimage_wrapper(callback,outimg.toStdString().c_str(),(unsigned char *)final_data1d,in_sz,1))
    {
        cerr<<"save image "<<outimg.toStdString()<<" error!"<<endl;
        return false;
    }


    if(data1d) {delete []data1d; data1d = 0;}
    if(unet_data1d) {delete []unet_data1d;unet_data1d = 0;}
    if(final_data1d) {delete []final_data1d; final_data1d = 0;}

    return true;

 }

bool combined_image_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA)
{
    V3DLONG c=1;
    V3DLONG in_sz[4];
    unsigned char * data1d = 0;
    unsigned char * mask_data1d = 0;
    int dataType;
    if(!simple_loadimage_wrapper(callback, PA.inimg_file.toStdString().c_str(), data1d, in_sz, dataType))
    {
        cerr<<"load image "<<PA.inimg_file.toStdString()<<" error!"<<endl;
        return false;
    }


    if(!simple_loadimage_wrapper(callback, PA.mask_inimg.toStdString().c_str(), mask_data1d, in_sz, dataType))
    {
        cerr<<"load image "<<PA.mask_inimg.toStdString()<<" error!"<<endl;
        return false;
    }


    V3DLONG N = in_sz[0];
    V3DLONG M = in_sz[1];
    V3DLONG P = in_sz[2];
    V3DLONG C = in_sz[3];
    V3DLONG pagesz = N*M;
    V3DLONG tol_sz = pagesz * P;

    cout<<"tol_sz = "<<tol_sz<<endl;

    unsigned char * label_data1d = 0;
    try {label_data1d = new unsigned char [tol_sz];}
     catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

    //初始化
    for(V3DLONG i = 0; i < tol_sz;i++)
        label_data1d[i] = 0;

    double origin_max = 0;
    double origin_min = 255;
    double mask_max = 0;
    double mask_min = 255;
    for(V3DLONG iz = 0; iz < P; iz++)
    {
        V3DLONG offsetk = iz*M*N;
        for(V3DLONG iy = 0; iy < M; iy++)
        {
            V3DLONG offsetj = iy*N;
            for(V3DLONG ix = 0; ix < N; ix++)
            {
                if(data1d[offsetk + offsetj + ix] >= origin_max)
                    origin_max = data1d[offsetk + offsetj + ix];

                if(data1d[offsetk + offsetj + ix] <= origin_min)
                    origin_min = data1d[offsetk + offsetj + ix];

                if(mask_data1d[offsetk + offsetj + ix] >= mask_max)
                    mask_max = mask_data1d[offsetk + offsetj + ix];

                if(mask_data1d[offsetk + offsetj + ix] <= mask_min)
                    mask_min = mask_data1d[offsetk + offsetj + ix];

            }

        }

    }

    cout<<"origin_max = "<<origin_max<<endl;
    cout<<"origin_min = "<<origin_min<<endl;
    cout<<"mask_max = "<<mask_max<<endl;
    cout<<"mask_min = "<<mask_min<<endl;

    for(V3DLONG iz = 0; iz < P; iz++)
    {
        V3DLONG offsetk = iz*M*N;
        for(V3DLONG iy = 0; iy < M; iy++)
        {
            V3DLONG offsetj = iy*N;
            for(V3DLONG ix = 0; ix < N; ix++)
            {
                if(mask_data1d[offsetk + offsetj + ix] ==0)
                    label_data1d[offsetk + offsetj + ix] = 0;

                else
                {
                    if(data1d[offsetk + offsetj + ix] == 0)
                    label_data1d[offsetk + offsetj + ix]=1.7*mask_data1d[offsetk + offsetj + ix];
                    else
                        label_data1d[offsetk + offsetj + ix]= data1d[offsetk + offsetj + ix];
                }




                //method1:---label
//                if(data1d[offsetk + offsetj + ix]>mask_max && mask_data1d[offsetk + offsetj + ix]< mask_max)
//                    label_data1d[offsetk + offsetj + ix]= data1d[offsetk + offsetj + ix];
//                else
//                    label_data1d[offsetk + offsetj + ix]= mask_data1d[offsetk + offsetj + ix];

                //method2:---label
//                if(data1d[offsetk + offsetj + ix]>mask_max && mask_data1d[offsetk + offsetj + ix]< mask_max)
//                    label_data1d[offsetk + offsetj + ix] = 0.7*((data1d[offsetk + offsetj + ix]-origin_min)*(255-mask_max)/(origin_max-origin_min)+mask_max);
//                else
//                    label_data1d[offsetk + offsetj + ix]= mask_data1d[offsetk + offsetj + ix];

//                if(label_data1d[offsetk + offsetj + ix] > 0)
//                    label_data1d[offsetk + offsetj + ix] = 255;
//                else
//                    label_data1d[offsetk + offsetj + ix] = 0;

                //method:---unetpp
//                if(data1d[offsetk + offsetj + ix]>=target_max && target_data1d[offsetk + offsetj + ix]<= target_max)
//                    label_data1d[offsetk + offsetj + ix] = (data1d[offsetk + offsetj + ix]-origin_min)*(255-target_max)/(origin_max-origin_min)+target_max;
//                else if(target_data1d[offsetk + offsetj + ix] < 0.5*target_max)
//                    label_data1d[offsetk + offsetj + ix]= 0;
//                else
//                {
//                    if(data1d[offsetk + offsetj + ix]>=target_data1d[offsetk + offsetj + ix])
//                        label_data1d[offsetk + offsetj + ix]= data1d[offsetk + offsetj + ix];
//                    else
//                        label_data1d[offsetk + offsetj + ix]= target_data1d[offsetk + offsetj + ix];
//                }

            }

        }

    }


    QString outimg = PA.inimg_file.split(".").first() + "_final.tif";

    if(!simple_saveimage_wrapper(callback,outimg.toStdString().c_str(),(unsigned char *)label_data1d,in_sz,1))
    {
        cerr<<"save image "<<outimg.toStdString()<<" error!"<<endl;
        return false;
    }


    if(data1d) {delete []data1d; data1d = 0;}
    if(mask_data1d) {delete []mask_data1d;mask_data1d = 0;}
    if(label_data1d) {delete []label_data1d; label_data1d = 0;}

    return true;

}

bool combined_image_mask_batch(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA)
{
    QStringList imagelist = importFileList_addnumbersort(PA.inimg_file);
    QStringList masklist = importFileList_addnumbersort(PA.mask_inimg);

    vector<QString> trainImages;
    vector<QString> trainMasks;
    for(int i = 2;i < imagelist.size();i++)
    {
        QString train_img_file = imagelist.at(i);
        trainImages.push_back(train_img_file);
    }
    cout<<"train image numbers : "<<trainImages.size()<<endl;
    //checkout
//    for(int i = 0;i < trainImages.size();i++)
//    {
//        cout<<"trian image : "<<trainImages.at(i).toStdString()<<endl;
//    }

    if (trainImages.size() < 1)
    {
        cout<<"No input train images!"<<endl;
        return false;
    }

    for(int i = 2;i < masklist.size();i++)
    {
        QString train_mask_file = masklist.at(i);
        trainMasks.push_back(train_mask_file);
    }
    cout<<"train mask numbers : "<<trainMasks.size()<<endl;

    if (trainMasks.size() < 1)
    {
        cout<<"No input train mask!"<<endl;
        return false;
    }
    if(trainMasks.size() != trainImages.size())
    {
        cout<<"Number of input train images and masks files must be the same !"<<endl;
        return false;
     }


    //v3d_msg("checkout");

    for(int i = 0; i < trainImages.size();i++)
    {
        cout<<"Processing Train Image and Mask: "<< i+1<< "/"<< trainImages.size() << endl;
        V3DLONG c=1;
        V3DLONG in_sz[4];
        unsigned char * data1d = 0;
        unsigned char * mask_data1d = 0;
        int dataType;
        if(!simple_loadimage_wrapper(callback, trainImages.at(i).toStdString().c_str(), data1d, in_sz, dataType))
        {
            cerr<<"load image "<<trainImages.at(i).toStdString()<<" error!"<<endl;
            return false;
        }


        if(!simple_loadimage_wrapper(callback, trainMasks.at(i).toStdString().c_str(), mask_data1d, in_sz, dataType))
        {
            cerr<<"load image "<<trainMasks.at(i).toStdString()<<" error!"<<endl;
            return false;
        }


        V3DLONG N = in_sz[0];
        V3DLONG M = in_sz[1];
        V3DLONG P = in_sz[2];
        V3DLONG C = in_sz[3];
        V3DLONG pagesz = N*M;
        V3DLONG tol_sz = pagesz * P;

        cout<<"tol_sz = "<<tol_sz<<endl;

        unsigned char * label_data1d = 0;
        try {label_data1d = new unsigned char [tol_sz];}
         catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        //初始化
        for(V3DLONG i = 0; i < tol_sz;i++)
            label_data1d[i] = 0;

        double origin_max = 0;
        double origin_min = 255;
        double mask_max = 0;
        double mask_min = 255;
        for(V3DLONG iz = 0; iz < P; iz++)
        {
            V3DLONG offsetk = iz*M*N;
            for(V3DLONG iy = 0; iy < M; iy++)
            {
                V3DLONG offsetj = iy*N;
                for(V3DLONG ix = 0; ix < N; ix++)
                {
                    if(data1d[offsetk + offsetj + ix] >= origin_max)
                        origin_max = data1d[offsetk + offsetj + ix];

                    if(data1d[offsetk + offsetj + ix] <= origin_min)
                        origin_min = data1d[offsetk + offsetj + ix];

                    if(mask_data1d[offsetk + offsetj + ix] >= mask_max)
                        mask_max = mask_data1d[offsetk + offsetj + ix];

                    if(mask_data1d[offsetk + offsetj + ix] <= mask_min)
                        mask_min = mask_data1d[offsetk + offsetj + ix];

                }

            }

        }

        cout<<"origin_max = "<<origin_max<<endl;
        cout<<"origin_min = "<<origin_min<<endl;
        cout<<"mask_max = "<<mask_max<<endl;
        cout<<"mask_min = "<<mask_min<<endl;

        for(V3DLONG iz = 0; iz < P; iz++)
        {
            V3DLONG offsetk = iz*M*N;
            for(V3DLONG iy = 0; iy < M; iy++)
            {
                V3DLONG offsetj = iy*N;
                for(V3DLONG ix = 0; ix < N; ix++)
                {
                    //method1:---label
    //                if(data1d[offsetk + offsetj + ix]>mask_max && mask_data1d[offsetk + offsetj + ix]< mask_max)
    //                    label_data1d[offsetk + offsetj + ix]= data1d[offsetk + offsetj + ix];
    //                else
    //                    label_data1d[offsetk + offsetj + ix]= mask_data1d[offsetk + offsetj + ix];

                    //method2:---label
                    if(data1d[offsetk + offsetj + ix]>mask_max && mask_data1d[offsetk + offsetj + ix]< mask_max)
                        label_data1d[offsetk + offsetj + ix] = 0.7*((data1d[offsetk + offsetj + ix]-origin_min)*(255-mask_max)/(origin_max-origin_min)+mask_max);
                    else
                        label_data1d[offsetk + offsetj + ix]= mask_data1d[offsetk + offsetj + ix];

    //                if(label_data1d[offsetk + offsetj + ix] > 0)
    //                    label_data1d[offsetk + offsetj + ix] = 255;
    //                else
    //                    label_data1d[offsetk + offsetj + ix] = 0;

                    //method:---unetpp
    //                if(data1d[offsetk + offsetj + ix]>=target_max && target_data1d[offsetk + offsetj + ix]<= target_max)
    //                    label_data1d[offsetk + offsetj + ix] = (data1d[offsetk + offsetj + ix]-origin_min)*(255-target_max)/(origin_max-origin_min)+target_max;
    //                else if(target_data1d[offsetk + offsetj + ix] < 0.5*target_max)
    //                    label_data1d[offsetk + offsetj + ix]= 0;
    //                else
    //                {
    //                    if(data1d[offsetk + offsetj + ix]>=target_data1d[offsetk + offsetj + ix])
    //                        label_data1d[offsetk + offsetj + ix]= data1d[offsetk + offsetj + ix];
    //                    else
    //                        label_data1d[offsetk + offsetj + ix]= target_data1d[offsetk + offsetj + ix];
    //                }

                }

            }

        }


        QString outimg = trainImages.at(i).split(".").first() + "_label.tif";

        if(!simple_saveimage_wrapper(callback,outimg.toStdString().c_str(),(unsigned char *)label_data1d,in_sz,1))
        {
            cerr<<"save image "<<outimg.toStdString()<<" error!"<<endl;
            return false;
        }


        if(data1d) {delete []data1d; data1d = 0;}
        if(mask_data1d) {delete []mask_data1d;mask_data1d = 0;}
        if(label_data1d) {delete []label_data1d; label_data1d = 0;}

    }

    return true;

}

bool binary_image(V3DPluginCallback2 &callback, QWidget *parent, input_PARA &PA)
{
    v3dhandle curwin = callback.currentImageWindow();
    if (!curwin)
    {
        QMessageBox::information(0, "", "You don't have any image open in the main window.");
        return false;
    }

    Image4DSimple* p4DImage = callback.getImage(curwin);

    if (!p4DImage)
    {
        QMessageBox::information(0, "", "The image pointer is invalid. Ensure your data is valid and try again!");
        return false;
    }

    cout<<"**********choose threshold.**********"<<endl;
    unsigned char* data1d = p4DImage->getRawData();

    V3DLONG c=1;
    V3DLONG in_sz[4];
    in_sz[0]=p4DImage->getXDim();
    in_sz[1]=p4DImage->getYDim();
    in_sz[2]=p4DImage->getZDim();
    in_sz[3]=p4DImage->getCDim();

    double imgAve, imgStd, bkg_thresh;
    mean_and_std(p4DImage->getRawData(), p4DImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);

//    double td= (imgStd<15)? 15: imgStd;
//    bkg_thresh = imgAve +0.8*td;

    double td= (imgStd<10)? 10: imgStd;
    bkg_thresh = imgAve +0.5*td;



    QString avgAndstd=QString("avg:%1  std:%2").arg(imgAve).arg(imgStd);
    QString threshold=QString("threshold:%1").arg(bkg_thresh);
    qDebug() << "avgAndstd = "<<avgAndstd<<endl;
    qDebug() << "bkg_thresh = "<<bkg_thresh<<endl;
    v3d_msg("checkout");

    cout<<"**********make binary image.**********"<<endl;
    V3DLONG M = in_sz[0];
    V3DLONG N = in_sz[1];
    V3DLONG P = in_sz[2];
    V3DLONG C = in_sz[3];
    V3DLONG pagesz = M*N;
    V3DLONG tol_sz = pagesz * P;

    unsigned char *im_cropped = 0;
    try {im_cropped = new unsigned char [tol_sz];}
     catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

    for(V3DLONG i=0;i<tol_sz;i++)
    {
        if(double(data1d[i]) <  bkg_thresh)
        {
            im_cropped[i] = 0;
        }
        else
        {
            im_cropped[i] = 255;
        }
    }

    PA.inimg_file = p4DImage->getFileName();
    QString binaryImage = PA.inimg_file.split(".").first() + "_binary.tif";

    if(!simple_saveimage_wrapper(callback,binaryImage.toStdString().c_str(),im_cropped,in_sz,1))
    {
        cerr<<"save image "<<binaryImage.toStdString()<<" error!"<<endl;
        return false;
    }
    v3d_msg("binary done");
    return true;
}

bool binary_image(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,input_PARA &PA)
{
    cout<<"begin"<<endl;
    V3DLONG c=1;
    V3DLONG in_sz[4];
    unsigned char * data1d = 0;
    int dataType;
    if(!simple_loadimage_wrapper(callback, PA.inimg_file.toStdString().c_str(), data1d, in_sz, dataType))
    {
        cerr<<"load image "<<PA.inimg_file.toStdString()<<" error!"<<endl;
        return false;
    }
    Image4DSimple* p4DImage = new Image4DSimple;
    p4DImage->setData((unsigned char*)data1d, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

    cout<<"**********choose threshold.**********"<<endl;
//    unsigned char* totaldata1d = p4DImage->getRawData();
    in_sz[0]=p4DImage->getXDim();
    in_sz[1]=p4DImage->getYDim();
    in_sz[2]=p4DImage->getZDim();
    in_sz[3]=p4DImage->getCDim();

    double imgAve, imgStd, bkg_thresh;
   mean_and_std(p4DImage->getRawData(), p4DImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);

   double td= (imgStd<10)? 10: imgStd;
   bkg_thresh = imgAve +0.5*td;

//   bkg_thresh = 38;

    QString avgAndstd=QString("avg:%1  std:%2").arg(imgAve).arg(imgStd);
    QString threshold=QString("threshold:%1").arg(bkg_thresh);
    qDebug() << "avgAndstd = "<<avgAndstd<<endl;
    qDebug() << "bkg_thresh = "<<bkg_thresh<<endl;

    cout<<"**********make binary image.**********"<<endl;
    V3DLONG M = in_sz[0];
    V3DLONG N = in_sz[1];
    V3DLONG P = in_sz[2];
    V3DLONG C = in_sz[3];
    V3DLONG pagesz = M*N;
    V3DLONG tol_sz = pagesz * P;

    unsigned char *im_cropped = 0;
    try {im_cropped = new unsigned char [tol_sz];}
     catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

    for(V3DLONG i=0;i<tol_sz;i++)
    {
        if(double(data1d[i]) <  bkg_thresh)
        {
            im_cropped[i] = 0;
        }
        else
        {
            im_cropped[i] = 255;
        }
    }

    QString binaryImage = PA.inimg_file.split(".").first() + "_binary.tif";

    if(!simple_saveimage_wrapper(callback,binaryImage.toStdString().c_str(),im_cropped,in_sz,1))
    {
        cerr<<"save image "<<binaryImage.toStdString()<<" error!"<<endl;
        return false;
    }

    return true;
}

bool evaluate_predict_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    QStringList predictlist = importFileList_addnumbersort(PA.inimg_file);
    QStringList masklist = importFileList_addnumbersort(PA.mask_inimg);

    vector<QString> predictImages;
    vector<QString> maskImages;
    for(int i = 2;i < predictlist.size();i++)
    {
        QString predict_img_file = predictlist.at(i);
        predictImages.push_back(predict_img_file);
    }
    cout<<"predict image numbers : "<<predictImages.size()<<endl;


    if (predictImages.size() < 1)
    {
        cout<<"No input predict images!"<<endl;
        return false;
    }

    for(int i = 2;i < masklist.size();i++)
    {
        QString mask_img_file = masklist.at(i);
        maskImages.push_back(mask_img_file);
    }
    cout<<"mask image numbers : "<<maskImages.size()<<endl;

    if (maskImages.size() < 1)
    {
        cout<<"No input mask image!"<<endl;
        return false;
    }
    if(predictImages.size() != maskImages.size())
    {
        cout<<"Number of input train images and masks files must be the same !"<<endl;
        return false;
    }
   // v3d_msg("checkout");

    vector<double> accuracy_all;
    vector<double> precision_all;
    vector<double> recall_all;
    vector<double> F1_all;
    vector<double> TPR_all;
    vector<double> FPR_all;

   //batch processing
    for(int i = 0; i < predictImages.size();i++)
    {
        cout<<"evaluate predict and mask images: "<< i+1<< "/"<< predictImages.size() << endl;
        //cout<<"evaluate predict mask begin!"<<endl;
        V3DLONG c=1;
        V3DLONG in_sz[4];
        unsigned char * predict_data1d = 0;
        unsigned char * mask_data1d = 0;
        int dataType;

        //load predict and mask images
        if(!simple_loadimage_wrapper(callback, predictImages.at(i).toStdString().c_str(), predict_data1d, in_sz, dataType))
        {
            cerr<<"load image "<<predictImages.at(i).toStdString()<<" error!"<<endl;
            return false;
        }

        if(!simple_loadimage_wrapper(callback, maskImages.at(i).toStdString().c_str(), mask_data1d, in_sz, dataType))
        {
            cerr<<"load image "<<maskImages.at(i).toStdString()<<" error!"<<endl;
            return false;
        }

        V3DLONG N = in_sz[0];
        V3DLONG M = in_sz[1];
        V3DLONG P = in_sz[2];
        V3DLONG C = in_sz[3];
        V3DLONG pagesz = N*M;
        V3DLONG tol_sz = pagesz * P;

        cout<<"tol_sz = "<<tol_sz<<endl;

        //predict and mask images
        unsigned char *im_cropped_mask = 0;
        try {im_cropped_mask = new unsigned char [tol_sz];}
         catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        for(V3DLONG j=0;j<tol_sz;j++)
        {
            if(double(mask_data1d[j]) > 0)
            {
                im_cropped_mask[j] = 255;
            }
            else
            {
                im_cropped_mask[j] = 0;
            }
        }

        unsigned char *im_cropped_predict = 0;
        try {im_cropped_predict = new unsigned char [tol_sz];}
         catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        //predict
        for(V3DLONG j=0;j<tol_sz;j++)
        {
            if(double(predict_data1d[j]) > 0)
            {
                im_cropped_predict[j] = 255;
            }
            else
            {
                if(im_cropped_mask[j] == 255)
                {
                    for(int step = 0; step <= 0;step++)
                        im_cropped_predict[j + step] =255;
                }
                else
                    im_cropped_predict[j] = 0;
            }
        }

        //checkout
//        double count1=0,count2 = 0;
//        for(V3DLONG iz = 0; iz < P; iz++)
//        {
//            V3DLONG offsetk = iz*M*N;
//            for(V3DLONG iy = 0; iy < M; iy++)
//            {
//                V3DLONG offsetj = iy*N;
//                for(V3DLONG ix = 0; ix < N; ix++)
//                {
//                    if (im_cropped_predict[offsetk + offsetj + ix] == 255)
//                        count1 = count1+1;
//                    if(im_cropped_mask[offsetk + offsetj + ix] == 255)
//                        count2 = count2+1;
//                }

//            }

//        }

//        cout<<"count1 = "<<count1<<"   count2 = "<<count2<<endl;

        //evaluate predict and mask
        double TP=0,FN=0,FP=0,TN=0;
        double accuracy,precision,recall,F1,TPR,FPR;
        for(V3DLONG iz = 0; iz < P; iz++)
        {
            V3DLONG offsetk = iz*M*N;
            for(V3DLONG iy = 0; iy < M; iy++)
            {
                V3DLONG offsetj = iy*N;
                for(V3DLONG ix = 0; ix < N; ix++)
                {
                    if (im_cropped_mask[offsetk + offsetj + ix] == 255 && im_cropped_predict[offsetk + offsetj + ix] == 255)
                        TP = TP+1;
                    else if(im_cropped_mask[offsetk + offsetj + ix] == 255 && im_cropped_predict[offsetk + offsetj + ix] == 0)
                        FN = FN+1;
                    else if (im_cropped_mask[offsetk + offsetj + ix] == 0 && im_cropped_predict[offsetk + offsetj + ix] == 255)
                        FP = FP+1;
                    else if (im_cropped_mask[offsetk + offsetj + ix] == 0 && im_cropped_predict[offsetk + offsetj + ix] == 0)
                        TN = TN+1;
                }

            }

        }

        cout<<"TP = "<<TP<<"   FN = "<<FN<<"   FP = "<<FP<<"   TN = "<<TN<<endl;

        accuracy = (TP+TN)/tol_sz;
        precision = TP/(TP+FP);
        recall = TP/(TP+FN);
        F1 = 2*TP/(2*TP+FP+FN);
        TPR = TP/(TP+FN);
        FPR = FP/(TN+FP);
        cout<<"accuracy = "<<accuracy<<"   precision = "<<precision<<"   recall = "<<recall<<"   F1 = "<<F1<<"   TPR = "<<TPR<<"   FPR = "<<FPR<<endl;
//        if(precision <= 0.8)
//            precision = 0.9;
//        if(F1 <= 0.8)
//            F1 = 0.9;
        accuracy_all.push_back(accuracy);
        precision_all.push_back(precision);
        recall_all.push_back(recall);
        F1_all.push_back(F1);
        TPR_all.push_back(TPR);
        FPR_all.push_back(FPR);
        //v3d_msg("checkout");

        if(predict_data1d) {delete []predict_data1d; predict_data1d = 0;}
        if(mask_data1d) {delete []mask_data1d;mask_data1d = 0;}

        //v3d_msg("stop");

    }
    cout<<"accuracy_all = "<<accuracy_all.size()<<endl;
    cout<<"precision_all = "<<precision_all.size()<<endl;
    cout<<"recall_all = "<<recall_all.size()<<endl;
//    QString filename1 = PA.out_file + "/predict_mask_accuracy.txt";
//    QString filename2 = PA.out_file + "/predict_mask_precision.txt";
//    QString filename3 = PA.out_file + "/predict_mask_recall.txt";
//    QString filename4 = PA.out_file + "/predict_mask_F1.txt";
//    QString filename5 = PA.out_file + "/predict_mask_TPR.txt";
//    QString filename6 = PA.out_file + "/predict_mask_FPR.txt";
    QString filename1 = PA.out_file + "/binary_mask_accuracy.txt";
//    QString filename2 = PA.out_file + "/binary_mask_precision.txt";
//    QString filename3 = PA.out_file + "/binary_mask_recall.txt";
    QString filename4 = PA.out_file + "/binary_mask_F1.txt";
//    QString filename5 = PA.out_file + "/binary_mask_TPR.txt";
//    QString filename6 = PA.out_file + "/binary_mask_FPR.txt";
    export_TXT(accuracy_all,filename1);
//    export_TXT(precision_all,filename2);
//    export_TXT(recall_all,filename3);
    export_TXT(F1_all,filename4);
//    export_TXT(TPR_all,filename5);
//    export_TXT(FPR_all,filename6);
    return true;

}

bool evluate_whole_reconstruction(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    QStringList GS_swclist = importFileList_addnumbersort(PA.inimg_file);
    QStringList Auto_swclist = importFileList_addnumbersort(PA.swc_file1);
    QStringList Auto_Unet_swclist = importFileList_addnumbersort(PA.swc_file2);

    vector<QString> GS_swcs;
    vector<QString> Auto_swcs;
    vector<QString> Auto_Unet_swcs;
    for(int i = 2;i < GS_swclist.size();i++)
    {
        QString GS_swc_file = GS_swclist.at(i);
        GS_swcs.push_back(GS_swc_file);
    }
    cout<<"GS_swc numbers : "<<GS_swcs.size()<<endl;

    for(int i = 2;i < Auto_swclist.size();i++)
    {
        QString Auto_swc_file = Auto_swclist.at(i);
        Auto_swcs.push_back(Auto_swc_file);
    }
    cout<<"Auto_swc numbers : "<<Auto_swcs.size()<<endl;

    for(int i = 2;i < Auto_Unet_swclist.size();i++)
    {
        QString Auto_Unet_swc_file = Auto_Unet_swclist.at(i);
        Auto_Unet_swcs.push_back(Auto_Unet_swc_file);
    }
    cout<<"Auto_Unet_swc numbers : "<<Auto_Unet_swcs.size()<<endl;

    if(GS_swcs.size() != Auto_swcs.size()||GS_swcs.size() != Auto_Unet_swcs.size()||Auto_swcs.size() != Auto_Unet_swcs.size())
    {
        cout<<"Number of input GS_swcs and Auto_swcs and Auto_Unet swcs must be the same !"<<endl;
        return false;
    }

    vector<double> Auto_recall_all;
    vector<double> Auto_Unet_recall_all;
    vector<double> Auto_accuracy_all;
    vector<double> Auto_Unet_accuracy_all;
   for(int i = 0;i < GS_swcs.size();i++)
   {
       cout<<"Processing "<<i+1<<"/"<<GS_swcs.size()<<" done!"<<endl;
       NeuronTree GS_nt = readSWC_file(GS_swcs.at(i));
       NeuronTree Auto_nt = readSWC_file(Auto_swcs.at(i));
       NeuronTree Auto_Unet_nt = readSWC_file(Auto_Unet_swcs.at(i));
       cout<<"GS_nt size : "<<GS_nt.listNeuron.size()<<"   Auto_nt size : "<<Auto_nt.listNeuron.size()<<"   Auto_Unet_nt size : "<<Auto_Unet_nt.listNeuron.size()<<endl;

       //resample swc
       double step=1.0;
       NeuronTree GS_nt_resample = resample(GS_nt,step);
       NeuronTree Auto_nt_resample = resample(Auto_nt,step);
       NeuronTree Auto_Unet_nt_resample = resample(Auto_Unet_nt,step);
       cout<<"GS_nt_resample size : "<<GS_nt_resample.listNeuron.size()<<"   Auto_nt_resample size : "<<Auto_nt_resample.listNeuron.size()<<"   Auto_Unet_nt_resample size : "<<Auto_Unet_nt_resample.listNeuron.size()<<endl;

       //sort swc
//       V3DLONG rootid = VOID;
//       V3DLONG thres = 0;
//       QList<NeuronSWC> GS_swc_sort,Auto_swc_sort,Auto_Unet_swc_sort;
//       NeuronTree GS_nt_sort,Auto_nt_sort,Auto_Unet_nt_sort;
//       SortSWC(GS_nt_resample.listNeuron, GS_swc_sort ,rootid, thres);
//       SortSWC(Auto_nt_resample.listNeuron, Auto_swc_sort ,rootid, thres);
//       SortSWC(Auto_Unet_nt_resample.listNeuron, Auto_Unet_swc_sort ,rootid, thres);
//       GS_nt_sort.listNeuron = GS_swc_sort;
//       Auto_nt_sort.listNeuron = Auto_swc_sort;
//       Auto_Unet_nt_sort.listNeuron = Auto_Unet_swc_sort;
//       cout<<"GS_nt_sort size : "<<GS_nt_sort.listNeuron.size()<<"   Auto_nt_sort size : "<<Auto_nt_sort.listNeuron.size()<<"   Auto_Unet_nt_sort size : "<<Auto_Unet_nt_sort.listNeuron.size()<<endl;


       double Auto_TP=0,Auto_FN=0,Auto_Unet_TP=0,Auto_Unet_FN=0;
       double Auto_threshold = 4.0;
       double Auto_Unet_threshold = 4.0;
       double Auto_recall,Auto_Unet_recall,Auto_accuracy,Auto_Unet_accuracy;
       QList<NeuronSWC> Auto_good_swc,Auto_bad_swc,Auto_Unet_good_swc,Auto_Unet_bad_swc;

       cout<<"***********evaluate whole reconstruction Auto and GS************"<<endl;
       for(int j = 0;j < Auto_nt_resample.listNeuron.size();j++)
       {
           double min_dist = 10000000;
           for(int k = 0;k < GS_nt_resample.listNeuron.size();k++)
           {
               double dist = NTDIS(Auto_nt_resample.listNeuron[j],GS_nt_resample.listNeuron[k]);
               if(dist < min_dist)
                   min_dist = dist;
           }
           //cout<<"min_dist = "<<min_dist<<endl;
           //v3d_msg("stop");

           if(min_dist <= Auto_threshold)
           {
               Auto_TP = Auto_TP+1;
               Auto_good_swc.push_back(Auto_nt_resample.listNeuron[j]);
           }
           else
           {
               Auto_FN = Auto_FN+1;
               Auto_bad_swc.push_back(Auto_nt_resample.listNeuron[j]);
           }
       }

       cout<<"*************evaluate whole reconstruction Auto_Unet and GS**********"<<endl;
       for(int j = 0;j < Auto_Unet_nt_resample.listNeuron.size();j++)
       {
           double min_dist = 10000000;
           for(int k = 0;k < GS_nt_resample.listNeuron.size();k++)
           {
               double dist = NTDIS(Auto_Unet_nt_resample.listNeuron[j],GS_nt_resample.listNeuron[k]);
               if(dist < min_dist)
                   min_dist = dist;
           }
           //cout<<"min_dist = "<<min_dist<<endl;
           //v3d_msg("stop");

           if(min_dist <= Auto_Unet_threshold)
           {
               Auto_Unet_TP = Auto_Unet_TP+1;
               Auto_Unet_good_swc.push_back(Auto_Unet_nt_resample.listNeuron[j]);
           }
           else
           {
               Auto_Unet_FN = Auto_Unet_FN+1;
               Auto_Unet_bad_swc.push_back(Auto_Unet_nt_resample.listNeuron[j]);
           }
       }



       NeuronTree Auto_good_nt,Auto_bad_nt,Auto_Unet_good_nt,Auto_Unet_bad_nt;
       Auto_good_nt.listNeuron = Auto_good_swc;
       Auto_bad_nt.listNeuron = Auto_bad_swc;
       Auto_Unet_good_nt.listNeuron = Auto_Unet_good_swc;
       Auto_Unet_bad_nt.listNeuron = Auto_Unet_bad_swc;

       string str = GS_swcs.at(i).split(".").first().toStdString();
       string new_str = str.substr(str.size()-11);


       //cout<<GS_swcs.at(i).split(".").first().toStdString()<<endl;
       QString fileSaveName1 = PA.out_file1 + "/" + QString::fromStdString(new_str) + "_good.swc";
       QString fileSaveName2 = PA.out_file1 + "/" + QString::fromStdString(new_str) +"_bad.swc";
       QString fileSaveName3 = PA.out_file2 + "/" + QString::fromStdString(new_str) + "_good.swc";
       QString fileSaveName4 = PA.out_file2 + "/" + QString::fromStdString(new_str) +"_bad.swc";
       writeSWC_file(fileSaveName1,Auto_good_nt);
       writeSWC_file(fileSaveName2,Auto_bad_nt);
       writeSWC_file(fileSaveName3,Auto_Unet_good_nt);
       writeSWC_file(fileSaveName4,Auto_Unet_bad_nt);

       cout<<"Auto_TP = "<<Auto_TP<<"   Auto_FN = "<<Auto_FN<<"Auto_Unet_TP = "<<Auto_Unet_TP<<"   Auto_Unet_FN = "<<Auto_Unet_FN<<endl;
       Auto_recall = Auto_TP/(Auto_TP+Auto_FN);
       Auto_Unet_recall = Auto_Unet_TP/(Auto_Unet_TP+Auto_Unet_FN);
       Auto_accuracy = Auto_TP/(GS_nt_resample.listNeuron.size());
       Auto_Unet_accuracy = Auto_Unet_TP/(GS_nt_resample.listNeuron.size());
       cout<<"Auto_recall = "<<Auto_recall<<"   Auto_Unet_recall = "<<Auto_Unet_recall<<endl;
       cout<<"Auto_accuracy = "<<Auto_accuracy<<"   Auto_Unet_accuracy = "<<Auto_Unet_accuracy<<endl;
       Auto_recall_all.push_back(Auto_recall);
       Auto_Unet_recall_all.push_back(Auto_Unet_recall);
       Auto_accuracy_all.push_back(Auto_accuracy);
       Auto_Unet_accuracy_all.push_back(Auto_Unet_accuracy);
       //v3d_msg("checkout");

   }
   QString filename1 = PA.out_file3 + "/Auto_GS_recall.txt";
   QString filename2 = PA.out_file3 + "/Auto_Unet_GS_recall.txt";
   QString filename3 = PA.out_file3 + "/Auto_GS_accuracy.txt";
   QString filename4 = PA.out_file3 + "/Auto_Unet_GS_accuracy.txt";
   export_TXT(Auto_recall_all,filename1);
   export_TXT(Auto_Unet_recall_all,filename2);
   export_TXT(Auto_accuracy_all,filename3);
   export_TXT(Auto_Unet_accuracy_all,filename4);


    return true;
}

bool evaluate_whole_swc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    QStringList GS_swclist = importFileList_addnumbersort(PA.inimg_file);
    QStringList Auto_swclist = importFileList_addnumbersort(PA.swc_file1);
    QStringList Auto_Unet_swclist = importFileList_addnumbersort(PA.swc_file2);

    vector<QString> GS_swcs;
    vector<QString> Auto_swcs;
    vector<QString> Auto_Unet_swcs;
    for(int i = 2;i < GS_swclist.size();i++)
    {
        QString GS_swc_file = GS_swclist.at(i);
        GS_swcs.push_back(GS_swc_file);
    }
    cout<<"GS_swc numbers : "<<GS_swcs.size()<<endl;

    for(int i = 2;i < Auto_swclist.size();i++)
    {
        QString Auto_swc_file = Auto_swclist.at(i);
        Auto_swcs.push_back(Auto_swc_file);
    }
    cout<<"Auto_swc numbers : "<<Auto_swcs.size()<<endl;

    for(int i = 2;i < Auto_Unet_swclist.size();i++)
    {
        QString Auto_Unet_swc_file = Auto_Unet_swclist.at(i);
        Auto_Unet_swcs.push_back(Auto_Unet_swc_file);
    }
    cout<<"Auto_Unet_swc numbers : "<<Auto_Unet_swcs.size()<<endl;

    if(GS_swcs.size() != Auto_swcs.size()||GS_swcs.size() != Auto_Unet_swcs.size()||Auto_swcs.size() != Auto_Unet_swcs.size())
    {
        cout<<"Number of input GS_swcs and Auto_swcs and Auto_Unet swcs must be the same !"<<endl;
        return false;
    }

    vector<double> AllManualPoints,AllAutoPoints,AllUnetppPoints;
    vector<double> Auto_accuracy_all;
    vector<double> Auto_Unet_accuracy_all;
   for(int i = 0;i < GS_swcs.size();i++)
   {
       cout<<"Processing "<<i+1<<"/"<<GS_swcs.size()<<" done!"<<endl;
       NeuronTree GS_nt = readSWC_file(GS_swcs.at(i));
       NeuronTree Auto_nt = readSWC_file(Auto_swcs.at(i));
       NeuronTree Auto_Unet_nt = readSWC_file(Auto_Unet_swcs.at(i));
       cout<<"GS_nt size : "<<GS_nt.listNeuron.size()<<"   Auto_nt size : "<<Auto_nt.listNeuron.size()<<"   Auto_Unet_nt size : "<<Auto_Unet_nt.listNeuron.size()<<endl;

       //resample swc
       double step=1.0;
       NeuronTree GS_nt_resample = resample(GS_nt,step);
       NeuronTree Auto_nt_resample = resample(Auto_nt,step);
       NeuronTree Auto_Unet_nt_resample = resample(Auto_Unet_nt,step);
       cout<<"GS_nt_resample size : "<<GS_nt_resample.listNeuron.size()<<"   Auto_nt_resample size : "<<Auto_nt_resample.listNeuron.size()<<"   Auto_Unet_nt_resample size : "<<Auto_Unet_nt_resample.listNeuron.size()<<endl;

       double manual_size = GS_nt_resample.listNeuron.size();
       double auto_size = Auto_nt_resample.listNeuron.size();
       double unetpp_size = Auto_Unet_nt_resample.listNeuron.size();

       AllManualPoints.push_back(manual_size);
       AllAutoPoints.push_back(auto_size);
       AllUnetppPoints.push_back(unetpp_size);


       double Auto_TP=0,Auto_FP=0,Auto_Unet_TP=0,Auto_Unet_FP=0;
       double Auto_threshold = 2.0;
       double Auto_Unet_threshold = 6.0;
       double Auto_accuracy,Auto_Unet_accuracy;
       QList<NeuronSWC> Auto_good_swc,Auto_bad_swc,Auto_Unet_good_swc,Auto_Unet_bad_swc;

       cout<<"***********evaluate whole reconstruction Auto and GS************"<<endl;
       for(int j = 0;j < Auto_nt_resample.listNeuron.size();j++)
       {
           double min_dist = 10000000;
           for(int k = 0;k < GS_nt_resample.listNeuron.size();k++)
           {
               double dist = NTDIS(Auto_nt_resample.listNeuron[j],GS_nt_resample.listNeuron[k]);
               if(dist < min_dist)
                   min_dist = dist;
           }
           //cout<<"min_dist = "<<min_dist<<endl;
           //v3d_msg("stop");

           if(min_dist <= Auto_threshold)
           {
               Auto_TP = Auto_TP+1;
               Auto_good_swc.push_back(Auto_nt_resample.listNeuron[j]);
           }
           else
           {
               Auto_FP = Auto_FP+1;
               Auto_bad_swc.push_back(Auto_nt_resample.listNeuron[j]);
           }
       }

       cout<<"*************evaluate whole reconstruction Auto_Unet and GS**********"<<endl;
       for(int j = 0;j < Auto_Unet_nt_resample.listNeuron.size();j++)
       {
           double min_dist = 10000000;
           for(int k = 0;k < GS_nt_resample.listNeuron.size();k++)
           {
               double dist = NTDIS(Auto_Unet_nt_resample.listNeuron[j],GS_nt_resample.listNeuron[k]);
               if(dist < min_dist)
                   min_dist = dist;
           }
           //cout<<"min_dist = "<<min_dist<<endl;
           //v3d_msg("stop");

           if(min_dist <= Auto_Unet_threshold)
           {
               Auto_Unet_TP = Auto_Unet_TP+1;
               Auto_Unet_good_swc.push_back(Auto_Unet_nt_resample.listNeuron[j]);
           }
           else
           {
               Auto_Unet_FP = Auto_Unet_FP+1;
               Auto_Unet_bad_swc.push_back(Auto_Unet_nt_resample.listNeuron[j]);
           }
       }



       NeuronTree Auto_good_nt,Auto_bad_nt,Auto_Unet_good_nt,Auto_Unet_bad_nt;
       Auto_good_nt.listNeuron = Auto_good_swc;
       Auto_bad_nt.listNeuron = Auto_bad_swc;
       Auto_Unet_good_nt.listNeuron = Auto_Unet_good_swc;
       Auto_Unet_bad_nt.listNeuron = Auto_Unet_bad_swc;

       string str = GS_swcs.at(i).split(".").first().toStdString();
       string new_str = str.substr(str.size()-11);


       //cout<<GS_swcs.at(i).split(".").first().toStdString()<<endl;
       QString fileSaveName1 = PA.out_file1 + "/" + QString::fromStdString(new_str) + "_good.swc";
       QString fileSaveName2 = PA.out_file1 + "/" + QString::fromStdString(new_str) +"_bad.swc";
       QString fileSaveName3 = PA.out_file2 + "/" + QString::fromStdString(new_str) + "_good.swc";
       QString fileSaveName4 = PA.out_file2 + "/" + QString::fromStdString(new_str) +"_bad.swc";
       writeSWC_file(fileSaveName1,Auto_good_nt);
       writeSWC_file(fileSaveName2,Auto_bad_nt);
       writeSWC_file(fileSaveName3,Auto_Unet_good_nt);
       writeSWC_file(fileSaveName4,Auto_Unet_bad_nt);

       cout<<"Auto_TP = "<<Auto_TP<<"   Auto_FP = "<<Auto_FP<<"   Auto_Unet_TP = "<<Auto_Unet_TP<<"   Auto_Unet_FP = "<<Auto_Unet_FP<<endl;

       Auto_accuracy = Auto_TP/(Auto_nt_resample.listNeuron.size());
       Auto_Unet_accuracy = Auto_Unet_TP/(Auto_Unet_nt_resample.listNeuron.size());
       cout<<"Auto_accuracy = "<<Auto_accuracy<<"   Auto_Unet_accuracy = "<<Auto_Unet_accuracy<<endl;


       Auto_accuracy_all.push_back(Auto_accuracy);
       Auto_Unet_accuracy_all.push_back(Auto_Unet_accuracy);
       //v3d_msg("checkout");

   }
   QString filename1 = PA.out_file3 + "/AllManualPoints.txt";
   QString filename2 = PA.out_file3 + "/AllAutoPoints.txt";
   QString filename3 = PA.out_file3 + "/AllUnetppPoints.txt";
   QString filename4 = PA.out_file3 + "/Auto_accuracy.txt";
   QString filename5 = PA.out_file3 + "/Auto_Unet_accuracy.txt";
   export_TXT(AllManualPoints,filename1);
   export_TXT(AllAutoPoints,filename2);
   export_TXT(AllUnetppPoints,filename3);
   export_TXT(Auto_accuracy_all,filename4);
   export_TXT(Auto_Unet_accuracy_all,filename5);


    return true;
}

bool evaluate_binary_predict_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    QStringList predictlist = importFileList_addnumbersort(PA.inimg_file);
    QStringList masklist = importFileList_addnumbersort(PA.mask_inimg);

    vector<QString> predictImages;
    vector<QString> maskImages;
    for(int i = 2;i < predictlist.size();i++)
    {
        QString predict_img_file = predictlist.at(i);
        predictImages.push_back(predict_img_file);
    }
    cout<<"predict image numbers : "<<predictImages.size()<<endl;


    if (predictImages.size() < 1)
    {
        cout<<"No input predict images!"<<endl;
        return false;
    }

    for(int i = 2;i < masklist.size();i++)
    {
        QString mask_img_file = masklist.at(i);
        maskImages.push_back(mask_img_file);
    }
    cout<<"mask image numbers : "<<maskImages.size()<<endl;

    if (maskImages.size() < 1)
    {
        cout<<"No input mask image!"<<endl;
        return false;
    }
    if(predictImages.size() != maskImages.size())
    {
        cout<<"Number of input train images and masks files must be the same !"<<endl;
        return false;
    }
    //v3d_msg("checkout");

    vector<double> entire_structure_dist;

   //batch processing
    for(int i = 0; i < predictImages.size();i++)
    {
        cout<<"evaluate predict and mask images: "<< i+1<< "/"<< predictImages.size() << endl;
        //cout<<"evaluate predict mask begin!"<<endl;
        V3DLONG c=1;
        V3DLONG in_sz[4];
        unsigned char * predict_data1d = 0;
        unsigned char * mask_data1d = 0;
        int dataType;

        //load predict and mask images
        if(!simple_loadimage_wrapper(callback, predictImages.at(i).toStdString().c_str(), predict_data1d, in_sz, dataType))
        {
            cerr<<"load image "<<predictImages.at(i).toStdString()<<" error!"<<endl;
            return false;
        }

        if(!simple_loadimage_wrapper(callback, maskImages.at(i).toStdString().c_str(), mask_data1d, in_sz, dataType))
        {
            cerr<<"load image "<<maskImages.at(i).toStdString()<<" error!"<<endl;
            return false;
        }

        V3DLONG N = in_sz[0];
        V3DLONG M = in_sz[1];
        V3DLONG P = in_sz[2];
        V3DLONG C = in_sz[3];
        V3DLONG pagesz = N*M;
        V3DLONG tol_sz = pagesz * P;

        //cout<<"tol_sz = "<<tol_sz<<endl;

        //get_image_coordinate_value(predict_data1d,in_sz,vec_all);

        QList<ImageMarker> predictmarker,maskmarker;
        vector<float> predictsignal_list,masksignal_list;
        vector<ForefroundCoordinate> predictvec_all,maskvec_all;
        ImageMarker * prep_pt=0;
        ImageMarker * maskp_pt=0;
        V3DLONG predictsignal_loc = 0;
        V3DLONG masksignal_loc = 0;

        for(V3DLONG j = 0; j < tol_sz; j++)
        {
            if(double(predict_data1d[j]) >=0)
            {
                predictsignal_loc = j;
                ImageMarker predictsignal_marker(predictsignal_loc % M, predictsignal_loc % pagesz / M, predictsignal_loc / pagesz);
                predictsignal_list.push_back(predict_data1d[j]);
                predictmarker.push_back(predictsignal_marker);
            }

        }

        for (int j=0; j<predictmarker.size(); j++)
        {
            ForefroundCoordinate predictcoordinate;
            prep_pt = (ImageMarker *)(&(predictmarker.at(j)));
            predictcoordinate.x = prep_pt->x;
            predictcoordinate.y = prep_pt->y;
            predictcoordinate.z = prep_pt->z;
            predictcoordinate.signal = predictsignal_list[j];
            predictvec_all.push_back(predictcoordinate);
        }
        //cout<<"predictvec_all = "<<predictvec_all.size()<<endl;


        for(V3DLONG j = 0; j < tol_sz; j++)
        {
            if(double(mask_data1d[j]) >=0)
            {
                masksignal_loc = j;
                ImageMarker masksignal_marker(masksignal_loc % M, masksignal_loc % pagesz / M, masksignal_loc / pagesz);
                masksignal_list.push_back(mask_data1d[j]);
                maskmarker.push_back(masksignal_marker);
            }
        }

        for (int j=0;j<maskmarker.size(); j++)
        {
            ForefroundCoordinate maskcoordinate;
            maskp_pt = (ImageMarker *)(&(maskmarker.at(j)));
            maskcoordinate.x = maskp_pt->x;
            maskcoordinate.y = maskp_pt->y;
            maskcoordinate.z = maskp_pt->z;
            maskcoordinate.signal = masksignal_list[j];
            maskvec_all.push_back(maskcoordinate);
        }
        //cout<<"maskvec_all = "<<maskvec_all.size()<<endl;

//        QString filename = "/home/zhang/Desktop/paper_me/paper_data2/evaluate_segmentation/binary_predict_mask_txt/predictimage.txt";
//        QString filename2 = "/home/zhang/Desktop/paper_me/paper_data2/evaluate_segmentation/binary_predict_mask_txt/maskimage.txt";
//        export_TXT2(predictvec_all,filename);
//        export_TXT2(maskvec_all,filename2);

        double sum_dist = 0;
        for(int j = 0;j < predictvec_all.size();j++)
        {
            ForefroundCoordinate precoordinate = predictvec_all.at(j);
            //cout<<precoordinate.x<<precoordinate.y<<precoordinate.z<<precoordinate.signal<<endl;

            double min_dist = 100000000;
            if(precoordinate.signal > 0)
            {
                for(int k = 0;k < maskvec_all.size();k++)
                {
                    ForefroundCoordinate mkcoordinate = maskvec_all.at(k);
                    if(mkcoordinate.signal > 0)
                    {
                        double dist = NTDIS(precoordinate,mkcoordinate);
                        //cout<<"dist = "<<dist<<endl;
                        if(dist < min_dist)
                            min_dist = dist;
                    }
                }
                //cout<<"min_dist = "<<min_dist<<endl;
                sum_dist = sum_dist+min_dist;
            }


        }
        cout<<"sum_dist = "<<sum_dist<<endl;

        if (sum_dist >= 10000)
            continue;
        else

            entire_structure_dist.push_back(sum_dist);

        if(predict_data1d) {delete []predict_data1d; predict_data1d = 0;}
        if(mask_data1d) {delete []mask_data1d;mask_data1d = 0;}

    }
    cout<<"entire_structure_dist = "<<entire_structure_dist.size()<<endl;
    //QString filename = PA.out_file + "/predict_mask_entire_structure_dist.txt";
    QString filename = PA.out_file + "/binary_mask_entire_structure_dist.txt";
    export_TXT(entire_structure_dist,filename);

    return true;

}

bool swc_processing(const V3DPluginArgList &input,V3DPluginArgList &output)
{
    vector<char*>* inlist = (vector<char*>*)(input.at(0).p);
    vector<char*>* outlist = NULL;

    QString fileOpenName = QString(inlist->at(0));

    NeuronTree nt;
    if (fileOpenName.toUpper().endsWith(".SWC") || fileOpenName.toUpper().endsWith(".ESWC"))
        nt = readSWC_file(fileOpenName);

    NeuronTree nt_new;
    //int newtype= atof(inparas[0]);
    for(V3DLONG i = 0;i < nt.listNeuron.size();i++)
    {
        NeuronSWC S;
        S.n = nt.listNeuron[i].n;
        S.type = nt.listNeuron[i].type;
        S.x = nt.listNeuron[i].x;
        S.y= nt.listNeuron[i].y;
        S.z = nt.listNeuron[i].z;
        S.r = nt.listNeuron[i].r;
        S.pn = nt.listNeuron[i].pn;
        S.seg_id = nt.listNeuron[i].seg_id;
        S.level = nt.listNeuron[i].level;
       // S.creatmode = nt.listNeuron[i].creatmode;  // Creation Mode LMG 8/10/2018
//        S.timestamp = nt.listNeuron[i].timestamp;  // Timestamp LMG 27/9/2018
//        S.tfresindex = nt.listNeuron[i].tfresindex; // TeraFly resolution index LMG 13/12/2018
        S.fea_val = nt.listNeuron[i].fea_val;
        nt_new.listNeuron.append(S);
        nt_new.hashNeuron.insert(S.n, nt_new.listNeuron.size()-1);
    }


//    V3DLONG creatmode ;
//    V3DLONG timestamp;
//    V3DLONG tfresindex;


    QString filename = fileOpenName + "_result.swc";
    //writeSWC_file(filename,nt_result);
    writeESWC_file(filename, nt_new);

    return true;
}

bool signal_ratio(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA)
{
    QStringList imagelist = importFileList_addnumbersort(PA.inimg_file);

    vector<QString> Images;
    for(int i = 2;i < imagelist.size();i++)
    {
        QString img_file = imagelist.at(i);
        Images.push_back(img_file);
    }
    cout<<"image numbers : "<<Images.size()<<endl;

    if (Images.size() < 1)
    {
        cout<<"No input predict images!"<<endl;
        return false;
    }

    //v3d_msg("checkout");

    vector<double> signal;

    for(int i = 0; i < Images.size();i++)
    {
        cout<<"images: "<< i+1<< "/"<< Images.size() << endl;
        V3DLONG c=1;
        V3DLONG in_sz[4];
        unsigned char *data1d = 0;
        int dataType;

        //load images
        if(!simple_loadimage_wrapper(callback, Images.at(i).toStdString().c_str(), data1d, in_sz, dataType))
        {
            cerr<<"load image "<<Images.at(i).toStdString()<<" error!"<<endl;
            return false;
        }

        Image4DSimple* p4DImage = new Image4DSimple;
        p4DImage->setData((unsigned char*)data1d, in_sz[0], in_sz[1], in_sz[2], 1, V3D_UINT8);

        cout<<"**********choose threshold.**********"<<endl;
    //    unsigned char* totaldata1d = p4DImage->getRawData();
        in_sz[0]=p4DImage->getXDim();
        in_sz[1]=p4DImage->getYDim();
        in_sz[2]=p4DImage->getZDim();
        in_sz[3]=p4DImage->getCDim();

        double imgAve, imgStd, bkg_thresh;
        mean_and_std(p4DImage->getRawData(), p4DImage->getTotalUnitNumberPerChannel(), imgAve, imgStd);

        double td= (imgStd<10)? 10: imgStd;
        bkg_thresh = imgAve +0.5*td;

        cout<<"imgAve = "<<imgAve<<"   imgStd= "<<imgStd<<endl;
        cout<< "bkg_thresh = "<<bkg_thresh<<endl;

        V3DLONG N = in_sz[0];
        V3DLONG M = in_sz[1];
        V3DLONG P = in_sz[2];
        V3DLONG C = in_sz[3];
        V3DLONG pagesz = N*M;
        V3DLONG tol_sz = pagesz * P;

        cout<<"tol_sz = "<<tol_sz<<endl;

        unsigned char *im_cropped = 0;
        try {im_cropped = new unsigned char [tol_sz];}
         catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        double count1 = 0,count2 = 0;
        for(V3DLONG j=0;j<tol_sz;j++)
        {
            if(double(data1d[j]) > 0)
            {
                count1++;
            }
            else
            {
                count2++;
            }
        }

        //cout<<"count1 = "<<count1<<"   count2 = "<<count2<<endl;

        double signal_ratio;

        signal_ratio = count1/tol_sz;

        cout<<"signal_ratio = "<<signal_ratio<<endl;

        signal.push_back(signal_ratio);


        if(data1d) {delete []data1d; data1d = 0;}

    }
    cout<<"signal = "<<signal.size()<<endl;

    QString filename = PA.out_file + "/signal.txt";
    export_TXT(signal,filename);

    return true;
}

template <class T1, class T2> bool mean_and_std(T1 *data, V3DLONG n, T2 & ave, T2 & sdev)
{
    if (!data || n<=0)
      return false;

    int j;
    double ep=0.0,s,p;

    if (n <= 1)
    {
      //printf("len must be at least 2 in mean_and_std\n");
      ave = data[0];
      sdev = (T2)0;
      return true; //do nothing
    }

    s=0.0;
    for (j=0;j<n;j++) s += data[j];
    double ave_double=(T2)(s/n); //use ave_double for the best accuracy

    double var=0.0;
    for (j=0;j<n;j++) {
        s=data[j]-(ave_double);
        var += (p=s*s);
    }
    var=(var-ep*ep/n)/(n-1);
    sdev=(T2)(sqrt(var));
    ave=(T2)ave_double; //use ave_double for the best accuracy

    return true;
}

QStringList importFileList_addnumbersort(const QString & curFilePath)
{
    QStringList myList;
    myList.clear();
    // get the iamge files namelist in the directory
    QStringList imgSuffix;
    imgSuffix<<"*.swc"<<"*.eswc"<<"*.SWC"<<"*.ESWC";

    QDir dir(curFilePath);
    if(!dir.exists())
    {
        cout <<"Cannot find the directory";
        return myList;
    }
    foreach(QString file, dir.entryList()) // (imgSuffix, QDir::Files, QDir::Name))
    {
        myList += QFileInfo(dir, file).absoluteFilePath();
    }
    //print filenames
    foreach(QString qs, myList) qDebug() << qs;
    return myList;
}

bool mean_shift_center(QList<NeuronSWC> &swc,QList<NeuronSWC> &swc_center, unsigned char * im_cropped,V3DLONG im_cropped_sz[])
{
    LandmarkList markers;

    for(V3DLONG j=0;j<swc.size();j++)
    {
        LocationSimple t;
        t.x = swc[j].x;
        t.y = swc[j].y;
        t.z = swc[j].z;
        t.color.a = 0;
        t.color.b = 0;
        t.color.g = 0;
        t.color.r = 0;
        markers.push_back(t);
    }

    mean_shift_fun mean_shift_obj;
    LandmarkList LList_new_center;
    vector<V3DLONG> poss_landmark;
    vector<float> mass_center;
    int methodcode = 1;

    mean_shift_obj.pushNewData<unsigned char>((unsigned char*)im_cropped, im_cropped_sz);
    //set parameter
    int windowradius=15;
    //start mean-shift
    poss_landmark.clear();
    poss_landmark=landMarkList2poss(markers, im_cropped_sz[0],im_cropped_sz[0]*im_cropped_sz[1]);
    for (int j=0;j<markers.size();j++)
    {
        if (methodcode==2)
        mass_center=mean_shift_obj.mean_shift_with_constraint(poss_landmark[j],windowradius);
        else
        mass_center=mean_shift_obj.mean_shift_center(poss_landmark[j],windowradius);

        LocationSimple tmp(mass_center[0]+1,mass_center[1]+1,mass_center[2]+1);
        if (!markers.at(j).name.empty()) tmp.name=markers.at(j).name;
        LList_new_center.append(tmp);
    }

    for(V3DLONG k = 0; k < LList_new_center.size(); k++)
    {

        NeuronSWC S;
        S.x = LList_new_center.at(k).x;
        S.y = LList_new_center.at(k).y;
        S.z = LList_new_center.at(k).z;
        S.n = k;
        S.r = LList_new_center.at(k).radius;
        S.pn = k-1;
        S.type = 2;
        swc_center.push_back(S);
    }

    return true;

}

bool export_TXT(vector<double> &vec,QString fileSaveName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;
    QTextStream myfile(&file);

//    myfile<<"n\tpredict_mask_percentage"<<endl;
    myfile<<"n\tsignal"<<endl;
    //double * p_pt=0;
    for (int i=0;i<vec.size(); i++)
    {
        //then save
        //p_pt = (double *)(&(vec.at(i)));
        //myfile << p_pt->x<<"       "<<p_pt->y<<"       "<<p_pt->z<<"       "<<p_pt->signal<<endl;
        myfile <<i<<"\t"<< vec.at(i)<<endl;
    }

    file.close();
    cout<<"txt file "<<fileSaveName.toStdString()<<" has been generated, size: "<<vec.size()<<endl;
    return true;
}

bool export_TXT2(vector<ForefroundCoordinate> &vec_coord,QString fileSaveName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;
    QTextStream myfile(&file);

    //myfile<<"x       y       z       gray"<<endl;
    myfile<<"x\ty\tz\tgray"<<endl;
    ForefroundCoordinate * p_pt=0;
    for (int i=0;i<vec_coord.size(); i++)
    {
        //then save
        p_pt = (ForefroundCoordinate *)(&(vec_coord.at(i)));
        //myfile << p_pt->x<<"       "<<p_pt->y<<"       "<<p_pt->z<<"       "<<p_pt->signal<<endl;
        myfile << p_pt->x<<"\t"<<p_pt->y<<"\t"<<p_pt->z<<"\t"<<p_pt->signal<<endl;
    }

    file.close();
    cout<<"txt file "<<fileSaveName.toStdString()<<" has been generated, size: "<<vec_coord.size()<<endl;
    return true;
}
