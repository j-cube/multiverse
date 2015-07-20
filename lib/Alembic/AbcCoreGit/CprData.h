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

#ifndef _Alembic_AbcCoreGit_CprData_h_
#define _Alembic_AbcCoreGit_CprData_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class CprData;
typedef Alembic::Util::shared_ptr<CprData> CprDataPtr;

// data class owned by CprImpl, or OrImpl if it is a "top" object
// it owns and makes child properties
class CprData : public Alembic::Util::enable_shared_from_this<CprData>
{
public:

    CprData( GitGroupPtr iGroup,
             std::size_t iThreadId,
             AbcA::ArchiveReader & iArchive,
             const std::vector< AbcA::MetaData > & iIndexedMetaData );

    ~CprData();

    size_t getNumProperties();

    const AbcA::PropertyHeader &
    getPropertyHeader( AbcA::CompoundPropertyReaderPtr iParent, size_t i );

    const AbcA::PropertyHeader *
    getPropertyHeader( AbcA::CompoundPropertyReaderPtr iParent,
                       const std::string &iName );

    AbcA::ScalarPropertyReaderPtr
    getScalarProperty( AbcA::CompoundPropertyReaderPtr iParent,
                       const std::string &iName );

    AbcA::ArrayPropertyReaderPtr
    getArrayProperty( AbcA::CompoundPropertyReaderPtr iParent,
                      const std::string &iName );

    AbcA::CompoundPropertyReaderPtr
    getCompoundProperty( AbcA::CompoundPropertyReaderPtr iParent,
                         const std::string &iName );

    const std::string& name() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->name(); }
    std::string relPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->relPathname(); }
    std::string absPathname() const               { ABCA_ASSERT(m_group, "invalid group"); return m_group->absPathname(); }

    bool readFromDisk();
    bool readFromDiskSubHeader(size_t i);

    std::string repr(bool extended=false) const;

    friend std::ostream& operator<< ( std::ostream& out, const CprData& value );
    friend std::ostream& operator<< ( std::ostream& out, CprDataPtr value );

private:
    GitGroupPtr getGroup() { return m_group; }

    AbcA::ArchiveReader& m_archive;

    GitGroupPtr m_group;

    // Property Headers and Made Property Pointers.
    struct SubProperty
    {
        std::size_t index;
        std::string name;
        bool read;
        PropertyHeaderPtr header;
        WeakBprPtr made;
        Alembic::Util::mutex lock;
    };

    typedef std::map<std::string, size_t> SubPropertiesMap;

    SubProperty * m_subProperties;
    SubPropertiesMap m_subPropertiesMap;

    bool m_read;
};

inline std::ostream& operator<< ( std::ostream& out, const CprData& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, CprDataPtr value )
{
    out << value->repr();
    return out;
}

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
