//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_CpwImpl_h_
#define _Alembic_AbcCoreGit_CpwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class CpwImpl
    : public AbcA::CompoundPropertyWriter
    , public Alembic::Util::enable_shared_from_this<CpwImpl>
{
public:

    // "top" compound creation called by the object
    CpwImpl( AbcA::ObjectWriterPtr iParent,
             CpwDataPtr iData,
             const AbcA::MetaData & iMeta );

    // child compound creation
    CpwImpl( AbcA::CompoundPropertyWriterPtr iParent,
             GitGroupPtr iParentGroup,
             PropertyHeaderPtr iHeader );

    virtual ~CpwImpl();

    //-*************************************************************************
    // FROM BasePropertyWriter ABSTRACT
    //-*************************************************************************

    virtual const AbcA::PropertyHeader & getHeader() const;

    virtual AbcA::ObjectWriterPtr getObject();

    virtual AbcA::CompoundPropertyWriterPtr getParent();

    virtual AbcA::CompoundPropertyWriterPtr asCompoundPtr();

    //-*************************************************************************
    // FROM CompoundPropertyWriter ABSTRACT
    //-*************************************************************************

    virtual size_t getNumProperties();

    virtual const AbcA::PropertyHeader & getPropertyHeader( size_t i );

    virtual const AbcA::PropertyHeader *
    getPropertyHeader( const std::string &iName );

    virtual AbcA::BasePropertyWriterPtr
    getProperty( const std::string & iName );

    virtual AbcA::ScalarPropertyWriterPtr
    createScalarProperty( const std::string & iName,
        const AbcA::MetaData & iMetaData,
        const AbcA::DataType & iDataType,
        uint32_t iTimeSamplingIndex );

    virtual AbcA::ArrayPropertyWriterPtr
    createArrayProperty( const std::string & iName,
        const AbcA::MetaData & iMetaData,
        const AbcA::DataType & iDataType,
        uint32_t iTimeSamplingIndex );

    virtual AbcA::CompoundPropertyWriterPtr
    createCompoundProperty( const std::string & iName,
        const AbcA::MetaData & iMetaData );

private:

    // The object we belong to.
    AbcA::ObjectWriterPtr m_object;

    // The parent compound property writer.
    AbcA::CompoundPropertyWriterPtr m_parent;

    // The header which defines this property.
    PropertyHeaderPtr m_header;

    // child data, this is owned by the object for "top" compounds
    CpwDataPtr m_data;
};

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
