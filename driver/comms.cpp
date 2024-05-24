#include "comms.h"
#include "settings.h"

#include <iostream>

MPI_Comm cart_communicator;

// Initialise MPI
void initialise_comms(int argc, char **argv) { MPI_Init(&argc, &argv); }

// Initialise the rank information
void initialise_ranks(Settings &settings) {
  MPI_Comm_rank(MPI_COMM_WORLD, &settings.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &settings.num_ranks);
}

// Teardown MPI
void finalise_comms() { MPI_Finalize(); }

// Sends a message out and receives a message in
void send_recv_message(Settings &settings, double *send_buffer, double *recv_buffer, int buffer_len, int neighbour, int send_tag,
                       int recv_tag) {
  START_PROFILING(settings.kernel_profile);

  MPI_Sendrecv(send_buffer, buffer_len, MPI_DOUBLE, neighbour, send_tag, //
               recv_buffer, buffer_len, MPI_DOUBLE, neighbour, recv_tag, //
               cart_communicator, MPI_STATUS_IGNORE);

  STOP_PROFILING(settings.kernel_profile, __func__);
}

// Reduce over all ranks to get sum
void sum_over_ranks(Settings &settings, double *a) {
  START_PROFILING(settings.kernel_profile);
  double temp = *a;
  MPI_Allreduce(&temp, a, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

// Reduce across all ranks to get minimum value
void min_over_ranks(Settings &settings, double *a) {
  START_PROFILING(settings.kernel_profile);
  double temp = *a;
  MPI_Allreduce(&temp, a, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

// Synchronise all ranks
void barrier() { MPI_Barrier(MPI_COMM_WORLD); }

// End the application
void abort_comms() { MPI_Abort(MPI_COMM_WORLD, 1); }

// Initialise a cartesian topology, given the x and y dimensions
void initialise_cart_topology(int x_dimension, int y_dimension, Settings &settings) {
  int dims[NUM_GRID_DIMENSIONS] = {x_dimension, y_dimension};
  int periods[NUM_GRID_DIMENSIONS] = {false, false};
  int reorder = true;
  MPI_Cart_create(MPI_COMM_WORLD, NUM_GRID_DIMENSIONS, dims, periods, reorder, &cart_communicator);

  MPI_Comm_rank(cart_communicator, &settings.cart_rank);

  settings.cart_coords = (int *)malloc(sizeof(int) * NUM_GRID_DIMENSIONS);
  MPI_Cart_coords(cart_communicator, settings.cart_rank, NUM_GRID_DIMENSIONS, settings.cart_coords);
}

void get_cart_neighbours_rank(int axis, int offset, int neighbours_rank[]) {
  MPI_Cart_shift(cart_communicator, axis, offset, &neighbours_rank[0], &neighbours_rank[1]);
}