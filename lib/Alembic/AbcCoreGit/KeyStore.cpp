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

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

#include <Alembic/AbcCoreGit/KeyStore.h>
#include <Alembic/AbcCoreGit/Utils.h>
#include <Alembic/AbcCoreGit/Git.h>

#include <Alembic/AbcCoreGit/msgpack_support.h>

#include <msgpack.hpp>


namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {


template <typename T>
const char* TypeStr<T>::name = "unknown";

template <>
const char* TypeStr< Util::bool_t >::name = "bool";

template <>
const char* TypeStr< Util::uint8_t >::name = "uint8";

template <>
const char* TypeStr< Util::int8_t >::name = "int8";

template <>
const char* TypeStr< Util::uint16_t >::name = "uint16";

template <>
const char* TypeStr< Util::int16_t >::name = "int16";

template <>
const char* TypeStr< Util::uint32_t >::name = "uint32";

template <>
const char* TypeStr< Util::int32_t >::name = "int32";

template <>
const char* TypeStr< Util::uint64_t >::name = "uint64";

template <>
const char* TypeStr< Util::int64_t >::name = "int64";

template <>
const char* TypeStr< Util::float16_t >::name = "float16";

template <>
const char* TypeStr< Util::float32_t >::name = "float32";

template <>
const char* TypeStr< Util::float64_t >::name = "float64";

template <>
const char* TypeStr<char>::name = "char";

template <>
const char* TypeStr< Util::string >::name = "string";

template <>
const char* TypeStr< Util::wstring >::name = "wstring";

static std::string pod2str(const Alembic::Util::PlainOldDataType& pod)
{
    std::ostringstream ss;
    ss << PODName( pod );
    return ss.str();
}

static uint8_t hexchar2int(char input)
{
    if ((input >= '0') && (input <= '9'))
        return (input - '0');
    if ((input >= 'A') && (input <= 'F'))
        return (input - 'A') + 10;
    if ((input >= 'a') && (input <= 'f'))
        return (input - 'a') + 10;
    ABCA_THROW( "invalid input char" );
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
static size_t hex2bin(uint8_t *dst, const char* src)
{
    uint8_t *dst_orig = dst;

    while (*src && src[1])
    {
        *dst      = hexchar2int(*(src++)) << 4;
        *(dst++) |= hexchar2int(*(src++));
    }

    return (dst - dst_orig);
}

/* --------------------------------------------------------------------
 *
 *   KeyStoreMap
 *
 * -------------------------------------------------------------------- */

KeyStoreMap::~KeyStoreMap()
{
    std::map <TypeInfoWrapper, KeyStoreBase*>::iterator it;
    for (it = m_map.begin(); it != m_map.end(); ++it)
    {
#if GLOBAL_TRACE_ENABLE
        const TypeInfoWrapper& tiw = (*it).first;
#endif
        KeyStoreBase* ksptr        = (*it).second;
        if (ksptr)
        {
            assert(ksptr);
            TRACE("calling destructor for KeyStore for type " << ksptr->typestr() << " (" << tiw.typeinfo().name() << ")");
            delete ksptr;
        }
        ksptr = NULL;
    }
}

bool KeyStoreMap::writeToDisk()
{
    std::map <TypeInfoWrapper, KeyStoreBase*>::iterator it;
    for (it = m_map.begin(); it != m_map.end(); ++it)
    {
#if GLOBAL_TRACE_ENABLE
        const TypeInfoWrapper& tiw = (*it).first;
#endif
        KeyStoreBase* ksptr   = (*it).second;
        if (ksptr)
        {
            assert(ksptr);
            TRACE("calling writeToDisk() for type " << ksptr->typestr() << " (" << tiw.typeinfo().name() << ")");
            ksptr->writeToDisk();
        }
    }
    return true;
}


/* --------------------------------------------------------------------
 *
 *   KeyStore
 *
 * -------------------------------------------------------------------- */

KeyStoreBase::~KeyStoreBase()
{
    if (mode() == WRITE)
    {
        if (! saved())
        {
            writeToDisk();
        }
    }
}

template <typename T>
KeyStore<T>::KeyStore(GitGroupPtr groupPtr, RWMode rwmode) :
    m_group(groupPtr), m_rwmode(rwmode), m_next_kid(0), m_saved(false), m_loaded(false),
    m_samples_unbundled(0), m_samples_bundled(0),
    m_write_info(false), m_write_packed(0)
{
    TRACE("KeyStore<T>::KeyStore() type: " << GetTypeStr<T>());
    if (mode() == READ)
    {
        if (! loaded())
        {
            readFromDisk();
        }
    }
}

template <typename T>
KeyStore<T>::~KeyStore()
{
}

template <typename T>
const std::string KeyStore<T>::typestr() const
{
    return GetTypeStr<T>();
}

template <typename T>
void KeyStore<T>::_ensureWriteInfo()
{
    assert(m_rwmode == WRITE);

    if (m_write_info)
        return;

    assert(! m_write_info);

    // m_git_tree = m_group->tree();
    m_basename = "keystore_" + GetTypeStr<T>();
    m_basepath = pathjoin(m_group->absPathname(), m_basename);

    m_write_info = true;
}

template <typename T>
bool KeyStore<T>::writeToDiskSampleData(size_t kid, const AbcA::ArraySample::Key& key, const std::vector<T>& data)
{
    assert(m_rwmode == WRITE);

    ensureWriteInfo();

    size_t estimated_size = data.size() * sizeof(T);

    if (estimated_size >= BUNDLE_SAMPLES_BELOW)
    {
        TRACE("KeyStore::writeToDiskSampleData() estimated_size:" << estimated_size << " >= " << BUNDLE_SAMPLES_BELOW);
        std::stringstream buffer;
        msgpack::packer<std::stringstream> pk(&buffer);

        mp_pack(pk, data);

        std::string packedSample = buffer.str();

        std::ostringstream ss;
        ss << "_" << key.digest.str();
        std::string suffix = ss.str();

        std::string name = m_basename + suffix + ".bin";

        m_group->add_file_from_memory(name, packedSample);

        m_write_packed += packedSample.length();
        m_samples_unbundled++;
    } else
    {
        // write later, in bundled form
        m_bundled_data[kid] = data;
    }

    return true;
}

template <typename T>
bool KeyStore<T>::writeToDisk()
{
    if (m_rwmode != WRITE)
        return false;

    assert(m_rwmode == WRITE);

    std::string basename = "keystore_" + GetTypeStr<T>();
    std::string basepath = pathjoin(m_group->absPathname(), basename);

    if (saved())
    {
        TRACE("KeyStore::writeToDisk() base path:'" << basepath << "_*' (skipping, already written)");
        ABCA_ASSERT( saved(), "data not written" );
        return true;
    }

    assert(! saved());

    TRACE("KeyStore::writeToDisk() base path:'" << basepath << "_*' (WRITING)");

    // pack & write header

    std::string name_header = basename + "_header" + ".bin";

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    mp_pack(pk, static_cast<size_t>(m_kid_to_key.size()));
    mp_pack(pk, static_cast<size_t>(m_next_kid));

    size_t n_samples = 0;
    {
        std::map< size_t, AbcA::ArraySample::Key >::const_iterator p_it;
        for (p_it = m_kid_to_key.begin(); p_it != m_kid_to_key.end(); ++p_it)
        {
            size_t kid                        = (*p_it).first;
            const AbcA::ArraySample::Key& key = (*p_it).second;

            msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string >
                tuple(kid, key.numBytes, pod2str(key.origPOD), pod2str(key.readPOD), key.digest.str());

            mp_pack(pk, tuple);
            n_samples++;
        }
    }

    std::string packedHeader = buffer.str();
    m_group->add_file_from_memory(name_header, packedHeader);

    size_t npacked = packedHeader.length();
    m_write_packed += npacked;
    TRACE("header packed to " << npacked << " bytes for # " << n_samples << " (different) samples of type " << GetTypeStr<T>());

    // pack & write bundled samples
    {
        std::string name_bundle = basename + "_bundle" + ".bin";

        std::stringstream buffer;
        msgpack::packer<std::stringstream> pk(&buffer);

        mp_pack(pk, static_cast<size_t>(m_bundled_data.size()));

        m_samples_bundled = 0;
        typename std::map< size_t, std::vector<T> >::const_iterator b_it;
        for (b_it = m_bundled_data.begin(); b_it != m_bundled_data.end(); ++b_it)
        {
            size_t kid                 = (*b_it).first;
            const std::vector<T>& data = (*b_it).second;

            mp_pack(pk, kid);
            mp_pack(pk, data);

            m_samples_bundled++;
        }

        std::string packedBundle = buffer.str();
        m_group->add_file_from_memory(name_bundle, packedBundle);

        size_t npacked = packedBundle.length();
        m_write_packed += npacked;
        TRACE("bundle packed to " << npacked << " bytes for # " << m_samples_bundled << " (different) bundled samples of type " << GetTypeStr<T>());
    }

#if 0
    // pack & write samples

    bool all_ok = true, ok;

    std::map< size_t, AbcA::ArraySample::Key >::const_iterator p_it;
    for (p_it = m_kid_to_key.begin(); p_it != m_kid_to_key.end(); ++p_it)
    {
        // size_t kid                        = (*p_it).first;
        // const AbcA::ArraySample::Key& key = (*p_it).second;

        ok = writeToDiskSample(basename, p_it, npacked);
        all_ok = all_ok && ok;
        if (! ok)
            break;

        all_npacked += npacked;
    }
#endif

    saved(true);
    TRACE("packed " << m_write_packed << " total bytes for # " << n_samples << " (different) samples of type " << GetTypeStr<T>());

    ABCA_ASSERT( saved(), "data not written" );
    return true;
}

template <typename T>
bool KeyStore<T>::readFromDisk()
{
    if (m_rwmode != READ)
        return false;

    assert(m_rwmode == READ);

    std::string basename = "keystore_" + GetTypeStr<T>();
    std::string basepath = pathjoin(m_group->absPathname(), basename);

    if (loaded())
    {
        TRACE("KeyStore::readFromDisk() base path:'" << basepath << "_*' (skipping, already read)");
        ABCA_ASSERT( loaded(), "data not read" );
        return true;
    }

    assert(! loaded());

    TRACE("KeyStore::readFromDisk() base path:'" << basepath << "_*' (READING)");

    size_t all_unpacked = 0, unpacked;

    GitTreePtr gitTree = m_group->tree();

    // read & unpack header

    std::string name_header = basename + "_header" + ".bin";
    boost::optional<std::string> optBinHeaderContents = gitTree->getChildFile(name_header);
#if 0
    if (! optBinHeaderContents)
    {
        ABCA_THROW( "can't read git blob '" << pathjoin(m_group->absPathname(), name_header) << "'" );
        return false;
    }
#endif

    size_t v_n_kid = 0;
    size_t v_next_kid = 0;

    if (optBinHeaderContents)
    {
        std::string packedHeader = *optBinHeaderContents;

        {
            msgpack::unpacker pac;

            // copy the buffer data to the unpacker object
            pac.reserve_buffer(packedHeader.size());
            memcpy(pac.buffer(), packedHeader.data(), packedHeader.size());
            pac.buffer_consumed(packedHeader.size());

            // deserialize it.
            msgpack::unpacked msg;

            m_key_to_kid.clear();
            m_kid_to_key.clear();
            m_next_kid = 0;

            pac.next(&msg);
            msgpack::object pko = msg.get();
            mp_unpack(pko, v_n_kid);

            pac.next(&msg);
            pko = msg.get();
            mp_unpack(pko, v_next_kid);

            for (size_t i = 0; i < v_n_kid; ++i)
            {
                msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string > tuple;

                pac.next(&msg);
                msgpack::object pko = msg.get();
                mp_unpack(pko, tuple);

                size_t      k_kid       = tuple.get<0>();
                size_t      k_num_bytes = tuple.get<1>();
                std::string k_orig_pod  = tuple.get<2>();
                std::string k_read_pod  = tuple.get<3>();
                std::string k_digest    = tuple.get<4>();

                AbcA::ArraySample::Key key;

                key.numBytes = k_num_bytes;
                key.origPOD = Alembic::Util::PODFromName( k_orig_pod );
                key.readPOD = Alembic::Util::PODFromName( k_read_pod );
                hex2bin(key.digest.d, k_digest.c_str());

                //std::string key_str = j_key.asString();

                m_kid_to_key[k_kid] = key;
                m_key_to_kid[key]   = k_kid;
            }

            // set m_next_kid
            m_next_kid = v_next_kid;
            assert(static_cast<size_t>(m_kid_to_key.size()) == v_n_kid);
        }

        all_unpacked += packedHeader.length();
    } else
    {
        std::cerr << "WARNING: can't read git blob '" << pathjoin(m_group->absPathname(), name_header) << "'" << std::endl;
    }

    // read samples

    bool all_ok = true;

    // read & unpack bundled samples

    {
        std::string name_bundle = basename + "_bundle" + ".bin";
        boost::optional<std::string> optBinBundleContents = gitTree->getChildFile(name_bundle);
        if (optBinBundleContents)
        {
            std::string packedBundle = *optBinBundleContents;

            msgpack::unpacker pac;

            // copy the buffer data to the unpacker object
            pac.reserve_buffer(packedBundle.size());
            memcpy(pac.buffer(), packedBundle.data(), packedBundle.size());
            pac.buffer_consumed(packedBundle.size());

            size_t n_bundled = 0;

            // deserialize it.
            msgpack::unpacked msg;

            pac.next(&msg);
            msgpack::object pko = msg.get();
            mp_unpack(pko, n_bundled);

            for (size_t i = 0; i < n_bundled; ++i)
            {
                size_t kid;
                std::vector<T> data;

                pac.next(&msg);
                pko = msg.get();
                mp_unpack(pko, kid);

                pac.next(&msg);
                pko = msg.get();
                mp_unpack(pko, data);

                m_kid_to_data[kid] = data;
                m_has_kid_data[kid] = true;
            }

            all_unpacked += packedBundle.length();

            TRACE("unpacked " << packedBundle.length() << " bytes for # " << n_bundled << " (different) bundled samples of type " << GetTypeStr<T>());
        }
    }

    // read & unpack unbundled samples

    {
        bool ok;

        size_t n_unbundled = 0;
        size_t unbundled_unpacked = 0;
        for (size_t i = 0; i < v_n_kid; ++i)
        {
            size_t kid = i;
            if ((! m_has_kid_data.count(kid)) || (! m_has_kid_data[kid]))
            {
                ok = readFromDiskSample(gitTree, basename, i, unpacked);
                all_ok = all_ok && ok;
                if (! ok)
                    break;

                n_unbundled++;
                unbundled_unpacked += unpacked;
                all_unpacked += unpacked;
            }
        }
        // TRACE("unpacked " << v_n_kid << " (different) samples");
        TRACE("unpacked " << unbundled_unpacked << " bytes for # " << n_unbundled << " (different) unbundled samples of type " << GetTypeStr<T>());
    }

    TRACE("unpacked " << all_unpacked << " total bytes for # " << v_n_kid << " (different) samples of type " << GetTypeStr<T>());
    loaded(true);

    ABCA_ASSERT( loaded(), "data not read" );
    return all_ok;
}

#if 0
template <typename T>
bool KeyStore<T>::writeToDiskSample(const std::string& basename, std::map< size_t, AbcA::ArraySample::Key >::const_iterator& p_it, size_t& npacked)
{
    assert(m_rwmode == WRITE);
    assert(! saved());

    npacked = 0;

    size_t kid                        = (*p_it).first;
    const AbcA::ArraySample::Key& key = (*p_it).second;

    std::ostringstream ss;
    ss << "_" << key.digest.str();
    std::string suffix = ss.str();

    // basename: "keystore_" + GetTypeStr<T>();
    std::string name = basename + suffix + ".bin";

    std::string packedSample = packSample(kid, key);
    m_group->add_file_from_memory(name, packedSample);

    npacked = packedSample.length();

    // TRACE("packed " << packedSample.length() << " bytes for sample kid:" << kid << " type " << GetTypeStr<T>());

    return true;
}
#endif
template <typename T>
bool KeyStore<T>::readFromDiskSample(GitTreePtr gitTree, const std::string& basename, size_t kid, size_t& unpacked)
{
    assert(m_rwmode == READ);
    assert(! loaded());

    unpacked = 0;

    const AbcA::ArraySample::Key& key = KidToKey(kid);

    std::ostringstream ss;
    ss << "_" << key.digest.str();
    std::string suffix = ss.str();

    // basename: "keystore_" + GetTypeStr<T>();
    std::string name_sample = basename + suffix + ".bin";

    boost::optional<std::string> optBinSampleContents = gitTree->getChildFile(name_sample);
    if (! optBinSampleContents)
    {
        ABCA_THROW( "can't read git blob '" << name_sample << "'" );
        return false;
    }

    std::string packedSample = *optBinSampleContents;

    bool ok = unpackSample(*optBinSampleContents, kid);

    if (ok)
        unpacked = packedSample.length();

    return ok;
}

#if 0
template <typename T>
std::string KeyStore<T>::packSample(size_t kid, const AbcA::ArraySample::Key& key)
{
    // TRACE("KeyStore::packSample(type:" << GetTypeStr<T>() << ", kid:" << kid << ")");

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    // msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string >
    //     tuple(kid, key.numBytes, pod2str(key.origPOD), pod2str(key.readPOD), key.digest.str());
    // msgpack::type::tuple< size_t, std::string, std::string, std::string >
    //     tuple(key.numBytes, pod2str(key.origPOD), pod2str(key.readPOD), key.digest.str());

    // mp_pack(pk, tuple);

    std::vector<T>& data = m_kid_to_data[kid];
    mp_pack(pk, data);

    return buffer.str();
}
#endif

template <typename T>
bool KeyStore<T>::unpackSample(const std::string& packedSample, size_t kid)
{
    // TRACE("KeyStore::unpackSample(type:" << GetTypeStr<T>() << ", kid:" << kid << ")");

    msgpack::unpacker pac;

    // copy the buffer data to the unpacker object
    pac.reserve_buffer(packedSample.size());
    memcpy(pac.buffer(), packedSample.data(), packedSample.size());
    pac.buffer_consumed(packedSample.size());

    // deserialize it.
    msgpack::unpacked msg;

    // // msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string > tuple;
    // msgpack::type::tuple< size_t, std::string, std::string, std::string > tuple;

    // pac.next(&msg);
    // msgpack::object pko = msg.get();
    // mp_unpack(pko, tuple);

    // // size_t      k_kid       = tuple.get<0>();
    // size_t      k_num_bytes = tuple.get<0>();
    // std::string k_orig_pod  = tuple.get<1>();
    // std::string k_read_pod  = tuple.get<2>();
    // std::string k_digest    = tuple.get<3>();

    // AbcA::ArraySample::Key key;

    // key.numBytes = k_num_bytes;
    // key.origPOD = Alembic::Util::PODFromName( k_orig_pod );
    // key.readPOD = Alembic::Util::PODFromName( k_read_pod );
    // hex2bin(key.digest.d, k_digest.c_str());

    // //std::string key_str = j_key.asString();

    // // m_kid_to_key[k_kid] = key;
    // // m_key_to_kid[key]   = k_kid;
    // m_kid_to_key[kid] = key;
    // m_key_to_kid[key] = kid;

    std::vector<T> data;
    pac.next(&msg);
    msgpack::object pko = msg.get();
    mp_unpack(pko, data);
    // m_kid_to_data[k_kid] = data;
    m_kid_to_data[kid] = data;
    m_has_kid_data[kid] = true;

    return true;
}

#if 0
template <typename T>
std::string KeyStore<T>::pack()
{
    TRACE("KeyStore::pack() " << GetTypeStr<T>());

    std::stringstream buffer;
    msgpack::packer<std::stringstream> pk(&buffer);

    mp_pack(pk, static_cast<size_t>(m_kid_to_key.size()));
    std::map< size_t, AbcA::ArraySample::Key >::const_iterator p_it;
    for (p_it = m_kid_to_key.begin(); p_it != m_kid_to_key.end(); ++p_it)
    {
        size_t kid                        = (*p_it).first;
        const AbcA::ArraySample::Key& key = (*p_it).second;

        msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string >
            tuple(kid, key.numBytes, pod2str(key.origPOD), pod2str(key.readPOD), key.digest.str());

        mp_pack(pk, tuple);

        std::vector<T>& data = m_kid_to_data[kid];
        mp_pack(pk, data);
    }
    mp_pack(pk, m_next_kid);

    return buffer.str();
}

template <typename T>
bool KeyStore<T>::unpack(const std::string& packed)
{
    TRACE("KeyStore::unpack() " << GetTypeStr<T>());

    msgpack::unpacker pac;

    // copy the buffer data to the unpacker object
    pac.reserve_buffer(packed.size());
    memcpy(pac.buffer(), packed.data(), packed.size());
    pac.buffer_consumed(packed.size());

    // deserialize it.
    msgpack::unpacked msg;

    m_key_to_kid.clear();
    m_kid_to_key.clear();
    m_next_kid = 0;
    size_t v_n_kid = 0;

    pac.next(&msg);
    msgpack::object pko = msg.get();
    mp_unpack(pko, v_n_kid);

    for (size_t i = 0; i < v_n_kid; ++i)
    {
        msgpack::type::tuple< size_t, size_t, std::string, std::string, std::string > tuple;

        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, tuple);

        size_t      k_kid       = tuple.get<0>();
        size_t      k_num_bytes = tuple.get<1>();
        std::string k_orig_pod  = tuple.get<2>();
        std::string k_read_pod  = tuple.get<3>();
        std::string k_digest    = tuple.get<4>();

        AbcA::ArraySample::Key key;

        key.numBytes = k_num_bytes;
        key.origPOD = Alembic::Util::PODFromName( k_orig_pod );
        key.readPOD = Alembic::Util::PODFromName( k_read_pod );
        hex2bin(key.digest.d, k_digest.c_str());

        //std::string key_str = j_key.asString();

        m_kid_to_key[k_kid] = key;
        m_key_to_kid[key]   = k_kid;

        std::vector<T> data;
        pac.next(&msg);
        pko = msg.get();
        mp_unpack(pko, data);
        m_kid_to_data[k_kid] = data;
        // TRACE("unpacked sample of extent " << data.size() << " (kid:" << k_kid << ")");
    }
    TRACE("unpacked " << v_n_kid << " (different) samples");

    pac.next(&msg);
    pko = msg.get();
    mp_unpack(pko, m_next_kid);

    return true;
}

template <typename T>
bool KeyStore<T>::writeToDisk()
{
    if (m_rwmode != WRITE)
        return false;

    assert(m_rwmode == WRITE);

    std::string name = "keystore_" + GetTypeStr<T>();
    std::string pathname = pathjoin(m_group->absPathname(), name + ".bin");

    if (! saved())
    {
        TRACE("KeyStore::writeToDisk() path:'" << pathname << "' (WRITING)");

        std::string packed = pack();
        m_group->add_file_from_memory(name + ".bin", packed);

        TRACE("packed " << packed.length() << " bytes for type " << GetTypeStr<T>());
        saved(true);
    } else
    {
        TRACE("KeyStore::writeToDisk() path:'" << pathname << "' (skipping, already written)");
    }

    ABCA_ASSERT( saved(), "data not written" );
    return true;
}

template <typename T>
bool KeyStore<T>::readFromDisk()
{
    bool ok = false;

    if (m_rwmode != READ)
        return false;

    assert(m_rwmode == READ);

    std::string name = "keystore_" + GetTypeStr<T>();
    std::string pathname = pathjoin(m_group->absPathname(), name + ".bin");

    if (! loaded())
    {
        TRACE("KeyStore::readFromDisk() path:'" << pathname << "' (READING)");

        GitGroupPtr parentGroup = m_group->parent();
        boost::optional<std::string> optBinContents = m_group->tree()->getChildFile(name + ".bin");
        if (! optBinContents)
        {
            ABCA_THROW( "can't read git blob '" << pathname + ".bin" << "'" );
            return false;
        }
        ok = unpack( *optBinContents );

        TRACE("unpacked " << (*optBinContents).length() << " bytes for type " << GetTypeStr<T>());
        loaded(true);
    } else
    {
        TRACE("KeyStore::readFromDisk() path:'" << pathname << "' (skipping, already read)");
        ok = true;
    }

    ABCA_ASSERT( loaded(), "data not read" );
    return ok;
}
#endif


/* force template instantiation for the types interesting to us */
template struct KeyStore< Util::bool_t >;
template struct KeyStore< Util::uint8_t >;
template struct KeyStore< Util::int8_t >;
template struct KeyStore< Util::uint16_t >;
template struct KeyStore< Util::int16_t >;
template struct KeyStore< Util::uint32_t >;
template struct KeyStore< Util::int32_t >;
template struct KeyStore< Util::uint64_t >;
template struct KeyStore< Util::int64_t >;
template struct KeyStore< Util::float16_t >;
template struct KeyStore< Util::float32_t >;
template struct KeyStore< Util::float64_t >;
template struct KeyStore< char >;
template struct KeyStore< Util::string >;
template struct KeyStore< Util::wstring >;


} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic
