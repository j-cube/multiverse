/*****************************************************************************/
/*  Copyright (c) 2013-2015 J CUBE I. Tokyo, Japan. All Rights Reserved.     */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License along  */
/*  with this program; if not, write to the Free Software Foundation, Inc.,  */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              */
/*                                                                           */
/*  J CUBE Inc.                                                              */
/*  6F Azabu Green Terrace                                                   */
/*  3-20-1 Minami-Azabu, Minato-ku, Tokyo                                    */
/*  info@-jcube.jp                                                           */
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
