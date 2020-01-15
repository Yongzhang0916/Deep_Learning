/* DeepNeuron_plugin.cpp
 * This is a test plugin, you can use it as a demo.
 * 2018-7-14 : by Yongzhang
 */
 
#include "v3d_message.h"
#include <vector>
#include <iostream>
#include "basic_surf_objs.h"
#include "get_sub_terafly.h"
#include "get_sample_area.h"
#include "get_expand_swc.h"

#include "DeepNeuron_plugin.h"
#include "DeepNeuron_main_func.h"
Q_EXPORT_PLUGIN2(DeepNeuron, DeepNeuronPlugin);

using namespace std;

//struct input_PARA
//{
//    QString inimg_file;
//    V3DLONG channel;
//};

bool get_expand_swc(V3DPluginCallback2 &callback,QWidget *parent);

QStringList DeepNeuronPlugin::menulist() const
{
	return QStringList() 
        <<tr("DeepNeuron_main_func")
        <<tr("get_sub_terafly")
        <<tr("get_sample_area")
        <<tr("get_expand_swc")
		<<tr("about");
}

QStringList DeepNeuronPlugin::funclist() const
{
	return QStringList()
        <<tr("DeepNeuron_main_func")
        <<tr("get_sub_terafly")
        <<tr("get_sample_area")
        <<tr("get_expand_swc")
		<<tr("help");
}

void DeepNeuronPlugin::domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent)
{
    if (menu_name == tr("DeepNeuron_main_func"))
	{
        bool bmenu = true;
        input_PARA PARA;

        DeepNeuron_main_func(PARA,callback,bmenu,parent);
        v3d_msg("DeepNeuron_main_func done.");
    }
    else if (menu_name == tr("get_sub_terafly"))
    {
        get_sub_terafly(callback,parent);
        v3d_msg("get_sub_terafly done.");
    }

    else if (menu_name == tr("get_sample_area"))
    {
        get_sample_area(callback,parent);
        v3d_msg("get_sample_area done.");
    }

    else if (menu_name == tr("get_expand_swc"))
    {
        get_expand_swc(callback,parent);
        v3d_msg("get_expand_swc done.");
    }

	else
	{
		v3d_msg(tr("This is a test plugin, you can use it as a demo.. "
            "Developed by Yongzhang, 2018-7-14"));
	}
}


bool DeepNeuronPlugin::dofunc(const QString & func_name, const V3DPluginArgList & input, V3DPluginArgList & output, V3DPluginCallback2 & callback,  QWidget * parent)
{
    if (func_name == tr("tracing_func"))
    {
        bool bmenu = false;
        input_PARA PARA;

        DeepNeuron_main_func(PARA,callback,bmenu,parent);
    }

    else if (func_name == tr("get_sub_terafly"))
    {
        get_sub_terafly(callback,parent);
        v3d_msg("get_sub_terafly done.");
    }

    else if (func_name == tr("get_sample_area"))
    {
        get_sample_area(callback,parent);
        v3d_msg("get_sample_area done.");
    }

    else if (func_name == tr("get_expand_swc"))
    {
        get_expand_swc(callback,parent);
        v3d_msg("get_expand_swc done.");
    }

    else if (func_name == tr("help"))
    {
        v3d_msg("To be implemented.");
    }
    else return false;

    return true;
}

