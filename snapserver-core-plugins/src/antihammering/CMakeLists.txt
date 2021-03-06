#
# File:
#      snapserver-core-plugins/src/antihammering/CMakeLists.txt
#
# Description:
#      Generate the antihammering plugin to make sure that too many hits
#      too quickly to any one of our sites.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2018  Made to Order Software Corp.  All Rights Reserved
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

project(plugin_antihammering)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    antihammering.qrc
)

snap_validate_xml( content.xml                              ${snapwebsites_plugins_src_SOURCE_DIR}/content/content.dtd    )
snap_validate_xml( editor/antihammering-settings-page.xml   ${snapwebsites_plugins_src_SOURCE_DIR}/editor/editor-form.dtd )
snap_validate_xml( layout/antihammering-settings-parser.xsl simplified-xslt20.dtd                                         )

add_library(antihammering SHARED
    antihammering.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    editor/antihammering-settings-page.xml
    layout/antihammering-settings-parser.xsl
)

target_link_libraries( antihammering Qt5::Core )

install(
    TARGETS antihammering LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    FILES "antihammering-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/antihammering
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
