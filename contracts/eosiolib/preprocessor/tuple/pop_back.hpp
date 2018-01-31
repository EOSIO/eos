# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2013.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_TUPLE_POP_BACK_HPP
# define BOOST_PREPROCESSOR_TUPLE_POP_BACK_HPP
#
# include <eosiolib/preprocessor/config/config.hpp>
#
# if BOOST_PP_VARIADICS
#
# include <eosiolib/preprocessor/array/pop_back.hpp>
# include <eosiolib/preprocessor/array/to_tuple.hpp>
# include <eosiolib/preprocessor/comparison/greater.hpp>
# include <eosiolib/preprocessor/control/iif.hpp>
# include <eosiolib/preprocessor/tuple/size.hpp>
# include <eosiolib/preprocessor/tuple/to_array.hpp>
#
# /* BOOST_PP_TUPLE_POP_BACK */
#
# define BOOST_PP_TUPLE_POP_BACK(tuple) \
	BOOST_PP_IIF \
		( \
		BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(tuple),1), \
		BOOST_PP_TUPLE_POP_BACK_EXEC, \
		BOOST_PP_TUPLE_POP_BACK_RETURN \
		) \
	(tuple) \
/**/
#
# define BOOST_PP_TUPLE_POP_BACK_EXEC(tuple) \
	BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_POP_BACK(BOOST_PP_TUPLE_TO_ARRAY(tuple))) \
/**/
#
# define BOOST_PP_TUPLE_POP_BACK_RETURN(tuple) tuple
#
# /* BOOST_PP_TUPLE_POP_BACK_Z */
#
# define BOOST_PP_TUPLE_POP_BACK_Z(z, tuple) \
	BOOST_PP_IIF \
		( \
		BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(tuple),1), \
		BOOST_PP_TUPLE_POP_BACK_Z_EXEC, \
		BOOST_PP_TUPLE_POP_BACK_Z_RETURN \
		) \
	(z, tuple) \
/**/
#
# define BOOST_PP_TUPLE_POP_BACK_Z_EXEC(z, tuple) \
	BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_POP_BACK_Z(z, BOOST_PP_TUPLE_TO_ARRAY(tuple))) \
/**/
#
# define BOOST_PP_TUPLE_POP_BACK_Z_RETURN(z, tuple) tuple
#
# endif // BOOST_PP_VARIADICS
#
# endif // BOOST_PREPROCESSOR_TUPLE_POP_BACK_HPP
