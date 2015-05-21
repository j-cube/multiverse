//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_ArImpl_h_
#define _Alembic_AbcCoreGit_ArImpl_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreFactory/IFactory.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
class OrData;

class ArImpl;
typedef Util::shared_ptr<ArImpl> ArImplPtr;

//-*****************************************************************************
class ArImpl
    : public AbcA::ArchiveReader
    , public Alembic::Util::enable_shared_from_this<ArImpl>
{
private:
    friend class ReadArchive;

    ArImpl( const std::string &iFileName, const Alembic::AbcCoreFactory::IOptions& iOptions );

public:

    virtual ~ArImpl();

    //-*************************************************************************
    // ABSTRACT FUNCTIONS
    //-*************************************************************************
    virtual const std::string &getName() const;

    virtual const AbcA::MetaData &getMetaData() const;

    virtual AbcA::ObjectReaderPtr getTop();

    virtual AbcA::TimeSamplingPtr getTimeSampling( Util::uint32_t iIndex );

    virtual AbcA::ArchiveReaderPtr asArchivePtr();

    virtual AbcA::ReadArraySampleCachePtr getReadArraySampleCachePtr()
    {
        return AbcA::ReadArraySampleCachePtr();
    }

    virtual void
    setReadArraySampleCachePtr( AbcA::ReadArraySampleCachePtr iPtr )
    {
    }

    virtual AbcA::index_t getMaxNumSamplesForTimeSamplingIndex(
        Util::uint32_t iIndex );

    virtual Util::uint32_t getNumTimeSamplings()
    {
        return m_timeSamples.size();
    }

    virtual Util::int32_t getArchiveVersion()
    {
        return m_archiveVersion;
    }

    const std::vector< AbcA::MetaData > & getIndexedMetaData();

    std::string relPathname() const;
    std::string absPathname() const;

    Alembic::AbcCoreFactory::IOptions options() const { return m_options; }
    bool hasRevisionSpec() const;
    std::string revisionString();

    bool readFromDisk();

    std::string repr(bool extended=false) const;

    friend std::ostream& operator<< ( std::ostream& out, const ArImpl& value );

private:
    void init();

    std::string m_fileName;

    // libgit2 repo/wt info
    GitRepoPtr m_repo_ptr;

    Alembic::Util::weak_ptr< AbcA::ObjectReader > m_top;
    Alembic::Util::shared_ptr < OrData > m_data;
    Alembic::Util::mutex m_orlock;

    Util::int32_t m_archiveVersion;

    std::vector <  AbcA::TimeSamplingPtr > m_timeSamples;
    std::vector <  AbcA::index_t > m_maxSamples;

    ObjectHeaderPtr m_header;

    std::vector< AbcA::MetaData > m_indexMetaData;

    Alembic::AbcCoreFactory::IOptions m_options;

    bool m_read;
};

inline std::ostream& operator<< ( std::ostream& out, const ArImpl& value )
{
    out << value.repr();
    return out;
}

inline ArImplPtr getArImplPtr(AbcA::ArchiveReaderPtr aArPtr)
{
    ABCA_ASSERT( aArPtr, "Invalid pointer to AbcA::ArchiveReader" );
    Util::shared_ptr< ArImpl > concretePtr =
       Alembic::Util::dynamic_pointer_cast< ArImpl,
         AbcA::ArchiveReader > ( aArPtr );
    return concretePtr;
}

inline ArImplPtr getArImplPtr(Alembic::Abc::IArchive& archive)
{
    Util::shared_ptr< ArImpl > concretePtr =
        Alembic::Util::dynamic_pointer_cast< ArImpl,
          AbcA::ArchiveReader > ( archive.getPtr() );
    return concretePtr;
}

#define CONCRETE_ARPTR(abstract)   getArImplPtr(abstract)

bool trashHistory(const std::string& archivePathname, std::string& errorMessage, const std::string& branchName = "master");

} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
