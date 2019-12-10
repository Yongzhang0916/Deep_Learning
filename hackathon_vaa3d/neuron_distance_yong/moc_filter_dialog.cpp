/****************************************************************************
** Meta object code from reading C++ file 'filter_dialog.h'
**
** Created: Thu Sep 5 21:14:37 2019
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../released_plugins/v3d_plugins/swc_to_maskimage/filter_dialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'filter_dialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_filter_dialog[] = {

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
      15,   14,   14,   14, 0x0a,
      26,   14,   14,   14, 0x0a,
      38,   14,   14,   14, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_filter_dialog[] = {
    "filter_dialog\0\0load_swc()\0loadImage()\0"
    "dialoguefinish(int)\0"
};

const QMetaObject filter_dialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_filter_dialog,
      qt_meta_data_filter_dialog, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &filter_dialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *filter_dialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *filter_dialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_filter_dialog))
        return static_cast<void*>(const_cast< filter_dialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int filter_dialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: load_swc(); break;
        case 1: loadImage(); break;
        case 2: dialoguefinish((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
