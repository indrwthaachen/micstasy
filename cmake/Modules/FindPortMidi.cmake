# Find PortMidi
# PortMidi_FOUND - system has PortMidi
# PortMidi_INCLUDE_DIRS - the PortMidi include directories
# PortMidi_LIBRARIES - link these to use PortMidi
# PortMidi_VERSION - detected version of PortMidi


include(LibFindMacros)

find_path(PortMidi_INCLUDE_DIR NAMES portmidi.h)
find_library(PortMidi_LIBRARY NAMES portmidi)


set(PortMidi_PROCESS_INCLUDES PortMidi_INCLUDE_DIR)
set(PortMidi_PROCESS_LIBS PortMidi_LIBRARY)


libfind_process(PortMidi)

