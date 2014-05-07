//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_OrData_h_
#define _Alembic_AbcCoreGit_OrData_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class CprData;

// data class owned by OrImpl, or ArImpl if it is a "top" object.
// it owns and makes child objects

class OrData : public Alembic::Util::enable_shared_from_this<OrData>
{
public:
    OrData( GitGroupPtr iGroup,
            const std::string & iParentName,
            size_t iThreadId,
            AbcA::ArchiveReader & iArchive,
            const std::vector< AbcA::MetaData > & iIndexedMetaData );

    ~OrData();

    AbcA::CompoundPropertyReaderPtr
    getProperties( AbcA::ObjectReaderPtr iParent );

    size_t getNumChildren();

    const AbcA::ObjectHeader &
    getChildHeader( AbcA::ObjectReaderPtr iParent, size_t i );

    const AbcA::ObjectHeader *
    getChildHeader( AbcA::ObjectReaderPtr iParent, const std::string &iName );

    AbcA::ObjectReaderPtr
    getChild( AbcA::ObjectReaderPtr iParent, const std::string &iName );

    AbcA::ObjectReaderPtr
    getChild( AbcA::ObjectReaderPtr iParent, size_t i );

    void getPropertiesHash( Util::Digest & oDigest, size_t iThreadId );

    void getChildrenHash( Util::Digest & oDigest, size_t iThreadId );

private:

    GitGroupPtr m_group;

    struct Child
    {
        ObjectHeaderPtr header;
        WeakOrPtr made;
        Alembic::Util::mutex lock;
    };

    typedef std::map<std::string, size_t> ChildrenMap;

    // The children
    Child * m_children;
    ChildrenMap m_childrenMap;

    // Our "top" property.
    Alembic::Util::weak_ptr< AbcA::CompoundPropertyReader > m_top;
    Alembic::Util::shared_ptr < CprData > m_data;
    Alembic::Util::mutex m_cprlock;
};

typedef Alembic::Util::shared_ptr<OrData> OrDataPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
