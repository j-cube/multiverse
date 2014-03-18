//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_ApwImpl_h_
#define _Alembic_AbcCoreGit_ApwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/WrittenSampleMap.h>
#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class ApwImpl
    : public AbcA::ArrayPropertyWriter
    , public Alembic::Util::enable_shared_from_this<ApwImpl>
{
protected:
    friend class CpwData;

    //-*************************************************************************
    ApwImpl( AbcA::CompoundPropertyWriterPtr iParent,
             GitGroupPtr iGroup,
             PropertyHeaderPtr iHeader,
             size_t iIndex );

    virtual AbcA::ArrayPropertyWriterPtr asArrayPtr();

public:
    virtual ~ApwImpl();

    // ArrayPropertyWriter overrides
    virtual void setSample( const AbcA::ArraySample & iSamp );
    virtual void setFromPreviousSample();
    virtual size_t getNumSamples();
    virtual void setTimeSamplingIndex( Util::uint32_t iIndex );

    // BasePropertyWriter overrides
    virtual const AbcA::PropertyHeader & getHeader() const;
    virtual AbcA::ObjectWriterPtr getObject();
    virtual AbcA::CompoundPropertyWriterPtr getParent();

    CpwImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

protected:
    // Previous written array sample identifier!
    WrittenSampleIDPtr m_previousWrittenSampleID;

private:
    // The parent compound property writer.
    AbcA::CompoundPropertyWriterPtr m_parent;

    // The header which defines this property.
    // And extra data the parent needs to eventually write out
    PropertyHeaderPtr m_header;

    // for accumulating our hierarchical hash
    Util::Digest m_hash;

    Alembic::Util::auto_ptr<AbstractTypedSampleStore> m_store;

    GitGroupPtr m_group;

    AbcA::Dimensions m_dims;

    size_t m_index;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
