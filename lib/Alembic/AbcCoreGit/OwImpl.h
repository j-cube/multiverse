/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
/*****************************************************************************/

#ifndef _Alembic_AbcCoreGit_OwImpl_h_
#define _Alembic_AbcCoreGit_OwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/OwData.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class AwImpl;

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
            GitGroupPtr iGroup,
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

    Alembic::Util::shared_ptr< AwImpl > getArchiveImpl() const;

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

inline std::ostream& operator<< ( std::ostream& out, const OwImpl& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, OwImplPtr value )
{
    out << value->repr();
    return out;
}

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
