# Copyright 2022, Contributors To LensorOS.
# All rights reserved.
#
# This file is part of LensorOS.
#
# LensorOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LensorOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LensorOS. If not, see <https://www.gnu.org/licenses/>.


cmake_minimum_required( VERSION 3.20 )
set( blazeit_VERSION 0.0.3 )
set( blazeit_LANGUAGES C )

# Get the directory of the repository.
cmake_path(
  GET
  CMAKE_CURRENT_LIST_DIR
  PARENT_PATH
  REPO_DIR
)
cmake_path(
  GET
  REPO_DIR
  PARENT_PATH
  REPO_DIR
)

project( LensorOS VERSION ${blazeit_VERSION} LANGUAGES ${blazeit_LANGUAGES} )

add_executable( blazeit main.c )
