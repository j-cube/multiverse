#ifndef MILLIWAYS_CONFIG_H
#define MILLIWAYS_CONFIG_H

/* Define to 1 if you have the <sys/endian.h> header file. */
/* #undef HAVE_ENDIAN_H */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if <tr1/memory> exists and defines std::tr1::shared_ptr. */
/* #undef HAVE_STD_TR1_SHARED_PTR */

/* Define to 1 if <memory> exists and defines std::shared_ptr. */
#define HAVE_STD_SHARED_PTR 1

/* Define to 1 if <boost/shared_ptr.hpp> exists and defines boost::shared_ptr. */
#define HAVE_BOOST_SHARED_PTR 1

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

/* Define to 1 if an explicit template for size_t is allowed even if all the uint*_t types are there */
#define ALLOWS_TEMPLATED_SIZE_T 1

/* Define to 1 if "typename" keyword is allowed outside templates (it is not in C++03) */
#define ALLOWS_TYPENAME_OUTSIDE_TEMPLATES 1

#if ALLOWS_TYPENAME_OUTSIDE_TEMPLATES
#define XTYPENAME typename
#else /* ! ALLOWS_TYPENAME_OUTSIDE_TEMPLATES */
#define XTYPENAME
#endif /* ! ALLOWS_TYPENAME_OUTSIDE_TEMPLATES */

#endif /* MILLIWAYS_CONFIG_H */
