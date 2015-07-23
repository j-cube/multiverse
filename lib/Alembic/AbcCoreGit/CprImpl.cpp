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

#include <Alembic/AbcCoreGit/CprImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
//-*****************************************************************************
// CLASS
//-*****************************************************************************
//-*****************************************************************************

//-*****************************************************************************
CprImpl::CprImpl( AbcA::CompoundPropertyReaderPtr iParent,
                  GitGroupPtr iGroup,
                  PropertyHeaderPtr iHeader,
                  std::size_t iThreadId,
                  const std::vector< AbcA::MetaData > & iIndexedMetaData )
    : m_parent( iParent )
    , m_header( iHeader )
{
    ABCA_ASSERT( m_parent, "Invalid parent in CprImpl(Compound)" );
    ABCA_ASSERT( m_header, "invalid header in CprImpl(Compound)" );

    AbcA::PropertyType pType = m_header->header.getPropertyType();
    if ( pType != AbcA::kCompoundProperty )
    {
        ABCA_THROW( "Tried to create compound property with the wrong "
                    "property type: " << pType );
    }

    // Set object.
    AbcA::ObjectReaderPtr optr = m_parent->getObject();
    ABCA_ASSERT( optr, "Invalid object in CprImpl::CprImpl(Compound)" );
    m_object = optr;

    m_data.reset( new CprData( iGroup, iThreadId, *( m_object->getArchive() ),
                               iIndexedMetaData ) );
}

//-*****************************************************************************
CprImpl::CprImpl( AbcA::ObjectReaderPtr iObject,
                  CprDataPtr iData )
    : m_object( iObject )
    , m_data( iData )
{
    ABCA_ASSERT( m_object, "Invalid object in CprImpl(Object)" );
    ABCA_ASSERT( m_data, "Invalid data in CprImpl(Object)" );

    std::string emptyName;
    m_header.reset( new PropertyHeaderAndFriends( emptyName,
                                              m_object->getMetaData() ) );
}

//-*****************************************************************************
CprImpl::~CprImpl()
{
    // Nothing
}

//-*****************************************************************************
const AbcA::PropertyHeader &CprImpl::getHeader() const
{
    ABCA_ASSERT( m_header, "Invalid header" );
    return m_header->header;
}

AbcA::ObjectReaderPtr CprImpl::getObject()
{
    return m_object;
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr CprImpl::getParent()
{
    return m_parent;
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr CprImpl::asCompoundPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t CprImpl::getNumProperties()
{
    return m_data->getNumProperties();
}

//-*****************************************************************************
const AbcA::PropertyHeader & CprImpl::getPropertyHeader( size_t i )
{
    return m_data->getPropertyHeader( asCompoundPtr(), i );
}

//-*****************************************************************************
const AbcA::PropertyHeader *
CprImpl::getPropertyHeader( const std::string &iName )
{
    return m_data->getPropertyHeader( asCompoundPtr(), iName );
}

//-*****************************************************************************
AbcA::ScalarPropertyReaderPtr
CprImpl::getScalarProperty( const std::string &iName )
{
    return m_data->getScalarProperty( asCompoundPtr(), iName );
}

//-*****************************************************************************
AbcA::ArrayPropertyReaderPtr
CprImpl::getArrayProperty( const std::string &iName )
{
    return m_data->getArrayProperty( asCompoundPtr(), iName );
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr
CprImpl::getCompoundProperty( const std::string &iName )
{
    return m_data->getCompoundProperty( asCompoundPtr(), iName );
}

CprImplPtr CprImpl::getTParent() const
{
    Util::shared_ptr< CprImpl > parent =
       Alembic::Util::dynamic_pointer_cast< CprImpl,
        AbcA::CompoundPropertyReader > ( m_parent );
    return parent;
}

std::string CprImpl::repr(bool extended) const
{
    std::ostringstream ss;
    if (extended)
    {
        if (m_parent)
        {
            CprImplPtr parentPtr = getTParent();

            ss << "<CprImpl(parent:" << parentPtr->repr() << ", header:'" << m_header->name() << "')>";
        } else
        {
            ss << "<CprImpl(TOP, " << ", header:'" << m_header->name() << "')>";
        }
    } else
    {
        ss << "'" << m_header->name() << "'";
    }
    return ss.str();
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
