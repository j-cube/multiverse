//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_AprImpl_h_
#define _Alembic_AbcCoreGit_AprImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/SampleStore.h>
#include <Alembic/AbcCoreGit/CprImpl.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class AprImpl;
typedef Alembic::Util::shared_ptr<AprImpl> AprImplPtr;

//-*****************************************************************************
class AprImpl :
    public AbcA::ArrayPropertyReader,
    public Alembic::Util::enable_shared_from_this<AprImpl>
{
public:
    AprImpl( AbcA::CompoundPropertyReaderPtr iParent,
             GitGroupPtr iGroup,
             PropertyHeaderPtr iHeader );

    // BasePropertyReader overrides
    virtual const AbcA::PropertyHeader & getHeader() const;
    virtual AbcA::ObjectReaderPtr getObject();
    virtual AbcA::CompoundPropertyReaderPtr getParent();
    virtual AbcA::ArrayPropertyReaderPtr asArrayPtr();

    // ArrayPropertyReader overrides
    virtual size_t getNumSamples();
    virtual bool isConstant();
    virtual void getSample( index_t iSampleIndex,
                            AbcA::ArraySamplePtr &oSample );
    virtual std::pair<index_t, chrono_t> getFloorIndex( chrono_t iTime );
    virtual std::pair<index_t, chrono_t> getCeilIndex( chrono_t iTime );
    virtual std::pair<index_t, chrono_t> getNearIndex( chrono_t iTime );
    virtual bool getKey( index_t iSampleIndex, AbcA::ArraySampleKey & oKey );
    virtual void getDimensions( index_t iSampleIndex,
                                Alembic::Util::Dimensions & oDim );
    virtual bool isScalarLike();
    virtual void getAs( index_t iSample, void *iIntoLocation,
                        Alembic::Util::PlainOldDataType iPod );

    CprImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

    const std::string& name() const               { return m_header->name(); }
    std::string relPathname() const;
    std::string absPathname() const;

    bool readFromDisk();

    friend std::ostream& operator<< ( std::ostream& out, const AprImpl& value );
    friend std::ostream& operator<< ( std::ostream& out, AprImplPtr value );

private:

    // Parent compound property writer. It must exist.
    AbcA::CompoundPropertyReaderPtr m_parent;

    // group from which all samples are read
    GitGroupPtr m_group;

    // Stores the PropertyHeader and other info
    PropertyHeaderPtr m_header;

    Alembic::Util::auto_ptr<AbstractTypedSampleStore> m_store;

    bool m_read;
};

inline std::ostream& operator<< ( std::ostream& out, const AprImpl& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, AprImplPtr value )
{
    out << value->repr();
    return out;
}


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
