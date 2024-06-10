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

#include "chunk.h"
#include "settings.h"

#define NUM_GRID_DIMENSIONS 2
#define NUM_NEIGHBOURS 4
enum CART_AXIS { X_AXIS, Y_AXIS };
enum CART_NEIGHBOUR { LEFT, RIGHT, DOWN, UP };

void barrier();
void abort_comms();
void finalise_comms();
void initialise_comms(int argc, char **argv);
void initialise_ranks(Settings &settings);
void sum_over_ranks(Settings &settings, double *a);
void min_over_ranks(Settings &settings, double *a);
void send_recv_message(Settings &settings, double *send_buffer, double *recv_buffer, int buffer_len,
                       int neighbour_rank, int send_tag, int recv_tag);

void initialise_cart_topology(int x_dimension, int y_dimension, Settings &settings);
void get_cart_neighbours_rank(int offset, int neighbours_rank[]);
void get_cart_coords(int cart_rank, int cart_coords[]);