//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

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
