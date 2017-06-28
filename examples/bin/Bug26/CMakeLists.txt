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
ADD_EXECUTABLE( bug26 bug26.cpp )
TARGET_LINK_LIBRARIES( bug26 ${FULL_ABC_LIBS} )
