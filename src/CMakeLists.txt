set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals program_options)
include_directories(${Boost_INCLUDE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/src/include")

find_package(Threads)

add_executable(hdlcd
main-hdlcd.cpp
AccessProtocol/ClientHandler.cpp
AccessProtocol/ClientHandlerCollection.cpp
AccessProtocol/LockGuard.cpp
SerialPort/HDLC/AliveState.cpp
SerialPort/HDLC/FCS16.cpp
SerialPort/HDLC/Frame.cpp
SerialPort/HDLC/FrameGenerator.cpp
SerialPort/HDLC/FrameParser.cpp
SerialPort/HDLC/ProtocolState.cpp
SerialPort/SerialPortLock.cpp
SerialPort/SerialPortHandler.cpp
SerialPort/SerialPortHandlerCollection.cpp
)

if(WIN32)
    set(ADDITIONAL_LIBRARIES wsock32 ws2_32)
else()
    set(ADDITIONAL_LIBRARIES "")
endif()

target_link_libraries(hdlcd
${Boost_LIBRARIES}
${CMAKE_THREAD_LIBS_INIT}
${ADDITIONAL_LIBRARIES}
)

install(TARGETS hdlcd RUNTIME DESTINATION bin)
