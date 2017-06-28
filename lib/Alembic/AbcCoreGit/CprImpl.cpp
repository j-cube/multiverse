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
