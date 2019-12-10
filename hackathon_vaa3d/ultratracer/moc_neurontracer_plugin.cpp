/****************************************************************************
** Meta object code from reading C++ file 'neurontracer_plugin.h'
**
** Created: Wed Nov 13 11:47:52 2019
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "neurontracer_plugin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'neurontracer_plugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_neurontracer[] = {

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

static const char qt_meta_stringdata_neurontracer[] = {
    "neurontracer\0"
};

const QMetaObject neurontracer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_neurontracer,
      qt_meta_data_neurontracer, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &neurontracer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *neurontracer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *neurontracer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_neurontracer))
        return static_cast<void*>(const_cast< neurontracer*>(this));
    if (!strcmp(_clname, "V3DPluginInterface2_1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< neurontracer*>(this));
    if (!strcmp(_clname, "com.janelia.v3d.V3DPluginInterface/2.1"))
        return static_cast< V3DPluginInterface2_1*>(const_cast< neurontracer*>(this));
    return QObject::qt_metacast(_clname);
}

int neurontracer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_neurontracer_app2_raw[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      23,   22,   22,   22, 0x0a,
      32,   22,   22,   22, 0x0a,
      53,   22,   22,   22, 0x0a,
      77,   22,   22,   22, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_neurontracer_app2_raw[] = {
    "neurontracer_app2_raw\0\0update()\0"
    "_slots_openrawFile()\0_slots_openmarkerFile()\0"
    "_slots_openteraflyFile()\0"
};

const QMetaObject neurontracer_app2_raw::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_neurontracer_app2_raw,
      qt_meta_data_neurontracer_app2_raw, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &neurontracer_app2_raw::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *neurontracer_app2_raw::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *neurontracer_app2_raw::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_neurontracer_app2_raw))
        return static_cast<void*>(const_cast< neurontracer_app2_raw*>(this));
    return QDialog::qt_metacast(_clname);
}

int neurontracer_app2_raw::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: update(); break;
        case 1: _slots_openrawFile(); break;
        case 2: _slots_openmarkerFile(); break;
        case 3: _slots_openteraflyFile(); break;
        default: ;
        }
        _id -= 4;
    }
    return _id;
}
static const uint qt_meta_data_neurontracer_app1_raw[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      23,   22,   22,   22, 0x0a,
      32,   22,   22,   22, 0x0a,
      53,   22,   22,   22, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_neurontracer_app1_raw[] = {
    "neurontracer_app1_raw\0\0update()\0"
    "_slots_openrawFile()\0_slots_openmarkerFile()\0"
};

const QMetaObject neurontracer_app1_raw::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_neurontracer_app1_raw,
      qt_meta_data_neurontracer_app1_raw, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &neurontracer_app1_raw::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *neurontracer_app1_raw::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *neurontracer_app1_raw::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_neurontracer_app1_raw))
        return static_cast<void*>(const_cast< neurontracer_app1_raw*>(this));
    return QDialog::qt_metacast(_clname);
}

int neurontracer_app1_raw::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: update(); break;
        case 1: _slots_openrawFile(); break;
        case 2: _slots_openmarkerFile(); break;
        default: ;
        }
        _id -= 3;
    }
    return _id;
}
static const uint qt_meta_data_neurontracer_most_raw[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      23,   22,   22,   22, 0x0a,
      32,   22,   22,   22, 0x0a,
      53,   22,   22,   22, 0x0a,
      77,   22,   22,   22, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_neurontracer_most_raw[] = {
    "neurontracer_most_raw\0\0update()\0"
    "_slots_openrawFile()\0_slots_openmarkerFile()\0"
    "_slots_openteraflyFile()\0"
};

const QMetaObject neurontracer_most_raw::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_neurontracer_most_raw,
      qt_meta_data_neurontracer_most_raw, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &neurontracer_most_raw::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *neurontracer_most_raw::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *neurontracer_most_raw::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_neurontracer_most_raw))
        return static_cast<void*>(const_cast< neurontracer_most_raw*>(this));
    return QDialog::qt_metacast(_clname);
}

int neurontracer_most_raw::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: update(); break;
        case 1: _slots_openrawFile(); break;
        case 2: _slots_openmarkerFile(); break;
        case 3: _slots_openteraflyFile(); break;
        default: ;
        }
        _id -= 4;
    }
    return _id;
}
static const uint qt_meta_data_neurontracer_neutube_raw[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   25,   25,   25, 0x0a,
      35,   25,   25,   25, 0x0a,
      56,   25,   25,   25, 0x0a,
      80,   25,   25,   25, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_neurontracer_neutube_raw[] = {
    "neurontracer_neutube_raw\0\0update()\0"
    "_slots_openrawFile()\0_slots_openmarkerFile()\0"
    "_slots_openteraflyFile()\0"
};

const QMetaObject neurontracer_neutube_raw::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_neurontracer_neutube_raw,
      qt_meta_data_neurontracer_neutube_raw, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &neurontracer_neutube_raw::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *neurontracer_neutube_raw::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *neurontracer_neutube_raw::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_neurontracer_neutube_raw))
        return static_cast<void*>(const_cast< neurontracer_neutube_raw*>(this));
    return QDialog::qt_metacast(_clname);
}

int neurontracer_neutube_raw::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: update(); break;
        case 1: _slots_openrawFile(); break;
        case 2: _slots_openmarkerFile(); break;
        case 3: _slots_openteraflyFile(); break;
        default: ;
        }
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
