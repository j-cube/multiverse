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

#ifndef _Alembic_AbcCoreGit_Foundation_h_
#define _Alembic_AbcCoreGit_Foundation_h_

#include <Alembic/AbcCoreAbstract/All.h>

#include <Alembic/Util/All.h>

#include <vector>
#include <string>
#include <map>

#include <iostream>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define ALEMBIC_GIT_FILE_VERSION 1

//-*****************************************************************************

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
namespace AbcA = ::Alembic::AbcCoreAbstract;

using AbcA::index_t;
using AbcA::chrono_t;

//-*****************************************************************************
typedef Alembic::Util::weak_ptr<AbcA::ObjectWriter> WeakOwPtr;
typedef Alembic::Util::weak_ptr<AbcA::BasePropertyWriter> WeakBpwPtr;

typedef Alembic::Util::weak_ptr<AbcA::ObjectReader> WeakOrPtr;
typedef Alembic::Util::weak_ptr<AbcA::BasePropertyReader> WeakBprPtr;

//-*****************************************************************************
struct PropertyHeaderAndFriends
{
    PropertyHeaderAndFriends()
    {
        isScalarLike = true;
        isHomogenous = true;
        nextSampleIndex = 0;
        firstChangedIndex = 0;
        lastChangedIndex = 0;
        timeSamplingIndex = 0;
    }

    // for compounds
    PropertyHeaderAndFriends( const std::string &iName,
        const AbcA::MetaData &iMetaData ) :
        header( iName, iMetaData )
    {
        isScalarLike = true;
        isHomogenous = true;
        nextSampleIndex = 0;
        firstChangedIndex = 0;
        lastChangedIndex = 0;
        timeSamplingIndex = 0;
    }

    // for scalar and array properties
    PropertyHeaderAndFriends( const std::string &iName,
        AbcA::PropertyType iPropType,
        const AbcA::MetaData &iMetaData,
        const AbcA::DataType &iDataType,
        const AbcA::TimeSamplingPtr & iTsamp,
        Util::uint32_t iTimeSamplingIndex ) :
        header( iName, iPropType, iMetaData, iDataType, iTsamp ),
        timeSamplingIndex( iTimeSamplingIndex )
    {
        isScalarLike = true;
        isHomogenous = true;
        nextSampleIndex = 0;
        firstChangedIndex = 0;
        lastChangedIndex = 0;
    }

    const std::string& name() const                     { return header.getName(); }
    const AbcA::MetaData& metadata() const              { return header.getMetaData(); }
    const AbcA::DataType& datatype() const              { return header.getDataType(); }
    AbcA::PropertyType propertytype() const             { return header.getPropertyType(); }

    // convenience function that makes sure the incoming index is ok, and
    // offsets it to the proper index within the Git group
    size_t verifyIndex( index_t iIndex )
    {
        // Verify sample index
        ABCA_ASSERT( iIndex >= 0 &&
                 iIndex < nextSampleIndex,
                 "Invalid sample index: " << iIndex
                 << ", should be between 0 and " << nextSampleIndex - 1 );

        Util::uint32_t index = ( Util::uint32_t ) iIndex;
        if ( index < firstChangedIndex )
        {
            return 0;
        }
        // constant case
        else if ( firstChangedIndex == lastChangedIndex &&
                  firstChangedIndex == 0 )
        {
            return 0;
        }
        else if ( index >= lastChangedIndex )
        {
            return ( size_t ) ( lastChangedIndex - firstChangedIndex + 1 );
        }

        return ( size_t ) ( index - firstChangedIndex + 1 );
    }

    // The header which defines this property.
    AbcA::PropertyHeader header;

    bool isScalarLike;

    bool isHomogenous;

    // Index of the next sample to write
    Util::uint32_t nextSampleIndex;

    // Index representing the first sample that is different from sample 0
    Util::uint32_t firstChangedIndex;

    // Index representing the last sample in which a change has occured
    // There is no need to repeat samples if they are the same between this
    // index and nextSampleIndex
    Util::uint32_t lastChangedIndex;

    // Index representing which TimeSampling from the ArchiveWriter to use.
    Util::uint32_t timeSamplingIndex;
};

typedef Alembic::Util::shared_ptr<PropertyHeaderAndFriends> PropertyHeaderPtr;
//typedef Alembic::Util::shared_ptr<AbcA::PropertyHeader> PropertyHeaderPtr;
typedef std::vector<PropertyHeaderPtr> PropertyHeaderPtrs;

typedef Alembic::Util::shared_ptr<AbcA::ObjectHeader> ObjectHeaderPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
