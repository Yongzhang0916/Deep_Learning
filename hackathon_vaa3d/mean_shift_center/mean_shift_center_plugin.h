/* Mean_Shift_Center_plugin.h
 * Search for center using mean-shift
 * 2015-3-4 : by Yujie Li
 */
 
#ifndef __MEAN_SHIFT_CENTER_PLUGIN_H__
#define __MEAN_SHIFT_CENTER_PLUGIN_H__

#include "mean_shift_dialog.h"
#include "ray_shoot_dialog.h"
#include "gradient_transform_dialog.h"


class mean_shift_plugin : public QObject, public V3DPluginInterface2_1
{
	Q_OBJECT
	Q_INTERFACES(V3DPluginInterface2_1);

public:
	float getPluginVersion() const {return 1.1f;}
	QStringList menulist() const;
	void domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent);
	QStringList funclist() const ;

    //do func functions
	bool dofunc(const QString &func_name, const V3DPluginArgList &input, V3DPluginArgList &output, V3DPluginCallback2 &callback, QWidget *parent);
    void mean_shift_center(V3DPluginCallback2 & callback, const V3DPluginArgList & input, V3DPluginArgList & output, int methodcode);
    void all_method_comp(V3DPluginCallback2 *callback);
    void ray_shoot(V3DPluginCallback2 & callback, const V3DPluginArgList & input,
                                      V3DPluginArgList & output);
    void gradient(V3DPluginCallback2 & callback, const V3DPluginArgList & input,
                                     V3DPluginArgList & output);

    void all_method_comp_func(V3DPluginCallback2 & callback, const V3DPluginArgList & input,
                                                 V3DPluginArgList & output);

    void load_image_marker(V3DPluginCallback2 & callback,const V3DPluginArgList & input,
                  unsigned char * & image1Dc_data,LandmarkList &LList,int &intype,V3DLONG sz_img[4]);
    //void write_marker(QString qs_output);
    QList <LocationSimple> readPosFile_usingMarkerCode(const char * posFile);
    QList <ImageMarker> readMarker_file(const QString & filename);
    void printHelp();

private:
    V3DLONG sz_img[4];
    unsigned char *image_data;
    int intype;
    LandmarkList LList,LList_new_center;
};

#endif

