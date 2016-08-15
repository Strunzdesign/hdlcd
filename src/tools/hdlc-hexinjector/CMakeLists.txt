set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

find_package(Threads)

add_executable(hdlc-hexinjector
main.cpp
)

if(WIN32)
    set(ADDITIONAL_LIBRARIES wsock32 ws2_32)
else()
    set(ADDITIONAL_LIBRARIES "")
endif()

target_link_libraries(hdlc-hexinjector
${Boost_LIBRARIES}
${CMAKE_THREAD_LIBS_INIT}
${ADDITIONAL_LIBRARIES}
)

install(TARGETS hdlc-hexinjector RUNTIME DESTINATION bin)

