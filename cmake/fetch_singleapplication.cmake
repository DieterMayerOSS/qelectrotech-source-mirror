# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

message(" - fetch_singleapplication")

# https://github.com/itay-grudev/SingleApplication/issues/18
#qmake
#DEFINES += QAPPLICATION_CLASS=QGuiApplication
set(QAPPLICATION_CLASS QApplication)

Include(FetchContent)

# SingleApplication selects its Qt major version from QT_DEFAULT_MAJOR_VERSION
# and defaults to Qt5 when it is unset. This module is included before QET's own
# find_package(QT ...), so detect the Qt version early and pass it through, so
# SingleApplication builds against the same Qt (5 or 6) as QElectroTech.
if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
  set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR})
endif()

FetchContent_Declare(
  SingleApplication
  GIT_REPOSITORY https://github.com/itay-grudev/SingleApplication.git
  GIT_TAG        v3.2.0)

FetchContent_MakeAvailable(SingleApplication)
