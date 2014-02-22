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
            ObjectHeaderPtr iHeader );

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

private:
    // The parent object, NULL if it is the "top" object
    AbcA::ObjectWriterPtr m_parent;

    // The archive ptr.
    AbcA::ArchiveWriterPtr m_archive;

    // The header which defines this property.
    ObjectHeaderPtr m_header;

    // child object data, this is owned by the archive for "top" objects
    OwDataPtr m_data;

};


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_OwImpl_h_ */
