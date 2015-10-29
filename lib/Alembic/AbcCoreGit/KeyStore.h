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

#ifndef _Alembic_AbcCoreGit_KeyStore_h_
#define _Alembic_AbcCoreGit_KeyStore_h_

#include <Alembic/AbcCoreGit/Foundation.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <typeinfo>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

enum RWMode { READ = 0, WRITE = 1 };

const size_t BUNDLE_SAMPLES_BELOW = 200;        // bundle samples together if below this size (in bytes)

struct KeyStoreBase
{
    virtual ~KeyStoreBase();
    virtual const std::string typestr() const   { return "UNKNOWN"; }

    virtual RWMode mode() const                 { return READ; }

    virtual bool saved() const                  { return false; }
    virtual void saved(bool value) = 0;

    virtual bool loaded() const                 { return false; }
    virtual void loaded(bool value) = 0;

    virtual bool writeToDisk()                  { return false; }
    virtual bool readFromDisk()                 { return false; }
};

template <typename T>
struct KeyStore : public KeyStoreBase
{
    KeyStore(GitGroupPtr groupPtr, RWMode rwmode);
    virtual ~KeyStore();

    virtual const std::string typestr() const;

    virtual RWMode mode() const { return m_rwmode; }

    bool hasKey(const AbcA::ArraySample::Key& key)               { return (m_key_to_kid.count(key) != 0); }
    bool hasKid(size_t kid)                                      { return (m_kid_to_key.count(kid) != 0); }
    size_t KeyToKid(const AbcA::ArraySample::Key& key)           { return m_key_to_kid[key]; }
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

    size_t addKey(const AbcA::ArraySample::Key& key, const std::vector<T>& data)
    {
        size_t kid = addKey(key);
        if (! m_has_kid_data.count(kid))
            addData(kid, key, data);
        assert(m_has_kid_data.count(kid));
        return kid;
    }

    bool hasData(size_t kid)                        { return (m_has_kid_data.count(kid)); }
    bool hasData(const AbcA::ArraySample::Key& key) { size_t kid = KeyToKid(key); return hasData(kid); }

    void addData(size_t kid, const std::vector<T>& data)
    {
        addData(kid, KidToKey(kid), data);
    }

    void addData(size_t kid, const AbcA::ArraySample::Key& key, const std::vector<T>& data)
    {
        assert(! m_has_kid_data.count(kid));
        m_has_kid_data[kid] = true;
        writeToDiskSampleData(kid, key, data);
    }

    const std::vector<T>& data(size_t kid)
    {
        assert(m_has_kid_data.count(kid));
        assert(m_kid_to_data.count(kid));
        return m_kid_to_data[kid];
    }

    const std::vector<T>& data(const AbcA::ArraySample::Key& key)
    {
        size_t kid = KeyToKid(key);
        assert(m_has_kid_data.count(kid));
        assert(m_kid_to_data.count(kid));
        return m_kid_to_data[kid];
    }

    virtual bool saved() const     { return m_saved; }
    virtual void saved(bool value) { m_saved = value; }

    virtual bool loaded() const     { return m_loaded; }
    virtual void loaded(bool value) { m_loaded = value; }

    // std::string pack();
    // bool unpack(const std::string& packed);

    virtual bool writeToDisk();
    virtual bool readFromDisk();

    bool writeToDiskSample(const std::string& basename, std::map< size_t, AbcA::ArraySample::Key >::const_iterator& p_it, size_t& npacked);
    bool readFromDiskSample(GitTreePtr gitTree, const std::string& basename, size_t kid, size_t& unpacked);

    // std::string packSample(size_t kid, const AbcA::ArraySample::Key& key);
    bool unpackSample(const std::string& packedSample, size_t kid);

    void ensureWriteInfo()   { if (! m_write_info) _ensureWriteInfo(); }
    void _ensureWriteInfo();
    bool writeToDiskSampleData(size_t kid, const AbcA::ArraySample::Key& key, const std::vector<T>& data);

    GitGroupPtr m_group;
    RWMode m_rwmode;

    std::map< AbcA::ArraySample::Key, size_t > m_key_to_kid;                   // key to progressive key id
    std::map< size_t, AbcA::ArraySample::Key > m_kid_to_key;                   // progressive key id to key
    std::map< size_t, std::vector<T> > m_kid_to_data;                          // kid to sample data
    std::map< size_t, bool > m_has_kid_data;                                   // has kid AND its data
    std::map< size_t, std::vector<T> > m_bundled_data;                         // sample data to write lazily in bundled form
    size_t m_next_kid;                                                         // next key id
    bool m_saved;
    bool m_loaded;

    size_t m_samples_unbundled;
    size_t m_samples_bundled;

    // write cached values
    bool m_write_info;                                                         // write info ready (m_git_tree, m_base_path, ...)
    // GitTreePtr m_git_tree;
    std::string m_basepath;
    std::string m_basename;
    size_t m_write_packed;
};

typedef Util::shared_ptr<KeyStoreBase> KeyStoreBasePtr;

class TypeInfoWrapper
{
public:
    TypeInfoWrapper(const std::type_info& info) : m_info(info)
    {}

    // TypeInfoWrapper& operator=(const TypeInfoWrapper& other)
    // {
    //     m_info = other.typeinfo();
    //     returh *this;
    // }

    const std::type_info& typeinfo() const { return m_info; }

    // Functions required by a key into a std::map.

    bool operator<(const TypeInfoWrapper& other) const
    {
        return typeinfo().before(other.typeinfo());
    }

private:
    const std::type_info& m_info;
};

class KeyStoreMap
{
    /*
     * Maintain different KeyStores based on their template type
     * (using typeid() operator to get type information)
     */

public:
    KeyStoreMap(GitGroupPtr groupPtr, RWMode rwmode) :
        m_group(groupPtr), m_rwmode(rwmode)
    {}
    ~KeyStoreMap();

    bool writeToDisk();

    void add(KeyStoreBase* keyStoreP)
    {
        m_map[TypeInfoWrapper(typeid(*keyStoreP))] = keyStoreP;
    }

    KeyStoreBase* get(const std::type_info& typeinfo)
    {
        return m_map[TypeInfoWrapper(typeinfo)];
    }

    template <class T> KeyStore<T>* get()
    {
        KeyStoreBase* ksbptr = m_map[TypeInfoWrapper(typeid(T))];
        return dynamic_cast< KeyStore<T>* >( ksbptr );
    }

    template <class T> KeyStore<T>* getOrCreate()
    {
        TypeInfoWrapper tiw = TypeInfoWrapper(typeid(T));
        if (m_map.count(tiw))
        {
            KeyStoreBase* ksbptr = m_map[tiw];
            ksbptr->readFromDisk();
            return dynamic_cast< KeyStore<T>* >( ksbptr );
        }
        KeyStore<T>* ksp = new KeyStore<T>(m_group, m_rwmode);
        m_map[tiw] = ksp;
        return ksp;
    }

private:
    GitGroupPtr m_group;
    RWMode m_rwmode;

    std::map <TypeInfoWrapper, KeyStoreBase*> m_map;
};

template <typename T>
struct TypeStr { static const char *name; };

template <typename T>
inline std::string GetTypeStr() { return TypeStr<T>::name; }



} // End namespace ALEMBIC_VERSION_NS

using namespace ALEMBIC_VERSION_NS;

} // End namespace AbcCoreGit
} // End namespace Alembic

#endif
