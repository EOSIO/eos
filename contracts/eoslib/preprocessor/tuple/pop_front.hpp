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
# ifndef BOOST_PREPROCESSOR_TUPLE_POP_FRONT_HPP
# define BOOST_PREPROCESSOR_TUPLE_POP_FRONT_HPP
#
# include <eoslib/preprocessor/config/config.hpp>
#
# if BOOST_PP_VARIADICS
#
# include <eoslib/preprocessor/array/pop_front.hpp>
# include <eoslib/preprocessor/array/to_tuple.hpp>
# include <eoslib/preprocessor/comparison/greater.hpp>
# include <eoslib/preprocessor/control/iif.hpp>
# include <eoslib/preprocessor/tuple/size.hpp>
# include <eoslib/preprocessor/tuple/to_array.hpp>
#
#
# /* BOOST_PP_TUPLE_POP_FRONT */
#
# define BOOST_PP_TUPLE_POP_FRONT(tuple) \
	BOOST_PP_IIF \
		( \
		BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(tuple),1), \
		BOOST_PP_TUPLE_POP_FRONT_EXEC, \
		BOOST_PP_TUPLE_POP_FRONT_RETURN \
		) \
	(tuple) \
/**/
#
# define BOOST_PP_TUPLE_POP_FRONT_EXEC(tuple) \
	BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_POP_FRONT(BOOST_PP_TUPLE_TO_ARRAY(tuple))) \
/**/
#
# define BOOST_PP_TUPLE_POP_FRONT_RETURN(tuple) tuple
#
# /* BOOST_PP_TUPLE_POP_FRONT_Z */
#
# define BOOST_PP_TUPLE_POP_FRONT_Z(z, tuple) \
	BOOST_PP_IIF \
		( \
		BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(tuple),1), \
		BOOST_PP_TUPLE_POP_FRONT_Z_EXEC, \
		BOOST_PP_TUPLE_POP_FRONT_Z_RETURN \
		) \
	(z, tuple) \
/**/
#
# define BOOST_PP_TUPLE_POP_FRONT_Z_EXEC(z, tuple) \
	BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_POP_FRONT_Z(z, BOOST_PP_TUPLE_TO_ARRAY(tuple))) \
/**/
#
# define BOOST_PP_TUPLE_POP_FRONT_Z_RETURN(z, tuple) tuple
#
# endif // BOOST_PP_VARIADICS
#
# endif // BOOST_PREPROCESSOR_TUPLE_POP_FRONT_HPP
