/* Improve_auto_plugin.cpp
 * This is a test plugin, you can use it as a demo.
 * 2019-8-19 : by Yongzhang
 */
 
#include "v3d_message.h"
#include <vector>
#include "basic_surf_objs.h"

#include "Improve_auto_plugin.h"
#include "branch_angle.h"
Q_EXPORT_PLUGIN2(Improve_auto, TestPlugin);

using namespace std;

struct input_PARA
{
    QString inimg_file;
    V3DLONG channel;
};
 
QStringList TestPlugin::menulist() const
{
	return QStringList() 
		<<tr("tracing_menu")
		<<tr("about");
}

QStringList TestPlugin::funclist() const
{
	return QStringList()
		<<tr("tracing_func")
		<<tr("help");
}

void TestPlugin::domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent)
{
	if (menu_name == tr("tracing_menu"))
	{
        input_PARA PARA;
        branch_angle(callback,parent,PARA);  //remove by parentpoints and angle

	}
	else
	{
		v3d_msg(tr("This is a test plugin, you can use it as a demo.. "
			"Developed by Yongzhang, 2019-8-19"));
	}
}

bool TestPlugin::dofunc(const QString & func_name, const V3DPluginArgList & input, V3DPluginArgList & output, V3DPluginCallback2 & callback,  QWidget * parent)
{
    input_PARA PARA;
    vector<char*> infiles, inparas, outfiles;
    if(input.size() >= 1) infiles = *((vector<char*> *)input.at(0).p);
    if(input.size() >= 2) inparas = *((vector<char*> *)input.at(1).p);
    if(output.size() >= 1) outfiles = *((vector<char*> *)output.at(0).p);

    if (func_name == tr("branch_angle"))
    {
        input_PARA PARA;
        branch_angle(callback,input,output,parent,PARA);
        cout<<"branch_angle done!"<<endl;
    }
    else if (func_name == tr("help"))
    {

        ////HERE IS WHERE THE DEVELOPERS SHOULD UPDATE THE USAGE OF THE PLUGIN


		printf("**** Usage of Improve_auto tracing **** \n");
        printf("Usage : vaa3d -x ImageProcessing -f branch_angle -i <inimg_file> -p <ch> ");


	}
	else return false;

	return true;
}
