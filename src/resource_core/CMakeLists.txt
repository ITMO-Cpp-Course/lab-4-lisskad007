add_library(resource_core STATIC
    file_handle.cpp
    resource_error.cpp
    resource_manager.cpp
)

target_link_libraries(resource_core PRIVATE project_warnings)
if (TARGET project_sanitizers)
        target_link_libraries(resource_core PRIVATE project_sanitizers)
endif ()

target_include_directories(resource_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
)
target_compile_definitions(resource_core PRIVATE _CRT_SECURE_NO_WARNINGS)