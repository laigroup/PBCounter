PROJECT(cmake_PBC)    # overwrites 'Makefile' (lower precedence than manually written 'GNUmakefile' wrt 'make')

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

# set -std=c++14
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS "-g -Ofast -static")
# close -static on mac
# SET(CMAKE_CXX_FLAGS "-g -Ofast")
# use address sanitizer fo debug
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

SET(lib_dir ${CMAKE_CURRENT_SOURCE_DIR}/libraries)

# CUDD
SET(cudd_dir ${lib_dir}/cudd)
SET(cudd_lib ${cudd_dir}/.libs)

# complie CUDD
ADD_CUSTOM_COMMAND(
  OUTPUT ${cudd_lib}
  COMMAND cd ${cudd_dir} && autoreconf && ./configure --enable-silent-rules --enable-obj && make -s
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(${cudd_dir}/cudd/.libs)

INCLUDE_DIRECTORIES(${cudd_dir})
INCLUDE_DIRECTORIES(${cudd_dir}/cudd)
INCLUDE_DIRECTORIES(${cudd_dir}/epd)
INCLUDE_DIRECTORIES(${cudd_dir}/mtr)
INCLUDE_DIRECTORIES(${cudd_dir}/st)

# sylvan
SET(sylvan_dir ${lib_dir}/sylvan)
SET(sylvan_build ${sylvan_dir}/build)

ADD_CUSTOM_COMMAND(
  OUTPUT ${sylvan_build}
  COMMAND mkdir -p ${sylvan_build} && cd ${sylvan_build} && cmake ${sylvan_dir}  -DBUILD_SHARED_LIBS=off && make -s
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(${sylvan_build}/src)
INCLUDE_DIRECTORIES(${sylvan_dir}/src)

# roundingSat
# SET(boost_dir /opt/homebrew/Cellar/boost/1.81.0_1/include)
SET(rs_dir ${lib_dir}/roundingsat/)
SET(rs_build ${rs_dir}/build/)

INCLUDE_DIRECTORIES(${boost_dir})
INCLUDE_DIRECTORIES(${rs_dir}/src)
# LINK_DIRECTORIES(${rs_build}/)

set(rs_source_files
    ${rs_dir}/src/Constr.cpp
    ${rs_dir}/src/ConstrExp.cpp
    ${rs_dir}/src/ConstrSimple.cpp
    ${rs_dir}/src/LpSolver.cpp
    ${rs_dir}/src/Solver.cpp
    ${rs_dir}/src/SolverStructs.cpp
    # ${rs_dir}/src/parsing.cpp
    ${rs_dir}/src/quit.cpp
    # ${rs_dir}/src/roundingsat.cpp
    # ${rs_dir}/src/run.cpp
    ${rs_dir}/src/used_licenses/gpl_3_0.cpp
    ${rs_dir}/src/used_licenses/zib_academic.cpp
    ${rs_dir}/src/used_licenses/roundingsat.cpp
    ${rs_dir}/src/used_licenses/MIT.cpp
    ${rs_dir}/src/used_licenses/licenses.cpp
    ${rs_dir}/src/used_licenses/boost.cpp
    ${rs_dir}/src/used_licenses/lgpl_3_0.cpp
)

# ADD_LIBRARY(rssat-lib STATIC ${RSSAT_LIB_SOURCES})
ADD_LIBRARY(rssat-lib STATIC ${rs_source_files})


INCLUDE_DIRECTORIES(${lib_dir})
INCLUDE_DIRECTORIES(src/interface)

FILE(GLOB cpp_files "src/implementation/*.cpp")

ADD_EXECUTABLE(PBCounter ${cpp_files} ${cudd_lib} ${sylvan_build})

TARGET_LINK_LIBRARIES(PBCounter cudd sylvan gmpxx gmp pthread rssat-lib)
