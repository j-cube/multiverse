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

#ifndef _Alembic_AbcCoreGit_SampleStore_h_
#define _Alembic_AbcCoreGit_SampleStore_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <Alembic/AbcCoreGit/Git.h>
#include <Alembic/AbcCoreGit/AwImpl.h>
#include <Alembic/AbcCoreGit/ArImpl.h>

#include <Alembic/AbcCoreGit/KeyStore.h>

// Use msgpack to store samples
#define MSGPACK_SAMPLES 1

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

class AwImpl;
typedef Util::shared_ptr<AwImpl> AwImplPtr;
class ArImpl;
typedef Util::shared_ptr<ArImpl> ArImplPtr;


//-*****************************************************************************
// Abstract Typed Sample Store Base
class AbstractTypedSampleStore
{
public:
    virtual ~AbstractTypedSampleStore() {}

    virtual RWMode mode() = 0;

    // virtual void copyFrom( const void* iData ) = 0;

    // virtual const void *getDataPtr() const = 0;

    virtual void getSample( void *iIntoLocation, int index ) = 0;
    virtual void getSample( AbcA::ArraySamplePtr& oSample, int index ) = 0;

    virtual size_t addSample( const void *iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims ) = 0;
    virtual size_t addSample( const AbcA::ArraySample& iSamp ) = 0;   // rank-N sample (N >= 1)
    virtual size_t setFromPreviousSample() = 0;                       // duplicate last added sample

    virtual bool getKey( AbcA::index_t iSampleIndex, AbcA::ArraySampleKey& oKey ) = 0;

    virtual size_t getNumSamples() const = 0;

    virtual void getDimensions(AbcA::index_t iSampleIndex, Alembic::Util::Dimensions & oDim) = 0;

    virtual const AbcA::DataType& getDataType() const = 0;
    virtual int extent() const = 0;
    virtual AbcA::PlainOldDataType getPod() const = 0;
    virtual std::string getTypeName() const = 0;
    virtual std::string getFullTypeName() const = 0;

    virtual std::string repr(bool extended = false) const = 0;

    // binary serialization
    virtual std::string pack() = 0;
    virtual void unpack(const std::string& packed) = 0;

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
    TypedSampleStore( AwImplPtr awimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
    TypedSampleStore( ArImplPtr arimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
    // TypedSampleStore( const void *iData, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
    virtual ~TypedSampleStore();

    virtual RWMode mode() { return m_rwmode; }

    // virtual void copyFrom( const std::vector<T>& iData );
    // virtual void copyFrom( const T* iData );
    // virtual void copyFrom( const void* iData );

    // const std::vector<T>& getData() const           { return m_data; }
    // std::vector<T>& getData()                       { return m_data; }
    // virtual const void *getDataPtr() const          { return static_cast<const void *>(&m_data.front()); }

    // const T& operator[](int index) const            { return m_data[index]; }
    // T& operator[](int index)                        { return m_data[index]; }

    // virtual void getSamplePieceT( T* iIntoLocation, size_t dataIndex, int index, int subIndex );
    virtual void getSampleT( T* iIntoLocation, int index );
    virtual void getSample( void *iIntoLocation, int index );
    virtual void getSample( AbcA::ArraySamplePtr& oSample, int index );

    // a sample is made of X T instances, where X is the extent. This adds only one out of X
    // virtual void addSamplePiece( const T& iSamp )   { m_data.push_back(iSamp); }

    virtual size_t adjustKey( const T* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims );
    virtual void   checkSamplePieceForAdd( const T& iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims );
    virtual size_t addSample( const T* iSamp, AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims );
    virtual size_t addSample( const T* iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims );
    virtual size_t addSample( const void *iSamp, const AbcA::ArraySample::Key& key, const AbcA::Dimensions& dims );

    virtual size_t addSample( const AbcA::ArraySample& iSamp );       // rank-N sample (N >= 1)

    virtual size_t setFromPreviousSample();                           // duplicate last added sample

    virtual bool getKey( AbcA::index_t iSampleIndex, AbcA::ArraySampleKey& oKey );

    virtual size_t getNumSamples() const;

    virtual void getDimensions(AbcA::index_t iSampleIndex, Alembic::Util::Dimensions & oDim)
    {
        size_t kid = sampleIndexToKid(static_cast<size_t>(iSampleIndex));
        oDim = getDimensions(kid);
    }

    virtual const AbcA::DataType& getDataType() const     { return m_dataType; }
    virtual int extent() const                            { return m_dataType.getExtent(); }
    virtual AbcA::PlainOldDataType getPod() const         { return m_dataType.getPod(); }
    virtual std::string getTypeName() const               { return PODName(m_dataType.getPod()); }
    virtual std::string getFullTypeName() const;

    virtual std::string repr(bool extended = false) const;

    // binary serialization
    virtual std::string pack();
    virtual void unpack(const std::string& packed);

protected:
#if 0
    bool hasKey(const AbcA::ArraySample::Key& key) const         { return (m_key_to_kid.count(key) != 0); }
    bool hasKid(size_t kid) const                                { return (m_kid_to_key.count(kid) != 0); }
    size_t KeyToKid(const AbcA::ArraySample::Key& key) const     { return m_key_to_kid[key]; }
    size_t KeyToKid(const AbcA::ArraySample::Key& key)           { return m_key_to_kid[key]; }
    const AbcA::ArraySample::Key& KidToKey(size_t kid) const     { return m_kid_to_key[kid]; }
    const AbcA::ArraySample::Key& KidToKey(size_t kid)           { return m_kid_to_key[kid]; }
    size_t addKey(const AbcA::ArraySample::Key& key)
    {
        if (! m_key_to_kid.count(key)) {
            size_t kid = m_next_kid++;
            m_key_to_kid[key] = kid;
            m_kid_to_key[kid] = key;
            return kid;
        } else
            return m_key_to_kid[key];
    }
#endif
    typedef KeyStore<T>* KeyStorePtr;

    KeyStorePtr ks();

    bool hasKey(const AbcA::ArraySample::Key& key)               { return ks()->hasKey(key);   }
    bool hasKid(size_t kid)                                      { return ks()->hasKid(kid);   }
    size_t KeyToKid(const AbcA::ArraySample::Key& key)           { return ks()->KeyToKid(key); }
    const AbcA::ArraySample::Key& KidToKey(size_t kid)           { return ks()->KidToKey(kid); }
    size_t addKey(const AbcA::ArraySample::Key& key)             { return ks()->addKey(key);   }
    size_t addKey(const AbcA::ArraySample::Key& key, const std::vector<T>& data)
        { return ks()->addKey(key, data); }
    bool hasData(size_t kid)                                     { return ks()->hasData(kid); }
    bool hasData(const AbcA::ArraySample::Key& key)              { return ks()->hasData(key); }
    void addData(size_t kid, const std::vector<T>& data)         { return ks()->addData(kid, data); }
    void addData(size_t kid, const AbcA::ArraySample::Key& key, const std::vector<T>& data)
                                                                 { return ks()->addData(kid, key, data); }
    const std::vector<T>& data(size_t kid)                       { return ks()->data(kid); }
    const std::vector<T>& data(const AbcA::ArraySample::Key& key) { return ks()->data(key); }

    bool hasIndex(size_t sampleIndex) const           { return (m_index_to_kid.count(sampleIndex) != 0); }
    size_t sampleIndexToKid(size_t sampleIndex) const { return m_index_to_kid.find(sampleIndex)->second; }
    size_t sampleIndexToKid(size_t sampleIndex)       { return m_index_to_kid.find(sampleIndex)->second; }

    bool hasDimensions(size_t kid) const                          { return (m_kid_dims.count(kid) != 0); }
    const AbcA::Dimensions& getDimensions(size_t kid) const       { return m_kid_dims.find(kid)->second; }
    AbcA::Dimensions getDimensions(size_t kid)                    { return m_kid_dims.find(kid)->second; }
    void setDimensions(size_t kid, const AbcA::Dimensions& dims)  { m_kid_dims[kid] = dims; }

    const AbcA::ArraySample::Key& sampleIndexToKey(size_t sampleIndex) const { size_t kid = sampleIndexToKid(sampleIndex); return KidToKey(kid); }
    const AbcA::ArraySample::Key& sampleIndexToKey(size_t sampleIndex)       { size_t kid = sampleIndexToKid(sampleIndex); return KidToKey(kid); }

    const std::vector<T>& kidToSampleData(size_t kid)            { return ks()->data(kid); }

    const std::vector<T>& sampleIndexToSampleData(size_t sampleIndex) { size_t kid = sampleIndexToKid(sampleIndex); return kidToSampleData(kid); }

    AbcA::DataType& dataType() { return m_dataType; }

    GitRepoPtr repo();
    GitGroupPtr group();

private:
    TypedSampleStore();
    TypedSampleStore( const TypedSampleStore<T>& iStore );

    AwImplPtr m_awimpl_ptr;
    ArImplPtr m_arimpl_ptr;
    RWMode m_rwmode;

    AbcA::DataType m_dataType;
    AbcA::Dimensions m_dimensions;

    std::map< size_t, size_t > m_index_to_kid;                                   // sample index to kid
    std::map< size_t, std::vector<size_t> > m_kid_indexes;                     // kid to array of sample indexes
    // std::map< size_t, std::vector<T> > m_kid_to_data;                          // kid to sample data
    std::map< size_t, AbcA::Dimensions > m_kid_dims;                           // kid to dimensions
    size_t m_next_index;                                                       // next sample index

    // std::map<size_t, AbcA::ArraySample::Key> m_index_key;                      // sample index to key
    // std::map< AbcA::ArraySample::Key, std::vector<size_t> > m_key_indexes;     // key to array of sample indexes
    // std::map< AbcA::ArraySample::Key, std::vector<T> > m_key_data;             // key to sample
    // size_t m_next_index;                                                       // next sample index

    // std::vector<T> m_data;
    // std::map< std::string, std::vector<T> > m_key_data;             // key to sample
    // std::map< std::string, std::vector<size_t> > m_key_pos;         // key to array of T-index
    // std::map<size_t, AbcA::ArraySample::Key> m_pos_key;             // T-index to key

};

AbstractTypedSampleStore* BuildSampleStore( AwImplPtr awimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );
AbstractTypedSampleStore* BuildSampleStore( ArImplPtr arimpl_ptr, const AbcA::DataType &iDataType, const AbcA::Dimensions &iDims );


} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
