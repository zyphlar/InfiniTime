if(DEFINED ENABLE_USERAPPS)
    set(USERAPP_TYPES ${ENABLE_USERAPPS} CACHE STRING "List of user apps to build into the firmware")
else ()
    set(USERAPP_TYPES "Apps::Navigation, Apps::Alarm, Apps::Steps, Apps::HeartRate, Apps::Music, Apps::Twos" CACHE STRING "List of user apps to build into the firmware")
    #Apps::Paint, Apps::Metronome, Apps::Paddle, Apps::StopWatch, Apps::Timer, 
endif ()

add_library(infinitime_apps INTERFACE)
target_sources(infinitime_apps INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Apps.h")
target_include_directories(infinitime_apps INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/")

# Generate the list of user apps to be compiled into the firmware
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Apps.h.in ${CMAKE_CURRENT_BINARY_DIR}/Apps.h)
