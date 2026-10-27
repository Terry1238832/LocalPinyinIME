cmake_minimum_required(VERSION 3.21)
include(cmake/LocalPinyinReleaseVersion.cmake)
project(LocalPinyinIME VERSION "${LOCALPINYIN_VERSION_NUMERIC}" LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
include(CTest)
add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
if(MSVC)
    # Some Chinese MSVC installations emit UTF-8 /showIncludes while CMake's
    # probe decodes it as the system code page. Repair that detected prefix so
    # Ninja tracks header changes and never links stale object files.
    if(CMAKE_CL_SHOWINCLUDES_PREFIX MATCHES "^娉ㄦ剰")
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "注意: 包含文件: ")
    endif()
    string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    add_compile_options(/utf-8 /W3 $<$<CONFIG:Release>:/Gy> $<$<CONFIG:Release>:/Gw>)
    add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /c65001")
endif()
set(LOCALPINYINIME_PYTHON_EXECUTABLE "" CACHE FILEPATH "Python interpreter for resource generation and checks")
if(LOCALPINYINIME_PYTHON_EXECUTABLE)
    set(Python3_EXECUTABLE "${LOCALPINYINIME_PYTHON_EXECUTABLE}")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(LOCALPINYINIME_DISPLAY_NAME "本地拼音 · 离线智能输入法")
set(LOCALPINYINIME_COMPANY_NAME "LocalPinyinIME contributors")
set(LOCALPINYINIME_COPYRIGHT "Copyright (c) 2026 LocalPinyinIME contributors")
configure_file(cmake/LocalPinyinVersion.h.in "${CMAKE_BINARY_DIR}/generated/localpinyin_version.h" @ONLY)
# One token file styles the candidate window, the installer and the settings app.
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_design_tokens.py"
    --header "${CMAKE_BINARY_DIR}/generated/localpinyin_design_tokens.h" COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/theme/design_tokens.json")
include_directories(src "${CMAKE_BINARY_DIR}/generated")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
function(localpinyin_add_version_resource target file_description original_filename file_type)
    set(LOCALPINYIN_VERSION_TARGET "${target}")
    set(LOCALPINYIN_VERSION_FILE_DESCRIPTION "${file_description}")
    set(LOCALPINYIN_VERSION_ORIGINAL_FILENAME "${original_filename}")
    set(LOCALPINYIN_VERSION_FILE_TYPE "${file_type}")
    configure_file(cmake/LocalPinyinVersion.rc.in "${CMAKE_BINARY_DIR}/generated/${target}_version.rc" @ONLY)
    target_sources(${target} PRIVATE "${CMAKE_BINARY_DIR}/generated/${target}_version.rc")
endfunction()
set(localpinyin_dictionary_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
get_property(localpinyin_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(localpinyin_multi_config)
    string(APPEND localpinyin_dictionary_output_dir "/$<CONFIG>")
endif()
string(APPEND localpinyin_dictionary_output_dir "/dictionary")
set(localpinyin_dictionary_sources)
set(localpinyin_dictionary_outputs)
foreach(name IN ITEMS core_zh_pinyin.tsv local_core_zh_pinyin.tsv context_zh.tsv context_zh.lm word_zh.lm standard_zh_pinyin.tsv continuations_zh.tsv english_terms.tsv
    english_words.tsv)
    list(APPEND localpinyin_dictionary_sources "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/${name}")
    list(APPEND localpinyin_dictionary_outputs "${localpinyin_dictionary_output_dir}/${name}")
endforeach()
# Usage-scene word packs (常用场景), dictionary/domains/<id>.tsv.
set(localpinyin_domain_sources)
set(localpinyin_domain_outputs)
foreach(id IN ITEMS coding work math study daily games)
    list(APPEND localpinyin_domain_sources "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/domains/${id}.tsv")
    list(APPEND localpinyin_domain_outputs "${localpinyin_dictionary_output_dir}/domains/${id}.tsv")
endforeach()
# The small tool panel's symbols, emoji and kaomoji (tools/generate_panel_data.py).
set(localpinyin_panel_sources)
set(localpinyin_panel_outputs)
foreach(name IN ITEMS symbols emoji kaomoji)
    list(APPEND localpinyin_panel_sources "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/panel/${name}.tsv")
    list(APPEND localpinyin_panel_outputs "${localpinyin_dictionary_output_dir}/panel/${name}.tsv")
endforeach()
# A single dependency owns the shared files. Parallel DLL/bridge builds must
# never copy into the same dictionary directory concurrently. Explicit outputs
# also refresh data-only edits and restore deleted output files without relinking.
add_custom_command(OUTPUT ${localpinyin_dictionary_outputs} ${localpinyin_domain_outputs} ${localpinyin_panel_outputs}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${localpinyin_dictionary_output_dir}/domains"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${localpinyin_dictionary_output_dir}/panel"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${localpinyin_dictionary_sources}
        "${localpinyin_dictionary_output_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${localpinyin_domain_sources}
        "${localpinyin_dictionary_output_dir}/domains"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${localpinyin_panel_sources}
        "${localpinyin_dictionary_output_dir}/panel"
    DEPENDS ${localpinyin_dictionary_sources} ${localpinyin_domain_sources} ${localpinyin_panel_sources} VERBATIM)
add_custom_target(localpinyin_dictionary_resources ALL DEPENDS ${localpinyin_dictionary_outputs} ${localpinyin_domain_outputs}
    ${localpinyin_panel_outputs})
function(localpinyin_copy_dictionary_resource target)
    add_dependencies(${target} localpinyin_dictionary_resources)
endfunction()
# One copy of shared code is compiled for the product and all regression tests.
add_library(pinyin_common STATIC src/common/logging.cpp src/common/utf_utils.cpp src/common/win32_utils.cpp
    src/common/json_lite.cpp src/engine/input_options.cpp)
target_link_libraries(pinyin_common PUBLIC user32 gdi32 shell32 shlwapi ole32 advapi32)
add_library(pinyin_engine STATIC src/engine/pinyin_engine.cpp src/engine/dictionary.cpp
    src/engine/user_lexicon.cpp src/engine/candidate_ranker.cpp src/engine/user_learning.cpp src/engine/context_model.cpp src/engine/word_model.cpp src/engine/continuation_model.cpp
    src/engine/double_pinyin.cpp src/engine/fuzzy_pinyin.cpp src/engine/english_terms.cpp src/engine/typo_correction.cpp
    src/engine/domain_lexicon.cpp src/engine/custom_phrases.cpp src/engine/input_tools.cpp src/engine/english_speller.cpp)
target_link_libraries(pinyin_engine PUBLIC pinyin_common)
add_library(pinyin_ui STATIC src/ime/candidate_ui.cpp src/settings/settings_store.cpp src/settings/modern_renderer.cpp
    src/ime/candidate_styles/layout_core.cpp src/ime/candidate_styles/paint_core.cpp
    src/ime/candidate_styles/adaptive_flow.cpp src/ime/candidate_styles/cursor_rail.cpp
    src/ime/candidate_styles/modern_type.cpp src/ime/candidate_styles/floating_glass.cpp
    src/ime/candidate_styles/capsule.cpp src/ime/candidate_styles/column_list.cpp
    src/ime/candidate_styles/split_flap.cpp src/ime/candidate_styles/highlighter.cpp)
target_link_libraries(pinyin_ui PUBLIC pinyin_common d2d1 dwrite windowscodecs)
add_library(pinyin_settings STATIC src/settings/dictionary_catalog.cpp)
target_link_libraries(pinyin_settings PUBLIC pinyin_ui pinyin_engine comctl32 advapi32)
# The clipboard history shared by its recorder and the settings bridge.
add_library(pinyin_clipboard STATIC src/clipboard/clipboard_store.cpp src/clipboard/picture_clipboard.cpp)
target_link_libraries(pinyin_clipboard PUBLIC pinyin_common bcrypt windowscodecs ole32)
# 记录 (encrypted with Windows CNG and DPAPI) and the tool panel's lists.
add_library(pinyin_vault STATIC src/vault/vault.cpp)
target_link_libraries(pinyin_vault PUBLIC pinyin_common bcrypt crypt32)
add_library(pinyin_panel STATIC src/panel/panel_data.cpp)
target_link_libraries(pinyin_panel PUBLIC pinyin_common)
add_library(pinyin_service STATIC src/ime/text_service.cpp src/ime/composition_manager.cpp
    src/ime/candidate_ui_element.cpp src/ime/candidate_window.cpp src/ime/runtime_diagnostics.cpp
    src/ime/language_bar.cpp src/ime/input_mode.cpp src/ime/guids.cpp src/ime/globals.cpp
    src/ime/online_translation.cpp src/ime/tool_panel.cpp src/ime/tool_panel_window.cpp)
target_link_libraries(pinyin_service PUBLIC pinyin_engine pinyin_ui pinyin_clipboard pinyin_vault pinyin_panel oleaut32 uuid advapi32
    winhttp windowscodecs runtimeobject)
add_library(LocalPinyinIME SHARED src/ime/dllmain.cpp src/ime/exports.cpp src/ime/class_factory.cpp
    src/ime/registration.cpp src/ime/LocalPinyinIME.def src/ime/LocalPinyinIME.rc)
target_link_libraries(LocalPinyinIME PRIVATE pinyin_service)
localpinyin_add_version_resource(LocalPinyinIME "本地拼音" "LocalPinyinIME.dll" "VFT_DLL")
localpinyin_copy_dictionary_resource(LocalPinyinIME)
add_executable(LocalPinyinSettingsBridge src/tools/settings_bridge.cpp src/ime/guids.cpp)
target_link_libraries(LocalPinyinSettingsBridge PRIVATE pinyin_settings pinyin_clipboard pinyin_vault windowscodecs ole32 uuid advapi32)
localpinyin_add_version_resource(LocalPinyinSettingsBridge "本地拼音设置服务" "LocalPinyinSettingsBridge.exe" "VFT_APP")
localpinyin_copy_dictionary_resource(LocalPinyinSettingsBridge)
# Records copies for the tool panel while clipboard history is on; reads the
# words in pictures with Windows.Media.Ocr (C++/WinRT from the Windows SDK).
add_executable(LocalPinyinClipboard WIN32 src/clipboard/clipboard_agent.cpp)
target_link_libraries(LocalPinyinClipboard PRIVATE pinyin_clipboard pinyin_ui windowscodecs ole32 shlwapi windowsapp)
target_compile_options(LocalPinyinClipboard PRIVATE /bigobj /EHsc)
localpinyin_add_version_resource(LocalPinyinClipboard "本地拼音剪贴板记录" "LocalPinyinClipboard.exe" "VFT_APP")
add_executable(LocalPinyinImeSetup src/setup/main.cpp src/setup/verify_expected_dll.cpp src/ime/guids.cpp)
target_link_libraries(LocalPinyinImeSetup PRIVATE ole32 uuid advapi32 shell32)
localpinyin_add_version_resource(LocalPinyinImeSetup "本地拼音安装工具" "LocalPinyinImeSetup.exe" "VFT_APP")
# The installer window. Release builds embed the silent Inno Setup package;
# without one the window still builds for --preview and --capture.
set(LOCALPINYIN_INNER_INSTALLER "" CACHE FILEPATH "Inno Setup package embedded in the installer window")
set(LOCALPINYIN_PAYLOAD_BYTES "" CACHE STRING "Total size of the installed files, for the progress bar")
add_executable(LocalPinyinInstaller WIN32 src/installer/installer_shell.cpp src/installer/installer.rc
    src/installer/installer.manifest)
target_link_libraries(LocalPinyinInstaller PRIVATE pinyin_ui dwmapi gdiplus shell32 ole32 oleaut32 uuid advapi32 user32)
# The execution level comes from installer.manifest (highestAvailable), not
# the linker's default asInvoker.
target_link_options(LocalPinyinInstaller PRIVATE /MANIFESTUAC:NO)
localpinyin_add_version_resource(LocalPinyinInstaller "本地拼音安装程序" "LocalPinyinInstaller.exe" "VFT_APP")
if(LOCALPINYIN_INNER_INSTALLER)
    file(TO_CMAKE_PATH "${LOCALPINYIN_INNER_INSTALLER}" localpinyin_inner_installer)
    file(WRITE "${CMAKE_BINARY_DIR}/generated/LocalPinyinInstaller_payload.rc"
        "201 RCDATA \"${localpinyin_inner_installer}\"
")
    target_sources(LocalPinyinInstaller PRIVATE "${CMAKE_BINARY_DIR}/generated/LocalPinyinInstaller_payload.rc")
    set_source_files_properties("${CMAKE_BINARY_DIR}/generated/LocalPinyinInstaller_payload.rc"
        PROPERTIES OBJECT_DEPENDS "${LOCALPINYIN_INNER_INSTALLER}")
endif()
if(LOCALPINYIN_PAYLOAD_BYTES)
    target_compile_definitions(LocalPinyinInstaller PRIVATE LOCALPINYIN_PAYLOAD_BYTES=${LOCALPINYIN_PAYLOAD_BYTES}ULL)
endif()
if(BUILD_TESTING)
    function(pinyin_test name)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE ${ARGN})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()
    foreach(name IN ITEMS test_dictionary test_pinyin_engine test_candidate_ranker test_user_learning
        test_candidate_selection test_dictionary_resource_layout test_dictionary_layers test_user_lexicon test_sentence_input_baseline test_contextual_engine test_continuation_engine test_personal_phrases
        test_context_language_model test_word_model test_learning_promotion test_input_features)
        pinyin_test(${name} pinyin_engine)
    endforeach()
    foreach(name IN ITEMS test_candidate_ui test_cross_app_compatibility test_settings_store)
        pinyin_test(${name} pinyin_ui)
    endforeach()
    pinyin_test(test_dictionary_catalog pinyin_settings)
    localpinyin_copy_dictionary_resource(test_dictionary_catalog)
    foreach(name IN ITEMS test_candidate_ui_element test_runtime_diagnostics test_preserved_key_registration)
        pinyin_test(${name} pinyin_service)
    endforeach()
    pinyin_test(test_language_bar pinyin_service)
    target_sources(test_language_bar PRIVATE src/ime/LocalPinyinIME.rc)
    foreach(name IN ITEMS test_registration_transaction test_tsf_category_registration_logic)
        pinyin_test(${name} pinyin_service)
    endforeach()
    pinyin_test(test_logging pinyin_common)
    # 本地英文's speller is platform independent and reads the source word list.
    add_executable(test_english_speller tests/test_english_speller.cpp)
    target_link_libraries(test_english_speller PRIVATE pinyin_engine)
    add_test(NAME test_english_speller COMMAND test_english_speller "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary")
    pinyin_test(test_online_translation pinyin_service)
    pinyin_test(test_clipboard_store pinyin_clipboard)
    pinyin_test(test_json_lite pinyin_common)
    pinyin_test(test_vault pinyin_vault)
    pinyin_test(test_tool_panel pinyin_service)
    localpinyin_copy_dictionary_resource(test_tool_panel)
    pinyin_test(test_chinese_punctuation pinyin_common)
    pinyin_test(test_verify_expected_dll pinyin_common)
    target_sources(test_verify_expected_dll PRIVATE src/setup/verify_expected_dll.cpp)
    pinyin_test(test_profile_visibility_verification pinyin_common)
    add_executable(test_tsf_integration tests/test_tsf_integration.cpp)
    target_link_libraries(test_tsf_integration PRIVATE pinyin_service ole32 oleaut32 uuid user32)
    add_dependencies(test_tsf_integration LocalPinyinIME)
    add_test(NAME test_tsf_integration COMMAND test_tsf_integration "$<TARGET_FILE:LocalPinyinIME>")
    add_executable(LocalPinyinImeDictionarySmoke src/tools/dictionary_smoke.cpp)
    target_link_libraries(LocalPinyinImeDictionarySmoke PRIVATE pinyin_engine)
    localpinyin_copy_dictionary_resource(LocalPinyinImeDictionarySmoke)
    add_executable(LocalPinyinImeEval src/tools/ime_eval.cpp)
    target_link_libraries(LocalPinyinImeEval PRIVATE pinyin_engine)
    localpinyin_copy_dictionary_resource(LocalPinyinImeEval)
    add_executable(LocalPinyinEnglishEval src/tools/english_eval.cpp)
    target_link_libraries(LocalPinyinEnglishEval PRIVATE pinyin_engine)
    add_executable(LocalPinyinCandidateCapture src/tools/candidate_capture.cpp)
    target_link_libraries(LocalPinyinCandidateCapture PRIVATE pinyin_service gdiplus)
    localpinyin_copy_dictionary_resource(LocalPinyinCandidateCapture)
    add_test(NAME test_settings_bridge COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_settings_bridge.py"
        --bridge "$<TARGET_FILE:LocalPinyinSettingsBridge>" --temp-root "${CMAKE_BINARY_DIR}/test-tmp")
    add_test(NAME dictionary_smoke_tool COMMAND LocalPinyinImeDictionarySmoke --resource-dir "$<TARGET_FILE_DIR:LocalPinyinIME>")
    add_test(NAME test_input_mode_toggle_paths COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_input_mode_toggle_paths.py"
        --text-service "${CMAKE_CURRENT_SOURCE_DIR}/src/ime/text_service.cpp"
        --input-mode "${CMAKE_CURRENT_SOURCE_DIR}/src/ime/input_mode.h")
    add_test(NAME test_local_core_lexicon COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_local_core_lexicon.py"
        --path "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/local_core_zh_pinyin.tsv"
        --metadata "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/local_core_zh_pinyin.metadata.json"
        --min-entries 8000 --max-entries 255390)
    add_test(NAME test_standard_dictionary COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_standard_dictionary.py"
        --path "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/standard_zh_pinyin.tsv"
        --metadata "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary/standard_zh_pinyin.metadata.json"
        --notices "${CMAKE_CURRENT_SOURCE_DIR}/resources/THIRD_PARTY_NOTICES.txt")
    add_test(NAME test_context_model_data COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_context_model.py"
        --dictionary-dir "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary"
        --cases-dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    add_test(NAME test_design_tokens_css COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_design_tokens.py"
        --css "${CMAKE_CURRENT_SOURCE_DIR}/desktop/src/tokens.css" --check)
    add_test(NAME test_continuation_model_data COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_continuations.py"
        --dictionary-dir "${CMAKE_CURRENT_SOURCE_DIR}/resources/dictionary")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-tmp" "${CMAKE_BINARY_DIR}/test-data")
    get_property(pinyin_tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${pinyin_tests} PROPERTIES
        ENVIRONMENT "TEMP=${CMAKE_BINARY_DIR}/test-tmp;TMP=${CMAKE_BINARY_DIR}/test-tmp;LOCALPINYINIME_LOCALAPPDATA=${CMAKE_BINARY_DIR}/test-data")
endif()
install(TARGETS LocalPinyinIME LocalPinyinSettingsBridge LocalPinyinClipboard LocalPinyinImeSetup RUNTIME DESTINATION .)
install(FILES resources/dictionary/core_zh_pinyin.tsv resources/dictionary/local_core_zh_pinyin.tsv resources/dictionary/context_zh.tsv resources/dictionary/context_zh.lm resources/dictionary/word_zh.lm resources/dictionary/standard_zh_pinyin.tsv resources/dictionary/continuations_zh.tsv resources/dictionary/english_terms.tsv
    resources/dictionary/english_words.tsv DESTINATION dictionary)
install(FILES ${localpinyin_domain_sources} DESTINATION dictionary/domains)
install(FILES ${localpinyin_panel_sources} DESTINATION dictionary/panel)
install(FILES resources/THIRD_PARTY_NOTICES.txt DESTINATION .)
install(FILES LICENSE DESTINATION .)
