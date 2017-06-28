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

class OrData;
typedef Alembic::Util::shared_ptr<OrData> OrDataPtr;

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

    const std::string& name() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->name(); }
    std::string relPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->relPathname(); }
    std::string absPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->absPathname(); }

    bool readFromDisk();
    bool readFromDiskChildHeader(size_t i);

    std::string repr(bool extended=false) const;

    friend std::ostream& operator<< ( std::ostream& out, const OrData& value );
    friend std::ostream& operator<< ( std::ostream& out, OrDataPtr value );

private:

    GitGroupPtr m_group;

    struct Child
    {
        std::size_t index;
        std::string name;
        bool read;
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

    bool m_read;
};

inline std::ostream& operator<< ( std::ostream& out, const OrData& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, OrDataPtr value )
{
    out << value->repr();
    return out;
}

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
