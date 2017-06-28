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

#ifndef _Alembic_AbcCoreGit_CpwImpl_h_
#define _Alembic_AbcCoreGit_CpwImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/CpwData.h>
#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/OwImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class AwImpl;

class CpwImpl;
typedef Util::shared_ptr<CpwImpl> CpwImplPtr;

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
             PropertyHeaderPtr iHeader,
             size_t iIndex );

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

    void fillHash( size_t iIndex, Util::uint64_t iHash0,
                   Util::uint64_t iHash1 );

    CpwImplPtr getTParent() const;

    std::string repr(bool extended=false) const;

    const std::string& name() const               { return m_data ? m_data->name() : m_header->name(); }
    std::string relPathname() const               { ABCA_ASSERT(m_data, "invalid data"); return m_data->relPathname(); }
    std::string absPathname() const               { ABCA_ASSERT(m_data, "invalid data"); return m_data->absPathname(); }

    void writeToDisk();

    Alembic::Util::shared_ptr< AwImpl > getArchiveImpl() const
    {
        return getOwImplPtr(m_object)->getArchiveImpl();
    }

private:

    // The object we belong to.
    AbcA::ObjectWriterPtr m_object;

    // The parent compound property writer.
    AbcA::CompoundPropertyWriterPtr m_parent;

    // The header which defines this property.
    PropertyHeaderPtr m_header;

    // child data, this is owned by the object for "top" compounds
    CpwDataPtr m_data;

    // Index position within parent
    size_t m_index;

    bool m_written;
};

inline std::ostream& operator<< ( std::ostream& out, const CpwImpl& value )
{
    out << value.repr();
    return out;
}

inline std::ostream& operator<< ( std::ostream& out, CpwImplPtr value )
{
    out << value->repr();
    return out;
}

inline CpwImplPtr getCpwImplPtr(AbcA::CompoundPropertyWriterPtr aCpwPtr)
{
    ABCA_ASSERT( aCpwPtr, "Invalid pointer to AbcA::CompoundPropertyWriter" );
    Util::shared_ptr< CpwImpl > concretePtr =
       Alembic::Util::dynamic_pointer_cast< CpwImpl,
        AbcA::CompoundPropertyWriter > ( aCpwPtr );
    return concretePtr;
}

#define CONCRETE_CPWPTR(aCpwPtr)   getCpwImplPtr(aCpwPtr)

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
