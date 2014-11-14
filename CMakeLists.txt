cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

# The project's name is VBA it uses C++ code
PROJECT(VBA C CXX)

# CMake include path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

# GCC doesn't enable C99 by default
IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
ENDIF()

# Set the version number with -DVERSION=X.X.X-uber

# Version number
IF( NOT VERSION )
	SET( VERSION "2.0.0-git" )
ENDIF( NOT VERSION )

# Look for some dependencies using builtin CMake scripts
FIND_PACKAGE ( ZLIB REQUIRED )
FIND_PACKAGE ( PNG REQUIRED )
FIND_PACKAGE ( SDL2 REQUIRED )
FIND_PACKAGE ( SDL2TTF REQUIRED )
FIND_PACKAGE ( LibArchive REQUIRED )
FIND_PACKAGE ( Glib REQUIRED )

# Set the default install dir
IF( NOT DATA_INSTALL_DIR )
	SET( DATA_INSTALL_DIR "share/vba" )
ENDIF( NOT DATA_INSTALL_DIR )

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

# C defines
ADD_DEFINITIONS (-DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_ZLIB_H)
ADD_DEFINITIONS (-DVERSION="${VERSION}" -DPKGDATADIR="${PKGDATADIR}")
ADD_DEFINITIONS (-DGBA_LOGGING)
#ADD_DEFINITIONS (-DLINK_EMULATION)

# Source files definition
SET(SRC_MAIN
	src/common/DisplayDriver.c
	src/common/GameDB.c
	src/common/GameInfos.c
	src/common/InputDriver.c
	src/common/Loader.c
	src/common/RingBuffer.c
	src/common/Settings.c
	src/common/SoundDriver.c
	src/common/Util.c
)

SET(SRC_GBA
	src/gba/Cartridge.c
	src/gba/CartridgeEEprom.c
	src/gba/CartridgeFlash.c
	src/gba/CartridgeRTC.c
	src/gba/CartridgeSram.c
	src/gba/CPU.cpp
	src/gba/CPUArm.cpp
	src/gba/CPUThumb.cpp
	src/gba/Display.c
	src/gba/GBA.cpp
	src/gba/Gfx.c
	src/gba/GfxHelpers.c
	src/gba/GfxMode0.c
	src/gba/GfxMode1.c
	src/gba/GfxMode2.c
	src/gba/GfxMode3.c
	src/gba/GfxMode4.c
	src/gba/GfxMode5.c
	src/gba/Globals.c
	src/gba/Link.cpp
	src/gba/MMU.cpp
	src/gba/Savestate.cpp
	src/gba/Sound.cpp
)

SET(SRC_APU
	src/apu/Blip_Buffer.cpp
	src/apu/Gb_Apu.cpp
	src/apu/Gb_Apu_State.cpp
	src/apu/Gb_Oscs.cpp
	src/apu/Multi_Buffer.cpp
)

SET(SRC_SDL
	src/sdl/DisplaySDL.c
	src/sdl/ErrorScreen.c
	src/sdl/GameScreen.cpp
	src/sdl/GUI.c
	src/sdl/InputSDL.c
	src/sdl/OSD.c
	src/sdl/PauseScreen.c
	src/sdl/VBA.cpp
	src/sdl/Timer.c
	src/sdl/SoundSDL.c
)

INCLUDE_DIRECTORIES(
	${LibArchive_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
	${SDL2TTF_INCLUDE_DIR}
	${Glib_INCLUDE_DIRS}
)

ADD_LIBRARY (
	vbacore
	${PROJECT_SRCS}
	${SRC_MAIN}
	${SRC_GBA}
	${SRC_APU}
)

ADD_EXECUTABLE (
	vba
	${SRC_SDL}
)

TARGET_LINK_LIBRARIES (
	vba
	vbacore
	${LibArchive_LIBRARIES}
	${PNG_LIBRARIES}
	${ZLIB_LIBRARIES}
	${SDL2_LIBRARY}
	${SDL2TTF_LIBRARY}
	${Glib_LIBRARIES}
)

# Installation
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vba DESTINATION bin)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/db/game-db.xml DESTINATION ${DATA_INSTALL_DIR}/db)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/db/game-db.xsd DESTINATION ${DATA_INSTALL_DIR}/db)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/fonts/DroidSans-Bold.ttf DESTINATION ${DATA_INSTALL_DIR}/fonts)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/fonts/DroidSans-Bold.txt DESTINATION ${DATA_INSTALL_DIR}/fonts)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/icon/vba.png DESTINATION ${DATA_INSTALL_DIR}/icon)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/flags DESTINATION ${DATA_INSTALL_DIR})
