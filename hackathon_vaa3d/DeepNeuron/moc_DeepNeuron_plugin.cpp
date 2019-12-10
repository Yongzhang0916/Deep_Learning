/****************************************************************************
** Meta object code from reading C++ file 'DeepNeuron_plugin.h'
**
** Created: Thu Aug 30 10:34:27 2018
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "DeepNeuron_plugin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DeepNeuron_plugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DeepNeuronPlugin[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_DeepNeuronPlugin[] = {
    "DeepNeuronPlugin\0"
};

const QMetaObject DeepNeuronPlugin::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_DeepNeuronPlugin,
      qt_meta_data_DeepNeuronPlugin, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DeepNeuronPlugin::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DeepNeuronPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DeepNeuronPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DeepNeuronPlugin))
        return static_cast<void*>(const_cast< DeepNeuronPlugin*>(this));
    if (!strcmp(_clname, "V3DPluginInterface2_1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< DeepNeuronPlugin*>(this));
    if (!strcmp(_clname, "com.janelia.v3d.V3DPluginInterface/2.1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< DeepNeuronPlugin*>(this));
    return QObject::qt_metacast(_clname);
}

int DeepNeuronPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
