/* neuron_dist_func.cpp
 * The plugin to calculate distance between two neurons. Distance is defined as the average distance among all nearest pairs in two neurons.
 * 2012-05-04 : by Yinan Wan
 */

#include <v3d_interface.h>
#include "v3d_message.h"
#include "basic_surf_objs.h"
#include "neuron_dist_func.h"
#include "neuron_dist_gui.h"
#include "neuron_sim_scores.h"
#include "customary_structs/vaa3d_neurontoolbox_para.h"
//#include "../swc_to_maskimage/filter_dialog.h"
#include "../../../released_plugins/v3d_plugins/swc_to_maskimage/filter_dialog.h"
#include "resampling.h"
#include "sort_swc.h"

#include <vector>
#include <iostream>

#include <fstream>

using namespace std;

QStringList importFileList_addnumbersort(const QString & curFilePath);
NeuronTree Unified_radius(NeuronTree &nt);
bool export_TXT(vector<NeuronDistSimple> &vec,QString fileSaveName);

const QString title = QObject::tr("Neuron Distantce");

int neuron_dist_io(V3DPluginCallback2 &callback, QWidget *parent)
{
	SelectNeuronDlg * selectDlg = new SelectNeuronDlg(parent);
	selectDlg->exec();

    NeuronDistSimple tmp_score = neuron_score_rounding_nearest_neighbor(&(selectDlg->nt1), &(selectDlg->nt2),1);
	QString message = QString("Distance between neuron 1:\n%1\n and neuron 2:\n%2\n").arg(selectDlg->name_nt1).arg(selectDlg->name_nt2);
    message += QString("entire-structure-average:from neuron 1 to 2 = %1\n").arg(tmp_score.dist_12_allnodes);
    message += QString("entire-structure-average:from neuron 2 to 1 = %1\n").arg(tmp_score.dist_21_allnodes);
    message += QString("average of bi-directional entire-structure-averages = %1\n").arg(tmp_score.dist_allnodes);
	message += QString("differen-structure-average = %1\n").arg(tmp_score.dist_apartnodes);
	message += QString("percent of different-structure = %1\n").arg(tmp_score.percent_apartnodes);

	v3d_msg(message);
	return 1;
}

bool neuron_dist_io(const V3DPluginArgList & input, V3DPluginArgList & output)
{
    input_file P;
    vector<char*> * pinfiles = (input.size() >= 1) ? (vector<char*> *) input[0].p : 0;
    vector<char*> * pparas = (input.size() >= 2) ? (vector<char*> *) input[1].p : 0;
    vector<char*> infiles = (pinfiles != 0) ? * pinfiles : vector<char*>();
    vector<char*> paras = (pparas != 0) ? * pparas : vector<char*>();
    //vector<char*>* outlist = NULL;
    vector<char*>* outlist = (vector<char*>*)(output.at(0).p);

    P.GS_swcfile = infiles[0];
    P.Ori_swcfile = infiles[1];
    P.Pre_swcfile = infiles[2];

    vector<char*> inparas;
    if(input.size() >= 4) inparas = *((vector<char*> *)input.at(1).p);
    double  d_thres = (inparas.size() >= 1) ? atof(inparas[0]) : 2;
    bool bmenu = 0;

    P.out_dis_score = QString(outlist->at(0));
    cout<<P.out_dis_score.toStdString()<<endl;


	cout<<"Welcome to neuron_dist_io"<<endl;

    QStringList GS_swclist = importFileList_addnumbersort(P.GS_swcfile);
    QStringList Ori_swclist = importFileList_addnumbersort(P.Ori_swcfile);
    QStringList Pre_swclist = importFileList_addnumbersort(P.Pre_swcfile);
    cout<<"GS_swclist :"<<GS_swclist.size()<<endl;
    cout<<"Ori_swclist : "<<Ori_swclist.size()<<endl;
    cout<<"Pre_swclist : "<<Pre_swclist.size()<<endl;

    vector<QString> GS_swcfile,Ori_swcfile,Pre_swcfile;

    //all GS swcfiles
    for(int i = 2;i < GS_swclist.size();i++)
    {
        QString tmp_file = GS_swclist.at(i);
        GS_swcfile.push_back(tmp_file);
    }
    cout<<"GS swcfile numbers : "<<GS_swcfile.size()<<endl;

    if (GS_swcfile.size() < 1)
    {
        cout<<"No input GS swcfile!"<<endl;
        return false;
    }

    //all original swcfiles
    for(int i = 2;i < Ori_swclist.size();i++)
    {
        QString tmp_file = Ori_swclist.at(i);
        Ori_swcfile.push_back(tmp_file);
    }
    cout<<"Ori_swcfile numbers : "<<Ori_swcfile.size()<<endl;
    if (Ori_swcfile.size() < 1)
    {
        cout<<"No input Ori_swcfile!"<<endl;
        return false;
    }

    //all predict swcfiles
    for(int i = 2;i < Pre_swclist.size();i++)
    {
        QString tmp_file = Pre_swclist.at(i);
        Pre_swcfile.push_back(tmp_file);
    }
    cout<<"Pre_swcfile numbers : "<<Pre_swcfile.size()<<endl;
    if (Pre_swcfile.size() < 1)
    {
        cout<<"No input Pre_swcfile!"<<endl;
        return false;
    }

    if(GS_swcfile.size() != Ori_swcfile.size()||GS_swcfile.size() != Pre_swcfile.size()||Ori_swcfile.size() != Pre_swcfile.size())
    {
        cout<<"Number of input GS_swcfile ,Ori_swcfile and Pre_swcfile must be the same !"<<endl;
        return false;
     }


    vector<NeuronDistSimple> dis_GS_Ori_score,dis_GS_Pre_score;
    for(int i = 0;i < GS_swcfile.size();i++)
    {

        //compare GS swcfiles and original swcfiles distance socre
        cout<<"Processing "<<i+1<<"/"<<GS_swcfile.size()<<" begin!"<<endl;
        QString name_nt1 = GS_swcfile.at(i);
        QString name_nt2= Ori_swcfile.at(i);
        QString name_nt3 = Pre_swcfile.at(i);
        NeuronTree nt1 = readSWC_file(name_nt1);
        NeuronTree nt2 = readSWC_file(name_nt2);
        NeuronTree nt3 = readSWC_file(name_nt3);
        double step=5.0;
        NeuronTree nt1_resample = resample(nt1,step);
        NeuronTree nt2_resample = resample(nt2,step);
        NeuronTree nt3_resample = resample(nt3,step);
       // cout<<"nt1_resample: "<<nt1_resample.listNeuron.size()<<"   nt2_resample: "<<nt2_resample.listNeuron.size()<<"   nt3_resample: "<<nt3_resample.listNeuron.size()<<endl;

        NeuronTree nt1_final = Unified_radius(nt1_resample);
        NeuronTree nt2_final = Unified_radius(nt2_resample);
        NeuronTree nt3_final = Unified_radius(nt3_resample);
        cout<<"nt1_final: "<<nt1_final.listNeuron.size()<<"   nt2_final: "<<nt2_final.listNeuron.size()<<"   nt3_final: "<<nt3_final.listNeuron.size()<<endl;


//        V3DLONG rootid = VOID;
//        V3DLONG thres = 0;
//        QList<NeuronSWC> swc_sort1,swc_sort2,swc_sort3;
//        NeuronTree nt1_sort,nt2_sort,nt3_sort;
//        SortSWC(nt1_resample.listNeuron, swc_sort1 ,rootid, thres);
//        SortSWC(nt2_resample.listNeuron, swc_sort2 ,rootid, thres);
//        SortSWC(nt3_resample.listNeuron, swc_sort3 ,rootid, thres);
//        nt1_sort.listNeuron = swc_sort1;
//        nt2_sort.listNeuron = swc_sort2;
//        nt3_sort.listNeuron = swc_sort3;

//        NeuronTree nt1_final = Unified_radius(nt1_sort);
//        NeuronTree nt2_final = Unified_radius(nt2_sort);
//        NeuronTree nt3_final = Unified_radius(nt3_sort);



        NeuronDistSimple GS_Ori_tmp_score = neuron_score_rounding_nearest_neighbor(&nt1_final, &nt2_final,bmenu,d_thres);
        NeuronDistSimple GS_Pre_tmp_score = neuron_score_rounding_nearest_neighbor(&nt1_final, &nt3_final,bmenu,d_thres);

        dis_GS_Ori_score.push_back(GS_Ori_tmp_score);
        dis_GS_Pre_score.push_back(GS_Pre_tmp_score);

    }

    QString filename1 = P.out_dis_score + "/dis_GS_Ori_score.txt";
    QString filename2 = P.out_dis_score + "/dis_GS_Pre_score.txt";
    export_TXT(dis_GS_Ori_score,filename1);
    export_TXT(dis_GS_Pre_score,filename2);




//    vector<char*> * inlist = (vector<char*>*)(input.at(0).p);
//    if (inlist->size()!=2)
//    {
//        cerr<<"plese specify only 2 input neurons for distance computing"<<endl;
//        return false;
//    }

//    vector<char*> inparas;
//    if(input.size() >= 2) inparas = *((vector<char*> *)input.at(1).p);
//    double  d_thres = (inparas.size() >= 1) ? atof(inparas[0]) : 2;
//    bool bmenu = 0;
//    QString name_nt1(inlist->at(0));
//    QString name_nt2(inlist->at(1));
//    NeuronTree nt1 = readSWC_file(name_nt1);
//    NeuronTree nt2 = readSWC_file(name_nt2);
//    NeuronDistSimple tmp_score = neuron_score_rounding_nearest_neighbor(&nt1, &nt2,bmenu,d_thres);

//    cout<<"\nDistance between neuron 1 "<<qPrintable(name_nt1)<<" and neuron 2 "<<qPrintable(name_nt2)<<" is: "<<endl;
//    cout<<"entire-structure-average (from neuron 1 to 2) = "<<tmp_score.dist_12_allnodes <<endl;
//    cout<<"entire-structure-average (from neuron 2 to 1)= "<<tmp_score.dist_21_allnodes <<endl;
//    cout<<"average of bi-directional entire-structure-averages = "<<tmp_score.dist_allnodes <<endl;
//    cout<<"differen-structure-average = "<<tmp_score.dist_apartnodes<<endl;
//    cout<<"percent of different-structure (from neuron 1 to 2) = "<<tmp_score.percent_12_apartnodes<<"%"<<endl<<endl;
//    cout<<"percent of different-structure (from neuron 2 to 1) = "<<tmp_score.percent_21_apartnodes<<"%"<<endl<<endl;
//    cout<<"percent of different-structure (average) = "<<tmp_score.percent_apartnodes<<"%"<<endl<<endl;

//    if (output.size() == 1)
//    {
//        char *outimg_file = ((vector<char*> *)(output.at(0).p))->at(0);

//        ofstream myfile;
//        myfile.open (outimg_file);
//        myfile << "input1 = ";
//        myfile << name_nt1.toStdString().c_str()  ;
//        myfile << "\ninput2 = ";
//        myfile << name_nt2.toStdString().c_str();
//        myfile << "\nentire-structure-average (from neuron 1 to 2) = ";
//        myfile << tmp_score.dist_12_allnodes;
//        myfile << "\nentire-structure-average (from neuron 2 to 1) = ";
//        myfile << tmp_score.dist_21_allnodes;
//        myfile << "\naverage of bi-directional entire-structure-averages = ";
//        myfile << tmp_score.dist_allnodes;
//        myfile << "\ndifferen-structure-average = ";
//        myfile << tmp_score.dist_apartnodes;
//        myfile << "\npercent of different-structure (from neuron 1 to 2) = ";
//        myfile << tmp_score.percent_12_apartnodes;
//        myfile << "\npercent of different-structure (from neuron 2 to 1) = ";
//        myfile << tmp_score.percent_21_apartnodes;
//        myfile << "\npercent of different-structure (average)= ";
//        myfile << tmp_score.percent_apartnodes;
//        myfile << "\n";
//        myfile.close();
//    }
	return true;
}

bool neuron_dist_toolbox(const V3DPluginArgList & input, V3DPluginCallback2 & callback)
{
	vaa3d_neurontoolbox_paras * paras = (vaa3d_neurontoolbox_paras *)(input.at(0).p);
	V3dR_MainWindow * win = paras->win;
	QList<NeuronTree> * nt_list = callback.getHandleNeuronTrees_Any3DViewer(win);
	NeuronTree nt = paras->nt;
	if (nt_list->size()<=1)
	{
		v3d_msg("You should have at least 2 neurons in the current 3D Viewer");
		return false;
	}

	QString message;
	int cur_idx = 0;

	for (V3DLONG i=0;i<nt_list->size();i++)
	{
		NeuronTree curr_nt = nt_list->at(i);
		if (curr_nt.file == nt.file) {cur_idx = i; continue;}
        NeuronDistSimple tmp_score = neuron_score_rounding_nearest_neighbor(&nt, &curr_nt,1);
        message += QString("\nneuron #%1:\n%2\n").arg(i+1).arg(curr_nt.file);
        message += QString("entire-structure-average (from neuron 1 to 2)= %1\n").arg(tmp_score.dist_12_allnodes);
        message += QString("entire-structure-average (from neuron 2 to 1)= %1\n").arg(tmp_score.dist_21_allnodes);
        message += QString("average of bi-directional entire-structure-averages = %1\n").arg(tmp_score.dist_allnodes);
		message += QString("differen-structure-average = %1\n").arg(tmp_score.dist_apartnodes);
		message += QString("percent of different-structure = %1\n").arg(tmp_score.percent_apartnodes);
	}
	message = QString("Distance between current neuron #%1 and\n").arg(cur_idx+1) + message;


	v3d_msg(message);

	return true;

}

int neuron_dist_mask(V3DPluginCallback2 &callback, QWidget *parent)
{
    SelectNeuronDlg * selectDlg = new SelectNeuronDlg(parent);
    selectDlg->exec();

    NeuronTree nt1 = selectDlg->nt1;
    NeuronTree nt2 = selectDlg->nt2;

    float dilate_ratio = QInputDialog::getDouble(parent, "dilate_ratio",
                                 "Enter dialate ratio:",
                                 3.0, 1.0, 100.0);
    for(V3DLONG i = 0; i <nt1.listNeuron.size(); i++)
        nt1.listNeuron[i].r = dilate_ratio;
    for(V3DLONG i = 0; i <nt2.listNeuron.size(); i++)
        nt2.listNeuron[i].r = dilate_ratio;

    double x_min,x_max,y_min,y_max,z_min,z_max;
    x_min=x_max=y_min=y_max=z_min=z_max=0;
    V3DLONG sx,sy,sz;
    unsigned char *pImMask_nt1 = 0;
    BoundNeuronCoordinates(nt1,x_min,x_max,y_min,y_max,z_min,z_max);
    sx=x_max;
    sy=y_max;
    sz=z_max;
    V3DLONG stacksz = sx*sy*sz;
    pImMask_nt1 = new unsigned char [stacksz];
    memset(pImMask_nt1,0,stacksz*sizeof(unsigned char));
    ComputemaskImage(nt1, pImMask_nt1, sx, sy, sz);

    double x_min_2,x_max_2,y_min_2,y_max_2,z_min_2,z_max_2;
    x_min_2=x_max_2=y_min_2=y_max_2=z_min_2=z_max_2=0;
    V3DLONG sx_2,sy_2,sz_2;

    unsigned char *pImMask_nt2 = 0;
    BoundNeuronCoordinates(nt2,x_min_2,x_max_2,y_min_2,y_max_2,z_min_2,z_max_2);
    sx_2=x_max_2;
    sy_2=y_max_2;
    sz_2=z_max_2;
    V3DLONG stacksz_2 = sx_2*sy_2*sz_2;
    pImMask_nt2 = new unsigned char [stacksz_2];
    memset(pImMask_nt2,0,stacksz_2*sizeof(unsigned char));
    ComputemaskImage(nt2, pImMask_nt2, sx_2, sy_2, sz_2);

    unsigned int nx=sx, ny=sy, nz=sz;
    if(sx_2 > nx) nx = sx_2;
    if(sy_2 > ny) ny = sy_2;
    if(sz_2 > nz) nz = sz_2;


    unsigned char *pData = new unsigned char[nx*ny*nz];
    memset(pData,0,nx*ny*nz*sizeof(unsigned char));

    for (V3DLONG k1 = 0; k1 < sz; k1++){
       for(V3DLONG j1 = 0; j1 < sy; j1++){
           for(V3DLONG i1 = 0; i1 < sx; i1++){
               if(pImMask_nt1[k1*sx*sy + j1*sx +i1] == 255)
                    pData[k1 * nx*ny + j1*nx + i1] = 127;
           }
       }
    }

    for (V3DLONG k1 = 0; k1 < sz_2; k1++){
       for(V3DLONG j1 = 0; j1 < sy_2; j1++){
           for(V3DLONG i1 = 0; i1 < sx_2; i1++){
               if(pImMask_nt2[k1*sx_2*sy_2 + j1*sx_2 +i1] == 255)
                    pData[k1 * nx*ny + j1*nx + i1] += 127;
           }
       }
    }


    V3DLONG AandB = 0, AorB = 0;
    for(V3DLONG i = 0; i < nx*ny*nz; i++)
    {
        if(pData[i] > 0) AorB++;
        if(pData[i] == 254) AandB++;

    }

    if(pImMask_nt1) {delete []pImMask_nt1; pImMask_nt1 = 0;}
    if(pImMask_nt2) {delete []pImMask_nt2; pImMask_nt2 = 0;}


    v3d_msg(QString("score is %1, %2").arg(AandB).arg(AorB));
    Image4DSimple tmp;
    tmp.setData(pData, nx, ny, nz, 1, V3D_UINT8);
    v3dhandle newwin = callback.newImageWindow();
    callback.setImage(newwin, &tmp);
    callback.setImageName(newwin, QString("Output_swc_mask"));
    callback.updateImageWindow(newwin);
    callback.open3DWindow(newwin);


    return 1;
}

void printHelp()
{
	cout<<"\nNeuron Distance: compute the distance between two neurons. distance is defined as the average distance among all nearest point pairs. 2012-05-04 by Yinan Wan"<<endl;
    cout<<"Usage: v3d -x neuron_distance -f neuron_distance -i <input_filename1> <input_filename2> -p <dist_thres> -o <output_file>"<<endl;
	cout<<"Parameters:"<<endl;
	cout<<"\t-i <input_filename1> <input_filename2>: input neuron structure file (*.swc *.eswc)"<<endl;
	cout<<"Distance result will be printed on the screen\n"<<endl;
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

NeuronTree Unified_radius(NeuronTree &nt)
{
    NeuronTree nt_result;
    QList<NeuronSWC> swc_result;
    for(int i = 0;i < nt.listNeuron.size();i++)
    {
        NeuronSWC cur;
        cur.x = nt.listNeuron[i].x;
        cur.y = nt.listNeuron[i].y;
        cur.z = nt.listNeuron[i].z;
//        cur.radius = nt.listNeuron[i].radius;
        cur.radius = 1;
        cur.parent = nt.listNeuron[i].parent;
        cur.n = nt.listNeuron[i].n;
        cur.type = nt.listNeuron[i].type;

        swc_result.push_back(cur);
    }

    nt_result.listNeuron = swc_result;

    return nt_result;

}

bool export_TXT(vector<NeuronDistSimple> &vec,QString fileSaveName)
{
    QFile file(fileSaveName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;
    QTextStream myfile(&file);

    myfile<<"n\tdist_12_allnodes\tdist_21_allnodes\tdist_allnodes\tdist_apartnodes\tpercent_12_apartnodes\tpercent_21_apartnodes\tpercent_apartnodes"<<endl;
    NeuronDistSimple * p_pt=0;
    for (int i=0;i<vec.size(); i++)
    {
        //then save
        p_pt = (NeuronDistSimple *)(&(vec.at(i)));
        //myfile << p_pt->x<<"       "<<p_pt->y<<"       "<<p_pt->z<<"       "<<p_pt->signal<<endl;
        myfile <<i<<"\t"<< p_pt->dist_12_allnodes<<"\t"<<p_pt->dist_21_allnodes<<"\t"<<p_pt->dist_allnodes<<"\t"<<p_pt->dist_apartnodes<<"\t"<<p_pt->percent_12_apartnodes<<"\t"<<p_pt->percent_21_apartnodes<<"\t"<<p_pt->percent_apartnodes<<endl;
    }

    file.close();
    cout<<"txt file "<<fileSaveName.toStdString()<<" has been generated, size: "<<vec.size()<<endl;
    return true;
}
