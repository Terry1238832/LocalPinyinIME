cmake_minimum_required(VERSION 3.21)

include(cmake/LocalPinyinReleaseVersion.cmake)

project(LocalPinyinIME VERSION "${LOCALPINYIN_VERSION_NUMERIC}" LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LOCALPINYINIME_WITH_SQLITE "Build user learning with sqlite3 when available" OFF)
set(LOCALPINYINIME_PYTHON_EXECUTABLE "" CACHE FILEPATH "Python 3 interpreter used by CTest Python checks")

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

set(LOCALPINYINIME_CORE_DICTIONARY_RESOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/core_zh_pinyin.tsv"
)
set(LOCALPINYINIME_LOCAL_CORE_DICTIONARY_RESOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/local_core_zh_pinyin.tsv"
)
set(LOCALPINYINIME_DICTIONARY_RESOURCES
    "${LOCALPINYINIME_CORE_DICTIONARY_RESOURCE}"
    "${LOCALPINYINIME_LOCAL_CORE_DICTIONARY_RESOURCE}"
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
            "${LOCALPINYINIME_CORE_DICTIONARY_RESOURCE}"
            "$<TARGET_FILE_DIR:${target}>/dictionary/core_zh_pinyin.tsv"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${LOCALPINYINIME_LOCAL_CORE_DICTIONARY_RESOURCE}"
            "$<TARGET_FILE_DIR:${target}>/dictionary/local_core_zh_pinyin.tsv"
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
    src/settings/settings_store.cpp
    src/engine/pinyin_engine.cpp
    src/engine/dictionary.cpp
    src/engine/user_lexicon.cpp
    src/engine/candidate_ranker.cpp
    src/engine/user_learning.cpp
    src/engine/sqlite_database.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)

add_library(LocalPinyinIME SHARED ${IME_SOURCES})
target_include_directories(LocalPinyinIME PRIVATE src src/ime src/engine src/common src/settings)
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
    src/engine/user_lexicon.cpp
    src/ime/candidate_ui.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)
target_include_directories(LocalPinyinSettings PRIVATE src src/engine src/common src/settings)
target_link_libraries(LocalPinyinSettings PRIVATE user32 gdi32 shell32 shlwapi comctl32 advapi32)
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
    src/engine/user_lexicon.cpp
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
    src/engine/user_lexicon.cpp
    src/engine/candidate_ranker.cpp
    src/engine/user_learning.cpp
    src/engine/sqlite_database.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)

foreach(test_name IN ITEMS test_dictionary test_pinyin_engine test_candidate_ranker test_user_learning test_candidate_selection test_dictionary_resource_layout test_dictionary_layers test_user_lexicon test_sentence_input_baseline)
    add_executable(${test_name} tests/${test_name}.cpp ${TEST_SUPPORT})
    target_include_directories(${test_name} PRIVATE src src/engine src/common)
    target_link_libraries(${test_name} PRIVATE shlwapi)
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    localpinyin_copy_dictionary_resource(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

add_executable(test_candidate_ui tests/test_candidate_ui.cpp src/ime/candidate_ui.cpp)
target_include_directories(test_candidate_ui PRIVATE src src/ime)
target_link_libraries(test_candidate_ui PRIVATE user32)
set_target_properties(test_candidate_ui PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME test_candidate_ui COMMAND test_candidate_ui)

add_executable(test_settings_store
    tests/test_settings_store.cpp
    src/settings/settings_store.cpp
    src/ime/candidate_ui.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)
target_include_directories(test_settings_store PRIVATE src src/settings src/ime src/common)
target_link_libraries(test_settings_store PRIVATE user32 shell32 shlwapi)
set_target_properties(test_settings_store PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME test_settings_store COMMAND test_settings_store)

add_executable(test_settings_window_layout
    tests/test_settings_window_layout.cpp
    src/settings/settings_window.cpp
    src/settings/settings_store.cpp
    src/engine/user_lexicon.cpp
    src/ime/candidate_ui.cpp
    src/common/logging.cpp
    src/common/utf_utils.cpp
    src/common/win32_utils.cpp
)
target_include_directories(test_settings_window_layout PRIVATE src src/settings src/engine src/ime src/common "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(test_settings_window_layout PRIVATE user32 gdi32 shell32 shlwapi comctl32 advapi32)
set_target_properties(test_settings_window_layout PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_test(NAME test_settings_window_layout COMMAND test_settings_window_layout)

add_test(
    NAME dictionary_smoke_tool
    COMMAND LocalPinyinImeDictionarySmoke
        --resource-dir "$<TARGET_FILE_DIR:LocalPinyinImeDictionarySmoke>"
)

install(TARGETS LocalPinyinIME LocalPinyinSettings LocalPinyinImeEnable LocalPinyinImeAudit LocalPinyinImeSetup LocalPinyinImeDictionarySmoke
        RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION data)
install(FILES ${LOCALPINYINIME_DICTIONARY_RESOURCES} DESTINATION bin/dictionary)

if (LOCALPINYINIME_PYTHON_EXECUTABLE)
    set(Python3_EXECUTABLE "${LOCALPINYINIME_PYTHON_EXECUTABLE}" CACHE FILEPATH "Python 3 interpreter" FORCE)
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if (NOT Python3_Interpreter_FOUND AND DEFINED ENV{USERPROFILE})
    set(_localpinyin_codex_python "$ENV{USERPROFILE}/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe")
    if (EXISTS "${_localpinyin_codex_python}")
        set(Python3_EXECUTABLE "${_localpinyin_codex_python}" CACHE FILEPATH "Python 3 interpreter" FORCE)
        find_package(Python3 COMPONENTS Interpreter QUIET)
    endif()
endif()
if (NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Python3 interpreter is required so CTest registers test_manage_user_lexicon and test_local_core_lexicon. Set LOCALPINYINIME_PYTHON_EXECUTABLE to a Python 3 executable if auto-detection fails.")
endif()

add_test(
    NAME test_manage_user_lexicon
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_manage_user_lexicon.py"
        --script "${CMAKE_CURRENT_SOURCE_DIR}/tools/manage_user_lexicon.py"
)
add_test(
    NAME test_local_core_lexicon
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_local_core_lexicon.py"
        --path "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/local_core_zh_pinyin.tsv"
        --metadata "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/local_core_zh_pinyin.metadata.json"
        --min-entries 8000
        --max-entries 12000
)
add_test(
    NAME test_installer_static
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_installer_static.py"
        --inno "${CMAKE_CURRENT_SOURCE_DIR}/installer/LocalPinyinIME.iss"
)
add_test(
    NAME test_lexicon_source_pipeline
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_lexicon_source_pipeline.py"
        --manifest "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/sources/lexicon_sources.json"
        --script "${CMAKE_CURRENT_SOURCE_DIR}/tools/import_lexicon_source.py"
)
add_test(
    NAME test_first_party_lexicon_audit
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_first_party_lexicon_audit.py"
        --script "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_first_party_lexicon.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}"
)
