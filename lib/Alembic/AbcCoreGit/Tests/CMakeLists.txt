##-****************************************************************************
##  multiverse - a next generation storage back-end for Alembic
##  Copyright 2015 J CUBE Inc. Tokyo, Japan.             
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##                                                                          
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
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

SET( GEOM_TEST_LIBS
     AlembicAbcGeom
     AlembicAbc
     AlembicAbcCoreGit
     AlembicAbcCoreAbstract
     AlembicUtil
     ${ALEMBIC_LIBGIT2_LIBS}
     ${ALEMBIC_MSGPACK_LIBS}
     ${ALEMBIC_JSONCPP_LIBS}
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

ADD_EXECUTABLE( AbcCoreGit_SubDTest
                MeshData.h
                MeshData.cpp
                SubDTest.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_SubDTest ${GEOM_TEST_LIBS} )

# ADD_TEST( AbcCoreGit_TEST1 AbcCoreGit_Test1 )
ADD_TEST( AbcCoreGit_ArchiveTESTS AbcCoreGit_ArchiveTests )
ADD_TEST( AbcCoreGit_ArrayPropertyTESTS AbcCoreGit_ArrayPropertyTests )
ADD_TEST( AbcCoreGit_ScalarPropertyTESTS AbcCoreGit_ScalarPropertyTests )
ADD_TEST( AbcCoreGit_TimeSamplingTESTS AbcCoreGit_TimeSamplingTests )
ADD_TEST( AbcCoreGit_ObjectTESTS AbcCoreGit_ObjectTests )
ADD_TEST( AbcCoreGit_ConstantPropsTest_TEST AbcCoreGit_ConstantPropsTest )
ADD_TEST( AbcCoreGit_SubDTESTS AbcCoreGit_SubDTest )
