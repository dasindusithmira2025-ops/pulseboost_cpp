/****************************************************************************
** Meta object code from reading C++ file 'feature_gate.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/PulseBoostAI/ui_backend/feature_gate.hpp"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'feature_gate.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.6.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS = QtMocHelpers::stringData(
    "pulseboost::FeatureGate",
    "tierChanged",
    "",
    "activatePro",
    "key",
    "refresh",
    "isPro",
    "trialExpired",
    "trialDaysLeft",
    "tierLabel"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS_t {
    uint offsetsAndSizes[20];
    char stringdata0[24];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[4];
    char stringdata5[8];
    char stringdata6[6];
    char stringdata7[13];
    char stringdata8[14];
    char stringdata9[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS = {
    {
        QT_MOC_LITERAL(0, 23),  // "pulseboost::FeatureGate"
        QT_MOC_LITERAL(24, 11),  // "tierChanged"
        QT_MOC_LITERAL(36, 0),  // ""
        QT_MOC_LITERAL(37, 11),  // "activatePro"
        QT_MOC_LITERAL(49, 3),  // "key"
        QT_MOC_LITERAL(53, 7),  // "refresh"
        QT_MOC_LITERAL(61, 5),  // "isPro"
        QT_MOC_LITERAL(67, 12),  // "trialExpired"
        QT_MOC_LITERAL(80, 13),  // "trialDaysLeft"
        QT_MOC_LITERAL(94, 9)   // "tierLabel"
    },
    "pulseboost::FeatureGate",
    "tierChanged",
    "",
    "activatePro",
    "key",
    "refresh",
    "isPro",
    "trialExpired",
    "trialDaysLeft",
    "tierLabel"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPEFeatureGateENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       4,   37, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    5 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   33,    2, 0x02,    6 /* Public */,
       5,    0,   36,    2, 0x02,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool, QMetaType::QString,    4,
    QMetaType::Void,

 // properties: name, type, flags
       6, QMetaType::Bool, 0x00015001, uint(0), 0,
       7, QMetaType::Bool, 0x00015001, uint(0), 0,
       8, QMetaType::Int, 0x00015001, uint(0), 0,
       9, QMetaType::QString, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::FeatureGate::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPEFeatureGateENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS_t,
        // property 'isPro'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'trialExpired'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'trialDaysLeft'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'tierLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FeatureGate, std::true_type>,
        // method 'tierChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'activatePro'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void pulseboost::FeatureGate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FeatureGate *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->tierChanged(); break;
        case 1: { bool _r = _t->activatePro((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 2: _t->refresh(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FeatureGate::*)();
            if (_t _q_method = &FeatureGate::tierChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<FeatureGate *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isPro(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->trialExpired(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->trialDaysLeft(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->tierLabel(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *pulseboost::FeatureGate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::FeatureGate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPEFeatureGateENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::FeatureGate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::FeatureGate::tierChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
