#ifndef MILLIWAYS_CONFIG_H
#define MILLIWAYS_CONFIG_H

#define COMPILER_SUPPORTS_CXX0X 1
#define COMPILER_SUPPORTS_CXX11 1

#define HAVE_STDINT_H 1
#define HAVE_STDDEF_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ASSERT_H 1
#define HAVE_STRING_H 1
#define HAVE_LIMITS_H 1
#define HAVE_ERRNO_H 1
#define HAVE_ARPA_INET_H 1
/* #undef HAVE_WINDOWS_H */
/* #undef HAVE_BASETSD_H */

/* Define to 1 if you have the system is little endian */
#define IS_LITTLE_ENDIAN 1

/* Define to 1 if you have the system is big endian */
/* #undef IS_BIG_ENDIAN */

/* Define to 1 if you have the htons function. */
#define HAVE_HTONS 1

/* Define to 1 if you have the ntohs function. */
#define HAVE_NTOHS 1

/* Define to 1 if you have the htonl function. */
#define HAVE_HTONL 1

/* Define to 1 if you have the ntohl function. */
#define HAVE_NTOHL 1

/* Define to 1 if you have the htonll function. */
/* #undef HAVE_HTONLL */

/* Define to 1 if you have the ntohll function. */
/* #undef HAVE_NTOHLL */

/* Define to 1 if you have the size_t type. */
#define HAVE_SIZE_T 1

/* Define to 1 if you have the ssize_t type. */
#define HAVE_SSIZE_T 1

/* Define to 1 if you have the Windows SSIZE_T type. */
/* #undef HAVE_WIN_SSIZE_T */

/* Define to 1 if you have the <sys/endian.h> header file. */
/* #undef HAVE_ENDIAN_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if <tr1/memory> exists and defines std::tr1::shared_ptr. */
#define HAVE_STD_TR1_SHARED_PTR 1

/* Define to 1 if <memory> exists and defines std::shared_ptr. */
/* #undef HAVE_STD_SHARED_PTR */

/* Define to 1 if <boost/shared_ptr.hpp> exists and defines boost::shared_ptr. */
/* #undef HAVE_BOOST_SHARED_PTR */

/* Define to 1 if <tr1/memory> exists and defines std::tr1::weak_ptr. */
#define HAVE_STD_TR1_WEAK_PTR 1

/* Define to 1 if <memory> exists and defines std::weak_ptr. */
/* #undef HAVE_STD_WEAK_PTR */

/* Define to 1 if <boost/weak_ptr.hpp> exists and defines boost::weak_ptr. */
/* #undef HAVE_BOOST_WEAK_PTR */

/* Define to 1 if <tr1/array> exists and defines std::tr1::array. */
#define HAVE_STD_TR1_ARRAY 1

/* Define to 1 if <array> exists and defines std::array. */
/* #undef HAVE_STD_ARRAY */

/* Define to 1 if <boost/array.hpp> exists and defines boost::array. */
/* #undef HAVE_BOOST_ARRAY */

/* Define to 1 if <tr1/unordered_map> exists and defines std::tr1::unordered_map. */
#define HAVE_STD_TR1_UNORDERED_MAP 1

/* Define to 1 if <unordered_map> exists and defines std::unordered_map. */
/* #undef HAVE_STD_UNORDERED_MAP */

/* Define to 1 if <boost/unordered_map.hpp> exists and defines boost::unordered_map. */
/* #undef HAVE_BOOST_UNORDERED_MAP */

/* Define to 1 if <functional> exists and defines std::function. */
/* #undef HAVE_STD_FUNCTION */

/* Define to 1 if <tr1/functional> exists and defines std::tr1::function. */
/* #undef HAVE_STD_TR1_FUNCTION */

/* Define to 1 if <boost/function.hpp> exists and defines boost::function. */
/* #undef HAVE_BOOST_FUNCTION */

/* Define to 1 if <functional> exists and defines std::mem_fn. */
/* #undef HAVE_STD_MEM_FN */

/* Define to 1 if <tr1/functional> exists and defines std::tr1::mem_fn. */
/* #undef HAVE_STD_TR1_MEM_FN */

/* Define to 1 if <boost/mem_fn.hpp> exists and defines boost::mem_fn. */
/* #undef HAVE_BOOST_MEM_FN */

/* Define to 1 if <functional> exists and defines std::bind. */
/* #undef HAVE_STD_BIND */

/* Define to 1 if <tr1/functional> exists and defines std::tr1::bind. */
/* #undef HAVE_STD_TR1_BIND */

/* Define to 1 if <boost/bind.hpp> exists and defines boost::bind. */
/* #undef HAVE_BOOST_BIND */

/* Define to 1 if <tr1/tuple> exists and defines std::tr1::tuple. */
/* #undef HAVE_STD_TR1_TUPLE */

/* Define to 1 if <tuple> exists and defines std::tuple. */
/* #undef HAVE_STD_TUPLE */

/* Define to 1 if <boost/tuple/tuple.hpp> exists and defines boost::tuple. */
/* #undef HAVE_BOOST_TUPLE */

/* Define to 1 if <boost/multiprecision/cpp_dec_float.hpp> exists and defines boost::multiprecision::cpp_dec_float. */
/* #undef HAVE_BOOST_DEC_FLOAT */

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_SSIZE_T 8

/* Define to 1 if an explicit template for size_t is not needed if all the uint*_t types are there */
/* #undef DOESNT_NEED_TEMPLATED_SIZE_T */

/* Define to 1 if an explicit template for ssize_t is not needed if all the int*_t types are there */
/* #undef DOESNT_NEED_TEMPLATED_SSIZE_T */

/* Define to 1 if an explicit template for size_t is allowed even if all the uint*_t types are there */
#define ALLOWS_TEMPLATED_SIZE_T 1

/* Define to 1 if an explicit template for ssize_t is allowed even if all the int*_t types are there */
#define ALLOWS_TEMPLATED_SSIZE_T 1

/* Define to 1 if "typename" keyword is allowed inside templates */
#define ALLOWS_TYPENAME_INSIDE_TEMPLATES 1

/* Define to 1 if "typename" keyword is allowed outside templates (it is not in C++03) */
#define ALLOWS_TYPENAME_OUTSIDE_TEMPLATES 1

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
/* #undef UINT32_IS_ULL */

/* Define to 1 if uint32_t is same as unsigned long */
/* #undef UINT32_IS_UL */

/* Define to 1 if uint32_t is same as unsigned int */
#define UINT32_IS_UI 1

/* Define to 1 if uint64_t is same as unsigned long long */
#define UINT64_IS_ULL 1

/* Define to 1 if uint64_t is same as unsigned long */
/* #undef UINT64_IS_UL */

/* Define to 1 if uint64_t is same as unsigned int */
/* #undef UINT64_IS_UI */

/* Define to 1 if size_t is same as unsigned long long */
/* #undef SIZE_T_IS_ULL */

/* Define to 1 if size_t is same as unsigned long */
#define SIZE_T_IS_UL 1

/* Define to 1 if size_t is same as unsigned int */
/* #undef SIZE_T_IS_UI */

/* Define to 1 if ssize_t is same as long long */
/* #undef SSIZE_T_IS_LL */

/* Define to 1 if ssize_t is same as long */
#define SSIZE_T_IS_L 1

/* Define to 1 if ssize_t is same as int */
/* #undef SSIZE_T_IS_I */

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
