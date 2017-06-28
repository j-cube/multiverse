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

#ifndef _Alembic_AbcCoreGit_SprImpl_h_
#define _Alembic_AbcCoreGit_SprImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/CprImpl.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class SprImpl;
typedef Alembic::Util::shared_ptr<SprImpl> SprImplPtr;

//-*****************************************************************************
// The Scalar Property Reader fills up bytes corresponding to memory for
// a single scalar sample at a particular index.
class SprImpl
    : public AbcA::ScalarPropertyReader
    , public Alembic::Util::enable_shared_from_this<SprImpl>
{
public:
    SprImpl( AbcA::CompoundPropertyReaderPtr iParent,
             GitGroupPtr iParentGroup,
             PropertyHeaderPtr iHeader );

    // BasePropertyReader overrides
    virtual const AbcA::PropertyHeader & getHeader() const;
    virtual AbcA::ObjectReaderPtr getObject();
    virtual AbcA::CompoundPropertyReaderPtr getParent();
    virtual AbcA::ScalarPropertyReaderPtr asScalarPtr();

    // ScalarPropertyReader overrides
    virtual size_t getNumSamples();
    virtual bool isConstant();
    virtual void getSample( index_t iSampleIndex,
                            void * iIntoLocation );
    virtual std::pair<index_t, chrono_t> getFloorIndex( chrono_t iTime );
    virtual std::pair<index_t, chrono_t> getCeilIndex( chrono_t iTime );
    virtual std::pair<index_t, chrono_t> getNearIndex( chrono_t iTime );

    CprImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

    const std::string& name() const               { return m_header->name(); }
    std::string relPathname() const;
    std::string absPathname() const;

    bool readFromDisk();

    friend std::ostream& operator<< ( std::ostream& out, const SprImpl& value );
    friend std::ostream& operator<< ( std::ostream& out, SprImplPtr value );

private:

    // Parent compound property writer. It must exist.
    AbcA::CompoundPropertyReaderPtr m_parent;

    // parent group from which all samples are read
    GitGroupPtr m_group;

    // Stores the PropertyHeader and other info
    PropertyHeaderPtr m_header;

    Alembic::Util::unique_ptr<AbstractTypedSampleStore> m_store;

    bool m_read;
};

inline std::ostream& operator<< ( std::ostream& out, const SprImpl& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, SprImplPtr value )
{
    out << value->repr();
    return out;
}


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
