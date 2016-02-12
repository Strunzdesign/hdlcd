set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

find_package(Threads)

add_executable(hdlc-hexdump-hdlc
main.cpp
)

set_property(TARGET hdlc-hexdump-hdlc PROPERTY CXX_STANDARD 11)

target_link_libraries(hdlc-hexdump-hdlc
${Boost_LIBRARIES}
${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS hdlc-hexdump-hdlc RUNTIME DESTINATION bin)

