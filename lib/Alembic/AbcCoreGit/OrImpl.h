//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_OrImpl_h_
#define _Alembic_AbcCoreGit_OrImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/OrData.h>
#include <Alembic/AbcCoreGit/ArImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class OrImpl
    : public AbcA::ObjectReader
    , public Alembic::Util::enable_shared_from_this<OrImpl>
{

public:

    OrImpl( Alembic::Util::shared_ptr< ArImpl > iArchive,
            OrDataPtr iData,
            ObjectHeaderPtr iHeader );

    OrImpl( AbcA::ObjectReaderPtr iParent,
            GitGroupPtr iParentGroup,
            std::size_t iIndex,
            ObjectHeaderPtr iHeader );

    virtual ~OrImpl();

    //-*************************************************************************
    // ABSTRACT
    //-*************************************************************************
    virtual const AbcA::ObjectHeader & getHeader() const;

    virtual AbcA::ArchiveReaderPtr getArchive();

    virtual AbcA::ObjectReaderPtr getParent();

    virtual AbcA::CompoundPropertyReaderPtr getProperties();

    virtual size_t getNumChildren();

    virtual const AbcA::ObjectHeader & getChildHeader( size_t i );

    virtual const AbcA::ObjectHeader * getChildHeader
    ( const std::string &iName );

    virtual AbcA::ObjectReaderPtr getChild( const std::string &iName );

    virtual AbcA::ObjectReaderPtr getChild( size_t i );

    virtual AbcA::ObjectReaderPtr asObjectPtr();

    virtual bool getPropertiesHash( Util::Digest & oDigest );

    virtual bool getChildrenHash( Util::Digest & oDigest );

private:

    Alembic::Util::shared_ptr< ArImpl > getArchiveImpl() const;

    // The parent object
    Alembic::Util::shared_ptr< OrImpl > m_parent;

    Alembic::Util::shared_ptr< ArImpl > m_archive;

    OrDataPtr m_data;

    ObjectHeaderPtr m_header;

};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
