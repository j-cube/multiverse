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

#ifndef _Alembic_AbcCoreGit_MetaDataMap_h_
#define _Alembic_AbcCoreGit_MetaDataMap_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// convenience class which is meant to map serialized meta data to an index
// It will only hold 254 strings, and won't hold any that are over 256 bytes
class MetaDataMap
{
public:
    MetaDataMap() {};
    ~MetaDataMap() {};

    // will return 0xff if iStr is too long, or we've run out of indices
    // 0 will be returned if iStr is empty
    Util::uint32_t getIndex( const std::string & iStr );
    void write( GitGroupPtr iParent );

    std::vector<std::string> toVector();
    void toJSON(rapidjson::Document& document, rapidjson::Value& dst);

private:
    std::map< std::string, Util::uint32_t > m_map;
};

typedef Alembic::Util::shared_ptr<MetaDataMap> MetaDataMapPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
