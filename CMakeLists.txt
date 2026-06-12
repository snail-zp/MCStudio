cmake_minimum_required(VERSION 3.21)

project(MCStudioIoTester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(ACS_ROOT "C:/Program Files (x86)/ACS Motion Control/SPiiPlus ADK Suite v3.13.01/ACSC/C_CPP")
set(ACS_RUNTIME_ROOT "C:/Program Files (x86)/ACS Motion Control/SPiiPlus Runtime Kit/Redist")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ACS_IMPORT_LIB "${ACS_ROOT}/ACSCL_x64.LIB")
    set(ACS_RUNTIME_DLL "${ACS_RUNTIME_ROOT}/x64/ACSCL_x64.dll")
    set(ACS_RUNTIME_EXTRA_DLLS
        "${ACS_RUNTIME_ROOT}/x64/RtosLib64.dll"
        "${ACS_RUNTIME_ROOT}/x64/mfc100.dll"
        "${ACS_RUNTIME_ROOT}/x64/msvcr100.dll"
    )
else()
    set(ACS_IMPORT_LIB "${ACS_ROOT}/ACSCL_x86.LIB")
    set(ACS_RUNTIME_DLL "${ACS_RUNTIME_ROOT}/x86/ACSCL_x86.dll")
    set(ACS_RUNTIME_EXTRA_DLLS
        "${ACS_RUNTIME_ROOT}/x86/RtosLib32.dll"
        "${ACS_RUNTIME_ROOT}/x86/mfc100.dll"
        "${ACS_RUNTIME_ROOT}/x86/msvcr100.dll"
    )
endif()

add_executable(MCStudioIoTester WIN32
    resources/mcstudio_app.rc
    src/app/main.cpp
    src/presentation/mainwindow.cpp
    src/presentation/mainwindow.h
    src/presentation/loggedpagewidget.cpp
    src/presentation/loggedpagewidget.h
    src/presentation/studiomainwindow.cpp
    src/presentation/studiomainwindow.h
    src/application/services/controllerservice.cpp
    src/application/services/controllerservice.h
    src/application/services/axisperformanceservice.cpp
    src/application/services/axisperformanceservice.h
    src/application/services/axisperformanceexecutionservice.cpp
    src/application/services/axisperformanceexecutionservice.h
    src/application/services/axisperformanceworkflowservice.cpp
    src/application/services/axisperformanceworkflowservice.h
    src/application/services/ioconfigurationservice.cpp
    src/application/services/ioconfigurationservice.h
    src/application/services/workstationcalibrationservice.cpp
    src/application/services/workstationcalibrationservice.h
    src/application/services/workstationcalibrationexecutionservice.cpp
    src/application/services/workstationcalibrationexecutionservice.h
    src/application/services/workstationcalibrationworkflowservice.cpp
    src/application/services/workstationcalibrationworkflowservice.h
    src/application/services/commoninterfaceservice.cpp
    src/application/services/commoninterfaceservice.h
    src/application/services/commoninterfaceworkflowservice.cpp
    src/application/services/commoninterfaceworkflowservice.h
    src/application/services/commoninterfaceexecutionservice.cpp
    src/application/services/commoninterfaceexecutionservice.h
    src/domain/interfaces/icontrolleradapter.h
    src/domain/models/controllertypes.h
    src/domain/models/axisperformancetest.h
    src/domain/models/axisperformanceresult.h
    src/domain/models/commoninterfacecommand.h
    src/domain/models/iopoint.h
    src/domain/models/workstationcalibration.h
    src/infrastructure/acs/acscontroller.cpp
    src/infrastructure/acs/acscontroller.h
    src/infrastructure/config/ioconfigrepository.cpp
    src/infrastructure/config/ioconfigrepository.h
    src/infrastructure/config/axisperformancerepository.cpp
    src/infrastructure/config/axisperformancerepository.h
    src/infrastructure/config/workstationcalibrationrepository.cpp
    src/infrastructure/config/workstationcalibrationrepository.h
    src/infrastructure/config/commoninterfacerepository.cpp
    src/infrastructure/config/commoninterfacerepository.h
    src/infrastructure/controllers/controlleradapterfactory.cpp
    src/infrastructure/controllers/controlleradapterfactory.h
    src/infrastructure/logging/filelogger.cpp
    src/infrastructure/logging/filelogger.h
    src/infrastructure/theme/thememanager.cpp
    src/infrastructure/theme/thememanager.h
    src/presentation/workstationcalibrationpage.cpp
    src/presentation/workstationcalibrationpage.h
    src/presentation/commoninterfacepage.cpp
    src/presentation/commoninterfacepage.h
    src/presentation/axisperformancepage.cpp
    src/presentation/axisperformancepage.h
    src/presentation/systemlogspage.cpp
    src/presentation/systemlogspage.h
)

target_include_directories(MCStudioIoTester PRIVATE
    ${ACS_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MCStudioIoTester PRIVATE
    Qt6::Widgets
    ${ACS_IMPORT_LIB}
)

add_custom_command(TARGET MCStudioIoTester POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ACS_RUNTIME_DLL}"
    "$<TARGET_FILE_DIR:MCStudioIoTester>"
)

foreach(acs_extra_dll IN LISTS ACS_RUNTIME_EXTRA_DLLS)
    add_custom_command(TARGET MCStudioIoTester POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${acs_extra_dll}"
        "$<TARGET_FILE_DIR:MCStudioIoTester>"
    )
endforeach()

add_custom_command(TARGET MCStudioIoTester POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:MCStudioIoTester>/Config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Config"
    "$<TARGET_FILE_DIR:MCStudioIoTester>/Config"
)
