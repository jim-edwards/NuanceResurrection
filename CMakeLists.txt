cmake_minimum_required(VERSION 3.5)
project(nuance)

###############################################################################
## Source files for main emulator

set(sources
    audio.cpp
    bdma_type12.cpp
    bdma_type5.cpp
    bdma_type8.cpp
    bdma_type9.cpp
    bios.cpp
    byteswap.cpp
    coff.cpp
    comm.cpp
    DecodeALU.cpp
    DecodeECU.cpp
    DecodeMEM.cpp
    DecodeMUL.cpp
    DecodeRCU.cpp
    dma.cpp
    EmulatorShell.ui
    EmitALU.cpp
    EmitECU.cpp
    EmitMEM.cpp
    EmitMisc.cpp
    EmitMUL.cpp
    EmitRCU.cpp
    ExecuteALU.cpp
    ExecuteECU.cpp
    ExecuteMEM.cpp
    ExecuteMisc.cpp
    ExecuteMUL.cpp
    ExecuteRCU.cpp
    external/gcc_compat/strcpy_s.cpp
    external/gcc_compat/memmove_s.cpp
    file.cpp
    FlashEEPROM.cpp
    InstructionCache.cpp
    joystick.cpp
    media.cpp
    memory.cpp
    MemoryManager.cpp
    mpe.cpp
    mpe_alloc.cpp
    MPEControlRegisters.cpp
    NuonMemoryManager.cpp
    nuonrom.cpp
    OverlayManager.cpp
    PageMap.cpp
    PatchManager.cpp
    PerformanceTimer.cpp
    PresentationEngine.cpp
    PrintALU.cpp
    PrintECU.cpp
    PrintMEM.cpp
    PrintMisc.cpp
    PrintMUL.cpp
    PrintRCU.cpp
    PropagateConstants_ALU.cpp
    PropagateConstants_ECU.cpp
    PropagateConstants_MEM.cpp
    PropagateConstants_MUL.cpp
    PropagateConstants_RCU.cpp
    PropagateConstantsHandlers.cpp
    ShaderProgram.cpp
    SuperBlock.cpp
    SuperBlockConstants.cpp
    Syscall.cpp
    TestConditionCode.cpp
    Utility.cpp
    timer.cpp
    NativeCodeCache.cpp
    NuonEnvironment.cpp
    video.cpp
    GLWindow.cpp
    NuanceMain.cpp
)

set_source_files_properties(EmitECU.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(EmitMEM.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(EmitMisc.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(EmitMUL.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(NativeCodeCache.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(PatchManager.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")
set_source_files_properties(SuperBlock.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")

set(data
    bitmap1.bmp
    led_off.bmp
    led_on.bmp
    Nuance.ico
    Nuance.rc
    NuanceLogo.bmp
)

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE
add_executable(nuance ${sources} ${data})

# just for example add some compiler flags
target_compile_options(nuance PUBLIC -std=c++17 -Wall -msse2)
# WIN32;_DEBUG;_WINDOWS;QT_DLL;QT_THREAD_SUPPORT

# this lets me include files relative to the root source directory with a <> pair
target_include_directories(nuance PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gcc_compat SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/gcc_compat)

# this copies all resource files in the build directory
# we need this, because we want to work with paths relative to the executable
file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

# Find Qt 6
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
target_link_libraries(nuance Qt6::Core Qt6::Widgets)

# AUto-build uic files
set(CMAKE_AUTOUIC ON)

# GLew
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
link_libraries(${GLEW_LIBRARIES})


# external\glew-2.2.0\lib\Release\Win32\glew32.lib user32.lib winmm.lib external\fmod-3.75\api\lib\fmodvc.lib opengl32.lib

###############################################################################
## packaging ##################################################################
###############################################################################

# all install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS nuance DESTINATION example_destination)

# this is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell CMake that we want it
# in the package
#install(DIRECTORY resources DESTINATION example_destination)

# now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here
set(CPACK_PACKAGE_NAME "Nuance")
set(CPACK_PACKAGE_VERSION "1.0.0")

# we don't want to split our program up into several things
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
