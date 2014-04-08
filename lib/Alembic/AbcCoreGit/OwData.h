//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_OwData_h_
#define _Alembic_AbcCoreGit_OwData_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/MetaDataMap.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// Forwards
class CpwData;

typedef int git_group_t;

// data class owned by OwImpl, or AwImpl if it is a "top" object.
// it owns and makes child properties
class OwData : public Alembic::Util::enable_shared_from_this<OwData>
{
public:
    OwData( GitGroupPtr iGroup,
            const std::string &iName,
            const AbcA::MetaData &iMetaData );

    ~OwData();

    AbcA::CompoundPropertyWriterPtr getProperties( AbcA::ObjectWriterPtr iParent );

    size_t getNumChildren();

    const AbcA::ObjectHeader & getChildHeader( size_t i );

    const AbcA::ObjectHeader * getChildHeader( const std::string &iName );

    AbcA::ObjectWriterPtr getChild( const std::string &iName );

    AbcA::ObjectWriterPtr createChild( AbcA::ObjectWriterPtr iParent,
                                       const std::string & iFullName,
                                       const AbcA::ObjectHeader &iHeader );

    GitGroupPtr getGroup()                  { return m_group; }
    GitGroupConstPtr getGroup() const       { return m_group; }

    void writeHeaders( MetaDataMapPtr iMetaDataMap, Util::SpookyHash & ioHash );

    void fillHash( std::size_t iIndex, Util::uint64_t iHash0,
                   Util::uint64_t iHash1 );

    const std::string& name() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->name(); }
    std::string relPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->relPathname(); }
    std::string absPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->absPathname(); }

    void writeToDisk();

private:
    // The group corresponding to the object
    GitGroupPtr m_group;

    typedef std::vector<ObjectHeaderPtr> ChildHeaders;
    typedef std::map<std::string,WeakOwPtr> MadeChildren;

    // The children
    ChildHeaders m_childHeaders;
    MadeChildren m_madeChildren;

    Alembic::Util::weak_ptr< AbcA::CompoundPropertyWriter > m_top;

    // Our "top" property
    Alembic::Util::shared_ptr < CpwData > m_data;

    // child hashes
    std::vector< Util::uint64_t > m_hashes;
};

typedef Alembic::Util::shared_ptr<OwData> OwDataPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_OwData_h_ */
