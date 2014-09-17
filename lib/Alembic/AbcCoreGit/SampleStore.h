//-*****************************************************************************
//
// Copyright (c) 2014,
//
// All rights reserved.
//
//-*****************************************************************************

#ifndef _Alembic_AbcCoreGit_SampleStore_h_
#define _Alembic_AbcCoreGit_SampleStore_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>

#include <json/json.h>


namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
// Abstract Typed Sample Store Base
class AbstractTypedSampleStore
{
public:
    virtual ~AbstractTypedSampleStore() {}

    virtual void copyFrom( const void* iData ) = 0;

    virtual const void *getDataPtr() const = 0;

    virtual void addSample( const void *iSamp ) = 0;
    virtual void addSample( const AbcA::ArraySample& iSamp ) = 0;   // rank-N sample (N >= 1)
    virtual void setFromPreviousSample() = 0;                       // duplicate last added sample

    virtual size_t getNumSamples() const = 0;

    virtual const AbcA::DataType& getDataType() const = 0;
    virtual const AbcA::Dimensions& getDimensions() const = 0;
    virtual int extent() const = 0;
    virtual size_t rank() const = 0;
    virtual AbcA::PlainOldDataType getPod() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual std::string getFullTypeName() const = 0;

    virtual std::string repr(bool extended = false) const = 0;

    virtual Json::Value json() const = 0;
    virtual void fromJson(const Json::Value& root) = 0;

    friend std::ostream& operator<< ( std::ostream& out, const AbstractTypedSampleStore& value );
};

inline std::ostream& operator<< ( std::ostream& out, const AbstractTypedSampleStore& value )
{
        out << value.repr();
        return out;
}

//-*****************************************************************************
// Typed Sample Store
template <typename T>
class TypedSampleStore
    : public AbstractTypedSampleStore
    , public Alembic::Util::enable_shared_from_this< TypedSampleStore<T> >
{
public:
    TypedSampleStore( const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
    TypedSampleStore( const void *iData, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
    virtual ~TypedSampleStore();

    virtual void copyFrom( const std::vector<T>& iData );
    virtual void copyFrom( const T* iData );
    virtual void copyFrom( const void* iData );

    const std::vector<T>& getData() const           { return m_data; }
    std::vector<T>& getData()                       { return m_data; }
    virtual const void *getDataPtr() const          { return static_cast<const void *>(&m_data.front()); }

    const T& operator[](int index) const            { return m_data[index]; }
    T& operator[](int index)                        { return m_data[index]; }

    // a sample is made of X T instances, where X is the extent. This adds only one out of X
    virtual void addSamplePiece( const T& iSamp )   { m_data.push_back(iSamp); }

    virtual void addSample( const T* iSamp );                       // rank-0 sample
    virtual void addSample( const void *iSamp );                    // rank-0 sample

    virtual void addSample( const AbcA::ArraySample& iSamp );       // rank-N sample (N >= 1)

    virtual void setFromPreviousSample();                           // duplicate last added sample

    virtual size_t getNumSamples() const;

    virtual const AbcA::DataType& getDataType() const     { return m_dataType; }
    virtual const AbcA::Dimensions& getDimensions() const { return m_dimensions; }
    virtual int extent() const                            { return m_dataType.getExtent(); }
    virtual size_t rank() const                           { return m_dimensions.rank(); }
    virtual AbcA::PlainOldDataType getPod() const         { return m_dataType.getPod(); }
    virtual std::string getTypeName() const               { return PODName(m_dataType.getPod()); }
    virtual std::string getFullTypeName() const;

    virtual std::string repr(bool extended = false) const;

    virtual Json::Value json() const;
    static Json::Value JsonFromValue( const T& iValue );
    static T ValueFromJson( const Json::Value& jsonValue );

    virtual void fromJson(const Json::Value& root);

private:
    TypedSampleStore();
    TypedSampleStore( const TypedSampleStore<T>& iStore );

    AbcA::DataType m_dataType;
    AbcA::Dimensions m_dimensions;

    std::vector<T> m_data;
};

AbstractTypedSampleStore* BuildSampleStore( const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
