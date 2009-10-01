cmake_minimum_required( VERSION 2.4.6 )
if( COMMAND cmake_policy )
	cmake_policy( SET CMP0003 NEW )
	cmake_policy( SET CMP0005 OLD )
endif( COMMAND cmake_policy )

# The project's name is VBA-M it uses C++ code
PROJECT(VBA-M C CXX)

# Set the version number with -DVERSION=X.X.X-uber

# Version number
IF( NOT VERSION )
	SET( VERSION "1.8.0-SVN" )
ENDIF( NOT VERSION )

# Look for some dependencies using builtin CMake scripts
FIND_PACKAGE ( ZLIB REQUIRED )
FIND_PACKAGE ( OpenGL REQUIRED )
FIND_PACKAGE ( SDL REQUIRED )
FIND_PACKAGE ( PkgConfig REQUIRED )

# These dependencies require pkg-config to be found
PKG_CHECK_MODULES ( GTKMM gtkmm-2.4 )
PKG_CHECK_MODULES ( GLIBMM glibmm-2.4 )
PKG_CHECK_MODULES ( GIOMM giomm-2.4 )
PKG_CHECK_MODULES ( GTKGLMM gtkglextmm-x11-1.2 )

# Set the default install dir
IF( NOT DATA_INSTALL_DIR )
	SET( DATA_INSTALL_DIR "share/vbam" )
ENDIF( NOT DATA_INSTALL_DIR )

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

# C defines
ADD_DEFINITIONS (-DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_ZLIB_H)
ADD_DEFINITIONS (-DVERSION='"${VERSION}"' -DPKGDATADIR='"${PKGDATADIR}"')
ADD_DEFINITIONS (-DGBA_LOGGING)
#ADD_DEFINITIONS (-DLINK_EMULATION)

# Compiler flags
SET( CMAKE_CXX_FLAGS "-O2 -Wall -g")

# Source files definition
SET(SRC_MAIN
	src/common/fex_mini.cpp
	src/common/Util.cpp
)

SET(SRC_GBA
	src/gba/Cartridge.cpp
	src/gba/CartridgeEEprom.cpp
	src/gba/CartridgeFlash.cpp
	src/gba/CartridgeRTC.cpp
	src/gba/CartridgeSram.cpp
	src/gba/CPU.cpp
	src/gba/CPUArm.cpp
	src/gba/CPUThumb.cpp
	src/gba/Display.cpp
	src/gba/GameInfos.cpp
	src/gba/GBA.cpp
	src/gba/Gfx.cpp
	src/gba/GfxHelpers.cpp
	src/gba/GfxMode0.cpp
	src/gba/GfxMode1.cpp
	src/gba/GfxMode2.cpp
	src/gba/GfxMode3.cpp
	src/gba/GfxMode4.cpp
	src/gba/GfxMode5.cpp
	src/gba/Globals.cpp
	src/gba/Link.cpp
	src/gba/MMU.cpp
	src/gba/Sound.cpp
)

SET(SRC_APU
	src/apu/Blip_Buffer.cpp
	src/apu/Effects_Buffer.cpp
	src/apu/Gb_Apu.cpp
	src/apu/Gb_Apu_State.cpp
	src/apu/Gb_Oscs.cpp
	src/apu/Multi_Buffer.cpp
)

SET(SRC_GTK
	src/gtk/ConfigFile.cpp
	src/gtk/Main.cpp
	src/gtk/System.cpp
	src/gtk/WindowCallbacks.cpp
	src/gtk/JoypadConfigDialog.cpp
	src/gtk/SettingsDialog.cpp
	src/gtk/ScreenArea.cpp
	src/gtk/ScreenAreaCairo.cpp
	src/gtk/ScreenAreaOpenGL.cpp
	src/gtk/Tools.cpp
	src/gtk/Window.cpp
	src/gtk/InputSDL.cpp
	src/gtk/GameXml.cpp
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
)

LINK_DIRECTORIES(
	${GLIBMM_LIBRARY_DIRS}
	${GIOMM_LIBRARY_DIRS}
	${GTKMM_LIBRARY_DIRS}
	${GTKGLMM_LIBRARY_DIRS}
)

ADD_LIBRARY (
	vbamcore
	${PROJECT_SRCS}
	${SRC_MAIN}
	${SRC_GBA}
	${SRC_APU}
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
	${SDL_LIBRARY}
	${GTKGLMM_LIBRARIES}
)

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gvbam DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/gvbam.desktop DESTINATION share/applications)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/GamesDB.txt DESTINATION ${DATA_INSTALL_DIR})
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/icons/ DESTINATION share/icons/hicolor PATTERN ".svn" EXCLUDE)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/ui DESTINATION ${DATA_INSTALL_DIR} PATTERN ".svn" EXCLUDE)
