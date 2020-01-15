/****************************************************************************
** Meta object code from reading C++ file 'neuron_dist_gui.h'
**
** Created: Thu Sep 5 21:14:36 2019
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "neuron_dist_gui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'neuron_dist_gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SelectNeuronDlg[] = {

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
      17,   16,   16,   16, 0x0a,
      39,   16,   16,   16, 0x0a,
      61,   16,   16,   16, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SelectNeuronDlg[] = {
    "SelectNeuronDlg\0\0_slots_openFileDlg1()\0"
    "_slots_openFileDlg2()\0_slots_runPlugin()\0"
};

const QMetaObject SelectNeuronDlg::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_SelectNeuronDlg,
      qt_meta_data_SelectNeuronDlg, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SelectNeuronDlg::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SelectNeuronDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SelectNeuronDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SelectNeuronDlg))
        return static_cast<void*>(const_cast< SelectNeuronDlg*>(this));
    return QDialog::qt_metacast(_clname);
}

int SelectNeuronDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _slots_openFileDlg1(); break;
        case 1: _slots_openFileDlg2(); break;
        case 2: _slots_runPlugin(); break;
        default: ;
        }
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
