/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.tcc : specialization of template functions defined in
 * adiosMPIFunctions.h
 *
 *  Created on: Aug 30, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_

#include "adiosMPIFunctions.h"

#include <algorithm> //std::foreach
#include <iostream>  //TODO: remove
#include <numeric>   //std::accumulate

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
// BroadcastValue specializations
template <>
size_t BroadcastValue(const size_t &input, MPI_Comm mpiComm,
                      const int rankSource)
{
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    size_t output = 0;

    if (rank == rankSource)
    {
        output = input;
    }

    MPI_Bcast(&output, 1, ADIOS2_MPI_SIZE_T, rankSource, mpiComm);

    return output;
}

template <>
std::string BroadcastValue(const std::string &input, MPI_Comm mpiComm,
                           const int rankSource)
{
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    const size_t inputSize = input.size();
    const size_t length = BroadcastValue(inputSize, mpiComm, rankSource);
    std::string output;

    if (rank == rankSource)
    {
        output = input;
    }
    else
    {
        output.resize(length);
    }

    MPI_Bcast(const_cast<char *>(output.data()), static_cast<int>(length),
              MPI_CHAR, rankSource, mpiComm);

    return output;
}

// ReduceValue specializations
template <>
unsigned int ReduceValues(const unsigned int source, MPI_Comm mpiComm,
                          MPI_Op operation, const int rankDestination)
{
    const unsigned int sourceLocal = source;
    unsigned int reduceValue = 0;
    MPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED, operation,
               rankDestination, mpiComm);
    return reduceValue;
}

template <>
unsigned long int ReduceValues(const unsigned long int source, MPI_Comm mpiComm,
                               MPI_Op operation, const int rankDestination)
{
    const unsigned long int sourceLocal = source;
    unsigned long int reduceValue = 0;
    MPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG, operation,
               rankDestination, mpiComm);
    return reduceValue;
}

template <>
unsigned long long int ReduceValues(const unsigned long long int source,
                                    MPI_Comm mpiComm, MPI_Op operation,
                                    const int rankDestination)
{
    const unsigned long long int sourceLocal = source;
    unsigned long long int reduceValue = 0;
    MPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG_LONG, operation,
               rankDestination, mpiComm);
    return reduceValue;
}

// BroadcastVector specializations
template <>
std::vector<char> BroadcastVector(const std::vector<char> &input,
                                  MPI_Comm mpiComm, const int rankSource)
{
    // First Broadcast the size, then the contents
    size_t inputSize = BroadcastValue(input.size(), mpiComm, rankSource);
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    std::vector<char> output;

    if (rank == rankSource)
    {
        MPI_Bcast(const_cast<char *>(input.data()), static_cast<int>(inputSize),
                  MPI_CHAR, rankSource, mpiComm);
        return input; // no copy
    }
    else
    {
        output.resize(inputSize);
        MPI_Bcast(output.data(), static_cast<int>(inputSize), MPI_CHAR,
                  rankSource, mpiComm);
    }

    return output;
}

template <>
std::vector<size_t> BroadcastVector(const std::vector<size_t> &input,
                                    MPI_Comm mpiComm, const int rankSource)
{
    // First Broadcast the size, then the contents
    size_t inputSize = BroadcastValue(input.size(), mpiComm, rankSource);
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    std::vector<size_t> output;

    if (rank == rankSource)
    {
        MPI_Bcast(const_cast<size_t *>(input.data()),
                  static_cast<int>(inputSize), ADIOS2_MPI_SIZE_T, rankSource,
                  mpiComm);
        return input; // no copy in rankSource
    }
    else
    {
        output.resize(inputSize);
        MPI_Bcast(output.data(), static_cast<int>(inputSize), ADIOS2_MPI_SIZE_T,
                  rankSource, mpiComm);
    }

    return output;
}

// GatherArrays specializations
template <>
void GatherArrays(const char *source, const size_t sourceCount,
                  char *destination, MPI_Comm mpiComm,
                  const int rankDestination)
{
    const int countsInt = static_cast<int>(sourceCount);
    int result = MPI_Gather(source, countsInt, MPI_CHAR, destination, countsInt,
                            MPI_CHAR, rankDestination, mpiComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type MPI_CHAR function\n");
    }
}

template <>
void GatherArrays(const size_t *source, const size_t sourceCount,
                  size_t *destination, MPI_Comm mpiComm,
                  const int rankDestination)
{
    const int countsInt = static_cast<int>(sourceCount);
    int result =
        MPI_Gather(source, countsInt, ADIOS2_MPI_SIZE_T, destination, countsInt,
                   ADIOS2_MPI_SIZE_T, rankDestination, mpiComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

// GathervArrays specializations
template <>
void GathervArrays(const char *source, const size_t sourceCount,
                   const size_t *counts, const size_t countsSize,
                   char *destination, MPI_Comm mpiComm,
                   const int rankDestination)
{
    int result = 0;
    int rank;
    MPI_Comm_rank(mpiComm, &rank);

    std::vector<int> countsInt, displacementsInt;

    if (rank == rankDestination)
    {
        countsInt = NewVectorTypeFromArray<size_t, int>(counts, countsSize);
        displacementsInt = GetGathervDisplacements(counts, countsSize);
    }

    result = MPI_Gatherv(source, static_cast<int>(sourceCount), MPI_CHAR,
                         destination, countsInt.data(), displacementsInt.data(),
                         MPI_CHAR, rankDestination, mpiComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gatherv type MPI_CHAR function\n");
    }
}

template <>
void GathervArrays(const size_t *source, const size_t sourceCount,
                   const size_t *counts, const size_t countsSize,
                   size_t *destination, MPI_Comm mpiComm,
                   const int rankDestination)
{
    int result = 0;
    int rank;
    MPI_Comm_rank(mpiComm, &rank);

    std::vector<int> countsInt =
        NewVectorTypeFromArray<size_t, int>(counts, countsSize);

    std::vector<int> displacementsInt =
        GetGathervDisplacements(counts, countsSize);

    result =
        MPI_Gatherv(source, static_cast<int>(sourceCount), ADIOS2_MPI_SIZE_T,
                    destination, countsInt.data(), displacementsInt.data(),
                    ADIOS2_MPI_SIZE_T, rankDestination, mpiComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_ */