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

#include <Alembic/AbcCoreGit/MetaDataMap.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
Util::uint32_t MetaDataMap::getIndex( const std::string & iStr )
{
    if ( iStr.empty() )
    {
        return 0;
    }
    // we only want small meta data strings in our map since they are the
    // most likely to be repeated over and over
    else if ( iStr.size() < 256 )
    {
        std::map< std::string, Util::uint32_t >::iterator it =
            m_map.find( iStr );

        if ( it != m_map.end() )
        {
            return it->second + 1;
        }
        // 255 is reserved for meta data which we need to
        // explicitly write (and 0 means empty metadata)
        else if ( it == m_map.end() && m_map.size() < 254 )
        {
            Util::uint32_t index = m_map.size();
            m_map[iStr] = index;
            return index + 1;
        }
    }

    // too long, or no room left for this entry
    return 255;
}

//-*****************************************************************************
void MetaDataMap::write( GitGroupPtr iParent )
{

    if ( m_map.empty() )
    {
        iParent->addEmptyData();
        return;
    }

    std::vector< std::string > mdVec;
    mdVec.resize( m_map.size() );

    // lets put each string into it's vector slot
    std::map< std::string, Util::uint32_t >::iterator it, itEnd;
    for ( it = m_map.begin(), itEnd = m_map.end(); it != itEnd; ++it )
    {
        mdVec[ it->second ] = it->first;
    }

    // now place it all into one continuous buffer
    std::vector< Util::uint8_t > buf;
    std::vector< std::string >::iterator jt, jtEnd;
    for ( jt = mdVec.begin(), jtEnd = mdVec.end(); jt != jtEnd; ++jt )
    {

        // all these strings are less than 256 chars so just push back size
        // as 1 byte
        buf.push_back( jt->size() );
        buf.insert( buf.end(), jt->begin(), jt->end() );
    }

    iParent->addData( buf.size(), ( const void * )&buf.front() );
}

//-*****************************************************************************
std::vector<std::string> MetaDataMap::toVector()
{
    std::vector<std::string> mdVec;

    if ( m_map.empty() )
    {
        return mdVec;
    }

    mdVec.resize( m_map.size() );

    // lets put each string into it's vector slot
    std::map< std::string, Util::uint32_t >::iterator it, itEnd;
    for ( it = m_map.begin(), itEnd = m_map.end(); it != itEnd; ++it )
    {
        mdVec[ it->second ] = it->first;
    }

    return mdVec;
}

//-*****************************************************************************
void MetaDataMap::toJSON(rapidjson::Document& document, rapidjson::Value& dst)
{
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    dst.SetArray();

    std::vector<std::string> mdVec = toVector();
    std::vector<std::string>::const_iterator it;
    for ( it = mdVec.begin(); it != mdVec.end(); ++it )
    {
        const std::string& el = *it;

        rapidjson::Value v(el.c_str(), el.length(), allocator);
        dst.PushBack( v, allocator );
    }
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
