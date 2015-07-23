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

#ifndef _Alembic_AbcCoreGit_MetaDataMap_h_
#define _Alembic_AbcCoreGit_MetaDataMap_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// convenience class which is meant to map serialized meta data to an index
// It will only hold 254 strings, and won't hold any that are over 256 bytes
class MetaDataMap
{
public:
    MetaDataMap() {};
    ~MetaDataMap() {};

    // will return 0xff if iStr is too long, or we've run out of indices
    // 0 will be returned if iStr is empty
    Util::uint32_t getIndex( const std::string & iStr );
    void write( GitGroupPtr iParent );

    std::vector<std::string> toVector();
    void toJSON(rapidjson::Document& document, rapidjson::Value& dst);

private:
    std::map< std::string, Util::uint32_t > m_map;
};

typedef Alembic::Util::shared_ptr<MetaDataMap> MetaDataMapPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
