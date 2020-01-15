/* unetpp_data_plugin.h
 * This is a test plugin, you can use it as a demo.
 * 2019-5-31 : by Yongzhang
 */
 
#ifndef __UNETPP_DATA_PLUGIN_H__
#define __UNETPP_DATA_PLUGIN_H__

#include <QtGui>
#include <v3d_interface.h>


struct input_PARA
{
    QString inimg_file,swc_file,some_swc,out_file,soma_file;
    QString out_imagefile,out_swcfile;
    QString mask_inimg,unet_inimg;
    QString out_file1,out_file2,out_file3;
    QString manual_swc,swc_file1,swc_file2;
    V3DLONG in_sz[3];

};

struct ForefroundCoordinate
{
    int x;
    int y;
    int z;
    int signal;

    bool operator == (const ForefroundCoordinate &b) const
    {
        return (x==b.x && y==b.y && z == b.z);
    }
};

struct Coordinate
{
    int x;
    int y;
    int z;
    int bri;

    bool operator == (const Coordinate &b) const
    {
        return (x==b.x && y==b.y && z == b.z);
    }
};


class unetpp_dataPlugin : public QObject, public V3DPluginInterface2_1
{
	Q_OBJECT
	Q_INTERFACES(V3DPluginInterface2_1);

public:
	float getPluginVersion() const {return 1.1f;}

	QStringList menulist() const;
	void domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent);

	QStringList funclist() const ;
	bool dofunc(const QString &func_name, const V3DPluginArgList &input, V3DPluginArgList &output, V3DPluginCallback2 &callback, QWidget *parent);
};


bool get_samples_allswc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P);
bool get_samples_someswc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P);
bool binary_image(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, input_PARA &P);
bool get_samples_soma(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output,input_PARA &P);
bool binary_image(V3DPluginCallback2 &callback, QWidget *parent, input_PARA &PARA);
bool combined_image_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA);
bool combined_image_mask_batch(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA);
bool evaluate_predict_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA);
//bool export_TXT(vector<double> &vec,QString fileSaveName);
bool evaluate_binary_predict_mask(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA);
//bool get_image_coordinate_value(unsigned char *data1d,vector<ForefroundCoordinate> &vec_all);
bool evluate_whole_reconstruction(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA);
bool processing_swc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA);
bool combined_image_predict(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input, V3DPluginArgList & output, input_PARA &PA);

QStringList importFileList_addnumbersort(const QString & curFilePath);
template <class T1, class T2> bool mean_and_std(T1 *data, V3DLONG n, T2 & ave, T2 & sdev);
bool mean_shift_center(QList<NeuronSWC> &swc,QList<NeuronSWC> &swc_center, unsigned char * im_cropped,V3DLONG im_cropped_sz[]);
bool swc_processing(const V3DPluginArgList &input,V3DPluginArgList &output);
bool signal_ratio(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA);
bool evaluate_whole_swc(V3DPluginCallback2 &callback, QWidget *parent, const V3DPluginArgList & input,V3DPluginArgList & output, input_PARA &PA);
#endif




