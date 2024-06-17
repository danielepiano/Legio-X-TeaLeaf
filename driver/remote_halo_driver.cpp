#include "chunk.h"
#include "comms.h"
#include "drivers.h"
#include "kernel_interface.h"

#include <iostream>

// Needs with-cartesian-bridge install !!
void interpolation_recovery(Chunk *chunk, Settings &settings,                            //
                            double *send_buffer, double *recv_buffer,                   //
                            int neighbour_rank, enum CART_NEIGHBOUR neighbour_direction //
) {
  int neighbour_coords[NUM_GRID_DIMENSIONS], dead_neighbours;
  get_cart_coords(neighbour_rank, neighbour_coords);

  // Count how many fields are being shared
  int num_fields = 0;
  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (settings.fields_to_exchange[ii]) {
      ++num_fields;
    }
  }

  int height = chunk->y - 2 * settings.halo_depth;
  int width = chunk->x - 2 * settings.halo_depth;
  int buffer_offset = 0;

  switch (neighbour_direction) {
    case LEFT: {
      dead_neighbours = settings.cart_coords[X_AXIS] - neighbour_coords[X_AXIS] - 1;
      if (!dead_neighbours) return; // If it's not the node next to me, then interpolate between received halo and my internal halo

      double *approx = new double[2 * height];

      // Iterate and approximate each field shared...
      for (int ff = 0; ff < num_fields; ++ff) {
        // Get
        for (int rr = 0; rr < height; ++rr) {
          // Right-most of each left row  -  left-most of each right row ...
          double delta = send_buffer[buffer_offset + rr * 2] - recv_buffer[buffer_offset + rr * 2 + 1];
          delta /= width * dead_neighbours + 1; // ... divided by the number of cells in between

          for (int cc = 0; cc < 2; ++cc) {
            int factor = (cc == 0) ? 2 : 1;
            approx[rr * 2 + cc] = send_buffer[buffer_offset + rr * 2] - factor * delta;
          }
        }
        // Replace interpolated values
        for (int ii = 0; ii < height * 2; ++ii) {
          recv_buffer[buffer_offset + ii] = approx[ii];
        }
        // Free temp array 'approx'
        delete[] approx;
        // Go on with the next field to interpolate
        buffer_offset += height * 2;
      }
      break;
    }
    case RIGHT: {
      dead_neighbours = neighbour_coords[X_AXIS] - settings.cart_coords[X_AXIS] - 1;
      if (!dead_neighbours) return; // If it's not the node next to me, then interpolate between received halo and my internal halo

      double *approx = new double[2 * height];

      // Iterate and approximate each field shared...
      for (int ff = 0; ff < num_fields; ++ff) {
        // Get
        for (int rr = 0; rr < height; ++rr) {
          // Right-most of each left row  -  left-most of each right row ...
          double delta = send_buffer[buffer_offset + rr * 2 + 1] - recv_buffer[buffer_offset + rr * 2];
          delta /= width * dead_neighbours + 1; // ... divided by the number of cells in between

          for (int cc = 0; cc < 2; ++cc) {
            approx[rr * 2 + cc] = send_buffer[buffer_offset + rr * 2 + 1] - (cc + 1) * delta;
          }
        }
        // Replace interpolated values
        for (int ii = 0; ii < height * 2; ++ii) {
          recv_buffer[buffer_offset + ii] = approx[ii];
        }
        // Free temp array 'approx'
        delete[] approx;
        // Go on with the next field to interpolate
        buffer_offset += height * 2;
      }
      break;
    }
    case DOWN: {
      dead_neighbours = settings.cart_coords[Y_AXIS] - neighbour_coords[Y_AXIS] - 1;
      if (!dead_neighbours) return; // If it's not the node next to me, then interpolate between received halo and my internal halo

      double *approx = new double[2 * width];

      // Iterate and approximate each field shared...
      for (int ff = 0; ff < num_fields; ++ff) {
        // Get
        for (int cc = 0; cc < width; ++cc) {
          // Down-most sent row  -  top-most received row ...
          double delta = send_buffer[buffer_offset + width + cc] - recv_buffer[buffer_offset + cc];
          delta /= height * dead_neighbours + 1; // ... divided by the number of cells in between

          for (int rr = 0; rr < 2; ++rr) {
            approx[rr * width + cc] = send_buffer[buffer_offset + width + cc] - (rr + 1) * delta;
          }
        }
        // Replace interpolated values
        for (int ii = 0; ii < width * 2; ++ii) {
          recv_buffer[buffer_offset + ii] = approx[ii];
        }
        // Free temp array 'approx'
        delete[] approx;
        // Go on with the next field to interpolate
        buffer_offset += width * 2;
      }
      break;
    }
    case UP: {
      dead_neighbours = neighbour_coords[Y_AXIS] - settings.cart_coords[Y_AXIS] - 1;
      if (!dead_neighbours) return; // If it's not the node next to me, then interpolate between received halo and my internal halo

      double *approx = new double[2 * width];

      // Iterate and approximate each field shared...
      for (int ff = 0; ff < num_fields; ++ff) {
        // Get
        for (int cc = 0; cc < width; ++cc) {
          // Down-most sent row  -  top-most received row ...
          double delta = send_buffer[buffer_offset + cc] - recv_buffer[buffer_offset + width + cc];
          delta /= height * dead_neighbours + 1; // ... divided by the number of cells in between

          for (int rr = 0; rr < 2; ++rr) {
            int factor = (rr == 0) ? 2 : 1;
            approx[rr * width + cc] = send_buffer[buffer_offset + cc] - factor * delta;
          }
        }
        // Replace interpolated values
        for (int ii = 0; ii < width * 2; ++ii) {
          recv_buffer[buffer_offset + ii] = approx[ii];
        }
        // Free temp array 'approx'
        delete[] approx;
        // Go on with the next field to interpolate
        buffer_offset += width * 2;
      }
      break;
    }
  }
}

// Attempts to pack buffers
int invoke_pack_or_unpack(Chunk *chunk, Settings &settings, int face, int depth, int offset, bool pack, FieldBufferType buffer) {
  int buffer_len = 0;

  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) {
      continue;
    }

    FieldBufferType field;
    switch (ii) {
      case FIELD_DENSITY: field = chunk->density; break;
      case FIELD_ENERGY0: field = chunk->energy0; break;
      case FIELD_ENERGY1: field = chunk->energy; break;
      case FIELD_U: field = chunk->u; break;
      case FIELD_P: field = chunk->p; break;
      case FIELD_SD: field = chunk->sd; break;
      default: die(__LINE__, __FILE__, "Incorrect field provided: %d.\n", ii + 1);
    }

    //    double *offset_buffer = buffer + buffer_len;
    //    buffer_len += depth * offset;
    //
    //    if (settings.kernel_language == Kernel_Language::C) {
    //      run_pack_or_unpack(chunk, settings, depth, face, pack, field, offset_buffer);
    //    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
    //    }

    if (settings.kernel_language == Kernel_Language::C) {
      run_pack_or_unpack(chunk, settings, depth, face, pack, field, buffer, buffer_len);
    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
    }
    buffer_len += depth * offset;
  }

  return buffer_len;
}

// Invokes the kernels that perform remote halo exchanges
void remote_halo_driver(Chunk *chunks, Settings &settings, int depth) {
#ifndef NO_MPI
  int neighbour_ranks[NUM_NEIGHBOURS], neighbour_offset = 1;
  get_cart_neighbour_ranks(neighbour_offset, neighbour_ranks);

  // Pack lr buffers and send messages
  if (neighbour_ranks[LEFT] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_LEFT, depth, chunks[0].y, true, chunks[0].left_send);
    run_send_recv_halo(&chunks[0], settings,                                                 //
                       chunks[0].left_send, chunks[0].left_recv,                             //
                       chunks[0].staging_left_send, chunks[0].staging_left_recv, buffer_len, //
                       neighbour_ranks[LEFT], 0, 1);
    if (settings.recv_ft_strategy == RecvFaultToleranceStrategy::INTERPOLATION) {
      interpolation_recovery(&chunks[0], settings, chunks[0].left_send, chunks[0].left_recv, neighbour_ranks[LEFT], LEFT);
    }
  }
  if (neighbour_ranks[RIGHT] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_RIGHT, depth, chunks[0].y, true, chunks[0].right_send);
    run_send_recv_halo(&chunks[0], settings,                                                   //
                       chunks[0].right_send, chunks[0].right_recv,                             //
                       chunks[0].staging_right_send, chunks[0].staging_right_recv, buffer_len, //
                       neighbour_ranks[RIGHT], 1, 0);
    if (settings.recv_ft_strategy == RecvFaultToleranceStrategy::INTERPOLATION) {
      interpolation_recovery(&chunks[0], settings, chunks[0].right_send, chunks[0].right_recv, neighbour_ranks[RIGHT], RIGHT);
    }
  }

  int buffer_len = 0;
  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) continue;
    buffer_len += depth * chunks[0].y;
  }
  // actually, forked version of TeaLeaf does not allow more than 1 chunk per rank !
  if (neighbour_ranks[LEFT] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].left_recv, chunks[0].staging_left_recv, buffer_len);
  }
  if (neighbour_ranks[RIGHT] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].right_recv, chunks[0].staging_right_recv, buffer_len);
  }

  // Unpack lr buffers
  // actually, forked version of TeaLeaf does not allow more than 1 chunk per rank !
  if (neighbour_ranks[LEFT] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_LEFT, depth, chunks[0].y, false, chunks[0].left_recv);
  }
  if (neighbour_ranks[RIGHT] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_RIGHT, depth, chunks[0].y, false, chunks[0].right_recv);
  }

  // Pack tb buffers and send messages
  if (neighbour_ranks[DOWN] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_BOTTOM, depth, chunks[0].x, true, chunks[0].bottom_send);
    run_send_recv_halo(&chunks[0], settings,                                                     //
                       chunks[0].bottom_send, chunks[0].bottom_recv,                             //
                       chunks[0].staging_bottom_send, chunks[0].staging_bottom_recv, buffer_len, //
                       neighbour_ranks[DOWN], 0, 1);
    if (settings.recv_ft_strategy == RecvFaultToleranceStrategy::INTERPOLATION) {
      interpolation_recovery(&chunks[0], settings, chunks[0].bottom_send, chunks[0].bottom_recv, neighbour_ranks[DOWN], DOWN);
    }
  }
  if (neighbour_ranks[UP] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_TOP, depth, chunks[0].x, true, chunks[0].top_send);
    run_send_recv_halo(&chunks[0], settings,                                               //
                       chunks[0].top_send, chunks[0].top_recv,                             //
                       chunks[0].staging_top_send, chunks[0].staging_top_recv, buffer_len, //
                       neighbour_ranks[UP], 1, 0);
    if (settings.recv_ft_strategy == RecvFaultToleranceStrategy::INTERPOLATION) {
      interpolation_recovery(&chunks[0], settings, chunks[0].top_send, chunks[0].top_recv, neighbour_ranks[UP], UP);
    }
  }

  buffer_len = 0;
  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) continue;
    buffer_len += depth * chunks[0].x;
  }
  if (neighbour_ranks[DOWN] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].bottom_recv, chunks[0].staging_bottom_recv, buffer_len);
  }
  if (neighbour_ranks[UP] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].top_recv, chunks[0].staging_top_recv, buffer_len);
  }

  // Unpack tb buffers
  if (neighbour_ranks[DOWN] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_BOTTOM, depth, chunks[0].x, false, chunks[0].bottom_recv);
  }
  if (neighbour_ranks[UP] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_TOP, depth, chunks[0].x, false, chunks[0].top_recv);
  }

#endif
}
