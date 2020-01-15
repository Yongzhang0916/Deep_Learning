/* DeepNeuron_plugin.h
 * This is a test plugin, you can use it as a demo.
 * 2018-7-14 : by Yongzhang
 */
 
#ifndef __DEEPNEURON_PLUGIN_H__
#define __DEEPNEURON_PLUGIN_H__

#include <QtGui>
#include <v3d_interface.h>

struct input_PARA
{
    V3DLONG para1;     //resample para
    V3DLONG para2;        //lens para
    V3DLONG para3;        //step para
    V3DLONG para4;        //prune para
    int model1;        //2 for choose piont_xy ,1 for choose point_xyz ,0 for local alignment
    V3DLONG model2;      //0 for 1 class.1 for 2 class,2 for 3 class
    V3DLONG channel;
};

class DeepNeuronPlugin : public QObject, public V3DPluginInterface2_1
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

#endif

