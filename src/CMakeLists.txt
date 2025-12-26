# Domain library (core data structures, no dependencies)
add_library(beater_domain STATIC
    domain/TimeTypes.cpp
    domain/Pattern.cpp
    domain/Region.cpp
    domain/Track.cpp
    domain/TempoMap.cpp
    domain/MeterMap.cpp
    domain/Instrument.cpp
    domain/Project.cpp
)

target_include_directories(beater_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(beater_domain PUBLIC
    nlohmann_json::nlohmann_json
)

# Engine library (audio processing, JACK integration)
add_library(beater_engine STATIC
    engine/JackAudioBackend.cpp
    engine/Transport.cpp
    engine/Sampler.cpp
    engine/SampleLibrary.cpp
    engine/Scheduler.cpp
    engine/Engine.cpp
)

target_include_directories(beater_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(beater_engine PUBLIC
    beater_domain
    ${JACK_LIBRARIES}
    ${SNDFILE_LIBRARIES}
)

target_include_directories(beater_engine PRIVATE
    ${JACK_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
)

# Serialization library
add_library(beater_serialization STATIC
    serialization/ProjectSerializer.cpp
)

target_include_directories(beater_serialization PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(beater_serialization PUBLIC
    beater_domain
    nlohmann_json::nlohmann_json
)

# UI library (only if Qt6 found)
if(Qt6_FOUND)
    # Qt resources
    qt_add_resources(BEATER_RESOURCES
        ${CMAKE_SOURCE_DIR}/resources/resources.qrc
    )
    
    add_library(beater_ui STATIC
        ui/MainWindow.cpp
        ui/TimelineWidget.cpp
        ui/TimelineView.cpp
        ui/PatternPalette.cpp
        ui/InstrumentPanel.cpp
        ${BEATER_RESOURCES}
    )

    target_include_directories(beater_ui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(beater_ui PUBLIC
        beater_domain
        beater_engine
        Qt6::Core
        Qt6::Widgets
    )

    # Main executable with UI
    add_executable(beater
        app/Main.cpp
    )

    target_link_libraries(beater PRIVATE
        beater_domain
        beater_engine
        beater_ui
        beater_serialization
        Qt6::Core
        Qt6::Widgets
    )
else()
    # Stub main executable without UI (for Phase 1 testing)
    message(STATUS "Building without UI - Phase 1 domain model only")
endif()

# Audio engine test program
add_executable(beater_test_audio
    app/TestAudio.cpp
)

target_link_libraries(beater_test_audio PRIVATE
    beater_domain
    beater_engine
)

# Pattern playback test program (Phase 3)
add_executable(beater_test_pattern
    app/TestPattern.cpp
)

target_link_libraries(beater_test_pattern PRIVATE
    beater_domain
    beater_engine
)

# Timeline playback test program (Phase 4)
add_executable(beater_test_timeline
    app/TestTimeline.cpp
)

target_link_libraries(beater_test_timeline PRIVATE
    beater_domain
    beater_engine
)
