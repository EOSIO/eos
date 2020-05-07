## connection unit tests
##

Import('env')
Import('env_cpp11')
Import('boostlibs')
Import('platform_libs')
Import('polyfill_libs')

env = env.Clone ()
env_cpp11 = env_cpp11.Clone ()

BOOST_LIBS = boostlibs(['unit_test_framework','system'],env) + [platform_libs]

objs = env.Object('connection_boost.o', ["connection.cpp"], LIBS = BOOST_LIBS)
objs = env.Object('connection_tu2_boost.o', ["connection_tu2.cpp"], LIBS = BOOST_LIBS)
prgs = env.Program('test_connection_boost', ["connection_boost.o","connection_tu2_boost.o"], LIBS = BOOST_LIBS)

if env_cpp11.has_key('WSPP_CPP11_ENABLED'):
   BOOST_LIBS_CPP11 = boostlibs(['unit_test_framework','system'],env_cpp11) + [platform_libs] + [polyfill_libs]
   objs += env_cpp11.Object('connection_stl.o', ["connection.cpp"], LIBS = BOOST_LIBS_CPP11)
   objs += env_cpp11.Object('connection_tu2_stl.o', ["connection_tu2.cpp"], LIBS = BOOST_LIBS_CPP11)
   prgs += env_cpp11.Program('test_connection_stl', ["connection_stl.o","connection_tu2_stl.o"], LIBS = BOOST_LIBS_CPP11)

Return('prgs')
