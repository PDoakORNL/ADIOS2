#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_dataSpacesReader helloDataSpacesReader.cpp)
target_link_libraries(hello_dataSpacesReader adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_dataSpacesReader_mpi helloDataSpacesReader.cpp)
  target_link_libraries(hello_dataSpacesReader_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
