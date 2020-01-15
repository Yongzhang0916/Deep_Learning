/****************************************************************************
** Meta object code from reading C++ file 'unetpp_data_plugin.h'
**
** Created: Tue Oct 15 22:49:52 2019
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "unetpp_data_plugin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'unetpp_data_plugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_unetpp_dataPlugin[] = {

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

static const char qt_meta_stringdata_unetpp_dataPlugin[] = {
    "unetpp_dataPlugin\0"
};

const QMetaObject unetpp_dataPlugin::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_unetpp_dataPlugin,
      qt_meta_data_unetpp_dataPlugin, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &unetpp_dataPlugin::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *unetpp_dataPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *unetpp_dataPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_unetpp_dataPlugin))
        return static_cast<void*>(const_cast< unetpp_dataPlugin*>(this));
    if (!strcmp(_clname, "V3DPluginInterface2_1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< unetpp_dataPlugin*>(this));
    if (!strcmp(_clname, "com.janelia.v3d.V3DPluginInterface/2.1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< unetpp_dataPlugin*>(this));
    return QObject::qt_metacast(_clname);
}

int unetpp_dataPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
