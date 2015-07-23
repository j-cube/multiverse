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

#ifndef _Alembic_AbcCoreGit_WrittenSampleMap_h_
#define _Alembic_AbcCoreGit_WrittenSampleMap_h_

#include <Alembic/AbcCoreAbstract/ArraySampleKey.h>
#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// A Written Sample ID is a receipt that contains information that
// refers to the exact location in a Git-backed archive that a sample was written to.
//
// It also contains the Key of the sample, so it may be verified.
//
// This object is used to "reuse" an already written sample by linking
// it from the previous usage.
//-*****************************************************************************
class WrittenSampleID
{
public:
    WrittenSampleID()
    {
        m_sampleKey.numBytes = 0;
        m_sampleKey.origPOD = Alembic::Util::kInt8POD;
        m_sampleKey.readPOD = Alembic::Util::kInt8POD;
        m_numPoints = 0;
    }

    WrittenSampleID( const AbcA::ArraySample::Key &iKey,
                     std::size_t iWhere,
                     std::size_t iNumPoints )
      : m_sampleKey( iKey ), m_where( iWhere ), m_numPoints( iNumPoints )
    {
    }

    const AbcA::ArraySample::Key &getKey() const { return m_sampleKey; }

    std::size_t getObjectLocation() const { return m_where; }

    std::size_t getNumPoints() { return m_numPoints; }

private:
    AbcA::ArraySample::Key m_sampleKey;
    std::size_t m_where;
    std::size_t m_numPoints;
};

//-*****************************************************************************
typedef Alembic::Util::shared_ptr<WrittenSampleID> WrittenSampleIDPtr;

//-*****************************************************************************
// This class handles the mapping.
class WrittenSampleMap
{
protected:
    friend class AwImpl;

    WrittenSampleMap() {}

public:

    // Returns 0 if it can't find it
    WrittenSampleIDPtr find( const AbcA::ArraySample::Key &key ) const
    {
        Map::const_iterator miter = m_map.find( key );
        if ( miter != m_map.end() )
        {
            return (*miter).second;
        }
        else
        {
            return WrittenSampleIDPtr();
        }
    }

    // Store. Will clobber if you've already stored it.
    void store( WrittenSampleIDPtr r )
    {
        if ( !r )
        {
            ABCA_THROW( "Invalid WrittenSampleIDPtr" );
        }

        m_map[r->getKey()] = r;
    }

    void clear()
    {
        m_map.clear();
    }

protected:
    typedef AbcA::UnorderedMapUtil<WrittenSampleIDPtr>::umap_type Map;
    Map m_map;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
