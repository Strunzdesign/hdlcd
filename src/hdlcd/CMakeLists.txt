set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

find_package(Threads)

add_executable(hdlcd
main.cpp
ClientHandler.cpp
SerialPortHandler.cpp
SerialPortHandlerCollection.cpp
FCS16.cpp
HDLC/Frame.cpp
HDLC/FrameGenerator.cpp
HDLC/FrameParser.cpp
HDLC/ProtocolState.cpp
)

set_property(TARGET hdlcd PROPERTY CXX_STANDARD 11)

target_link_libraries(hdlcd
${Boost_LIBRARIES}
${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS hdlcd RUNTIME DESTINATION bin)

