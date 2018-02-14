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

#ifndef _Alembic_AbcCoreGit_SpwImpl_h_
#define _Alembic_AbcCoreGit_SpwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/WrittenSampleMap.h>
#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// Scalar Property Writer.
class SpwImpl
    : public AbcA::ScalarPropertyWriter
    , public Alembic::Util::enable_shared_from_this<SpwImpl>
{
protected:
    friend class CpwData;

    //-*************************************************************************
    SpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
             GitGroupPtr iGroup,
             PropertyHeaderPtr iHeader,
             size_t iIndex );

    AbcA::ScalarPropertyWriterPtr asScalarPtr();

public:
    virtual ~SpwImpl();

    // ScalarPropertyWriter overrides
    virtual void setSample( const void *iSamp );
    virtual void setFromPreviousSample();
    virtual size_t getNumSamples();
    virtual void setTimeSamplingIndex( Util::uint32_t iIndex );

    // BasePropertyWriter overrides
    virtual const AbcA::PropertyHeader & getHeader() const;
    virtual AbcA::ObjectWriterPtr getObject();
    virtual AbcA::CompoundPropertyWriterPtr getParent();

    CpwImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

    const std::string& name() const               { return m_header->name(); }
    std::string relPathname() const;
    std::string absPathname() const;

    void writeToDisk();

    Alembic::Util::shared_ptr< AwImpl > getArchiveImpl() const;

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

    AbcA::Dimensions m_rank0_dims;
    Alembic::Util::shared_ptr<AbstractTypedSampleStore> m_store;

    GitGroupPtr m_group;

    size_t m_index;

    bool m_written;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
