if(ESP_PLATFORM)
    idf_component_register(
        SRCS            "sht3x.c"
        INCLUDE_DIRS    "."
    )
else()
    add_library(sht3x STATIC sht3x.c)
    target_include_directories(sht3x PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()