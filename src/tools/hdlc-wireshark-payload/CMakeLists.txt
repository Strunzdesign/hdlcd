set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

find_package(Threads)

add_executable(hdlc-wireshark-payload
main.cpp
)

set_property(TARGET hdlc-wireshark-payload PROPERTY CXX_STANDARD 11)

target_link_libraries(hdlc-wireshark-payload
${Boost_LIBRARIES}
${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS hdlc-wireshark-payload RUNTIME DESTINATION bin)

