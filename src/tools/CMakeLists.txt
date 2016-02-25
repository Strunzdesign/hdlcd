cmake_minimum_required(VERSION 2.6)

add_subdirectory(hdlc-dissector)
add_subdirectory(hdlc-hexdump-hdlc)
add_subdirectory(hdlc-hexdump-payload)
add_subdirectory(hdlc-hexinjector)
add_subdirectory(hdlc-pcapstreamer-hdlc)
add_subdirectory(hdlc-pcapstreamer-payload)
add_subdirectory(hdlc-suspender)
add_subdirectory(logclient)
if(NOT WIN32)
    # On MS Windows, this tool currently has problems with either posix threads or async IO on STDIN...
    add_subdirectory(hdlc-hexchanger)
endif()
