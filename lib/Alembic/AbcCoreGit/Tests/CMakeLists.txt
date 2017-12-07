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

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib ${PROJECT_BINARY_DIR}/lib)

SET( CXX_FILES
    ArchiveTests.cpp
    ArrayPropertyTests.cpp
    HashesTests.cpp
    ScalarPropertyTests.cpp
    TimeSamplingTests.cpp )

# #-******************************************************************************
# ADD_EXECUTABLE( AbcCoreGit_Test1 test1.cpp )
# TARGET_LINK_LIBRARIES( AbcCoreGit_Test1 ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ArchiveTests ArchiveTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ArchiveTests ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ArrayPropertyTests ArrayPropertyTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ArrayPropertyTests ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ScalarPropertyTests ScalarPropertyTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ScalarPropertyTests ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_TimeSamplingTests TimeSamplingTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_TimeSamplingTests ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ObjectTests ObjectTests.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ObjectTests ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_ConstantPropsTest ConstantPropsNumSampsTest.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_ConstantPropsTest ${CORE_LIBS} )

ADD_EXECUTABLE( AbcCoreGit_SubDTest
                MeshData.h
                MeshData.cpp
                SubDTest.cpp )
TARGET_LINK_LIBRARIES( AbcCoreGit_SubDTest ${CORE_LIBS} )

# ADD_TEST( AbcCoreGit_TEST1 AbcCoreGit_Test1 )
ADD_TEST( AbcCoreGit_ArchiveTESTS AbcCoreGit_ArchiveTests )
ADD_TEST( AbcCoreGit_ArrayPropertyTESTS AbcCoreGit_ArrayPropertyTests )
ADD_TEST( AbcCoreGit_ScalarPropertyTESTS AbcCoreGit_ScalarPropertyTests )
ADD_TEST( AbcCoreGit_TimeSamplingTESTS AbcCoreGit_TimeSamplingTests )
ADD_TEST( AbcCoreGit_ObjectTESTS AbcCoreGit_ObjectTests )
ADD_TEST( AbcCoreGit_ConstantPropsTest_TEST AbcCoreGit_ConstantPropsTest )
ADD_TEST( AbcCoreGit_SubDTESTS AbcCoreGit_SubDTest )
