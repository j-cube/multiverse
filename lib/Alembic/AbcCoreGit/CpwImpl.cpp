//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#include <Alembic/AbcCoreGit/CpwImpl.h>
#include <Alembic/AbcCoreGit/WriteUtil.h>
#include <Alembic/AbcCoreGit/Utils.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************

// "top" compound creation called by object writers.
CpwImpl::CpwImpl( AbcA::ObjectWriterPtr iParent,
                  CpwDataPtr iData,
                  const AbcA::MetaData & iMeta )
    : m_object( iParent)
    , m_header( new PropertyHeaderAndFriends( "", iMeta ) )
    , m_data( iData )
{
    TRACE("CpwImpl::CpwImpl(ObjectWriterPtr)");

    // we don't need to write the property info, the object has done it already

    ABCA_ASSERT( m_object, "Invalid object" );
    ABCA_ASSERT( m_data, "Invalid compound data" );
}

// With the compound property writer as an input.
CpwImpl::CpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
                  GitGroupPtr iParentGroup,
                  PropertyHeaderPtr iHeader )
  : m_parent( iParent )
  , m_header( iHeader )
{
    // Check the validity of all inputs.
    ABCA_ASSERT( m_parent, "Invalid parent" );
    ABCA_ASSERT( m_header, "Invalid header" );

    if (iParentGroup)
        TRACE("CpwImpl::CpwImpl(CompoundPropertyWriterPtr, parentGroup:" << iParentGroup->repr() <<
            " name:" << m_header->name() << ")");
    else
        TRACE("CpwImpl::CpwImpl(parent, parentGroup:NULL, name:" << m_header->name() << ")");

    // Set the object.
    m_object = m_parent->getObject();
    ABCA_ASSERT( m_object, "Invalid parent object" );

    ABCA_ASSERT( m_header->header.getName() != "" &&
                 m_header->header.getName().find('/') == std::string::npos,
                 "Invalid name" );

    m_data.reset( new CpwData( m_header->name(), iParentGroup ) );

    // Write the property header.
    UNIMPLEMENTED("WritePropertyInfo()");
#if 0
    WritePropertyInfo( iParentGroup, m_header, false, 0, 0, 0, 0 );
#endif
}

//-*****************************************************************************
CpwImpl::~CpwImpl()
{
    // Nothing!
}

//-*****************************************************************************
const AbcA::PropertyHeader &CpwImpl::getHeader() const
{
    return m_header->header;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr CpwImpl::getObject()
{
    return m_object;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr CpwImpl::getParent()
{
    // this will be NULL for "top" compound properties
    return m_parent;
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr CpwImpl::asCompoundPtr()
{
    return shared_from_this();
}

//-*****************************************************************************
size_t CpwImpl::getNumProperties()
{
    return m_data->getNumProperties();
}

//-*****************************************************************************
const AbcA::PropertyHeader & CpwImpl::getPropertyHeader( size_t i )
{
    return m_data->getPropertyHeader( i );
}

//-*****************************************************************************
const AbcA::PropertyHeader *
CpwImpl::getPropertyHeader( const std::string &iName )
{
    return m_data->getPropertyHeader( iName );
}

//-*****************************************************************************
AbcA::BasePropertyWriterPtr CpwImpl::getProperty( const std::string & iName )
{
    return m_data->getProperty( iName );
}

//-*****************************************************************************
AbcA::ScalarPropertyWriterPtr
CpwImpl::createScalarProperty( const std::string & iName,
                      const AbcA::MetaData & iMetaData,
                      const AbcA::DataType & iDataType,
                      uint32_t iTimeSamplingIndex )
{
    TRACE("call m_data->createScalarProperty()");
    return m_data->createScalarProperty( asCompoundPtr(), iName, iMetaData,
                                         iDataType, iTimeSamplingIndex );
}

//-*****************************************************************************
AbcA::ArrayPropertyWriterPtr
CpwImpl::createArrayProperty( const std::string & iName,
                              const AbcA::MetaData & iMetaData,
                              const AbcA::DataType & iDataType,
                              uint32_t iTimeSamplingIndex )
{
    TRACE("call m_data->createArrayProperty()");
    return m_data->createArrayProperty( asCompoundPtr(), iName, iMetaData,
                                        iDataType, iTimeSamplingIndex );
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr
CpwImpl::createCompoundProperty( const std::string & iName,
                                 const AbcA::MetaData & iMetaData )
{
    TRACE("call m_data->createCompoundProperty");
    return m_data->createCompoundProperty( asCompoundPtr(), iName, iMetaData );
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic

