# puts all .cpp files inside current directory to the SOURCES variable
file(GLOB SOURCES *.cpp)

# compiles the files defined by SOURCES to generante the executable defined by EXEC
foreach( sourcefile ${SOURCES} )

    message("processing ${sourcefile}")
    set(SOURCE_TEST_FILES ${sourcefile})
    string( REPLACE ".cpp" "" benchmark ${sourcefile} )
    get_filename_component(exe_name ${benchmark} NAME)
    set(TARGET_BENCH_NAME ${exe_name})

    #include parent header files
    get_filename_component(PARENT_DIR ../framework ABSOLUTE)
    include_directories(${PARENT_DIR})

    # add lock-free queue libraries
    include_directories(PARENT_DIR ../thirdparty/mgark/include)
        
    add_executable(${TARGET_BENCH_NAME})
    target_sources(${TARGET_BENCH_NAME} PRIVATE ${SOURCE_TEST_FILES})
    target_compile_options(${TARGET_BENCH_NAME} PRIVATE ${COMPILE_FLAGS})
    target_link_libraries(${TARGET_BENCH_NAME} PRIVATE Threads::Threads Catch2::Catch2) 

endforeach( sourcefile ${SOURCES} )


