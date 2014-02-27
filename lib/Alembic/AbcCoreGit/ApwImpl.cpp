//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/ApwImpl.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
ApwImpl::ApwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  GitGroupPtr iGroup,
                  PropertyHeaderPtr iHeader,
                  size_t iIndex ) :
    m_parent( iParent ), m_header( iHeader ), m_group( iGroup ), m_dims( 1 ),
    m_index( iIndex )
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid property header" );
    ABCA_ASSERT( m_group, "Invalid group" );

    TRACE("ApwImpl::ApwImpl(parent:" << CONCRETE_CPWPTR(m_parent)->repr() << ", group:" << m_group->repr() << ", header:'" << m_header->name() << "', index:" << m_index << ")");

    if ( m_header->header.getPropertyType() != AbcA::kArrayProperty )
    {
        ABCA_THROW( "Attempted to create a ArrayPropertyWriter from a "
                    "non-array property type" );
    }
}


//-*****************************************************************************
ApwImpl::~ApwImpl()
{
    AbcA::ArchiveWriterPtr archive = m_parent->getObject()->getArchive();

    index_t maxSamples = archive->getMaxNumSamplesForTimeSamplingIndex(
            m_header->timeSamplingIndex );

    Util::uint32_t numSamples = m_header->nextSampleIndex;

    // a constant property, we wrote the same sample over and over
    if ( m_header->lastChangedIndex == 0 && m_header->nextSampleIndex > 0 )
    {
        numSamples = 1;
    }

    if ( maxSamples < numSamples )
    {
        archive->setMaxNumSamplesForTimeSamplingIndex(
            m_header->timeSamplingIndex, numSamples );
    }

    Util::SpookyHash hash;
    hash.Init(0, 0);
    HashPropertyHeader( m_header->header, hash );

    // mix in the accumulated hash if we have samples
    if ( numSamples != 0 )
    {
        hash.Update( m_hash.d, 16 );
    }

    Util::uint64_t hash0, hash1;
    hash.Final( &hash0, &hash1 );
    Util::shared_ptr< CpwImpl > parent =
        Alembic::Util::dynamic_pointer_cast< CpwImpl,
            AbcA::CompoundPropertyWriter > ( m_parent );
    parent->fillHash( m_index, hash0, hash1 );
}

//-*****************************************************************************
void ApwImpl::setFromPreviousSample()
{

    // Make sure we aren't writing more samples than we have times for
    // This applies to acyclic sampling only
    ABCA_ASSERT(
        !m_header->header.getTimeSampling()->getTimeSamplingType().isAcyclic()
        || m_header->header.getTimeSampling()->getNumStoredTimes() >
        m_header->nextSampleIndex,
        "Can not set more samples than we have times for when using "
        "Acyclic sampling." );

    ABCA_ASSERT( m_header->nextSampleIndex > 0,
        "Can't set from previous sample before any samples have been written" );

    Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
    HashDimensions( m_dims, digest );
    Util::SpookyHash::ShortEnd(m_hash.words[0], m_hash.words[1],
                              digest.words[0], digest.words[1]);
    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
void ApwImpl::setSample( const AbcA::ArraySample & iSamp )
{
    // Make sure we aren't writing more samples than we have times for
    // This applies to acyclic sampling only
    ABCA_ASSERT(
        !m_header->header.getTimeSampling()->getTimeSamplingType().isAcyclic()
        || m_header->header.getTimeSampling()->getNumStoredTimes() >
        m_header->nextSampleIndex,
        "Can not write more samples than we have times for when using "
        "Acyclic sampling." );

    ABCA_ASSERT( iSamp.getDataType() == m_header->header.getDataType(),
        "DataType on ArraySample iSamp: " << iSamp.getDataType() <<
        ", does not match the DataType of the Array property: " <<
        m_header->header.getDataType() );

    // The Key helps us analyze the sample.
     AbcA::ArraySample::Key key = iSamp.getKey();

     // mask out the non-string POD since Git can safely share the same data
     // even if it originated from a different POD
     // the non-fixed sizes of our strings (plus added null characters) makes
     // determing the sie harder so strings are handled seperately
    if ( key.origPOD != Alembic::Util::kStringPOD &&
         key.origPOD != Alembic::Util::kWstringPOD )
    {
        key.origPOD = Alembic::Util::kInt8POD;
        key.readPOD = Alembic::Util::kInt8POD;
    }

    // We need to write the sample
    UNIMPLEMENTED("WrittenSampleIDPtr m_previousWrittenSampleID");
#if 0
    if ( m_header->nextSampleIndex == 0  ||
         !( m_previousWrittenSampleID &&
            key == m_previousWrittenSampleID->getKey() ) )
    {

        // we only need to repeat samples if this is not the first change
        if (m_header->firstChangedIndex != 0)
        {
            // copy the samples from after the last change to the latest index
            for ( index_t smpI = m_header->lastChangedIndex + 1;
                smpI < m_header->nextSampleIndex; ++smpI )
            {
                assert( smpI > 0 );
                CopyWrittenData( m_group, m_previousWrittenSampleID );
                WriteDimensions( m_group, m_dims,
                                 iSamp.getDataType().getPod() );
            }
        }

        // Write this sample, which will update its internal
        // cache of what the previously written sample was.
        AbcA::ArchiveWriterPtr awp = this->getObject()->getArchive();

        // Write the sample.
        // This distinguishes between string, wstring, and regular arrays.
        m_previousWrittenSampleID =
            WriteData( GetWrittenSampleMap( awp ), m_group, iSamp, key );

        m_dims = iSamp.getDimensions();
        WriteDimensions( m_group, m_dims, iSamp.getDataType().getPod() );

        // if we haven't written this already, isScalarLike will be true
        if ( m_header->isScalarLike && m_dims.numPoints() != 1 )
        {
            m_header->isScalarLike = false;
        }

        if ( m_header->isHomogenous && m_previousWrittenSampleID &&
             m_dims.numPoints() !=
             m_previousWrittenSampleID->getNumPoints() )
        {
            m_header->isHomogenous = false;
        }

        if (m_header->firstChangedIndex == 0)
        {
            m_header->firstChangedIndex = m_header->nextSampleIndex;
        }

        // this index is now the last change
        m_header->lastChangedIndex = m_header->nextSampleIndex;
    }

    Util::Digest digest = m_previousWrittenSampleID->getKey().digest;
    HashDimensions( m_dims, digest );
    if ( m_header->nextSampleIndex == 0 )
    {
        m_hash = digest;
    }
    else
    {
        Util::SpookyHash::ShortEnd(m_hash.words[0], m_hash.words[1],
                                   digest.words[0], digest.words[1]);
    }
#endif /* 0 */

    m_header->nextSampleIndex ++;
}

//-*****************************************************************************
AbcA::ArrayPropertyWriterPtr ApwImpl::asArrayPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t ApwImpl::getNumSamples()
{
    return ( size_t )m_header->nextSampleIndex;
}

//-*****************************************************************************
void ApwImpl::setTimeSamplingIndex( Util::uint32_t iIndex )
{
    // will assert if TimeSamplingPtr not found
    AbcA::TimeSamplingPtr ts =
        m_parent->getObject()->getArchive()->getTimeSampling(
            iIndex );

    ABCA_ASSERT( !ts->getTimeSamplingType().isAcyclic() ||
        ts->getNumStoredTimes() >= m_header->nextSampleIndex,
        "Already have written more samples than we have times for when using "
        "Acyclic sampling." );

    m_header->header.setTimeSampling(ts);
    m_header->timeSamplingIndex = iIndex;
}

//-*****************************************************************************
const AbcA::PropertyHeader & ApwImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr ApwImpl::getObject()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent->getObject();
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr ApwImpl::getParent()
{
    ABCA_ASSERT( m_parent, "Invalid parent" );
    return m_parent;
}

CpwImplPtr ApwImpl::getTParent() const
{
    Util::shared_ptr< CpwImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( m_parent );
    return parent;
}

std::string ApwImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        CpwImplPtr parentPtr = getTParent();

        ss << "<ApwImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "', group:" << m_group->repr() << ")>";
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
