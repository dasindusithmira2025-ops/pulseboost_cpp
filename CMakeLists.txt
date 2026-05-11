cmake_minimum_required(VERSION 3.20)

project(PulseBoostAI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_definitions(
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="PulseBoost AI"
)

if(NOT DEFINED CMAKE_PREFIX_PATH AND DEFINED ENV{CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Sql
    Concurrent
    Network
)

qt_standard_project_setup()

add_executable(PulseBoost WIN32
    # App entry point
    app/main.cpp

    # Common utilities
    common/wmi_wrapper.cpp

    # Core – system scanning & threaded telemetry
    core/system_scanner.cpp
    core/process_manager.cpp
    core/memory_analyzer.cpp
    core/disk_analyzer.cpp
    core/registry_optimizer.cpp
    core/startup_optimizer.cpp
    core/service_manager.cpp
    core/telemetry_engine.cpp
    core/auto_updater.cpp
    core/crash_reporter.cpp
    core/license_manager.cpp
    core/pulse_bench.cpp
    core/system_advisor.cpp
    core/game_optimizer.cpp
    core/backend_daemon.cpp

    # AI agent pipeline
    ai/ai_diagnostics_engine.cpp
    ai/system_prediction.cpp
    ai/prompt_builder.cpp
    ai/chat_router.cpp
    ai/agent_engine.cpp
    ai/context_builder.cpp
    ai/session_memory.cpp
    ai/proactive_monitor.cpp
    ai/pulse_model.cpp
    ai/model/intent_classifier.cpp
    ai/model/context_fusion.cpp
    ai/model/response_generator.cpp
    ai/model/adaptive_engine.cpp

    # Optimization modules
    modules/junk_cleaner.cpp
    modules/duplicate_file_finder.cpp
    modules/large_file_scanner.cpp
    modules/game_mode.cpp
    modules/developer_mode.cpp
    modules/network_optimizer.cpp
    modules/ram_optimizer.cpp
    modules/defrag_trigger.cpp
    modules/startup_optimizer.cpp
    modules/disk_analyzer.cpp
    modules/process_optimizer.cpp
    modules/service_manager.cpp
    modules/safety_guard.cpp
    modules/tweak_engine.cpp

    # UI backend bridge (C++ ↔ QML)
    ui_backend/ui_controller.cpp
    ui_backend/notification_manager.cpp
    ui_backend/feature_gate.cpp
    ui_backend/ui_preferences.cpp

    # Data layer
    data/telemetry_logger.cpp
    data/optimization_history.cpp
    data/telemetry_cache.cpp

    # QML resources
    qml.qrc

    # Headers for MOC generation
    include/PulseBoostAI/ai/chat_router.hpp
    include/PulseBoostAI/ai/agent_engine.hpp
    include/PulseBoostAI/ai/proactive_monitor.hpp
    include/PulseBoostAI/ai/pulse_model.hpp
    include/PulseBoostAI/ai/model/intent_classifier.hpp
    include/PulseBoostAI/ai/model/context_fusion.hpp
    include/PulseBoostAI/ai/model/response_generator.hpp
    include/PulseBoostAI/ai/model/adaptive_engine.hpp
    include/PulseBoostAI/ui_backend/ui_controller.hpp
    include/PulseBoostAI/ui_backend/notification_manager.hpp
    include/PulseBoostAI/ui_backend/feature_gate.hpp
    include/PulseBoostAI/ui_backend/ui_preferences.hpp
    include/PulseBoostAI/common/wmi_wrapper.hpp
    include/PulseBoostAI/core/auto_updater.hpp
    include/PulseBoostAI/core/crash_reporter.hpp
    include/PulseBoostAI/core/license_manager.hpp
    include/PulseBoostAI/core/pulse_bench.hpp
    include/PulseBoostAI/core/system_advisor.hpp
    include/PulseBoostAI/core/game_optimizer.hpp
    include/PulseBoostAI/core/backend_daemon.hpp
)

set_target_properties(PulseBoost PROPERTIES OUTPUT_NAME "PulseBoostAI")

target_include_directories(PulseBoost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(PulseBoost PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    _CRT_SECURE_NO_WARNINGS
)

# Optional ONNX Runtime integration.
if(DEFINED ONNXRUNTIME_DIR)
    find_library(ONNXRUNTIME_LIB onnxruntime PATHS ${ONNXRUNTIME_DIR}/lib NO_DEFAULT_PATH)
    if(ONNXRUNTIME_LIB)
        target_include_directories(PulseBoost PRIVATE ${ONNXRUNTIME_DIR}/include)
        target_link_libraries(PulseBoost PRIVATE ${ONNXRUNTIME_LIB})
        target_compile_definitions(PulseBoost PRIVATE PULSEBOOST_WITH_ONNXRUNTIME=1)
    endif()
endif()

target_link_libraries(PulseBoost PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Network
    advapi32
    iphlpapi
    ole32
    oleaut32
    pdh
    powrprof
    psapi
    runtimeobject
    shell32
    srclient
    wbemuuid
    wininet
    urlmon
    dbghelp
    crypt32
)




