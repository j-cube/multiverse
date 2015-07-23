/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
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
