include(ECMMarkNonGuiExecutable)

add_executable(desktoptojson main.cpp desktoptojson.cpp
 ../lib/plugin/desktopfileparser.cpp)
if(NOT CMAKE_TOOLCHAIN_FILE)
    add_executable(KF5::desktoptojson ALIAS desktoptojson)
endif()

# Mark it as non-gui so we won't create an app bundle on Mac OS X
ecm_mark_nongui_executable(desktoptojson)

target_link_libraries(desktoptojson Qt5::Core)
target_compile_definitions(desktoptojson PRIVATE BUILDING_DESKTOPTOJSON_TOOL=1)

install(TARGETS desktoptojson EXPORT KF5CoreAddonsToolingTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
