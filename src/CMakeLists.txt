if(MSVC)
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost REQUIRED COMPONENTS system signals program_options date_time regex)
else()
    set(Boost_USE_STATIC_LIBS OFF) 
    set(Boost_USE_MULTITHREADED ON)  
    set(Boost_USE_STATIC_RUNTIME OFF) 
    find_package(Boost REQUIRED COMPONENTS system signals program_options)
endif()
include_directories(${Boost_INCLUDE_DIR})
include_directories("${PROJECT_SOURCE_DIR}/src/include")
include_directories(
    "HdlcdServer"
    "SerialPort"
    "SerialPort/HDLC"
)

find_package(Threads)

add_executable(${PROJECT_NAME}
    main-hdlcd.cpp
    HdlcdServer/HdlcdServerHandler.cpp
    HdlcdServer/HdlcdServerHandlerCollection.cpp
    HdlcdServer/LockGuard.cpp
    SerialPort/HDLC/AliveState.cpp
    SerialPort/HDLC/FCS16.cpp
    SerialPort/HDLC/HdlcFrame.cpp
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

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ADDITIONAL_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
