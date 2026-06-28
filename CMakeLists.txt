cmake_minimum_required(VERSION 3.21)

include(cmake/LocalPinyinReleaseVersion.cmake)

project(LocalPinyinIME VERSION "${LOCALPINYIN_VERSION_NUMERIC}" LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LOCALPINYINIME_WITH_SQLITE "Build user learning with sqlite3 when available" OFF)

add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)

if (MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /c65001")
endif()

set(LOCALPINYINIME_DISPLAY_NAME "LocalPinyinIME - Offline Pinyin IME")
set(LOCALPINYINIME_COMPANY_NAME "LocalPinyinIME contributors")
set(LOCALPINYINIME_COPYRIGHT "Copyright (c) 2026 LocalPinyinIME contributors")

configure_file(
    cmake/LocalPinyinVersion.h.in
    "${CMAKE_BINARY_DIR}/generated/localpinyin_version.h"
    @ONLY
)

set(LOCALPINYINIME_DICTIONARY_RESOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/core_zh_pinyin.tsv"
)

function(localpinyin_add_version_resource target file_description original_filename file_type)
    set(LOCALPINYIN_VERSION_TARGET "${target}")
    set(LOCALPINYIN_VERSION_FILE_DESCRIPTION "${file_description}")
    set(LOCALPINYIN_VERSION_ORIGINAL_FILENAME "${original_filename}")
    set(LOCALPINYIN_VERSION_FILE_TYPE "${file_type}")
    configure_file(
        cmake/LocalPinyinVersion.rc.in
        "${CMAKE_BINARY_DIR}/generated/${target}_version.rc"
        @ONLY
    )
    target_sources(${target} PRIVATE "${CMAKE_BINARY_DIR}/generated/${target}_version.rc")
    target_include_directories(${target} PRIVATE "${CMAKE_BINARY_DIR}/generated")
endfunction()

function(localpinyin_copy_dictionary_resource target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>/dictionary"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${LOCALPINYINIME_DICTIONARY_RESOURCE}"
            "$<TARGET_FILE_DIR:${target}>/dictionary/core_zh_pinyin.tsv"
        VERBATIM
    )
endfunction()

set(IME_SOURCES
    src/ime/dllmain.cpp
    src/ime/exports.cpp
    src/ime/LocalPinyinIME.def
    src/ime/class_factory.cpp
    src/ime/globals.cpp
    src/ime/guids.cpp
    src/ime/registration.cpp
    src/ime/text_service.cpp
    src/ime/key_event_sink.cpp
    src/ime/composition_manager.cpp
    src/ime/candidate_ui.cpp
    src/ime/candidate_window.cpp
    src/ime/language_bar.cpp
    src/ime/input_mode.cpp
    src/engine/pinyin_engine.cpp
    src/engine/dictionary.cpp
    src/engine/candidate_ranker.cpp
    src/engine/user_learning.cpp
    src/engine/sqlite_database.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)

add_library(LocalPinyinIME SHARED ${IME_SOURCES})
target_include_directories(LocalPinyinIME PRIVATE src src/ime src/engine src/common)
target_link_libraries(LocalPinyinIME PRIVATE ole32 uuid advapi32 user32 gdi32 shlwapi)
localpinyin_add_version_resource(LocalPinyinIME
    "${LOCALPINYINIME_DISPLAY_NAME}"
    "LocalPinyinIME.dll"
    "VFT_DLL"
)
localpinyin_copy_dictionary_resource(LocalPinyinIME)

set_target_properties(LocalPinyinIME PROPERTIES
    OUTPUT_NAME "LocalPinyinIME"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_executable(LocalPinyinSettings WIN32
    src/settings/main.cpp
    src/settings/settings_window.cpp
    src/settings/settings_store.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)
target_include_directories(LocalPinyinSettings PRIVATE src src/common src/settings)
target_link_libraries(LocalPinyinSettings PRIVATE user32 shell32 shlwapi)
set_target_properties(LocalPinyinSettings PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
localpinyin_add_version_resource(LocalPinyinSettings
    "LocalPinyinIME Settings"
    "LocalPinyinSettings.exe"
    "VFT_APP"
)
localpinyin_copy_dictionary_resource(LocalPinyinSettings)

add_executable(LocalPinyinImeEnable
    src/enable/main.cpp
    src/ime/guids.cpp
)
target_include_directories(LocalPinyinImeEnable PRIVATE src src/ime)
target_link_libraries(LocalPinyinImeEnable PRIVATE ole32 uuid)
set_target_properties(LocalPinyinImeEnable PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
localpinyin_add_version_resource(LocalPinyinImeEnable
    "LocalPinyinIME Current User Enable Tool"
    "LocalPinyinImeEnable.exe"
    "VFT_APP"
)
localpinyin_copy_dictionary_resource(LocalPinyinImeEnable)

add_executable(LocalPinyinImeAudit
    src/audit/main.cpp
    src/ime/guids.cpp
)
target_include_directories(LocalPinyinImeAudit PRIVATE src src/ime)
target_link_libraries(LocalPinyinImeAudit PRIVATE ole32 uuid)
set_target_properties(LocalPinyinImeAudit PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
localpinyin_add_version_resource(LocalPinyinImeAudit
    "LocalPinyinIME Read-only Audit Tool"
    "LocalPinyinImeAudit.exe"
    "VFT_APP"
)
localpinyin_copy_dictionary_resource(LocalPinyinImeAudit)

add_executable(LocalPinyinImeSetup
    src/setup/main.cpp
    src/ime/guids.cpp
)
target_include_directories(LocalPinyinImeSetup PRIVATE src src/ime "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(LocalPinyinImeSetup PRIVATE ole32 uuid advapi32)
set_target_properties(LocalPinyinImeSetup PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
localpinyin_add_version_resource(LocalPinyinImeSetup
    "LocalPinyinIME Setup and Verification Tool"
    "LocalPinyinImeSetup.exe"
    "VFT_APP"
)
localpinyin_copy_dictionary_resource(LocalPinyinImeSetup)

add_executable(LocalPinyinImeDictionarySmoke
    src/tools/dictionary_smoke.cpp
    src/engine/pinyin_engine.cpp
    src/engine/dictionary.cpp
    src/engine/candidate_ranker.cpp
    src/engine/user_learning.cpp
    src/engine/sqlite_database.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)
target_include_directories(LocalPinyinImeDictionarySmoke PRIVATE src src/engine src/common)
target_link_libraries(LocalPinyinImeDictionarySmoke PRIVATE shlwapi)
set_target_properties(LocalPinyinImeDictionarySmoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
localpinyin_add_version_resource(LocalPinyinImeDictionarySmoke
    "LocalPinyinIME Dictionary Smoke Tool"
    "LocalPinyinImeDictionarySmoke.exe"
    "VFT_APP"
)
localpinyin_copy_dictionary_resource(LocalPinyinImeDictionarySmoke)

enable_testing()

set(TEST_SUPPORT
    src/engine/pinyin_engine.cpp
    src/engine/dictionary.cpp
    src/engine/candidate_ranker.cpp
    src/engine/user_learning.cpp
    src/engine/sqlite_database.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)

foreach(test_name IN ITEMS test_dictionary test_pinyin_engine test_candidate_ranker test_user_learning test_candidate_selection test_dictionary_resource_layout)
    add_executable(${test_name} tests/${test_name}.cpp ${TEST_SUPPORT})
    target_include_directories(${test_name} PRIVATE src src/engine src/common)
    target_link_libraries(${test_name} PRIVATE shlwapi)
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    localpinyin_copy_dictionary_resource(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

add_test(
    NAME dictionary_smoke_tool
    COMMAND LocalPinyinImeDictionarySmoke
        --dictionary "$<TARGET_FILE_DIR:LocalPinyinImeDictionarySmoke>/dictionary/core_zh_pinyin.tsv"
)

install(TARGETS LocalPinyinIME LocalPinyinSettings LocalPinyinImeEnable LocalPinyinImeAudit LocalPinyinImeSetup LocalPinyinImeDictionarySmoke
        RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION data)
install(FILES "${LOCALPINYINIME_DICTIONARY_RESOURCE}" DESTINATION bin/dictionary)
