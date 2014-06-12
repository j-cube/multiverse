//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_MetaDataMap_h_
#define _Alembic_AbcCoreGit_MetaDataMap_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <json/json.h>

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
    Json::Value toJSON();

private:
    std::map< std::string, Util::uint32_t > m_map;
};

typedef Alembic::Util::shared_ptr<MetaDataMap> MetaDataMapPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
