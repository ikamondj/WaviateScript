cmake_minimum_required(VERSION 3.24)

project(WaviateScript VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)

option(WAVIATESCRIPT_PREMIUM "Build WaviateScript with WSPremium features" OFF)
option(WAVIATESCRIPT_COPY_PLUGIN_AFTER_BUILD "Ask JUCE to copy built plugin artifacts to the local plugin folders" OFF)

set(WAVIATESCRIPT_JUCE_DIR "" CACHE PATH "Path to a local JUCE 8.0.12 checkout")
set(WAVIATESCRIPT_PREMIUM_SOURCE_DIR "" CACHE PATH "Path to the WSPremium source checkout")
set(WAVIATESCRIPT_FFTW_DIR "" CACHE PATH "Optional FFTW install prefix containing include/ and lib/")

set(WAVIATESCRIPT_JUCE_TAG "8.0.12" CACHE STRING "Pinned JUCE tag or commit used when WaviateScript fetches JUCE")

if(WAVIATESCRIPT_JUCE_DIR)
    if(NOT EXISTS "${WAVIATESCRIPT_JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "WAVIATESCRIPT_JUCE_DIR does not point to a JUCE checkout with CMakeLists.txt: ${WAVIATESCRIPT_JUCE_DIR}")
    endif()

    set(WAVIATESCRIPT_JUCE_SOURCE_DIR "${WAVIATESCRIPT_JUCE_DIR}")
    add_subdirectory("${WAVIATESCRIPT_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG "${WAVIATESCRIPT_JUCE_TAG}"
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
    set(WAVIATESCRIPT_JUCE_SOURCE_DIR "${juce_SOURCE_DIR}")
endif()

if(NOT COMMAND juce_add_plugin AND EXISTS "${WAVIATESCRIPT_JUCE_SOURCE_DIR}/extras/Build/CMake/JUCEUtils.cmake")
    include("${WAVIATESCRIPT_JUCE_SOURCE_DIR}/extras/Build/CMake/JUCEUtils.cmake")
endif()

if(NOT COMMAND juce_add_plugin)
    message(FATAL_ERROR "JUCE was found at '${WAVIATESCRIPT_JUCE_SOURCE_DIR}', but juce_add_plugin is unavailable. Check that the JUCE checkout is complete and accessible.")
endif()

if(WAVIATESCRIPT_PREMIUM AND NOT WAVIATESCRIPT_PREMIUM_SOURCE_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/WSPremium/WSPremium.jucer")
        set(WAVIATESCRIPT_PREMIUM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/WSPremium" CACHE PATH "Path to the WSPremium source checkout" FORCE)
    endif()
endif()

if(WAVIATESCRIPT_PREMIUM)
    if(NOT WAVIATESCRIPT_PREMIUM_SOURCE_DIR OR NOT EXISTS "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/WSPremium.jucer")
        message(FATAL_ERROR "WAVIATESCRIPT_PREMIUM=ON requires WAVIATESCRIPT_PREMIUM_SOURCE_DIR to point at a WSPremium checkout containing WSPremium.jucer.")
    endif()
endif()

find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
message(STATUS "Using Clang package from ${Clang_DIR}")

set(WAVIATESCRIPT_FFTW_HINTS)
if(WAVIATESCRIPT_FFTW_DIR)
    list(APPEND WAVIATESCRIPT_FFTW_HINTS "${WAVIATESCRIPT_FFTW_DIR}")
endif()

find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    HINTS ${WAVIATESCRIPT_FFTW_HINTS}
    PATH_SUFFIXES include
)

find_library(FFTW3F_LIBRARY
    NAMES fftw3f libfftw3f-3 fftw3f-3
    HINTS ${WAVIATESCRIPT_FFTW_HINTS}
    PATH_SUFFIXES lib lib64
)

find_library(FFTW3_LIBRARY
    NAMES fftw3 libfftw3-3 fftw3-3
    HINTS ${WAVIATESCRIPT_FFTW_HINTS}
    PATH_SUFFIXES lib lib64
)

find_library(FFTW3L_LIBRARY
    NAMES fftw3l libfftw3l-3 fftw3l-3
    HINTS ${WAVIATESCRIPT_FFTW_HINTS}
    PATH_SUFFIXES lib lib64
)

if(NOT FFTW3_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find fftw3.h. Set WAVIATESCRIPT_FFTW_DIR or FFTW3_INCLUDE_DIR.")
endif()

set(WAVIATESCRIPT_PLUGIN_FORMATS Standalone VST3)
if(APPLE)
    list(APPEND WAVIATESCRIPT_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(WaviateScript
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "yourcompany"
    COMPANY_WEBSITE "www.yourcompany.com"
    BUNDLE_ID "com.yourcompany.WaviateScript"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${WAVIATESCRIPT_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lh86
    FORMATS ${WAVIATESCRIPT_PLUGIN_FORMATS}
    PRODUCT_NAME "WaviateScript"
)

juce_generate_juce_header(WaviateScript)

target_sources(WaviateScript
    PRIVATE
        Source/AbstractCompiler.cpp
        Source/AbstractCompiler.h
        Source/CFileTemplateGenerator.cpp
        Source/CFileTemplateGenerator.h
        Source/ClangExternalCompiler.h
        Source/CodeEditor.cpp
        Source/CodeEditor.h
        Source/Compilers.h
        Source/CppFileTemplateGenerator.cpp
        Source/CppFileTemplateGenerator.h
        Source/LockFreeFifo.h
        Source/NewFileTemplateGenerator.cpp
        Source/NewFileTemplateGenerator.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/WasmCompiler.cpp
        Source/WasmCompiler.h
        Source/Waviate.h
        Source/Waviate.hpp
        Source/WaviateInput.h
)

target_compile_features(WaviateScript PRIVATE cxx_std_20)

target_compile_definitions(WaviateScript
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(WaviateScript
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${FFTW3_INCLUDE_DIR}"
        "${LLVM_INCLUDE_DIRS}"
        "${CLANG_INCLUDE_DIRS}"
)

target_link_libraries(WaviateScript
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(FFTW3F_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3F_LIBRARY}")
endif()

if(FFTW3_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3_LIBRARY}")
endif()

if(FFTW3L_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3L_LIBRARY}")
endif()

set(WAVIATESCRIPT_CLANG_TARGETS
    clangFrontend
    clangDriver
    clangParse
    clangSema
    clangSerialization
    clangAST
    clangLex
    clangBasic
    clangCodeGen
    clangTooling
    clangToolingCore
    clangAnalysis
    clangEdit
    clangASTMatchers
    clangDynamicASTMatchers
)

foreach(clang_target IN LISTS WAVIATESCRIPT_CLANG_TARGETS)
    if(TARGET "${clang_target}")
        target_link_libraries(WaviateScript PRIVATE "${clang_target}")
    endif()
endforeach()

if(COMMAND llvm_map_components_to_libnames)
    llvm_map_components_to_libnames(WAVIATESCRIPT_LLVM_LIBS
        native
        nativecodegen
        orcjit
        mcjit
        executionengine
        core
        support
        target
        analysis
        passes
        scalaropts
        instcombine
        bitreader
        bitwriter
        irreader
        object
    )
    target_link_libraries(WaviateScript PRIVATE ${WAVIATESCRIPT_LLVM_LIBS})
endif()

if(WAVIATESCRIPT_PREMIUM)
    set(WAVIATESCRIPT_PREMIUM_SOURCES
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/ChannelInterface.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/GameControllerInputEvent.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/GameControllerInterface.cpp"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/GameControllerInterface.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/OSCInput.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/OSCInterface.cpp"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/OSCInterface.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/RustCompiler.cpp"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/RustCompiler.h"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/RustFileTemplateGenerator.cpp"
        "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}/RustFileTemplateGenerator.h"
    )

    add_library(WSPremium STATIC ${WAVIATESCRIPT_PREMIUM_SOURCES})
    target_compile_features(WSPremium PUBLIC cxx_std_20)
    target_compile_definitions(WSPremium PUBLIC WAV_SCRIPT_PREMIUM=1 JUCE_STRICT_REFCOUNTEDPOINTER=1)
    target_include_directories(WSPremium
        PUBLIC
            "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    )
    target_link_libraries(WSPremium
        PUBLIC
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_osc
    )

    target_compile_definitions(WaviateScript PRIVATE WAV_SCRIPT_PREMIUM=1)
    target_include_directories(WaviateScript PRIVATE "${WAVIATESCRIPT_PREMIUM_SOURCE_DIR}")
    target_link_libraries(WaviateScript PRIVATE WSPremium)
endif()

set(WAVIATESCRIPT_OUTPUT_ROOT "${CMAKE_BINARY_DIR}/WaviateScript_artefacts" CACHE PATH "Root folder for built plugin artifacts")

set_target_properties(WaviateScript PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${WAVIATESCRIPT_OUTPUT_ROOT}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${WAVIATESCRIPT_OUTPUT_ROOT}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${WAVIATESCRIPT_OUTPUT_ROOT}/bin"
)
