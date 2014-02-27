//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

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

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
