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

#ifndef _Alembic_AbcCoreGit_CpwData_h_
#define _Alembic_AbcCoreGit_CpwData_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/MetaDataMap.h>
#include <Alembic/AbcCoreGit/Git.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class AwImpl;

// data class owned by CpwImpl, or OwImpl if it is a "top" object
// it owns and makes child properties as well as the group GitGroupPtr
// when necessary
class CpwData : public Alembic::Util::enable_shared_from_this<CpwData>
{
public:

    CpwData( const std::string & iName, GitGroupPtr iGroup );

    ~CpwData();

    size_t getNumProperties();

    const AbcA::PropertyHeader & getPropertyHeader( size_t i );

    const AbcA::PropertyHeader * getPropertyHeader( const std::string &iName );

    AbcA::BasePropertyWriterPtr getProperty( const std::string & iName );

    AbcA::ScalarPropertyWriterPtr
    createScalarProperty( AbcA::CompoundPropertyWriterPtr iParent,
        const std::string & iName,
        const AbcA::MetaData & iMetaData,
        const AbcA::DataType & iDataType,
        uint32_t iTimeSamplingIndex );

    AbcA::ArrayPropertyWriterPtr
    createArrayProperty( AbcA::CompoundPropertyWriterPtr iParent,
        const std::string & iName,
        const AbcA::MetaData & iMetaData,
        const AbcA::DataType & iDataType,
        uint32_t iTimeSamplingIndex );

    AbcA::CompoundPropertyWriterPtr
    createCompoundProperty( AbcA::CompoundPropertyWriterPtr iParent,
        const std::string & iName,
        const AbcA::MetaData & iMetaData );

    void writePropertyHeaders( MetaDataMapPtr iMetaDataMap );

    void fillHash( size_t iIndex, Util::uint64_t iHash0,
                   Util::uint64_t iHash1 );

    void computeHash( Util::SpookyHash & ioHash );

    const std::string& name() const               { return m_name; }
    std::string relPathname()                     { GitGroupPtr group = getGroup(); ABCA_ASSERT(group, "invalid group"); return group->relPathname(); }
    std::string absPathname()                     { GitGroupPtr group = getGroup(); ABCA_ASSERT(group, "invalid group"); return group->absPathname(); }

    void writeToDisk();

private:
    friend class CpwImpl;

    GitGroupPtr getGroup() { return m_group; }

    // The group corresponding to this property.
    GitGroupPtr m_group;

    // if m_group gets created it will be given this name
    std::string m_name;

    typedef std::map<std::string, WeakBpwPtr> MadeProperties;

    PropertyHeaderPtrs m_propertyHeaders;
    MadeProperties m_madeProperties;

    // child hashes
    std::vector< Util::uint64_t > m_hashes;
};

typedef Alembic::Util::shared_ptr<CpwData> CpwDataPtr;

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
