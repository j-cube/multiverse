#ifndef MILLIWAYS_CONFIG_H
#define MILLIWAYS_CONFIG_H

#cmakedefine COMPILER_SUPPORTS_CXX0X 1
#cmakedefine COMPILER_SUPPORTS_CXX11 1

#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_STDDEF_H 1
#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine HAVE_ASSERT_H 1
#cmakedefine HAVE_STRING_H 1
#cmakedefine HAVE_LIMITS_H 1
#cmakedefine HAVE_ERRNO_H 1
#cmakedefine HAVE_ARPA_INET_H 1
#cmakedefine HAVE_WINDOWS_H 1
#cmakedefine HAVE_BASETSD_H 1

/* Define to 1 if you have the system is little endian */
#cmakedefine IS_LITTLE_ENDIAN 1

/* Define to 1 if you have the system is big endian */
#cmakedefine IS_BIG_ENDIAN 1

/* Define to 1 if you have the htons function. */
#cmakedefine HAVE_HTONS 1

/* Define to 1 if you have the ntohs function. */
#cmakedefine HAVE_NTOHS 1

/* Define to 1 if you have the htonl function. */
#cmakedefine HAVE_HTONL 1

/* Define to 1 if you have the ntohl function. */
#cmakedefine HAVE_NTOHL 1

/* Define to 1 if you have the htonll function. */
#cmakedefine HAVE_HTONLL 1

/* Define to 1 if you have the ntohll function. */
#cmakedefine HAVE_NTOHLL 1

/* Define to 1 if you have the size_t type. */
#cmakedefine HAVE_SIZE_T 1

/* Define to 1 if you have the ssize_t type. */
#cmakedefine HAVE_SSIZE_T 1

/* Define to 1 if you have the Windows SSIZE_T type. */
#cmakedefine HAVE_WIN_SSIZE_T 1

/* Define to 1 if you have the <sys/endian.h> header file. */
#cmakedefine HAVE_ENDIAN_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if <tr1/memory> exists and defines std::tr1::shared_ptr. */
#cmakedefine HAVE_STD_TR1_SHARED_PTR 1

/* Define to 1 if <memory> exists and defines std::shared_ptr. */
#cmakedefine HAVE_STD_SHARED_PTR 1

/* Define to 1 if <boost/shared_ptr.hpp> exists and defines boost::shared_ptr. */
#cmakedefine HAVE_BOOST_SHARED_PTR 1

/* Define to 1 if <tr1/memory> exists and defines std::tr1::weak_ptr. */
#cmakedefine HAVE_STD_TR1_WEAK_PTR 1

/* Define to 1 if <memory> exists and defines std::weak_ptr. */
#cmakedefine HAVE_STD_WEAK_PTR 1

/* Define to 1 if <boost/weak_ptr.hpp> exists and defines boost::weak_ptr. */
#cmakedefine HAVE_BOOST_WEAK_PTR 1

/* Define to 1 if <tr1/array> exists and defines std::tr1::array. */
#cmakedefine HAVE_STD_TR1_ARRAY 1

/* Define to 1 if <array> exists and defines std::array. */
#cmakedefine HAVE_STD_ARRAY 1

/* Define to 1 if <boost/array.hpp> exists and defines boost::array. */
#cmakedefine HAVE_BOOST_ARRAY 1

/* Define to 1 if <tr1/unordered_map> exists and defines std::tr1::unordered_map. */
#cmakedefine HAVE_STD_TR1_UNORDERED_MAP 1

/* Define to 1 if <unordered_map> exists and defines std::unordered_map. */
#cmakedefine HAVE_STD_UNORDERED_MAP 1

/* Define to 1 if <boost/unordered_map.hpp> exists and defines boost::unordered_map. */
#cmakedefine HAVE_BOOST_UNORDERED_MAP 1

/* Define to 1 if <functional> exists and defines std::function. */
#cmakedefine HAVE_STD_FUNCTION 1

/* Define to 1 if <tr1/functional> exists and defines std::tr1::function. */
#cmakedefine HAVE_STD_TR1_FUNCTION 1

/* Define to 1 if <boost/function.hpp> exists and defines boost::function. */
#cmakedefine HAVE_BOOST_FUNCTION 1

/* Define to 1 if <functional> exists and defines std::mem_fn. */
#cmakedefine HAVE_STD_MEM_FN 1

/* Define to 1 if <tr1/functional> exists and defines std::tr1::mem_fn. */
#cmakedefine HAVE_STD_TR1_MEM_FN 1

/* Define to 1 if <boost/mem_fn.hpp> exists and defines boost::mem_fn. */
#cmakedefine HAVE_BOOST_MEM_FN 1

/* Define to 1 if <functional> exists and defines std::bind. */
#cmakedefine HAVE_STD_BIND 1

/* Define to 1 if <tr1/functional> exists and defines std::tr1::bind. */
#cmakedefine HAVE_STD_TR1_BIND 1

/* Define to 1 if <boost/bind.hpp> exists and defines boost::bind. */
#cmakedefine HAVE_BOOST_BIND 1

/* Define to 1 if <tr1/tuple> exists and defines std::tr1::tuple. */
#cmakedefine HAVE_STD_TR1_TUPLE 1

/* Define to 1 if <tuple> exists and defines std::tuple. */
#cmakedefine HAVE_STD_TUPLE 1

/* Define to 1 if <boost/tuple/tuple.hpp> exists and defines boost::tuple. */
#cmakedefine HAVE_BOOST_TUPLE 1

/* Define to 1 if <boost/multiprecision/cpp_dec_float.hpp> exists and defines boost::multiprecision::cpp_dec_float. */
#cmakedefine HAVE_BOOST_DEC_FLOAT 1

#define SIZEOF_SHORT @SIZEOF_SHORT@
#define SIZEOF_INT @SIZEOF_INT@
#define SIZEOF_LONG @SIZEOF_LONG@
#define SIZEOF_UNSIGNED_LONG @SIZEOF_UNSIGNED_LONG@
#define SIZEOF_LONG_LONG @SIZEOF_LONG_LONG@
#define SIZEOF_FLOAT @SIZEOF_FLOAT@
#define SIZEOF_DOUBLE @SIZEOF_DOUBLE@
#define SIZEOF_SIZE_T @SIZEOF_SIZE_T@
#define SIZEOF_SSIZE_T @SIZEOF_SSIZE_T@

/* Define to 1 if an explicit template for size_t is not needed if all the uint*_t types are there */
#cmakedefine DOESNT_NEED_TEMPLATED_SIZE_T 1

/* Define to 1 if an explicit template for ssize_t is not needed if all the int*_t types are there */
#cmakedefine DOESNT_NEED_TEMPLATED_SSIZE_T 1

/* Define to 1 if an explicit template for size_t is allowed even if all the uint*_t types are there */
#cmakedefine ALLOWS_TEMPLATED_SIZE_T 1

/* Define to 1 if an explicit template for ssize_t is allowed even if all the int*_t types are there */
#cmakedefine ALLOWS_TEMPLATED_SSIZE_T 1

/* Define to 1 if "typename" keyword is allowed inside templates */
#cmakedefine ALLOWS_TYPENAME_INSIDE_TEMPLATES 1

/* Define to 1 if "typename" keyword is allowed outside templates (it is not in C++03) */
#cmakedefine ALLOWS_TYPENAME_OUTSIDE_TEMPLATES 1

#if ALLOWS_TYPENAME_INSIDE_TEMPLATES
#define ITYPENAME typename
#else /* ! ALLOWS_TYPENAME_INSIDE_TEMPLATES */
#define ITYPENAME
#endif /* ! ALLOWS_TYPENAME_INSIDE_TEMPLATES */

#if ALLOWS_TYPENAME_OUTSIDE_TEMPLATES
#define XTYPENAME typename
#else /* ! ALLOWS_TYPENAME_OUTSIDE_TEMPLATES */
#define XTYPENAME
#endif /* ! ALLOWS_TYPENAME_OUTSIDE_TEMPLATES */

/* Define to 1 if uint32_t is same as unsigned long long */
#cmakedefine UINT32_IS_ULL 1

/* Define to 1 if uint32_t is same as unsigned long */
#cmakedefine UINT32_IS_UL 1

/* Define to 1 if uint32_t is same as unsigned int */
#cmakedefine UINT32_IS_UI 1

/* Define to 1 if uint64_t is same as unsigned long long */
#cmakedefine UINT64_IS_ULL 1

/* Define to 1 if uint64_t is same as unsigned long */
#cmakedefine UINT64_IS_UL 1

/* Define to 1 if uint64_t is same as unsigned int */
#cmakedefine UINT64_IS_UI 1

/* Define to 1 if size_t is same as unsigned long long */
#cmakedefine SIZE_T_IS_ULL 1

/* Define to 1 if size_t is same as unsigned long */
#cmakedefine SIZE_T_IS_UL 1

/* Define to 1 if size_t is same as unsigned int */
#cmakedefine SIZE_T_IS_UI 1

/* Define to 1 if ssize_t is same as long long */
#cmakedefine SSIZE_T_IS_LL 1

/* Define to 1 if ssize_t is same as long */
#cmakedefine SSIZE_T_IS_L 1

/* Define to 1 if ssize_t is same as int */
#cmakedefine SSIZE_T_IS_I 1

#if defined(HAVE_STD_SHARED_PTR)
  #define USE_STD_MEMORY
  #define cxx_mem std
#elif defined(HAVE_STD_TR1_SHARED_PTR)
  #define USE_TR1_MEMORY
  #define cxx_mem std::tr1
#elif defined(HAVE_BOOST_SHARED_PTR)
  #define USE_BOOST_MEMORY
  #define cxx_mem boost
#else
  #error "no shared pointer implementation available!"
#endif

#if defined(HAVE_STD_ARRAY)
  #define USE_STD_ARRAY
  #define cxx_a std
#elif defined(HAVE_STD_TR1_ARRAY)
  #define USE_TR1_ARRAY
  #define cxx_a std::tr1
#elif defined(HAVE_BOOST_ARRAY)
  #define USE_BOOST_ARRAY
  #define cxx_a boost
#else
  #error "no templated array implementation available!"
#endif

#if defined(HAVE_STD_UNORDERED_MAP)
  #define USE_STD_UNORDERED_MAP
  #define cxx_um std
#elif defined(HAVE_STD_TR1_UNORDERED_MAP)
  #define USE_TR1_UNORDERED_MAP
  #define cxx_um std::tr1
#elif defined(HAVE_BOOST_UNORDERED_MAP)
  #define USE_BOOST_UNORDERED_MAP
  #define cxx_um boost
#else
  #error "no unoredered map implementation available!"
#endif

#endif /* MILLIWAYS_CONFIG_H */
