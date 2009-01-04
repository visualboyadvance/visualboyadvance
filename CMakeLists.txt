cmake_minimum_required( VERSION 2.4.6 )
if( COMMAND cmake_policy )
    cmake_policy( SET CMP0003 NEW )
    cmake_policy( SET CMP0005 OLD )
endif( COMMAND cmake_policy )

# The project's name is VBA-M it uses C and C++ code
PROJECT(VBA-M C CXX)

# Both the SDL and the GTK+ frontends are built by default
# To disable building the SDL frontend add -DNO_SDL=1 on the commandline
# To disable building the GTK+ frontend add -DNO_GTK=1 on the commandline
# To disable building the debugger, add -DNO_DEBUGGER=1.
# The SDL frontend can't be built without debugger support
# To use ASM scalers, add -DUSE_ASM_SCALERS=1
# To use the ASM core, add -DUSE_ASM_CORE=1
# Set the version number with -DVERSION=X.X.X-uber

# Version number
IF( NOT VERSION )
    SET( VERSION "1.8.0-SVN" )
ENDIF( NOT VERSION )

# Look for some dependencies using builtin CMake scripts
FIND_PACKAGE ( ZLIB REQUIRED )
FIND_PACKAGE ( PNG REQUIRED )
FIND_PACKAGE ( OpenGL REQUIRED )
FIND_PACKAGE ( SDL REQUIRED )
FIND_PACKAGE ( PkgConfig REQUIRED )

# These dependencies require pkg-config to be found
PKG_CHECK_MODULES ( GTKMM gtkmm-2.4 )
PKG_CHECK_MODULES ( GLIBMM glibmm-2.4 )
PKG_CHECK_MODULES ( GIOMM giomm-2.4 )
PKG_CHECK_MODULES ( GLADEMM libglademm-2.4 )
PKG_CHECK_MODULES ( GTKGLMM gtkglextmm-x11-1.2 )

# Set the default install dir
IF( NOT DATA_INSTALL_DIR )
    SET( DATA_INSTALL_DIR "share/vbam" )
ENDIF( NOT DATA_INSTALL_DIR )

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

# Set the configuration file location
IF( NOT SYSCONFDIR )
    SET( SYSCONFDIR "/etc" )
ENDIF( NOT SYSCONFDIR )

# C defines
ADD_DEFINITIONS (-DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_ZLIB_H -DFINAL_VERSION -DSDL -DUSE_OPENGL -DSYSCONFDIR='"${SYSCONFDIR}"')

ADD_DEFINITIONS (-DVERSION='"${VERSION}"' -DPKGDATADIR='"${PKGDATADIR}"' -DPACKAGE='')
ADD_DEFINITIONS (-DNO_DEBUGGER)
ADD_DEFINITIONS (-DC_CORE)

# Compiler flags
SET( CMAKE_C_FLAGS "-O3 -Wall")
SET( CMAKE_CXX_FLAGS "-O3 -Wall")

# Source files definition
SET(SRC_MAIN
    src/Util.cpp
    src/common/Patch.cpp
    src/common/memgzio.c
)

SET(SRC_GBA
    src/gba/agbprint.cpp
    src/gba/bios.cpp
    src/gba/Cheats.cpp
    src/gba/EEprom.cpp
    src/gba/Flash.cpp
    src/gba/GBA.cpp
    src/gba/GBAGfx.cpp
    src/gba/GBA-thumb.cpp
    src/gba/GBA-arm.cpp
    src/gba/Globals.cpp
    src/gba/Mode0.cpp
    src/gba/Mode1.cpp
    src/gba/Mode2.cpp
    src/gba/Mode3.cpp
    src/gba/Mode4.cpp
    src/gba/Mode5.cpp
    src/gba/RTC.cpp
    src/gba/Sound.cpp
    src/gba/Sram.cpp
)

SET(SRC_GB
    src/gb/GB.cpp
    src/gb/gbCheats.cpp
    src/gb/gbDis.cpp
    src/gb/gbGfx.cpp
    src/gb/gbGlobals.cpp
    src/gb/gbMemory.cpp
    src/gb/gbPrinter.cpp
    src/gb/gbSGB.cpp
    src/gb/gbSound.cpp
)

SET(SRC_APU
    src/apu/Blip_Buffer.cpp
    src/apu/Effects_Buffer.cpp
    src/apu/Gb_Apu.cpp
    src/apu/Gb_Apu_State.cpp
    src/apu/Gb_Oscs.cpp
    src/apu/Multi_Buffer.cpp
)

SET(SRC_FEX_MINI
    src/common/fex_mini.cpp
)

SET(SRC_FILTERS
    src/filters/2xSaI.cpp
    src/filters/admame.cpp
    src/filters/bilinear.cpp
    src/filters/hq2x.cpp
    src/filters/interframe.cpp
    src/filters/pixel.cpp
    src/filters/scanline.cpp
)

SET(SRC_HQ
    src/filters/hq/c/hq_implementation.cpp
)

SET(SRC_GTK
    src/gtk/configfile.cpp
    src/gtk/main.cpp
    src/gtk/system.cpp
    src/gtk/windowcallbacks.cpp
    src/gtk/filters.cpp
    src/gtk/gameboyconfig.cpp
    src/gtk/gameboyadvanceconfig.cpp
    src/gtk/joypadconfig.cpp
    src/gtk/directoriesconfig.cpp
    src/gtk/displayconfig.cpp
    src/gtk/soundconfig.cpp
    src/gtk/screenarea.cpp
    src/gtk/screenarea-cairo.cpp
    src/gtk/screenarea-opengl.cpp
    src/gtk/tools.cpp
    src/gtk/window.cpp
    src/gtk/inputSDL.cpp
    src/common/SoundSDL.cpp
)

INCLUDE_DIRECTORIES(
	    ${ZLIB_INCLUDE_DIR}
	    ${PNG_INCLUDE_DIR}
	    ${SDL_INCLUDE_DIR}
)

INCLUDE_DIRECTORIES(
	${GLIBMM_INCLUDE_DIRS}
	${GIOMM_INCLUDE_DIRS}
	${GTKMM_INCLUDE_DIRS}
	${GTKGLMM_INCLUDE_DIRS}
	${GLADEMM_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
	${GLIBMM_LIBRARY_DIRS}
	${GIOMM_LIBRARY_DIRS}
	${GTKMM_LIBRARY_DIRS}
	${GTKGLMM_LIBRARY_DIRS}
	${GLADEMM_LIBRARY_DIRS}
)

ADD_LIBRARY (
	vbamcore
	${PROJECT_SRCS}
	${SRC_MAIN}
	${SRC_GBA}
	${SRC_GB}
	${SRC_APU}
	${SRC_FEX_MINI}
	${SRC_FILTERS}
)

ADD_EXECUTABLE (
	gvbam
	WIN32
	MACOSX_BUNDLE
	${SRC_GTK}
)

TARGET_LINK_LIBRARIES (
	gvbam
	vbamcore
	${ZLIB_LIBRARY}
	${PNG_LIBRARY}
	${GLADEMM_LIBRARIES}
	${SDL_LIBRARY}
	${GTKGLMM_LIBRARIES}
)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gvbam DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/gtk/gvbam.desktop DESTINATION share/applications)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/gtk/icons/ DESTINATION share/icons/hicolor PATTERN ".svn" EXCLUDE)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/gtk/ui DESTINATION ${DATA_INSTALL_DIR} PATTERN ".svn" EXCLUDE)
