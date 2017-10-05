
file(GLOB_RECURSE WEBPAGE_SOURCE_FILES *.md *.css *.html *.png)

# message("WEBPAGE_SOURCE_FILES: ${WEBPAGE_SOURCE_FILES}")

add_custom_target(
        webpage ALL
        COMMAND ${HUGO_PATH}
        --verbose
        --source ${CMAKE_SOURCE_DIR}/webpage
        --destination ${CMAKE_CURRENT_BINARY_DIR}/public
        SOURCES ${WEBPAGE_SOURCE_FILES}
)
