//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_OwImpl_h_
#define _Alembic_AbcCoreGit_OwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class OwImpl;
typedef Util::shared_ptr<OwImpl> OwImplPtr;

//-*****************************************************************************
// These are _always_ created as shared ptrs.
class OwImpl
    : public AbcA::ObjectWriter
    , public Alembic::Util::enable_shared_from_this<OwImpl>
{
public:

    OwImpl( AbcA::ArchiveWriterPtr iArchive,
            OwDataPtr iData,
            const AbcA::MetaData & iMetaData );

    OwImpl( AbcA::ObjectWriterPtr iParent,
            GitGroupPtr iParentGroup,
            ObjectHeaderPtr iHeader,
            size_t iIndex );

    virtual ~OwImpl();

    //-*************************************************************************
    // FROM ABSTRACT
    //-*************************************************************************

    virtual const AbcA::ObjectHeader & getHeader() const;

    virtual AbcA::ArchiveWriterPtr getArchive();

    virtual AbcA::ObjectWriterPtr getParent();

    virtual AbcA::CompoundPropertyWriterPtr getProperties();

    virtual size_t getNumChildren();

    virtual const AbcA::ObjectHeader & getChildHeader( size_t i );

    virtual const AbcA::ObjectHeader *
    getChildHeader( const std::string &iName );

    virtual AbcA::ObjectWriterPtr getChild( const std::string &iName );

    virtual AbcA::ObjectWriterPtr
    createChild( const AbcA::ObjectHeader &iHeader );

    virtual AbcA::ObjectWriterPtr asObjectPtr();

    void fillHash( size_t iIndex, Util::uint64_t iHash0,
                   Util::uint64_t iHash1 );

    OwImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

    const std::string& name() const               { ABCA_ASSERT(m_data, "invalid OwData"); return m_data->name(); }
    std::string relPathname() const               { ABCA_ASSERT(m_data, "invalid OwData"); return m_data->relPathname(); }
    std::string absPathname() const               { ABCA_ASSERT(m_data, "invalid OwData"); return m_data->absPathname(); }

    void writeToDisk();

private:
    // The parent object, NULL if it is the "top" object
    AbcA::ObjectWriterPtr m_parent;

    // The archive ptr.
    AbcA::ArchiveWriterPtr m_archive;

    // The header which defines this property.
    ObjectHeaderPtr m_header;

    // child object data, this is owned by the archive for "top" objects
    OwDataPtr m_data;

    size_t m_index;
};

inline OwImplPtr getOwImplPtr(AbcA::ObjectWriterPtr aOwPtr)
{
    ABCA_ASSERT( aOwPtr, "Invalid pointer to AbcA::ObjectWriter" );
    Util::shared_ptr< OwImpl > concretePtr =
       Alembic::Util::dynamic_pointer_cast< OwImpl,
        AbcA::ObjectWriter > ( aOwPtr );
    return concretePtr;
}

#define CONCRETE_OWPTR(aOwPtr)   getOwImplPtr(aOwPtr)


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_OwImpl_h_ */
