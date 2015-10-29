/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*                                                                           */
/*  Copyright 2015 J CUBE Inc. Tokyo, Japan.                                 */
/*                                                                           */
/*  Licensed under the Apache License, Version 2.0 (the "License");          */
/*  you may not use this file except in compliance with the License.         */
/*  You may obtain a copy of the License at                                  */
/*                                                                           */
/*      http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                           */
/*  Unless required by applicable law or agreed to in writing, software      */
/*  distributed under the License is distributed on an "AS IS" BASIS,        */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*  See the License for the specific language governing permissions and      */
/*  limitations under the License.                                           */
/*****************************************************************************/

#ifndef _Alembic_AbcCoreGit_ReadWriteUtil_h_
#define _Alembic_AbcCoreGit_ReadWriteUtil_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/WrittenSampleMap.h>
#include <Alembic/AbcCoreGit/MetaDataMap.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
void HashPropertyHeader( const AbcA::PropertyHeader & iHeader,
                         Util::SpookyHash & ioHash );

//-*****************************************************************************
// Hashes the dimensions and the current sample hash (ioHash), and then stores
// the result on ioHash.
void HashDimensions( const AbcA::Dimensions & iDims,
                     Util::Digest & ioHash );

//-*****************************************************************************
WrittenSampleMap& GetWrittenSampleMap(
    AbcA::ArchiveWriterPtr iArchive );

WrittenSampleIDPtr getWrittenSampleID( WrittenSampleMap &iMap,
                                       const AbcA::ArraySample &iSamp,
                                       const AbcA::ArraySample::Key &iKey,
                                       size_t iWhere );

//-*****************************************************************************
void
WritePropertyInfo( std::vector< Util::uint8_t > & ioData,
                   const AbcA::PropertyHeader &iHeader,
                   bool isScalarLike,
                   bool isHomogenous,
                   Util::uint32_t iTimeSamplingIndex,
                   Util::uint32_t iNumSamples,
                   Util::uint32_t iFirstChangedIndex,
                   Util::uint32_t iLastChangedIndex,
                   MetaDataMapPtr iMap );

//-*****************************************************************************
void
WriteObjectHeader( std::vector< Util::uint8_t > & ioData,
                   const AbcA::ObjectHeader &iHeader,
                   MetaDataMapPtr iMap );

#ifdef OBSOLETE
//-*****************************************************************************
void
WriteTimeSampling( std::vector< Util::uint8_t > & ioData,
                   Util::uint32_t  iMaxSample,
                   const AbcA::TimeSampling &iTsmp );
#endif /* OBSOLETE */

void jsonWriteTimeSampling( rapidjson::Document& document, rapidjson::Value& dst,
                            Util::uint32_t iMaxSample,
                            const AbcA::TimeSampling &iTsmp );

void jsonReadTimeSamples( const rapidjson::Value& jsonTimeSamples,
                       std::vector < AbcA::TimeSamplingPtr > & oTimeSamples,
                       std::vector < AbcA::index_t > & oMaxSamples );

void ConvertData( Alembic::Util::PlainOldDataType fromPod,
                Alembic::Util::PlainOldDataType toPod,
                char * fromBuffer,
                void * toBuffer,
                std::size_t iSize );

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_ReadWriteUtil_h_ */
