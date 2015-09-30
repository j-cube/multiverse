##-****************************************************************************
##  multiverse - a next generation storage back-end for Alembic
##  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.
##
##  This program is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.
##
##    J CUBE Inc.
##    6F Azabu Green Terrace
##    3-20-1 Minami-Azabu, Minato-ku, Tokyo
##    info@-jcube.jp
##-****************************************************************************


SET( TEST_LIBS
     AlembicAbcCoreGit
     AlembicAbcCoreAbstract
     AlembicUtil
     ${ALEMBIC_LIBGIT2_LIBS}
     ${ALEMBIC_MSGPACK_LIBS}
     ${ALEMBIC_JSONCPP_LIBS}
     ${ALEMBIC_ILMBASE_LIBS}
     ${ALEMBIC_LIBGIT2_LIBS}
     ${ALEMBIC_MSGPACK_LIBS}
     ${ALEMBIC_ILMBASE_LIBS}
     ${CMAKE_THREAD_LIBS_INIT} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
     ${ZLIB_LIBRARIES} ${EXTERNAL_MATH_LIBS} )

SET( CXX_FILES
    ArchiveTests.cpp
    ArrayPropertyTests.cpp
    HashesTests.cpp
    ScalarPropertyTests.cpp
    TimeSamplingTests.cpp )

# #-******************************************************************************
# ADD_EXECUTABLE( AbcCoreGit_Test1 test1.cpp )
# TARGET_LINK_LIBRARIES( AbcCoreGit_Test1 ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ArchiveTests ArchiveTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ArchiveTests ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ArrayPropertyTests ArrayPropertyTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ArrayPropertyTests ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ScalarPropertyTests ScalarPropertyTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ScalarPropertyTests ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_TimeSamplingTests TimeSamplingTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_TimeSamplingTests ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ObjectTests ObjectTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ObjectTests ${TEST_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ConstantPropsTest ConstantPropsNumSampsTest.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ConstantPropsTest ${TEST_LIBS} )


# ADD_TEST( AbcCoreGit_TEST1 AbcCoreGit_Test1 )
ADD_TEST( AbcCoreGit_ArchiveTESTS AbcCoreGit_ArchiveTests )
ADD_TEST( AbcCoreGit_ArrayPropertyTESTS AbcCoreGit_ArrayPropertyTests )
ADD_TEST( AbcCoreGit_ScalarPropertyTESTS AbcCoreGit_ScalarPropertyTests )
ADD_TEST( AbcCoreGit_TimeSamplingTESTS AbcCoreGit_TimeSamplingTests )
ADD_TEST( AbcCoreGit_ObjectTESTS AbcCoreGit_ObjectTests )
ADD_TEST( AbcCoreGit_ConstantPropsTest_TEST AbcCoreGit_ConstantPropsTest )
