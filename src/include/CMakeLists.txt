set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system signals)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

install(FILES
    HdlcdAccessClient.h
    HdlcdPacketCtrl.h
    HdlcdPacketData.h
    HdlcdPacketEndpoint.h
    HdlcdPacket.h
DESTINATION include)
