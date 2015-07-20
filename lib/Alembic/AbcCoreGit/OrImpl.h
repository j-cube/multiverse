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

#ifndef _Alembic_AbcCoreGit_OrImpl_h_
#define _Alembic_AbcCoreGit_OrImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/OrData.h>
#include <Alembic/AbcCoreGit/ArImpl.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class OrImpl;
typedef Util::shared_ptr<OrImpl> OrImplPtr;

//-*****************************************************************************
class OrImpl
    : public AbcA::ObjectReader
    , public Alembic::Util::enable_shared_from_this<OrImpl>
{

public:

    OrImpl( Alembic::Util::shared_ptr< ArImpl > iArchive,
            OrDataPtr iData,
            ObjectHeaderPtr iHeader );

    OrImpl( AbcA::ObjectReaderPtr iParent,
            GitGroupPtr iParentGroup,
            std::size_t iIndex,
            ObjectHeaderPtr iHeader );

    virtual ~OrImpl();

    //-*************************************************************************
    // ABSTRACT
    //-*************************************************************************
    virtual const AbcA::ObjectHeader & getHeader() const;

    virtual AbcA::ArchiveReaderPtr getArchive();

    virtual AbcA::ObjectReaderPtr getParent();

    virtual AbcA::CompoundPropertyReaderPtr getProperties();

    virtual size_t getNumChildren();

    virtual const AbcA::ObjectHeader & getChildHeader( size_t i );

    virtual const AbcA::ObjectHeader * getChildHeader
    ( const std::string &iName );

    virtual AbcA::ObjectReaderPtr getChild( const std::string &iName );

    virtual AbcA::ObjectReaderPtr getChild( size_t i );

    virtual AbcA::ObjectReaderPtr asObjectPtr();

    virtual bool getPropertiesHash( Util::Digest & oDigest );

    virtual bool getChildrenHash( Util::Digest & oDigest );

    Alembic::Util::shared_ptr< ArImpl > getArchiveImpl() const;

private:

    // The parent object
    Alembic::Util::shared_ptr< OrImpl > m_parent;

    Alembic::Util::shared_ptr< ArImpl > m_archive;

    OrDataPtr m_data;

    ObjectHeaderPtr m_header;

};

inline OrImplPtr getOrImplPtr(AbcA::ObjectReaderPtr aOrPtr)
{
    ABCA_ASSERT( aOrPtr, "Invalid pointer to AbcA::ObjectReader" );
    Util::shared_ptr< OrImpl > concretePtr =
       Alembic::Util::dynamic_pointer_cast< OrImpl,
        AbcA::ObjectReader > ( aOrPtr );
    return concretePtr;
}

#define CONCRETE_OWPTR(aOwPtr)   getOwImplPtr(aOwPtr)

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
