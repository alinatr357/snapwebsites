#
# File:
#      snaplistd/src/CMakeLists.txt
#
# Description:
#      A daemon to duplicate the list data between front ends and the pagelist
#      backend system.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016-2018  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
project( snaplistd )

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}
    snaplistd.h
    snaplistd.cpp
    snaplistd_interrupt.cpp
    snaplistd_messenger.cpp
    snaplistd_mysql_timer.cpp
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    libdbproxy
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# vim: ts=4 sw=4 et
