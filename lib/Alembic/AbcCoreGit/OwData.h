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
class AwImpl;
class CpwData;

typedef int git_group_t;

// data class owned by OwImpl, or AwImpl if it is a "top" object.
// it owns and makes child properties
class OwData : public Alembic::Util::enable_shared_from_this<OwData>
{
public:
    OwData( GitGroupPtr iGroup,
            ObjectHeaderPtr iHeader );

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

    Alembic::Util::shared_ptr< AwImpl > getArchiveImpl() const;

private:
    // The group corresponding to the object
    GitGroupPtr m_group;

    // The header for this object.
    ObjectHeaderPtr m_header;

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

    bool m_written;
};

typedef Alembic::Util::shared_ptr<OwData> OwDataPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif /* _Alembic_AbcCoreGit_OwData_h_ */
