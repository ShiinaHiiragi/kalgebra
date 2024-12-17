set(LIB_PATH "lib")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_PATH}")
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

# install(TARGETS kalgebra ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
# install(PROGRAMS org.kde.kalgebra.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
# install(FILES kalgebra.xml DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/syntax )
# install(FILES org.kde.kalgebra.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

install(TARGETS kalgebra RUNTIME DESTINATION bin)

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

install(CODE "set(LIB_PATH \"${LIB_PATH}\")")
install(CODE "set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")")
install(CODE "set(CMAKE_INSTALL_RPATH \"${CMAKE_INSTALL_RPATH}\")")
install(CODE [[
    function(get_dependencies LIBRARY)
        set(RETURN_VALUE "")
        file(GET_RUNTIME_DEPENDENCIES
            LIBRARIES ${LIBRARY}
            RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
            UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
        )

        # foreach(FILE ${RESOLVED_DEPS})
        #     message(STATUS "Resolved from ${LIBRARY}: ${FILE}")
        # endforeach()

        foreach(FILE ${UNRESOLVED_DEPS})
            message(STATUS "Unresolved from ${LIBRARY}: ${FILE}")
        endforeach()

        foreach(FILE ${RESOLVED_DEPS})
            list(FIND GLOBAL_DEPS ${FILE} INDEX_GLOBAL_DEPS)
            list(FIND RETURN_VALUE ${FILE} INDEX_RETURN_VALUE)
            list(FIND INSTALLED_DEPS ${FILE} INDEX_INSTALLED_DEPS)
            if((NOT FILE MATCHES "^/lib/x86_64-linux-gnu")
                AND (${INDEX_GLOBAL_DEPS} EQUAL -1)
                AND (${INDEX_RETURN_VALUE} EQUAL -1)
                AND (${INDEX_INSTALLED_DEPS} EQUAL -1)
            )
                list(APPEND RETURN_VALUE ${FILE})
            endif()
        endforeach()

        set(RETURN_VALUE ${RETURN_VALUE} PARENT_SCOPE)
        return()
    endfunction()

    function(copy_dependencies SOURCE_FILE INSTALL_SUFFIX ADD_SELF)
        set(GLOBAL_DEPS "")
        set(INSTALLED_DEPS "")
        set(TEMP_RPATH "${CMAKE_INSTALL_PREFIX}/${INSTALL_SUFFIX}")

        if (ADD_SELF EQUAL 1)
            list(APPEND GLOBAL_DEPS ${SOURCE_FILE})
        else()
            get_dependencies(${SOURCE_FILE})
            set(GLOBAL_DEPS ${RETURN_VALUE})
        endif()

        while(GLOBAL_DEPS)
            list(GET GLOBAL_DEPS 0 FIRST_LIBRARY)
            list(REMOVE_AT GLOBAL_DEPS 0)
            list(APPEND INSTALLED_DEPS ${FIRST_LIBRARY})

            message(">> Installing  ${FIRST_LIBRARY}")
            file(INSTALL
                DESTINATION ${TEMP_RPATH}
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${FIRST_LIBRARY}"
            )

            get_dependencies(${FIRST_LIBRARY})
            set(MERGED "")
            list(APPEND MERGED ${GLOBAL_DEPS} ${RETURN_VALUE})
            set(GLOBAL_DEPS ${MERGED})
        endwhile()

        find_program(BASH_PROGRAM bash)
        execute_process(
            COMMAND ${BASH_PROGRAM} -c "find \"${TEMP_RPATH}\" -type f | while read file; do if file \"$file\" | grep -q \"ELF\"; then patchelf --set-rpath \"${TEMP_RPATH}\" $file; fi; done"
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

    copy_dependencies($<TARGET_FILE:kalgebra> ${LIB_PATH} 0)
    copy_dependencies(
        "/home/PJLAB/wangyian/aqt/6.5.0/gcc_64/plugins/platforms/libqxcb.so"
        bin/platforms
        1
    )
]])
