/****************************************************************************
** Meta object code from reading C++ file 'gemini_client.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/PulseBoostAI/ai/gemini_client.hpp"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gemini_client.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS = QtMocHelpers::stringData(
    "pulseboost::GeminiClient",
    "streamChunkReceived",
    "",
    "chunk",
    "streamFinished",
    "errorOccurred",
    "errorText",
    "onReadyRead",
    "onReplyFinished",
    "onErrorOccurred",
    "QNetworkReply::NetworkError",
    "code"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS_t {
    uint offsetsAndSizes[24];
    char stringdata0[25];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[15];
    char stringdata5[14];
    char stringdata6[10];
    char stringdata7[12];
    char stringdata8[16];
    char stringdata9[16];
    char stringdata10[28];
    char stringdata11[5];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS = {
    {
        QT_MOC_LITERAL(0, 24),  // "pulseboost::GeminiClient"
        QT_MOC_LITERAL(25, 19),  // "streamChunkReceived"
        QT_MOC_LITERAL(45, 0),  // ""
        QT_MOC_LITERAL(46, 5),  // "chunk"
        QT_MOC_LITERAL(52, 14),  // "streamFinished"
        QT_MOC_LITERAL(67, 13),  // "errorOccurred"
        QT_MOC_LITERAL(81, 9),  // "errorText"
        QT_MOC_LITERAL(91, 11),  // "onReadyRead"
        QT_MOC_LITERAL(103, 15),  // "onReplyFinished"
        QT_MOC_LITERAL(119, 15),  // "onErrorOccurred"
        QT_MOC_LITERAL(135, 27),  // "QNetworkReply::NetworkError"
        QT_MOC_LITERAL(163, 4)   // "code"
    },
    "pulseboost::GeminiClient",
    "streamChunkReceived",
    "",
    "chunk",
    "streamFinished",
    "errorOccurred",
    "errorText",
    "onReadyRead",
    "onReplyFinished",
    "onErrorOccurred",
    "QNetworkReply::NetworkError",
    "code"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPEGeminiClientENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    0,   53,    2, 0x06,    3 /* Public */,
       5,    1,   54,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   57,    2, 0x08,    6 /* Private */,
       8,    0,   58,    2, 0x08,    7 /* Private */,
       9,    1,   59,    2, 0x08,    8 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::GeminiClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPEGeminiClientENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GeminiClient, std::true_type>,
        // method 'streamChunkReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'streamFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReplyFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onErrorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply::NetworkError, std::false_type>
    >,
    nullptr
} };

void pulseboost::GeminiClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GeminiClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->streamChunkReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->streamFinished(); break;
        case 2: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onReadyRead(); break;
        case 4: _t->onReplyFinished(); break;
        case 5: _t->onErrorOccurred((*reinterpret_cast< std::add_pointer_t<QNetworkReply::NetworkError>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply::NetworkError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GeminiClient::*)(QString );
            if (_t _q_method = &GeminiClient::streamChunkReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GeminiClient::*)();
            if (_t _q_method = &GeminiClient::streamFinished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GeminiClient::*)(QString );
            if (_t _q_method = &GeminiClient::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *pulseboost::GeminiClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::GeminiClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPEGeminiClientENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::GeminiClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::GeminiClient::streamChunkReceived(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void pulseboost::GeminiClient::streamFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void pulseboost::GeminiClient::errorOccurred(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
