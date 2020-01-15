#include "markers_distance.h"
#include "../neurontracing_vn2/app2/my_surf_objs.h"

#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))

bool markers_distance(V3DPluginCallback2 &callback,const V3DPluginArgList &input,V3DPluginArgList &output,QWidget *parent,input_PARA &PARA)
{
    vector<char*>* inlist = (vector<char*>*)(input.at(0).p);
    vector<char*>* outlist = NULL;
    vector<char*>* paralist = NULL;

    QString fileOpenName = QString(inlist->at(0));
    QStringList listmarkers = importFileList_addnumbersort(fileOpenName);
    vector<QString> markers;
    for(int i = 2;i < listmarkers.size();i++)
    {
        QString marker_file = listmarkers.at(i);
        markers.push_back(marker_file);
    }
    cout<<"marker numbers : "<<markers.size()<<endl;

    vector<vector<double> > dist_matrix;
    for(int i=0;i<markers.size();i++)
    {
        vector<double> v;
        for(int j=0;j<markers.size();j++)
        {
            v.push_back(0);
        }
        dist_matrix.push_back(v);
    }
//    cout<<"distance matrix:"<<endl;
//    for(int i = 0;i <markers.size();i++)
//    {
//        for(int j = 0;j<markers.size();j++)
//        {
//            cout<<dist_matrix[i][j]<<"   ";
//        }
//        cout<<endl;
//    }

    //create all marker
    QList<ImageMarker> all_markers;
    all_markers.clear();
    vector<double> all_near_dist;
    all_near_dist.clear();
    for(int i = 0;i < markers.size();i++)
    {
        vector<MyMarker> file_inmarkers;
        //cout<<"filename: "<<markers.at(i).toStdString()<<endl;
        file_inmarkers = readMarker_file(string(qPrintable(markers.at(i))));
        for(int k = 0; k < file_inmarkers.size(); k++)
        {
            ImageMarker m;
            m.x = file_inmarkers[k].x;
            m.y = file_inmarkers[k].y;
            m.z = file_inmarkers[k].z;
            m.radius = 0;
            //m.color= 255;
            all_markers.push_back(m);
        }
    }
    cout<<"all markers size: "<<all_markers.size()<<endl;
    writeMarker_file(fileOpenName+"_all.marker",all_markers);

    //choose distance
    double dist,radius;
    for(int i = 0;i < markers.size();i++)
    {
        QList<ImageMarker> near_dist_markers;
        near_dist_markers.clear();
        LocationSimple t;
        vector<MyMarker> file_inmarkers;
        //cout<<"filename: "<<markers.at(i).toStdString()<<endl;
        file_inmarkers = readMarker_file(string(qPrintable(markers.at(i))));

        for(int k = 0; k < file_inmarkers.size(); k++)
        {
            t.x = file_inmarkers[k].x + 1;
            t.y = file_inmarkers[k].y + 1;
            t.z = file_inmarkers[k].z + 1;

            ImageMarker m;
            m.x = file_inmarkers[k].x;
            m.y = file_inmarkers[k].y;
            m.z = file_inmarkers[k].z;
            m.radius = 0;
            //m.color= 255;
            near_dist_markers.push_back(m);
        }
        cout<<"t.x = "<<t.x <<"  t.y = "<<t.y<<"   t.z = "<<t.z<<endl;

        double min_dist = 1000000000;
        ImageMarker m;
        for(int j = 0;j< markers.size();j++)
        {
            LocationSimple t2;
            vector<MyMarker> file2_inmarkers;
            //cout<<"filename: "<<markers.at(j).toStdString()<<endl;
            file2_inmarkers = readMarker_file(string(qPrintable(markers.at(j))));
            for(int l = 0; l < file2_inmarkers.size();l++)
            {
                t2.x = file2_inmarkers[l].x + 1;
                t2.y = file2_inmarkers[l].y + 1;
                t2.z = file2_inmarkers[l].z + 1;
            }

            cout<<"t2.x = "<<t2.x <<"  t2.y = "<<t2.y<<"   t2.z = "<<t2.z<<endl;
            if(t.x == t2.x && t.y == t2.y && t.z == t2.z)
                continue;

            //dist = NTDIS(t,t2);
            dist  = max3numbers(fabs(t.x - t2.x),fabs(t.y - t2.y),4*fabs(t.z - t2.z));
            cout<<"dist = "<<dist<<endl;
            dist_matrix[i][j] = dist;
            if(dist<min_dist)
            {
                min_dist = dist;
                radius = 2*max3numbers(fabs(t.x - t2.x),fabs(t.y - t2.y),4*fabs(t.z - t2.z));

            }
        }
        cout<<"min_dist = "<<min_dist<<endl;
        cout<<"radius = "<<radius<<endl;
        //near_dist_markers.push_back(m);
        //v3d_msg("stop");


        if(radius >= 512)
            near_dist_markers[0].radius= 512;
        else if(radius >= 256)
            near_dist_markers[0].radius= 256;
        else
            near_dist_markers[0].radius= 128;
        all_near_dist.push_back(near_dist_markers[0].radius);
        cout<<"near_dist_markers[0].radius = "<<near_dist_markers[0].radius<<endl;

        writeMarker_file(markers.at(i)+"_result.marker",near_dist_markers);

    }
    for(int m = 0;m < all_near_dist.size();m++)
        cout<<all_near_dist.at(m)<<endl;
   // v3d_msg("checkout near_dist_markers[0].radius");

//    cout<<"distance matrix:"<<endl;
//    for(int i = 0;i <markers.size();i++)
//    {
//        for(int j = 0;j<markers.size();j++)
//        {
//            cout<<dist_matrix[i][j]<<"  ";
//        }
//        cout<<endl;
//    }
//    v3d_msg("show origin distance");


    return true;

}

int max3numbers(int x,int y,int z)
{
    int temp;
    if(x>y)
        temp = x;
    else
        temp = y;
    if(temp >z)
        return temp;
    else
        return z;

}

int min3numbers(int x,int y,int z)
{
    int temp;
    if(x>y)
        temp = y;
    else
        temp = x;
    if(temp >z)
        return z;
    else
        return temp;

}

QStringList importFileList_addnumbersort(const QString & curFilePath)
{
    QStringList myList;
    myList.clear();
    // get the iamge files namelist in the directory
    QStringList imgSuffix;
    imgSuffix<<"*.swc"<<"*.eswc"<<"*.SWC"<<"*.ESWC"<<"*.marker";

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
