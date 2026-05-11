/****************************************************************************
** Meta object code from reading C++ file 'ui_preferences.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/PulseBoostAI/ui_backend/ui_preferences.hpp"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ui_preferences.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS = QtMocHelpers::stringData(
    "pulseboost::UiPreferences",
    "smartAlertsEnabledChanged",
    "",
    "nativeTrayMessagesChanged",
    "minimizeToTrayOnCloseChanged",
    "alertCooldownSecondsChanged",
    "backgroundMonitoringEnabledChanged",
    "scheduleEnabledChanged",
    "scheduleModeChanged",
    "scheduleHourChanged",
    "scheduleCleanChanged",
    "scheduleRamChanged",
    "scheduleDnsChanged",
    "scheduleSecurityChanged",
    "lastScheduleRunStampChanged",
    "onboardingCompletedChanged",
    "smartAlertsEnabled",
    "nativeTrayMessages",
    "minimizeToTrayOnClose",
    "alertCooldownSeconds",
    "backgroundMonitoringEnabled",
    "scheduleEnabled",
    "scheduleMode",
    "scheduleHour",
    "scheduleClean",
    "scheduleRam",
    "scheduleDns",
    "scheduleSecurity",
    "lastScheduleRunStamp",
    "onboardingCompleted"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS_t {
    uint offsetsAndSizes[60];
    char stringdata0[26];
    char stringdata1[26];
    char stringdata2[1];
    char stringdata3[26];
    char stringdata4[29];
    char stringdata5[28];
    char stringdata6[35];
    char stringdata7[23];
    char stringdata8[20];
    char stringdata9[20];
    char stringdata10[21];
    char stringdata11[19];
    char stringdata12[19];
    char stringdata13[24];
    char stringdata14[28];
    char stringdata15[27];
    char stringdata16[19];
    char stringdata17[19];
    char stringdata18[22];
    char stringdata19[21];
    char stringdata20[28];
    char stringdata21[16];
    char stringdata22[13];
    char stringdata23[13];
    char stringdata24[14];
    char stringdata25[12];
    char stringdata26[12];
    char stringdata27[17];
    char stringdata28[21];
    char stringdata29[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS = {
    {
        QT_MOC_LITERAL(0, 25),  // "pulseboost::UiPreferences"
        QT_MOC_LITERAL(26, 25),  // "smartAlertsEnabledChanged"
        QT_MOC_LITERAL(52, 0),  // ""
        QT_MOC_LITERAL(53, 25),  // "nativeTrayMessagesChanged"
        QT_MOC_LITERAL(79, 28),  // "minimizeToTrayOnCloseChanged"
        QT_MOC_LITERAL(108, 27),  // "alertCooldownSecondsChanged"
        QT_MOC_LITERAL(136, 34),  // "backgroundMonitoringEnabledCh..."
        QT_MOC_LITERAL(171, 22),  // "scheduleEnabledChanged"
        QT_MOC_LITERAL(194, 19),  // "scheduleModeChanged"
        QT_MOC_LITERAL(214, 19),  // "scheduleHourChanged"
        QT_MOC_LITERAL(234, 20),  // "scheduleCleanChanged"
        QT_MOC_LITERAL(255, 18),  // "scheduleRamChanged"
        QT_MOC_LITERAL(274, 18),  // "scheduleDnsChanged"
        QT_MOC_LITERAL(293, 23),  // "scheduleSecurityChanged"
        QT_MOC_LITERAL(317, 27),  // "lastScheduleRunStampChanged"
        QT_MOC_LITERAL(345, 26),  // "onboardingCompletedChanged"
        QT_MOC_LITERAL(372, 18),  // "smartAlertsEnabled"
        QT_MOC_LITERAL(391, 18),  // "nativeTrayMessages"
        QT_MOC_LITERAL(410, 21),  // "minimizeToTrayOnClose"
        QT_MOC_LITERAL(432, 20),  // "alertCooldownSeconds"
        QT_MOC_LITERAL(453, 27),  // "backgroundMonitoringEnabled"
        QT_MOC_LITERAL(481, 15),  // "scheduleEnabled"
        QT_MOC_LITERAL(497, 12),  // "scheduleMode"
        QT_MOC_LITERAL(510, 12),  // "scheduleHour"
        QT_MOC_LITERAL(523, 13),  // "scheduleClean"
        QT_MOC_LITERAL(537, 11),  // "scheduleRam"
        QT_MOC_LITERAL(549, 11),  // "scheduleDns"
        QT_MOC_LITERAL(561, 16),  // "scheduleSecurity"
        QT_MOC_LITERAL(578, 20),  // "lastScheduleRunStamp"
        QT_MOC_LITERAL(599, 19)   // "onboardingCompleted"
    },
    "pulseboost::UiPreferences",
    "smartAlertsEnabledChanged",
    "",
    "nativeTrayMessagesChanged",
    "minimizeToTrayOnCloseChanged",
    "alertCooldownSecondsChanged",
    "backgroundMonitoringEnabledChanged",
    "scheduleEnabledChanged",
    "scheduleModeChanged",
    "scheduleHourChanged",
    "scheduleCleanChanged",
    "scheduleRamChanged",
    "scheduleDnsChanged",
    "scheduleSecurityChanged",
    "lastScheduleRunStampChanged",
    "onboardingCompletedChanged",
    "smartAlertsEnabled",
    "nativeTrayMessages",
    "minimizeToTrayOnClose",
    "alertCooldownSeconds",
    "backgroundMonitoringEnabled",
    "scheduleEnabled",
    "scheduleMode",
    "scheduleHour",
    "scheduleClean",
    "scheduleRam",
    "scheduleDns",
    "scheduleSecurity",
    "lastScheduleRunStamp",
    "onboardingCompleted"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPEUiPreferencesENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
      14,  112, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,   15 /* Public */,
       3,    0,   99,    2, 0x06,   16 /* Public */,
       4,    0,  100,    2, 0x06,   17 /* Public */,
       5,    0,  101,    2, 0x06,   18 /* Public */,
       6,    0,  102,    2, 0x06,   19 /* Public */,
       7,    0,  103,    2, 0x06,   20 /* Public */,
       8,    0,  104,    2, 0x06,   21 /* Public */,
       9,    0,  105,    2, 0x06,   22 /* Public */,
      10,    0,  106,    2, 0x06,   23 /* Public */,
      11,    0,  107,    2, 0x06,   24 /* Public */,
      12,    0,  108,    2, 0x06,   25 /* Public */,
      13,    0,  109,    2, 0x06,   26 /* Public */,
      14,    0,  110,    2, 0x06,   27 /* Public */,
      15,    0,  111,    2, 0x06,   28 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      16, QMetaType::Bool, 0x00015103, uint(0), 0,
      17, QMetaType::Bool, 0x00015103, uint(1), 0,
      18, QMetaType::Bool, 0x00015103, uint(2), 0,
      19, QMetaType::Int, 0x00015103, uint(3), 0,
      20, QMetaType::Bool, 0x00015103, uint(4), 0,
      21, QMetaType::Bool, 0x00015103, uint(5), 0,
      22, QMetaType::QString, 0x00015103, uint(6), 0,
      23, QMetaType::Int, 0x00015103, uint(7), 0,
      24, QMetaType::Bool, 0x00015103, uint(8), 0,
      25, QMetaType::Bool, 0x00015103, uint(9), 0,
      26, QMetaType::Bool, 0x00015103, uint(10), 0,
      27, QMetaType::Bool, 0x00015103, uint(11), 0,
      28, QMetaType::QString, 0x00015103, uint(12), 0,
      29, QMetaType::Bool, 0x00015103, uint(13), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::UiPreferences::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPEUiPreferencesENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS_t,
        // property 'smartAlertsEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'nativeTrayMessages'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'minimizeToTrayOnClose'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'alertCooldownSeconds'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'backgroundMonitoringEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'scheduleEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'scheduleMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'scheduleHour'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'scheduleClean'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'scheduleRam'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'scheduleDns'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'scheduleSecurity'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'lastScheduleRunStamp'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'onboardingCompleted'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UiPreferences, std::true_type>,
        // method 'smartAlertsEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'nativeTrayMessagesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'minimizeToTrayOnCloseChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'alertCooldownSecondsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'backgroundMonitoringEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleHourChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleCleanChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleRamChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleDnsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'scheduleSecurityChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastScheduleRunStampChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onboardingCompletedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void pulseboost::UiPreferences::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UiPreferences *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->smartAlertsEnabledChanged(); break;
        case 1: _t->nativeTrayMessagesChanged(); break;
        case 2: _t->minimizeToTrayOnCloseChanged(); break;
        case 3: _t->alertCooldownSecondsChanged(); break;
        case 4: _t->backgroundMonitoringEnabledChanged(); break;
        case 5: _t->scheduleEnabledChanged(); break;
        case 6: _t->scheduleModeChanged(); break;
        case 7: _t->scheduleHourChanged(); break;
        case 8: _t->scheduleCleanChanged(); break;
        case 9: _t->scheduleRamChanged(); break;
        case 10: _t->scheduleDnsChanged(); break;
        case 11: _t->scheduleSecurityChanged(); break;
        case 12: _t->lastScheduleRunStampChanged(); break;
        case 13: _t->onboardingCompletedChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::smartAlertsEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::nativeTrayMessagesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::minimizeToTrayOnCloseChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::alertCooldownSecondsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::backgroundMonitoringEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleModeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleHourChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleCleanChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleRamChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleDnsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::scheduleSecurityChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::lastScheduleRunStampChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (UiPreferences::*)();
            if (_t _q_method = &UiPreferences::onboardingCompletedChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UiPreferences *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->smartAlertsEnabled(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->nativeTrayMessages(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->minimizeToTrayOnClose(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->alertCooldownSeconds(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->backgroundMonitoringEnabled(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->scheduleEnabled(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->scheduleMode(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->scheduleHour(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->scheduleClean(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->scheduleRam(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->scheduleDns(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->scheduleSecurity(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->lastScheduleRunStamp(); break;
        case 13: *reinterpret_cast< bool*>(_v) = _t->onboardingCompleted(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UiPreferences *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setSmartAlertsEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 1: _t->setNativeTrayMessages(*reinterpret_cast< bool*>(_v)); break;
        case 2: _t->setMinimizeToTrayOnClose(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setAlertCooldownSeconds(*reinterpret_cast< int*>(_v)); break;
        case 4: _t->setBackgroundMonitoringEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 5: _t->setScheduleEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 6: _t->setScheduleMode(*reinterpret_cast< QString*>(_v)); break;
        case 7: _t->setScheduleHour(*reinterpret_cast< int*>(_v)); break;
        case 8: _t->setScheduleClean(*reinterpret_cast< bool*>(_v)); break;
        case 9: _t->setScheduleRam(*reinterpret_cast< bool*>(_v)); break;
        case 10: _t->setScheduleDns(*reinterpret_cast< bool*>(_v)); break;
        case 11: _t->setScheduleSecurity(*reinterpret_cast< bool*>(_v)); break;
        case 12: _t->setLastScheduleRunStamp(*reinterpret_cast< QString*>(_v)); break;
        case 13: _t->setOnboardingCompleted(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
    (void)_a;
}

const QMetaObject *pulseboost::UiPreferences::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::UiPreferences::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPEUiPreferencesENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::UiPreferences::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::UiPreferences::smartAlertsEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void pulseboost::UiPreferences::nativeTrayMessagesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void pulseboost::UiPreferences::minimizeToTrayOnCloseChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void pulseboost::UiPreferences::alertCooldownSecondsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void pulseboost::UiPreferences::backgroundMonitoringEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void pulseboost::UiPreferences::scheduleEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void pulseboost::UiPreferences::scheduleModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void pulseboost::UiPreferences::scheduleHourChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void pulseboost::UiPreferences::scheduleCleanChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void pulseboost::UiPreferences::scheduleRamChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void pulseboost::UiPreferences::scheduleDnsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void pulseboost::UiPreferences::scheduleSecurityChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void pulseboost::UiPreferences::lastScheduleRunStampChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void pulseboost::UiPreferences::onboardingCompletedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}
QT_WARNING_POP
