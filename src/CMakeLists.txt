set(BIN_DIR "bin")
set(LIB_DIR "lib")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--disable-new-dtags")

add_executable(kalgebra)
target_compile_options(kalgebra PRIVATE -fexceptions)

target_sources(kalgebra PRIVATE
    askname.h
    consolehtml.cpp
    consolehtml.h
    consolemodel.cpp
    consolemodel.h
    dictionary.cpp
    dictionary.h
    functionedit.cpp
    functionedit.h
    http_message.cpp
    http_message.h
    http_server.cpp
    http_server.h
    json.h
    kalgebra.cpp
    kalgebra.h
    main.cpp
    uri.h
    varedit.cpp
    varedit.h
    variablesdelegate.cpp
    variablesdelegate.h
    viewportwidget.cpp
    viewportwidget.h
)

file(GLOB
    ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-kalgebra.png"
)
ecm_add_app_icon(kalgebra ICONS ${ICONS_SRCS})

target_link_libraries(kalgebra
    Qt::Widgets
    Qt::PrintSupport
    Qt::WebEngineWidgets
    KF6::I18n
    KF6::CoreAddons
    KF6::WidgetsAddons
    KF6::ConfigWidgets
    KF6::XmlGui
    KF6::KIOCore
    KF6::I18n
    KF6::Analitza
    KF6::AnalitzaWidgets
    KF6::AnalitzaGui
    KF6::AnalitzaPlot
    Qt6::OpenGLWidgets
)

find_program(BASH_PROGRAM bash)
if (BASH_PROGRAM)
    message(STATUS "BASH FOUND")
else()
    message(FATAL_ERROR "BASH NOT FOUND")
endif()

execute_process(
    COMMAND ${BASH_PROGRAM} -c "patchelf -h"
    RESULT_VARIABLE RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)
if (RESULT EQUAL 0)
    message(STATUS "PATCHELF FOUND")
else()
    message(FATAL_ERROR "PATCHELF NOT FOUND; RUN `apt install patchelf` FIRST")
endif()

install(TARGETS kalgebra RUNTIME DESTINATION ${BIN_DIR})
install(CODE "set(BIN_DIR \"${BIN_DIR}\")")
install(CODE "set(LIB_DIR \"${LIB_DIR}\")")
install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
install(CODE [[
    function(execute_patchelf COMMAND_LINES)
        find_program(BASH_PROGRAM bash)
        execute_process(
            COMMAND ${BASH_PROGRAM} -c "${COMMAND_LINES}"
            RESULT_VARIABLE RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if (RESULT EQUAL 0)
            message(STATUS "RUNPATH MODIFIED")
        else()
            message(FATAL_ERROR "UNKNOWN ERROR OCCURRED WHEN MODIFYING RUNPATH")
        endif()
    endfunction()

    function(get_dependencies SOURCE_LIB FILTER_DEPS)
        set(RETURN_VALUE "")
        file(GET_RUNTIME_DEPENDENCIES
            LIBRARIES ${SOURCE_LIB}
            RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
            UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
        )

        # foreach(FILE ${RESOLVED_DEPS})
        #     message(STATUS "Resolved from ${SOURCE_LIB}: ${FILE}")
        # endforeach()

        foreach(FILE ${UNRESOLVED_DEPS})
            message(STATUS "Unresolved from ${SOURCE_LIB}: ${FILE}")
        endforeach()

        foreach(FILE ${RESOLVED_DEPS})
            list(FIND FILTER_DEPS ${FILE} INDEX_FILTER_DEPS)
            list(FIND RETURN_VALUE ${FILE} INDEX_RETURN_VALUE)
            if((NOT FILE MATCHES "^/lib/x86_64-linux-gnu")
                AND (${INDEX_FILTER_DEPS} EQUAL -1)
                AND (${INDEX_RETURN_VALUE} EQUAL -1)
                AND (NOT FILE MATCHES "^/home/PJLAB/wangyian/aqt")
            )
                list(APPEND RETURN_VALUE ${FILE})
            endif()
        endforeach()

        set(RETURN_VALUE ${RETURN_VALUE} PARENT_SCOPE)
        return()
    endfunction()

    function(set_dependencies SOURCE_LIBS PATH_NAME)
        set(GLOBAL_DEPS ${SOURCE_LIBS})
        set(INSTALLED_DEPS "")
        set(LIB_RPATH "${CMAKE_INSTALL_PREFIX}/${PATH_NAME}")

        while(GLOBAL_DEPS)
            list(GET GLOBAL_DEPS 0 FIRST_LIB)
            list(REMOVE_AT GLOBAL_DEPS 0)
            message(">> Installing  ${FIRST_LIB}")

            file(INSTALL
                DESTINATION ${LIB_RPATH}
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${FIRST_LIB}"
            )
            list(APPEND INSTALLED_DEPS ${FIRST_LIB})

            set(FILTER_DEPS "")
            list(APPEND FILTER_DEPS ${GLOBAL_DEPS} ${INSTALLED_DEPS})
            get_dependencies(${FIRST_LIB} "${FILTER_DEPS}")
            list(APPEND GLOBAL_DEPS ${RETURN_VALUE})
        endwhile()
        execute_patchelf("find \"${LIB_RPATH}\" -type f | while read file; do if file \"$file\" | grep -q \"ELF\"; then patchelf --set-rpath \\\$ORIGIN $file; fi; done")
    endfunction()

    set(KDE_LIB "")
    get_dependencies($<TARGET_FILE:kalgebra> IGNORE)
    foreach(LIB_ITEM ${RETURN_VALUE})
        if(LIB_ITEM MATCHES "^/home/PJLAB/wangyian/kde/usr")
            list(APPEND KDE_LIB ${LIB_ITEM})
        endif()
    endforeach()
    set_dependencies("${KDE_LIB}" ${LIB_DIR})
    execute_patchelf("patchelf --set-rpath \\\$ORIGIN/../${LIB_DIR} ${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/kalgebra")
]])
