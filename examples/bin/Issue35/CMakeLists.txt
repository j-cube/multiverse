##-****************************************************************************
##  Copyright (c) 2013-2015 J CUBE I. Tokyo, Japan. All Rights Reserved.
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License along
##  with this program; if not, write to the Free Software Foundation, Inc.,
##  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
##  J CUBE Inc.
##  6F Azabu Green Terrace
##  3-20-1 Minami-Azabu, Minato-ku, Tokyo
##  info@-jcube.jp
##-****************************************************************************

SET( FULL_ABC_LIBS
     AlembicAbcGeom
     AlembicAbcCoreFactory
     AlembicAbc
     AlembicAbcCoreHDF5
     AlembicAbcCoreOgawa
     AlembicAbcCoreGit
     AlembicAbcCoreAbstract
     AlembicOgawa
     AlembicUtil
     ${ALEMBIC_HDF5_LIBS}
     ${ALEMBIC_LIBGIT2_LIBS}
     ${ALEMBIC_MSGPACK_LIBS}
     ${ALEMBIC_JSONCPP_LIBS}
     ${ALEMBIC_ILMBASE_LIBS}
     ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
     ${ZLIB_LIBRARIES} ${EXTERNAL_MATH_LIBS} )

#-******************************************************************************
ADD_EXECUTABLE( issue35 issue35.cpp )
TARGET_LINK_LIBRARIES( issue35 ${FULL_ABC_LIBS} )
