option(OPEN_KITTEN_BUILD_TESTS "Build native engine tests" ON)
option(OPEN_KITTEN_BUILD_UI "Build the Qt native browser shell" ON)
option(OPEN_KITTEN_BUILD_RUST "Build the Rust security component" ON)

add_library(kitten_engine_core STATIC
    src/engine/css.cpp
    src/engine/html_parser.cpp
    src/engine/layout.cpp
    src/engine/url_policy.cpp
)
target_include_directories(kitten_engine_core PUBLIC include)
if(MSVC)
    target_compile_options(kitten_engine_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(kitten_engine_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

if(OPEN_KITTEN_BUILD_UI)
    find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Network Widgets)
    qt_standard_project_setup()
    qt_add_executable(open-kitten-native
        src/app/main.cpp
        src/app/resources.qrc
        src/browser/browser_tab.cpp
        src/browser/browser_window.cpp
        src/browser/engine_view.cpp
        src/browser/modern_web_view.cpp
        src/browser/security_bridge.cpp
    )
    if(WIN32)
        target_sources(open-kitten-native PRIVATE src/app/open_kitten.rc)
    endif()
    target_include_directories(open-kitten-native PRIVATE include src)
    if(WIN32)
        set(WEBVIEW2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/webview2/build/native")
        if(NOT EXISTS "${WEBVIEW2_ROOT}/include/WebView2.h" OR
           NOT EXISTS "${WEBVIEW2_ROOT}/x64/WebView2Loader.dll.lib")
            message(FATAL_ERROR
                "WebView2 SDK files are missing. See third_party/webview2/README.md")
        endif()
        target_include_directories(open-kitten-native PRIVATE "${WEBVIEW2_ROOT}/include")
        target_link_libraries(open-kitten-native PRIVATE
            "${WEBVIEW2_ROOT}/x64/WebView2Loader.dll.lib")
        target_compile_definitions(open-kitten-native PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
        add_custom_command(TARGET open-kitten-native POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${WEBVIEW2_ROOT}/x64/WebView2Loader.dll"
                "$<TARGET_FILE_DIR:open-kitten-native>"
            VERBATIM)
    endif()
    target_link_libraries(open-kitten-native PRIVATE kitten_engine_core
        Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets)
    if(MSVC)
        target_compile_options(open-kitten-native PRIVATE /W4 /permissive- /EHsc)
    else()
        target_compile_options(open-kitten-native PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
    set_target_properties(open-kitten-native PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "OpenKitten"
    )

    if(OPEN_KITTEN_BUILD_RUST)
        find_program(CARGO_EXECUTABLE cargo REQUIRED)
        set(RUST_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/rust/security_core/Cargo.toml")
        set(RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/rust-target")
        if(WIN32)
            set(RUST_LIBRARY "${RUST_TARGET_DIR}/release/kitten_security_core.dll")
        elseif(APPLE)
            set(RUST_LIBRARY "${RUST_TARGET_DIR}/release/libkitten_security_core.dylib")
        else()
            set(RUST_LIBRARY "${RUST_TARGET_DIR}/release/libkitten_security_core.so")
        endif()
        add_custom_command(
            OUTPUT "${RUST_LIBRARY}"
            COMMAND "${CARGO_EXECUTABLE}" build --release --manifest-path "${RUST_MANIFEST}"
                    --target-dir "${RUST_TARGET_DIR}"
            DEPENDS "${RUST_MANIFEST}" rust/security_core/Cargo.lock rust/security_core/src/lib.rs
            COMMENT "Building Kitten Security Core (Rust)"
            VERBATIM
        )
        add_custom_target(kitten_security_core ALL DEPENDS "${RUST_LIBRARY}")
        add_dependencies(open-kitten-native kitten_security_core)
        add_custom_command(TARGET open-kitten-native POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${RUST_LIBRARY}" "$<TARGET_FILE_DIR:open-kitten-native>"
            VERBATIM)
    endif()

    install(TARGETS open-kitten-native RUNTIME DESTINATION . BUNDLE DESTINATION .)
    if(WIN32)
        install(FILES "${WEBVIEW2_ROOT}/x64/WebView2Loader.dll" DESTINATION .)
    endif()
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icon.png" DESTINATION assets)
    if(OPEN_KITTEN_BUILD_RUST)
        install(FILES "${RUST_LIBRARY}" DESTINATION .)
    endif()
    qt_generate_deploy_app_script(
        TARGET open-kitten-native
        OUTPUT_SCRIPT open_kitten_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${open_kitten_deploy_script}")
endif()

if(OPEN_KITTEN_BUILD_TESTS)
    enable_testing()
    add_executable(kitten_engine_tests tests/engine_tests.cpp)
    target_link_libraries(kitten_engine_tests PRIVATE kitten_engine_core)
    add_test(NAME kitten_engine_tests COMMAND kitten_engine_tests)
endif()
