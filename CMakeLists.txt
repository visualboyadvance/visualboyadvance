cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

# The project's name is VBA-M it uses C++ code
PROJECT(VBA-M C CXX)

# CMake include path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# Set the version number with -DVERSION=X.X.X-uber

# Version number
IF( NOT VERSION )
	SET( VERSION "1.8.0-SVN" )
ENDIF( NOT VERSION )

# Look for some dependencies using builtin CMake scripts
FIND_PACKAGE ( ZLIB REQUIRED )
FIND_PACKAGE ( SDL REQUIRED )
FIND_PACKAGE ( LibArchive REQUIRED )
FIND_PACKAGE ( PkgConfig REQUIRED )

# These dependencies require pkg-config to be found
PKG_CHECK_MODULES ( GTKMM gtkmm-3.0 )
PKG_CHECK_MODULES ( GLIBMM glibmm-2.4 )
PKG_CHECK_MODULES ( GIOMM giomm-2.4 )

# Set the default install dir
IF( NOT DATA_INSTALL_DIR )
	SET( DATA_INSTALL_DIR "share/vba-ttb" )
ENDIF( NOT DATA_INSTALL_DIR )

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

# C defines
ADD_DEFINITIONS (-DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_ZLIB_H)
ADD_DEFINITIONS (-DVERSION="${VERSION}" -DPKGDATADIR="${PKGDATADIR}")
ADD_DEFINITIONS (-DGBA_LOGGING)
#ADD_DEFINITIONS (-DLINK_EMULATION)

# Source files definition
SET(SRC_MAIN
	src/common/GameDB.cpp
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
	src/gtk/Main.cpp
	src/gtk/System.cpp
	src/gtk/WindowCallbacks.cpp
	src/gtk/JoypadConfigDialog.cpp
	src/gtk/SettingsDialog.cpp
	src/gtk/ScreenArea.cpp
	src/gtk/ScreenAreaCairo.cpp
	src/gtk/Tools.cpp
	src/gtk/Window.cpp
	src/gtk/InputSDL.cpp
	src/common/SoundSDL.cpp
)

INCLUDE_DIRECTORIES(
	${LibArchive_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
)

INCLUDE_DIRECTORIES(
	${GLIBMM_INCLUDE_DIRS}
	${GIOMM_INCLUDE_DIRS}
	${GTKMM_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
	${GLIBMM_LIBRARY_DIRS}
	${GIOMM_LIBRARY_DIRS}
	${GTKMM_LIBRARY_DIRS}
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
	${LibArchive_LIBRARY}
	${ZLIB_LIBRARY}
	${SDL_LIBRARY}
	${GTKMM_LIBRARIES}
)

# GSettings
include(GSettings)
add_schemas(gvbam ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/)

# Installation
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gvbam DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/gvbam.desktop DESTINATION share/applications)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/game-db.xml DESTINATION ${DATA_INSTALL_DIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/game-db.xsd DESTINATION ${DATA_INSTALL_DIR})
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/icons/ DESTINATION share/icons/hicolor)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/gtk/ui DESTINATION ${DATA_INSTALL_DIR})
