#
# Copyright (C) 2016 Emmanuel Durand
#
# This file is part of doodle2stl.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Splash is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software.  If not, see <http://www.gnu.org/licenses/>.
#

include_directories(${OPENCV_INCLUDE_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})

add_executable(doodle2stl main.cpp)

target_link_libraries(doodle2stl ${OPENCV_LIBRARIES})
target_compile_features(doodle2stl PRIVATE cxx_variadic_templates)

install(TARGETS doodle2stl DESTINATION "bin/")
