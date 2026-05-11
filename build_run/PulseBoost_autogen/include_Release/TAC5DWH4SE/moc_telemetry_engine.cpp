/****************************************************************************
** Meta object code from reading C++ file 'telemetry_engine.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../core/telemetry_engine.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'telemetry_engine.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS = QtMocHelpers::stringData(
    "pulseboost::TelemetryWorker",
    "snapshotReady",
    "",
    "pulseboost::SystemSnapshot",
    "snapshot",
    "start",
    "stop",
    "onCpuRamTick",
    "onDiskTick",
    "onProcessTick"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS_t {
    uint offsetsAndSizes[20];
    char stringdata0[28];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[27];
    char stringdata4[9];
    char stringdata5[6];
    char stringdata6[5];
    char stringdata7[13];
    char stringdata8[11];
    char stringdata9[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 27),  // "pulseboost::TelemetryWorker"
        QT_MOC_LITERAL(28, 13),  // "snapshotReady"
        QT_MOC_LITERAL(42, 0),  // ""
        QT_MOC_LITERAL(43, 26),  // "pulseboost::SystemSnapshot"
        QT_MOC_LITERAL(70, 8),  // "snapshot"
        QT_MOC_LITERAL(79, 5),  // "start"
        QT_MOC_LITERAL(85, 4),  // "stop"
        QT_MOC_LITERAL(90, 12),  // "onCpuRamTick"
        QT_MOC_LITERAL(103, 10),  // "onDiskTick"
        QT_MOC_LITERAL(114, 13)   // "onProcessTick"
    },
    "pulseboost::TelemetryWorker",
    "snapshotReady",
    "",
    "pulseboost::SystemSnapshot",
    "snapshot",
    "start",
    "stop",
    "onCpuRamTick",
    "onDiskTick",
    "onProcessTick"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPETelemetryWorkerENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,   53,    2, 0x0a,    3 /* Public */,
       6,    0,   54,    2, 0x0a,    4 /* Public */,
       7,    0,   55,    2, 0x08,    5 /* Private */,
       8,    0,   56,    2, 0x08,    6 /* Private */,
       9,    0,   57,    2, 0x08,    7 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::TelemetryWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPETelemetryWorkerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TelemetryWorker, std::true_type>,
        // method 'snapshotReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<pulseboost::SystemSnapshot, std::false_type>,
        // method 'start'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCpuRamTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDiskTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProcessTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void pulseboost::TelemetryWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TelemetryWorker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->snapshotReady((*reinterpret_cast< std::add_pointer_t<pulseboost::SystemSnapshot>>(_a[1]))); break;
        case 1: _t->start(); break;
        case 2: _t->stop(); break;
        case 3: _t->onCpuRamTick(); break;
        case 4: _t->onDiskTick(); break;
        case 5: _t->onProcessTick(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TelemetryWorker::*)(pulseboost::SystemSnapshot );
            if (_t _q_method = &TelemetryWorker::snapshotReady; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *pulseboost::TelemetryWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::TelemetryWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPETelemetryWorkerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::TelemetryWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::TelemetryWorker::snapshotReady(pulseboost::SystemSnapshot _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS = QtMocHelpers::stringData(
    "pulseboost::TelemetryEngine",
    "snapshotReady",
    "",
    "pulseboost::SystemSnapshot",
    "snapshot"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS_t {
    uint offsetsAndSizes[10];
    char stringdata0[28];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[27];
    char stringdata4[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS = {
    {
        QT_MOC_LITERAL(0, 27),  // "pulseboost::TelemetryEngine"
        QT_MOC_LITERAL(28, 13),  // "snapshotReady"
        QT_MOC_LITERAL(42, 0),  // ""
        QT_MOC_LITERAL(43, 26),  // "pulseboost::SystemSnapshot"
        QT_MOC_LITERAL(70, 8)   // "snapshot"
    },
    "pulseboost::TelemetryEngine",
    "snapshotReady",
    "",
    "pulseboost::SystemSnapshot",
    "snapshot"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPETelemetryEngineENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::TelemetryEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPETelemetryEngineENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TelemetryEngine, std::true_type>,
        // method 'snapshotReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<pulseboost::SystemSnapshot, std::false_type>
    >,
    nullptr
} };

void pulseboost::TelemetryEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TelemetryEngine *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->snapshotReady((*reinterpret_cast< std::add_pointer_t<pulseboost::SystemSnapshot>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TelemetryEngine::*)(pulseboost::SystemSnapshot );
            if (_t _q_method = &TelemetryEngine::snapshotReady; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *pulseboost::TelemetryEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::TelemetryEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPETelemetryEngineENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::TelemetryEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::TelemetryEngine::snapshotReady(pulseboost::SystemSnapshot _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
