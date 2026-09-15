find_package(Threads REQUIRED)
add_library(test_audio_device OBJECT fakeportaudio.cpp)
target_link_libraries(test_audio_device PUBLIC PkgConfig::PORTAUDIO Threads::Threads)
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE player_core test_audio_device Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME core COMMAND test_core)
set_tests_properties(core PROPERTIES TIMEOUT 45 ENVIRONMENT "QT_FORCE_STDERR_LOGGING=1")
add_executable(test_qml test_qml.cpp ../resources.qrc)
target_link_libraries(test_qml PRIVATE player_app test_audio_device Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::QuickControls2)
add_test(NAME qml COMMAND test_qml)
set_tests_properties(qml PROPERTIES TIMEOUT 45
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_FORCE_STDERR_LOGGING=1")
