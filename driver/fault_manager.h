#pragma once

#ifndef NO_MPI
  // XXX OpenMPI pulls in CXX headers which we don't link against, prevent that:
  #define OMPI_SKIP_MPICXX
  #include <mpi.h>
  #if __has_include("mpi-ext.h") // C23, but everyone supports this already
    #include "mpi-ext.h"         // for CUDA-aware MPI checks
  #endif
#else
  #include "mpi_shim.h"
#endif

enum class RecvFaultToleranceStrategy { STATIC, MIRROR, BRIDGE, INTERPOLATION };

void recover_on_fault(MPI_Comm communicator, int rank, int neighbour_rank, int rc,                                   //
                      RecvFaultToleranceStrategy ft_recv_strategy, double static_value, double interpolation_factor, //
                      double *send_buffer, double *recv_buffer, int buffer_len);