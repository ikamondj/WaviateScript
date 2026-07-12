cmake_minimum_required(VERSION 3.24)

project(WaviateScript VERSION 1.0.0 LANGUAGES C CXX)

# Workaround for new MSVC versions where CMake does not detect known features
if(MSVC AND NOT CMAKE_CXX_COMPILE_FEATURES)
    set(CMAKE_CXX_COMPILE_FEATURES
        cxx_std_98
        cxx_std_11
        cxx_std_14
        cxx_std_17
        cxx_std_20
        cxx_std_23
    )
endif()
if(MSVC AND NOT CMAKE_C_COMPILE_FEATURES)
    set(CMAKE_C_COMPILE_FEATURES
        c_std_99
        c_std_11
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CTest)

option(WAVIATESCRIPT_PREMIUM "Build WaviateScript with WSPremium features" OFF)
option(WAVIATESCRIPT_COPY_PLUGIN_AFTER_BUILD "Ask JUCE to copy built plugin artifacts to the local plugin folders" OFF)
option(WAVIATESCRIPT_REQUIRE_FFTW "Fail CMake configure when FFTW is not available" OFF)
option(WAVIATESCRIPT_BUILD_TESTS "Build the WaviateScript unit test executable" ${BUILD_TESTING})

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

function(waviate_first_existing_path output_variable)
    foreach(candidate IN LISTS ARGN)
        if(candidate AND EXISTS "${candidate}")
            set("${output_variable}" "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

if(WIN32)
    if(NOT LLVM_DIR)
        waviate_first_existing_path(WAVIATESCRIPT_DETECTED_LLVM_DIR
            "$ENV{LLVM_DIR}"
            "C:/src/llvm-dbg/install/lib/cmake/llvm"
            "C:/src/llvm-dbg/build/lib/cmake/llvm"
            "$ENV{ProgramFiles}/LLVM/lib/cmake/llvm"
            "$ENV{ProgramW6432}/LLVM/lib/cmake/llvm"
        )
        if(WAVIATESCRIPT_DETECTED_LLVM_DIR)
            set(LLVM_DIR "${WAVIATESCRIPT_DETECTED_LLVM_DIR}" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
        endif()
    endif()

    if(NOT Clang_DIR)
        waviate_first_existing_path(WAVIATESCRIPT_DETECTED_CLANG_DIR
            "$ENV{Clang_DIR}"
            "C:/src/llvm-dbg/install/lib/cmake/clang"
            "C:/src/llvm-dbg/build/lib/cmake/clang"
            "$ENV{ProgramFiles}/LLVM/lib/cmake/clang"
            "$ENV{ProgramW6432}/LLVM/lib/cmake/clang"
        )
        if(WAVIATESCRIPT_DETECTED_CLANG_DIR)
            set(Clang_DIR "${WAVIATESCRIPT_DETECTED_CLANG_DIR}" CACHE PATH "Path to ClangConfig.cmake" FORCE)
        endif()
    endif()
endif()

find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
message(STATUS "Using Clang package from ${Clang_DIR}")

set(WAVIATESCRIPT_LLVM_HAS_DEBUG_CONFIG OFF)
if(TARGET LLVMSupport)
    get_target_property(WAVIATESCRIPT_LLVM_IMPORTED_CONFIGS LLVMSupport IMPORTED_CONFIGURATIONS)
    if(WAVIATESCRIPT_LLVM_IMPORTED_CONFIGS)
        list(FIND WAVIATESCRIPT_LLVM_IMPORTED_CONFIGS DEBUG WAVIATESCRIPT_LLVM_DEBUG_CONFIG_INDEX)
        if(NOT WAVIATESCRIPT_LLVM_DEBUG_CONFIG_INDEX EQUAL -1)
            set(WAVIATESCRIPT_LLVM_HAS_DEBUG_CONFIG ON)
        endif()
    endif()
endif()

function(waviate_configure_msvc_llvm_compat target_name)
    if(MSVC)
        if(WAVIATESCRIPT_LLVM_HAS_DEBUG_CONFIG)
            set_property(TARGET "${target_name}" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        else()
            # The packaged Windows LLVM/Clang binaries are built against the release CRT.
            set_property(TARGET "${target_name}" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
            target_compile_definitions("${target_name}" PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
        endif()
    endif()
endfunction()

if(WIN32 AND TARGET LLVMDebugInfoPDB)
    set(WAVIATESCRIPT_DIAGUIDS_LIB "")
    if(CMAKE_GENERATOR_INSTANCE AND EXISTS "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64/diaguids.lib")
        set(WAVIATESCRIPT_DIAGUIDS_LIB "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64/diaguids.lib")
    elseif(EXISTS "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
        set(WAVIATESCRIPT_DIAGUIDS_LIB "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
    endif()

    if(WAVIATESCRIPT_DIAGUIDS_LIB)
        get_target_property(WAVIATESCRIPT_LLVM_PDB_LINK_LIBS LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
        if(WAVIATESCRIPT_LLVM_PDB_LINK_LIBS)
            string(REPLACE
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib"
                "${WAVIATESCRIPT_DIAGUIDS_LIB}"
                WAVIATESCRIPT_LLVM_PDB_LINK_LIBS
                "${WAVIATESCRIPT_LLVM_PDB_LINK_LIBS}"
            )
            set_target_properties(LLVMDebugInfoPDB PROPERTIES
                INTERFACE_LINK_LIBRARIES "${WAVIATESCRIPT_LLVM_PDB_LINK_LIBS}"
            )
            message(STATUS "Adjusted LLVMDebugInfoPDB to use ${WAVIATESCRIPT_DIAGUIDS_LIB}")
        endif()
    else()
        message(WARNING "Could not locate a DIA SDK diaguids.lib to override LLVM's stale imported path. LLVMDebugInfoPDB may fail to link.")
    endif()
endif()

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

if(FFTW3_INCLUDE_DIR)
    message(STATUS "Using FFTW headers from ${FFTW3_INCLUDE_DIR}")
elseif(WAVIATESCRIPT_REQUIRE_FFTW)
    message(FATAL_ERROR "Could not find fftw3.h. Set WAVIATESCRIPT_FFTW_DIR or FFTW3_INCLUDE_DIR.")
else()
    message(STATUS "FFTW not found; continuing because the current public build does not require it.")
endif()

set(WAVIATESCRIPT_PLUGIN_FORMATS Standalone VST3)
if(APPLE)
    list(APPEND WAVIATESCRIPT_PLUGIN_FORMATS AU)
endif()

set(WAVIATESCRIPT_CORE_SOURCES
    Source/AbstractCompiler.cpp
    Source/AbstractCompiler.h
    Source/ClangExternalCompiler.h
    Source/CompilePipeline.cpp
    Source/CompilePipeline.h
    Source/WasmCompiler.cpp
    Source/WasmCompiler.h
    Source/Waviate.h
    Source/Waviate.hpp
    Source/WaviateCppLanguageModel.cpp
    Source/WaviateCppLanguageModel.h
    Source/WaviateShaderSerializer.cpp
    Source/WaviateShaderSerializer.h
    Source/WaviateInput.h
    Source/WaviateComplex.h
    Source/WaviateAudio.cpp
    Source/WaviateAudio.h
    Source/WaviateFuelInstrumentation.cpp
    Source/WaviateRuntimeCapabilities.cpp
    Source/WaviateRuntimeCapabilities.h
    Source/WaviateSafety.cpp
    Source/WaviateSafetyValidator.cpp
)

add_library(WaviateScriptCore STATIC ${WAVIATESCRIPT_CORE_SOURCES})
# target_compile_features(WaviateScriptCore PUBLIC cxx_std_20)
target_include_directories(WaviateScriptCore
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${LLVM_INCLUDE_DIRS}"
        "${CLANG_INCLUDE_DIRS}"
)
target_link_libraries(WaviateScriptCore
    PUBLIC
        juce::juce_core
)
waviate_configure_msvc_llvm_compat(WaviateScriptCore)

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
        Source/CFileTemplateGenerator.cpp
        Source/CFileTemplateGenerator.h
        Source/CodeEditor.cpp
        Source/CodeEditor.h
        Source/CodeEditorCompletion.cpp
        Source/CodeEditorCompletion.h
        Source/CppFileTemplateGenerator.cpp
        Source/CppFileTemplateGenerator.h
        Source/LockFreeFifo.h
        Source/MarketplaceClient.cpp
        Source/MarketplaceClient.h
        Source/NewFileTemplateGenerator.cpp
        Source/NewFileTemplateGenerator.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/AudioClipsPanel.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/AudioProcessingFlow.inl
)

# target_compile_features(WaviateScript PRIVATE cxx_std_20)

target_compile_definitions(WaviateScript
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(WaviateScript
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

if(FFTW3_INCLUDE_DIR)
    target_include_directories(WaviateScript PRIVATE "${FFTW3_INCLUDE_DIR}")
endif()

target_link_libraries(WaviateScript
    PRIVATE
        WaviateScriptCore
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
waviate_configure_msvc_llvm_compat(WaviateScript)

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
        target_link_libraries(WaviateScriptCore PUBLIC "${clang_target}")
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
    target_link_libraries(WaviateScriptCore PUBLIC ${WAVIATESCRIPT_LLVM_LIBS})
endif()

if(FFTW3F_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3F_LIBRARY}")
endif()

if(FFTW3_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3_LIBRARY}")
endif()

if(FFTW3L_LIBRARY)
    target_link_libraries(WaviateScript PRIVATE "${FFTW3L_LIBRARY}")
endif()

if(WAVIATESCRIPT_BUILD_TESTS)
    add_executable(WaviateScriptTests
        Tests/TestMain.cpp
        Tests/TestSupport/WaviateUnitTest.h
        Tests/Compile/CompileTestHelpers.cpp
        Tests/Compile/CompileTestHelpers.h
        Tests/Compile/CompilePipelineTests.cpp
        Tests/Serialization/ShaderSerializerTests.cpp
    )
    # target_compile_features(WaviateScriptTests PRIVATE cxx_std_20)
    target_include_directories(WaviateScriptTests
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    )
    target_compile_definitions(WaviateScriptTests
        PRIVATE
            WAVIATESCRIPT_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(WaviateScriptTests
        PRIVATE
            WaviateScriptCore
            juce::juce_core
    )
    waviate_configure_msvc_llvm_compat(WaviateScriptTests)
    set_target_properties(WaviateScriptTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin"
    )
    add_test(NAME WaviateScript.UnitTests COMMAND WaviateScriptTests)
    set_tests_properties(WaviateScript.UnitTests PROPERTIES
        CONFIGURATIONS "Release;RelWithDebInfo;MinSizeRel"
    )
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
    # target_compile_features(WSPremium PUBLIC cxx_std_20)
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
    waviate_configure_msvc_llvm_compat(WSPremium)

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

if(WIN32)
    set(WAVIATESCRIPT_LLVM_BIN_DIR "")
    if(DEFINED LLVM_TOOLS_BINARY_DIR AND EXISTS "${LLVM_TOOLS_BINARY_DIR}/LLVM-C.dll")
        set(WAVIATESCRIPT_LLVM_BIN_DIR "${LLVM_TOOLS_BINARY_DIR}")
    elseif(EXISTS "${LLVM_DIR}/../../../bin/LLVM-C.dll")
        get_filename_component(WAVIATESCRIPT_LLVM_BIN_DIR "${LLVM_DIR}/../../../bin" ABSOLUTE)
    endif()

    set(WAVIATESCRIPT_ZLIB_BIN_DIR "")
    set(WAVIATESCRIPT_ZSTD_BIN_DIR "")
    if(WAVIATESCRIPT_LLVM_BIN_DIR)
        if(EXISTS "${WAVIATESCRIPT_LLVM_BIN_DIR}/zlib1.dll")
            set(WAVIATESCRIPT_ZLIB_BIN_DIR "${WAVIATESCRIPT_LLVM_BIN_DIR}")
        endif()
        if(EXISTS "${WAVIATESCRIPT_LLVM_BIN_DIR}/zstd.dll")
            set(WAVIATESCRIPT_ZSTD_BIN_DIR "${WAVIATESCRIPT_LLVM_BIN_DIR}")
        endif()
    endif()

    foreach(vcpkg_candidate IN ITEMS "$ENV{VcpkgInstalledDir}" "$ENV{VCPKG_INSTALLED_DIR}" "$ENV{USERPROFILE}/vcpkg/installed")
        if(NOT WAVIATESCRIPT_ZLIB_BIN_DIR AND EXISTS "${vcpkg_candidate}/x64-windows/bin/zlib1.dll")
            set(WAVIATESCRIPT_ZLIB_BIN_DIR "${vcpkg_candidate}/x64-windows/bin")
        endif()
        if(NOT WAVIATESCRIPT_ZSTD_BIN_DIR AND EXISTS "${vcpkg_candidate}/x64-windows/bin/zstd.dll")
            set(WAVIATESCRIPT_ZSTD_BIN_DIR "${vcpkg_candidate}/x64-windows/bin")
        endif()
    endforeach()

    set(WAVIATESCRIPT_RUNTIME_DLLS)
    if(WAVIATESCRIPT_LLVM_BIN_DIR)
        foreach(runtime_name IN ITEMS LLVM-C.dll libclang.dll)
            if(EXISTS "${WAVIATESCRIPT_LLVM_BIN_DIR}/${runtime_name}")
                list(APPEND WAVIATESCRIPT_RUNTIME_DLLS "${WAVIATESCRIPT_LLVM_BIN_DIR}/${runtime_name}")
            endif()
        endforeach()
    endif()
    if(WAVIATESCRIPT_ZLIB_BIN_DIR AND EXISTS "${WAVIATESCRIPT_ZLIB_BIN_DIR}/zlib1.dll")
        list(APPEND WAVIATESCRIPT_RUNTIME_DLLS "${WAVIATESCRIPT_ZLIB_BIN_DIR}/zlib1.dll")
    endif()
    if(WAVIATESCRIPT_ZSTD_BIN_DIR AND EXISTS "${WAVIATESCRIPT_ZSTD_BIN_DIR}/zstd.dll")
        list(APPEND WAVIATESCRIPT_RUNTIME_DLLS "${WAVIATESCRIPT_ZSTD_BIN_DIR}/zstd.dll")
    endif()
    list(REMOVE_DUPLICATES WAVIATESCRIPT_RUNTIME_DLLS)

    if(WAVIATESCRIPT_RUNTIME_DLLS)
        message(STATUS "Will stage LLVM runtime DLLs: ${WAVIATESCRIPT_RUNTIME_DLLS}")
    else()
        message(WARNING "No LLVM runtime DLLs were discovered for staging. Standalone or VST3 binaries may fail to launch if LLVM-C.dll, zlib1.dll, or zstd.dll are not on PATH.")
    endif()

    foreach(plugin_runtime_target IN ITEMS WaviateScript_Standalone WaviateScript_VST3 WaviateScriptTests)
        if(TARGET "${plugin_runtime_target}" AND WAVIATESCRIPT_RUNTIME_DLLS)
            add_custom_command(
                TARGET "${plugin_runtime_target}" POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        ${WAVIATESCRIPT_RUNTIME_DLLS}
                        "$<TARGET_FILE_DIR:${plugin_runtime_target}>"
                COMMAND_EXPAND_LISTS
                COMMENT "Staging LLVM runtime DLLs for ${plugin_runtime_target}"
            )
        endif()
    endforeach()
endif()
