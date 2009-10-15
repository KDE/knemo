set( backend_SRCS
    backendbase.cpp
)

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( backend_SRCS ${backend_SRCS} netlinkbackend.cpp )
else ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( backend_SRCS ${backend_SRCS} bsdbackend.cpp )
endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

kde4_add_library( knemo_backend ${backend_SRCS} )

target_link_libraries( knemo_backend ${LIBIW_LIBRARY} ${LIBNL_LIBRARY} ) 
