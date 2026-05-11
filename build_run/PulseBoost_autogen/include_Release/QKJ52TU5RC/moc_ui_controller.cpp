/****************************************************************************
** Meta object code from reading C++ file 'ui_controller.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.6.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/PulseBoostAI/ui_backend/ui_controller.hpp"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ui_controller.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS = QtMocHelpers::stringData(
    "pulseboost::UiController",
    "metricsChanged",
    "",
    "chartSeriesChanged",
    "processListChanged",
    "storageChanged",
    "actionsChanged",
    "notificationCenterChanged",
    "gameModeChanged",
    "snapshotsChanged",
    "actionFeedback",
    "message",
    "success",
    "uiStateChanged",
    "runClean",
    "runOptimize",
    "enableGameMode",
    "disableGameMode",
    "killProcess",
    "pid",
    "suspendProcess",
    "createRestorePoint",
    "sortedProcesses",
    "sortKey",
    "descending",
    "storageTreemap",
    "width",
    "height",
    "findLargeFiles",
    "flushDns",
    "optimizeTcp",
    "optimizeRam",
    "optimizeDisk",
    "checkLatency",
    "fetchStartupItems",
    "disableStartupItem",
    "name",
    "location",
    "command",
    "delayStartupItem",
    "delaySeconds",
    "exportChartSeriesCsv",
    "takeSystemSnapshot",
    "restoreSystemSnapshot",
    "snapshotId",
    "getScheduledOptimizations",
    "setScheduledOptimization",
    "taskId",
    "enabled",
    "type",
    "intervalHours",
    "listTweaks",
    "applyTweak",
    "id",
    "revertTweak",
    "applyOptimizationPreset",
    "presetId",
    "optimizeDetectedGame",
    "query",
    "launchOptimizedGame",
    "revertGameOptimization",
    "setAiPreferences",
    "mode",
    "apiKey",
    "refreshAll",
    "cpuUsage",
    "ramUsage",
    "diskUsage",
    "gpuUsage",
    "networkMbps",
    "healthScore",
    "cpuScore",
    "memoryScore",
    "diskScore",
    "securityScore",
    "forecastHealth24h",
    "summary",
    "startupCount",
    "chartSeries",
    "thermalSeries",
    "processList",
    "driverList",
    "driverSummary",
    "storageCategories",
    "recentActions",
    "healthLabel",
    "savedTodayMb",
    "networkOverview",
    "memoryOverview",
    "thermalOverview",
    "recoverableRamMb",
    "notifications",
    "gameModeActive",
    "gameModeStatus",
    "systemSnapshots",
    "pulseScore",
    "latestBenchmarkDelta",
    "advisorItems",
    "optimizationPresets",
    "detectedGames",
    "aiMode",
    "aiCloudConfigured",
    "uiDataReady",
    "uiErrorMessage",
    "telemetryAgeMs"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS_t {
    uint offsetsAndSizes[210];
    char stringdata0[25];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[19];
    char stringdata4[19];
    char stringdata5[15];
    char stringdata6[15];
    char stringdata7[26];
    char stringdata8[16];
    char stringdata9[17];
    char stringdata10[15];
    char stringdata11[8];
    char stringdata12[8];
    char stringdata13[15];
    char stringdata14[9];
    char stringdata15[12];
    char stringdata16[15];
    char stringdata17[16];
    char stringdata18[12];
    char stringdata19[4];
    char stringdata20[15];
    char stringdata21[19];
    char stringdata22[16];
    char stringdata23[8];
    char stringdata24[11];
    char stringdata25[15];
    char stringdata26[6];
    char stringdata27[7];
    char stringdata28[15];
    char stringdata29[9];
    char stringdata30[12];
    char stringdata31[12];
    char stringdata32[13];
    char stringdata33[13];
    char stringdata34[18];
    char stringdata35[19];
    char stringdata36[5];
    char stringdata37[9];
    char stringdata38[8];
    char stringdata39[17];
    char stringdata40[13];
    char stringdata41[21];
    char stringdata42[19];
    char stringdata43[22];
    char stringdata44[11];
    char stringdata45[26];
    char stringdata46[25];
    char stringdata47[7];
    char stringdata48[8];
    char stringdata49[5];
    char stringdata50[14];
    char stringdata51[11];
    char stringdata52[11];
    char stringdata53[3];
    char stringdata54[12];
    char stringdata55[24];
    char stringdata56[9];
    char stringdata57[21];
    char stringdata58[6];
    char stringdata59[20];
    char stringdata60[23];
    char stringdata61[17];
    char stringdata62[5];
    char stringdata63[7];
    char stringdata64[11];
    char stringdata65[9];
    char stringdata66[9];
    char stringdata67[10];
    char stringdata68[9];
    char stringdata69[12];
    char stringdata70[12];
    char stringdata71[9];
    char stringdata72[12];
    char stringdata73[10];
    char stringdata74[14];
    char stringdata75[18];
    char stringdata76[8];
    char stringdata77[13];
    char stringdata78[12];
    char stringdata79[14];
    char stringdata80[12];
    char stringdata81[11];
    char stringdata82[14];
    char stringdata83[18];
    char stringdata84[14];
    char stringdata85[12];
    char stringdata86[13];
    char stringdata87[16];
    char stringdata88[15];
    char stringdata89[16];
    char stringdata90[17];
    char stringdata91[14];
    char stringdata92[15];
    char stringdata93[15];
    char stringdata94[16];
    char stringdata95[11];
    char stringdata96[21];
    char stringdata97[13];
    char stringdata98[20];
    char stringdata99[14];
    char stringdata100[7];
    char stringdata101[18];
    char stringdata102[12];
    char stringdata103[15];
    char stringdata104[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS_t qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 24),  // "pulseboost::UiController"
        QT_MOC_LITERAL(25, 14),  // "metricsChanged"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 18),  // "chartSeriesChanged"
        QT_MOC_LITERAL(60, 18),  // "processListChanged"
        QT_MOC_LITERAL(79, 14),  // "storageChanged"
        QT_MOC_LITERAL(94, 14),  // "actionsChanged"
        QT_MOC_LITERAL(109, 25),  // "notificationCenterChanged"
        QT_MOC_LITERAL(135, 15),  // "gameModeChanged"
        QT_MOC_LITERAL(151, 16),  // "snapshotsChanged"
        QT_MOC_LITERAL(168, 14),  // "actionFeedback"
        QT_MOC_LITERAL(183, 7),  // "message"
        QT_MOC_LITERAL(191, 7),  // "success"
        QT_MOC_LITERAL(199, 14),  // "uiStateChanged"
        QT_MOC_LITERAL(214, 8),  // "runClean"
        QT_MOC_LITERAL(223, 11),  // "runOptimize"
        QT_MOC_LITERAL(235, 14),  // "enableGameMode"
        QT_MOC_LITERAL(250, 15),  // "disableGameMode"
        QT_MOC_LITERAL(266, 11),  // "killProcess"
        QT_MOC_LITERAL(278, 3),  // "pid"
        QT_MOC_LITERAL(282, 14),  // "suspendProcess"
        QT_MOC_LITERAL(297, 18),  // "createRestorePoint"
        QT_MOC_LITERAL(316, 15),  // "sortedProcesses"
        QT_MOC_LITERAL(332, 7),  // "sortKey"
        QT_MOC_LITERAL(340, 10),  // "descending"
        QT_MOC_LITERAL(351, 14),  // "storageTreemap"
        QT_MOC_LITERAL(366, 5),  // "width"
        QT_MOC_LITERAL(372, 6),  // "height"
        QT_MOC_LITERAL(379, 14),  // "findLargeFiles"
        QT_MOC_LITERAL(394, 8),  // "flushDns"
        QT_MOC_LITERAL(403, 11),  // "optimizeTcp"
        QT_MOC_LITERAL(415, 11),  // "optimizeRam"
        QT_MOC_LITERAL(427, 12),  // "optimizeDisk"
        QT_MOC_LITERAL(440, 12),  // "checkLatency"
        QT_MOC_LITERAL(453, 17),  // "fetchStartupItems"
        QT_MOC_LITERAL(471, 18),  // "disableStartupItem"
        QT_MOC_LITERAL(490, 4),  // "name"
        QT_MOC_LITERAL(495, 8),  // "location"
        QT_MOC_LITERAL(504, 7),  // "command"
        QT_MOC_LITERAL(512, 16),  // "delayStartupItem"
        QT_MOC_LITERAL(529, 12),  // "delaySeconds"
        QT_MOC_LITERAL(542, 20),  // "exportChartSeriesCsv"
        QT_MOC_LITERAL(563, 18),  // "takeSystemSnapshot"
        QT_MOC_LITERAL(582, 21),  // "restoreSystemSnapshot"
        QT_MOC_LITERAL(604, 10),  // "snapshotId"
        QT_MOC_LITERAL(615, 25),  // "getScheduledOptimizations"
        QT_MOC_LITERAL(641, 24),  // "setScheduledOptimization"
        QT_MOC_LITERAL(666, 6),  // "taskId"
        QT_MOC_LITERAL(673, 7),  // "enabled"
        QT_MOC_LITERAL(681, 4),  // "type"
        QT_MOC_LITERAL(686, 13),  // "intervalHours"
        QT_MOC_LITERAL(700, 10),  // "listTweaks"
        QT_MOC_LITERAL(711, 10),  // "applyTweak"
        QT_MOC_LITERAL(722, 2),  // "id"
        QT_MOC_LITERAL(725, 11),  // "revertTweak"
        QT_MOC_LITERAL(737, 23),  // "applyOptimizationPreset"
        QT_MOC_LITERAL(761, 8),  // "presetId"
        QT_MOC_LITERAL(770, 20),  // "optimizeDetectedGame"
        QT_MOC_LITERAL(791, 5),  // "query"
        QT_MOC_LITERAL(797, 19),  // "launchOptimizedGame"
        QT_MOC_LITERAL(817, 22),  // "revertGameOptimization"
        QT_MOC_LITERAL(840, 16),  // "setAiPreferences"
        QT_MOC_LITERAL(857, 4),  // "mode"
        QT_MOC_LITERAL(862, 6),  // "apiKey"
        QT_MOC_LITERAL(869, 10),  // "refreshAll"
        QT_MOC_LITERAL(880, 8),  // "cpuUsage"
        QT_MOC_LITERAL(889, 8),  // "ramUsage"
        QT_MOC_LITERAL(898, 9),  // "diskUsage"
        QT_MOC_LITERAL(908, 8),  // "gpuUsage"
        QT_MOC_LITERAL(917, 11),  // "networkMbps"
        QT_MOC_LITERAL(929, 11),  // "healthScore"
        QT_MOC_LITERAL(941, 8),  // "cpuScore"
        QT_MOC_LITERAL(950, 11),  // "memoryScore"
        QT_MOC_LITERAL(962, 9),  // "diskScore"
        QT_MOC_LITERAL(972, 13),  // "securityScore"
        QT_MOC_LITERAL(986, 17),  // "forecastHealth24h"
        QT_MOC_LITERAL(1004, 7),  // "summary"
        QT_MOC_LITERAL(1012, 12),  // "startupCount"
        QT_MOC_LITERAL(1025, 11),  // "chartSeries"
        QT_MOC_LITERAL(1037, 13),  // "thermalSeries"
        QT_MOC_LITERAL(1051, 11),  // "processList"
        QT_MOC_LITERAL(1063, 10),  // "driverList"
        QT_MOC_LITERAL(1074, 13),  // "driverSummary"
        QT_MOC_LITERAL(1088, 17),  // "storageCategories"
        QT_MOC_LITERAL(1106, 13),  // "recentActions"
        QT_MOC_LITERAL(1120, 11),  // "healthLabel"
        QT_MOC_LITERAL(1132, 12),  // "savedTodayMb"
        QT_MOC_LITERAL(1145, 15),  // "networkOverview"
        QT_MOC_LITERAL(1161, 14),  // "memoryOverview"
        QT_MOC_LITERAL(1176, 15),  // "thermalOverview"
        QT_MOC_LITERAL(1192, 16),  // "recoverableRamMb"
        QT_MOC_LITERAL(1209, 13),  // "notifications"
        QT_MOC_LITERAL(1223, 14),  // "gameModeActive"
        QT_MOC_LITERAL(1238, 14),  // "gameModeStatus"
        QT_MOC_LITERAL(1253, 15),  // "systemSnapshots"
        QT_MOC_LITERAL(1269, 10),  // "pulseScore"
        QT_MOC_LITERAL(1280, 20),  // "latestBenchmarkDelta"
        QT_MOC_LITERAL(1301, 12),  // "advisorItems"
        QT_MOC_LITERAL(1314, 19),  // "optimizationPresets"
        QT_MOC_LITERAL(1334, 13),  // "detectedGames"
        QT_MOC_LITERAL(1348, 6),  // "aiMode"
        QT_MOC_LITERAL(1355, 17),  // "aiCloudConfigured"
        QT_MOC_LITERAL(1373, 11),  // "uiDataReady"
        QT_MOC_LITERAL(1385, 14),  // "uiErrorMessage"
        QT_MOC_LITERAL(1400, 14)   // "telemetryAgeMs"
    },
    "pulseboost::UiController",
    "metricsChanged",
    "",
    "chartSeriesChanged",
    "processListChanged",
    "storageChanged",
    "actionsChanged",
    "notificationCenterChanged",
    "gameModeChanged",
    "snapshotsChanged",
    "actionFeedback",
    "message",
    "success",
    "uiStateChanged",
    "runClean",
    "runOptimize",
    "enableGameMode",
    "disableGameMode",
    "killProcess",
    "pid",
    "suspendProcess",
    "createRestorePoint",
    "sortedProcesses",
    "sortKey",
    "descending",
    "storageTreemap",
    "width",
    "height",
    "findLargeFiles",
    "flushDns",
    "optimizeTcp",
    "optimizeRam",
    "optimizeDisk",
    "checkLatency",
    "fetchStartupItems",
    "disableStartupItem",
    "name",
    "location",
    "command",
    "delayStartupItem",
    "delaySeconds",
    "exportChartSeriesCsv",
    "takeSystemSnapshot",
    "restoreSystemSnapshot",
    "snapshotId",
    "getScheduledOptimizations",
    "setScheduledOptimization",
    "taskId",
    "enabled",
    "type",
    "intervalHours",
    "listTweaks",
    "applyTweak",
    "id",
    "revertTweak",
    "applyOptimizationPreset",
    "presetId",
    "optimizeDetectedGame",
    "query",
    "launchOptimizedGame",
    "revertGameOptimization",
    "setAiPreferences",
    "mode",
    "apiKey",
    "refreshAll",
    "cpuUsage",
    "ramUsage",
    "diskUsage",
    "gpuUsage",
    "networkMbps",
    "healthScore",
    "cpuScore",
    "memoryScore",
    "diskScore",
    "securityScore",
    "forecastHealth24h",
    "summary",
    "startupCount",
    "chartSeries",
    "thermalSeries",
    "processList",
    "driverList",
    "driverSummary",
    "storageCategories",
    "recentActions",
    "healthLabel",
    "savedTodayMb",
    "networkOverview",
    "memoryOverview",
    "thermalOverview",
    "recoverableRamMb",
    "notifications",
    "gameModeActive",
    "gameModeStatus",
    "systemSnapshots",
    "pulseScore",
    "latestBenchmarkDelta",
    "advisorItems",
    "optimizationPresets",
    "detectedGames",
    "aiMode",
    "aiCloudConfigured",
    "uiDataReady",
    "uiErrorMessage",
    "telemetryAgeMs"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSpulseboostSCOPEUiControllerENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      42,   14, // methods
      40,  362, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  266,    2, 0x06,   41 /* Public */,
       3,    0,  267,    2, 0x06,   42 /* Public */,
       4,    0,  268,    2, 0x06,   43 /* Public */,
       5,    0,  269,    2, 0x06,   44 /* Public */,
       6,    0,  270,    2, 0x06,   45 /* Public */,
       7,    0,  271,    2, 0x06,   46 /* Public */,
       8,    0,  272,    2, 0x06,   47 /* Public */,
       9,    0,  273,    2, 0x06,   48 /* Public */,
      10,    2,  274,    2, 0x06,   49 /* Public */,
      13,    0,  279,    2, 0x06,   52 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      14,    0,  280,    2, 0x02,   53 /* Public */,
      15,    0,  281,    2, 0x02,   54 /* Public */,
      16,    0,  282,    2, 0x02,   55 /* Public */,
      17,    0,  283,    2, 0x02,   56 /* Public */,
      18,    1,  284,    2, 0x02,   57 /* Public */,
      20,    1,  287,    2, 0x02,   59 /* Public */,
      21,    0,  290,    2, 0x02,   61 /* Public */,
      22,    2,  291,    2, 0x102,   62 /* Public | MethodIsConst  */,
      25,    2,  296,    2, 0x102,   65 /* Public | MethodIsConst  */,
      28,    0,  301,    2, 0x02,   68 /* Public */,
      29,    0,  302,    2, 0x02,   69 /* Public */,
      30,    0,  303,    2, 0x02,   70 /* Public */,
      31,    0,  304,    2, 0x02,   71 /* Public */,
      32,    0,  305,    2, 0x02,   72 /* Public */,
      33,    0,  306,    2, 0x102,   73 /* Public | MethodIsConst  */,
      34,    0,  307,    2, 0x102,   74 /* Public | MethodIsConst  */,
      35,    3,  308,    2, 0x02,   75 /* Public */,
      39,    4,  315,    2, 0x02,   79 /* Public */,
      41,    0,  324,    2, 0x02,   84 /* Public */,
      42,    0,  325,    2, 0x02,   85 /* Public */,
      43,    1,  326,    2, 0x02,   86 /* Public */,
      45,    0,  329,    2, 0x102,   88 /* Public | MethodIsConst  */,
      46,    4,  330,    2, 0x02,   89 /* Public */,
      51,    0,  339,    2, 0x102,   94 /* Public | MethodIsConst  */,
      52,    1,  340,    2, 0x02,   95 /* Public */,
      54,    1,  343,    2, 0x02,   97 /* Public */,
      55,    1,  346,    2, 0x02,   99 /* Public */,
      57,    1,  349,    2, 0x02,  101 /* Public */,
      59,    1,  352,    2, 0x02,  103 /* Public */,
      60,    0,  355,    2, 0x02,  105 /* Public */,
      61,    2,  356,    2, 0x02,  106 /* Public */,
      64,    0,  361,    2, 0x02,  109 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   11,   12,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Bool, QMetaType::Int,   19,
    QMetaType::Void,
    QMetaType::QVariantList, QMetaType::QString, QMetaType::Bool,   23,   24,
    QMetaType::QVariantList, QMetaType::Int, QMetaType::Int,   26,   27,
    QMetaType::QVariantList,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Int,
    QMetaType::QVariantList,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::QString,   36,   37,   38,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Int,   36,   37,   38,   40,
    QMetaType::QString,
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::QString,   44,
    QMetaType::QVariantList,
    QMetaType::Bool, QMetaType::QString, QMetaType::Bool, QMetaType::QString, QMetaType::Int,   47,   48,   49,   50,
    QMetaType::QVariantList,
    QMetaType::Bool, QMetaType::QString,   53,
    QMetaType::Bool, QMetaType::QString,   53,
    QMetaType::Bool, QMetaType::QString,   56,
    QMetaType::Bool, QMetaType::QString,   58,
    QMetaType::Bool, QMetaType::QString,   58,
    QMetaType::Bool,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   62,   63,
    QMetaType::Void,

 // properties: name, type, flags
      65, QMetaType::Double, 0x00015001, uint(0), 0,
      66, QMetaType::Double, 0x00015001, uint(0), 0,
      67, QMetaType::Double, 0x00015001, uint(0), 0,
      68, QMetaType::Double, 0x00015001, uint(0), 0,
      69, QMetaType::Double, 0x00015001, uint(0), 0,
      70, QMetaType::Int, 0x00015001, uint(0), 0,
      71, QMetaType::Int, 0x00015001, uint(0), 0,
      72, QMetaType::Int, 0x00015001, uint(0), 0,
      73, QMetaType::Int, 0x00015001, uint(0), 0,
      74, QMetaType::Int, 0x00015001, uint(0), 0,
      75, QMetaType::Double, 0x00015001, uint(0), 0,
      76, QMetaType::QString, 0x00015001, uint(0), 0,
      77, QMetaType::Int, 0x00015001, uint(0), 0,
      78, QMetaType::QVariantList, 0x00015001, uint(1), 0,
      79, QMetaType::QVariantList, 0x00015001, uint(1), 0,
      80, QMetaType::QVariantList, 0x00015001, uint(2), 0,
      81, QMetaType::QVariantList, 0x00015001, uint(0), 0,
      82, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      83, QMetaType::QVariantList, 0x00015001, uint(3), 0,
      84, QMetaType::QVariantList, 0x00015001, uint(4), 0,
      85, QMetaType::QString, 0x00015001, uint(0), 0,
      86, QMetaType::Double, 0x00015001, uint(4), 0,
      87, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      88, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      89, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      90, QMetaType::Double, 0x00015001, uint(0), 0,
      91, QMetaType::QVariantList, 0x00015001, uint(5), 0,
      92, QMetaType::Bool, 0x00015001, uint(6), 0,
      93, QMetaType::QVariantMap, 0x00015001, uint(6), 0,
      94, QMetaType::QVariantList, 0x00015001, uint(7), 0,
      95, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      96, QMetaType::QVariantMap, 0x00015001, uint(0), 0,
      97, QMetaType::QVariantList, 0x00015001, uint(0), 0,
      98, QMetaType::QVariantList, 0x00015401, uint(-1), 0,
      99, QMetaType::QVariantList, 0x00015001, uint(6), 0,
     100, QMetaType::QString, 0x00015001, uint(9), 0,
     101, QMetaType::Bool, 0x00015001, uint(9), 0,
     102, QMetaType::Bool, 0x00015001, uint(9), 0,
     103, QMetaType::QString, 0x00015001, uint(9), 0,
     104, QMetaType::Int, 0x00015001, uint(9), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject pulseboost::UiController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSpulseboostSCOPEUiControllerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS_t,
        // property 'cpuUsage'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'ramUsage'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'diskUsage'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gpuUsage'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'networkMbps'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'healthScore'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'cpuScore'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'memoryScore'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'diskScore'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'securityScore'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'forecastHealth24h'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'summary'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'startupCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'chartSeries'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'thermalSeries'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'processList'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'driverList'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'driverSummary'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'storageCategories'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'recentActions'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'healthLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'savedTodayMb'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'networkOverview'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'memoryOverview'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'thermalOverview'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'recoverableRamMb'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'notifications'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'gameModeActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'gameModeStatus'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'systemSnapshots'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'pulseScore'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'latestBenchmarkDelta'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'advisorItems'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'optimizationPresets'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'detectedGames'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'aiMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'aiCloudConfigured'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'uiDataReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'uiErrorMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'telemetryAgeMs'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UiController, std::true_type>,
        // method 'metricsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chartSeriesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'processListChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'storageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'actionsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'notificationCenterChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'gameModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'snapshotsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'actionFeedback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'uiStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runClean'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runOptimize'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enableGameMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disableGameMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'killProcess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'suspendProcess'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'createRestorePoint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sortedProcesses'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'storageTreemap'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'findLargeFiles'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'flushDns'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'optimizeTcp'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'optimizeRam'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'optimizeDisk'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'checkLatency'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'fetchStartupItems'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'disableStartupItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'delayStartupItem'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'exportChartSeriesCsv'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'takeSystemSnapshot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'restoreSystemSnapshot'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getScheduledOptimizations'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'setScheduledOptimization'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'listTweaks'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'applyTweak'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'revertTweak'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'applyOptimizationPreset'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'optimizeDetectedGame'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'launchOptimizedGame'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'revertGameOptimization'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setAiPreferences'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void pulseboost::UiController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UiController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->metricsChanged(); break;
        case 1: _t->chartSeriesChanged(); break;
        case 2: _t->processListChanged(); break;
        case 3: _t->storageChanged(); break;
        case 4: _t->actionsChanged(); break;
        case 5: _t->notificationCenterChanged(); break;
        case 6: _t->gameModeChanged(); break;
        case 7: _t->snapshotsChanged(); break;
        case 8: _t->actionFeedback((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 9: _t->uiStateChanged(); break;
        case 10: _t->runClean(); break;
        case 11: _t->runOptimize(); break;
        case 12: _t->enableGameMode(); break;
        case 13: _t->disableGameMode(); break;
        case 14: _t->killProcess((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 15: { bool _r = _t->suspendProcess((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: _t->createRestorePoint(); break;
        case 17: { QVariantList _r = _t->sortedProcesses((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 18: { QVariantList _r = _t->storageTreemap((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 19: { QVariantList _r = _t->findLargeFiles();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 20: { bool _r = _t->flushDns();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 21: { bool _r = _t->optimizeTcp();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 22: { bool _r = _t->optimizeRam();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 23: { bool _r = _t->optimizeDisk();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 24: { int _r = _t->checkLatency();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 25: { QVariantList _r = _t->fetchStartupItems();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 26: { bool _r = _t->disableStartupItem((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 27: { bool _r = _t->delayStartupItem((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 28: { QString _r = _t->exportChartSeriesCsv();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 29: { bool _r = _t->takeSystemSnapshot();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 30: { bool _r = _t->restoreSystemSnapshot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 31: { QVariantList _r = _t->getScheduledOptimizations();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 32: { bool _r = _t->setScheduledOptimization((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 33: { QVariantList _r = _t->listTweaks();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 34: { bool _r = _t->applyTweak((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 35: { bool _r = _t->revertTweak((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 36: { bool _r = _t->applyOptimizationPreset((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 37: { bool _r = _t->optimizeDetectedGame((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 38: { bool _r = _t->launchOptimizedGame((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 39: { bool _r = _t->revertGameOptimization();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 40: { bool _r = _t->setAiPreferences((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 41: _t->refreshAll(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::metricsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::chartSeriesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::processListChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::storageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::actionsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::notificationCenterChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::gameModeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::snapshotsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (UiController::*)(QString , bool );
            if (_t _q_method = &UiController::actionFeedback; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (UiController::*)();
            if (_t _q_method = &UiController::uiStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UiController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< double*>(_v) = _t->cpuUsage(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->ramUsage(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->diskUsage(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->gpuUsage(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->networkMbps(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->healthScore(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->cpuScore(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->memoryScore(); break;
        case 8: *reinterpret_cast< int*>(_v) = _t->diskScore(); break;
        case 9: *reinterpret_cast< int*>(_v) = _t->securityScore(); break;
        case 10: *reinterpret_cast< double*>(_v) = _t->forecastHealth24h(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->summary(); break;
        case 12: *reinterpret_cast< int*>(_v) = _t->startupCount(); break;
        case 13: *reinterpret_cast< QVariantList*>(_v) = _t->chartSeries(); break;
        case 14: *reinterpret_cast< QVariantList*>(_v) = _t->thermalSeries(); break;
        case 15: *reinterpret_cast< QVariantList*>(_v) = _t->processList(); break;
        case 16: *reinterpret_cast< QVariantList*>(_v) = _t->driverList(); break;
        case 17: *reinterpret_cast< QVariantMap*>(_v) = _t->driverSummary(); break;
        case 18: *reinterpret_cast< QVariantList*>(_v) = _t->storageCategories(); break;
        case 19: *reinterpret_cast< QVariantList*>(_v) = _t->recentActions(); break;
        case 20: *reinterpret_cast< QString*>(_v) = _t->healthLabel(); break;
        case 21: *reinterpret_cast< double*>(_v) = _t->savedTodayMb(); break;
        case 22: *reinterpret_cast< QVariantMap*>(_v) = _t->networkOverview(); break;
        case 23: *reinterpret_cast< QVariantMap*>(_v) = _t->memoryOverview(); break;
        case 24: *reinterpret_cast< QVariantMap*>(_v) = _t->thermalOverview(); break;
        case 25: *reinterpret_cast< double*>(_v) = _t->recoverableRamMb(); break;
        case 26: *reinterpret_cast< QVariantList*>(_v) = _t->notifications(); break;
        case 27: *reinterpret_cast< bool*>(_v) = _t->gameModeActive(); break;
        case 28: *reinterpret_cast< QVariantMap*>(_v) = _t->gameModeStatus(); break;
        case 29: *reinterpret_cast< QVariantList*>(_v) = _t->systemSnapshots(); break;
        case 30: *reinterpret_cast< QVariantMap*>(_v) = _t->pulseScore(); break;
        case 31: *reinterpret_cast< QVariantMap*>(_v) = _t->latestBenchmarkDelta(); break;
        case 32: *reinterpret_cast< QVariantList*>(_v) = _t->advisorItems(); break;
        case 33: *reinterpret_cast< QVariantList*>(_v) = _t->optimizationPresets(); break;
        case 34: *reinterpret_cast< QVariantList*>(_v) = _t->detectedGames(); break;
        case 35: *reinterpret_cast< QString*>(_v) = _t->aiMode(); break;
        case 36: *reinterpret_cast< bool*>(_v) = _t->aiCloudConfigured(); break;
        case 37: *reinterpret_cast< bool*>(_v) = _t->uiDataReady(); break;
        case 38: *reinterpret_cast< QString*>(_v) = _t->uiErrorMessage(); break;
        case 39: *reinterpret_cast< int*>(_v) = _t->telemetryAgeMs(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *pulseboost::UiController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *pulseboost::UiController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSpulseboostSCOPEUiControllerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int pulseboost::UiController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 42)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 42;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 42)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 42;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 40;
    }
    return _id;
}

// SIGNAL 0
void pulseboost::UiController::metricsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void pulseboost::UiController::chartSeriesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void pulseboost::UiController::processListChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void pulseboost::UiController::storageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void pulseboost::UiController::actionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void pulseboost::UiController::notificationCenterChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void pulseboost::UiController::gameModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void pulseboost::UiController::snapshotsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void pulseboost::UiController::actionFeedback(QString _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void pulseboost::UiController::uiStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}
QT_WARNING_POP
