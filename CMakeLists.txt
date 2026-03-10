cmake_minimum_required(VERSION 3.20)

project(PulseBoostAI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT DEFINED CMAKE_PREFIX_PATH AND DEFINED ENV{CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    Concurrent
)

qt_standard_project_setup()

add_executable(PulseBoost WIN32
    # App entry point
    app/main.cpp

    # Core – system scanning & threaded telemetry
    core/system_scanner.cpp
    core/process_manager.cpp
    core/memory_analyzer.cpp
    core/disk_analyzer.cpp
    core/registry_optimizer.cpp
    core/startup_optimizer.cpp
    core/service_manager.cpp
    core/telemetry_engine.cpp

    # AI agent pipeline
    ai/ai_diagnostics_engine.cpp
    ai/system_prediction.cpp
    ai/prompt_builder.cpp
    ai/llm_client.cpp
    ai/chat_router.cpp
    ai/agent_engine.cpp

    # Optimization modules
    modules/junk_cleaner.cpp
    modules/duplicate_file_finder.cpp
    modules/large_file_scanner.cpp
    modules/game_mode.cpp
    modules/developer_mode.cpp
    modules/network_optimizer.cpp
    modules/startup_optimizer.cpp
    modules/disk_analyzer.cpp
    modules/process_optimizer.cpp
    modules/service_manager.cpp
    modules/safety_guard.cpp

    # UI backend bridge (C++ ↔ QML)
    ui_backend/ui_controller.cpp

    # Data layer
    data/telemetry_logger.cpp
    data/optimization_history.cpp
    data/telemetry_cache.cpp

    # QML resources
    qml.qrc

    # Headers for MOC generation
    include/PulseBoostAI/ai/llm_client.hpp
    include/PulseBoostAI/ai/chat_router.hpp
    include/PulseBoostAI/ai/agent_engine.hpp
    include/PulseBoostAI/ui_backend/ui_controller.hpp
)

set_target_properties(PulseBoost PROPERTIES OUTPUT_NAME "PulseBoost")

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

target_link_libraries(PulseBoost PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Concurrent
    advapi32
    iphlpapi
    ole32
    oleaut32
    pdh
    powrprof
    psapi
    shell32
    wbemuuid
)
