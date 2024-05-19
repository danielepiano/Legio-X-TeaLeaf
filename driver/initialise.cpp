#include <cfloat>
#include <cstring>

#include "application.h"
#include "chunk.h"
#include "comms.h"
#include "drivers.h"
#include "kernel_interface.h"
#include "settings.h"

// Decomposes the field into multiple chunks
void decompose_field(Settings &settings, Chunk *chunks) {
  // Calculates the num chunks field is to be decomposed into
  settings.num_chunks = settings.num_ranks * settings.num_chunks_per_rank;

  int num_chunks = settings.num_chunks;

  double best_metric = DBL_MAX;
  auto x_cells = static_cast<double>(settings.grid_x_cells);
  auto y_cells = static_cast<double>(settings.grid_y_cells);
  int x_chunks = 0;
  int y_chunks = 0;

  // Decompose by minimal area to perimeter
  for (int xx = 1; xx <= num_chunks; ++xx) {
    if (num_chunks % xx) continue;

    // Calculate number of chunks grouped by x split
    int yy = num_chunks / xx;

    if (num_chunks % yy) continue;

    double perimeter = ((x_cells / xx) * (x_cells / xx) + (y_cells / yy) * (y_cells / yy)) * 2;
    double area = (x_cells / xx) * (y_cells / yy);

    double current_metric = perimeter / area;

    // Save improved decompositions
    if (current_metric < best_metric) {
      x_chunks = xx;
      y_chunks = yy;
      best_metric = current_metric;
    }
  }

  // Check that the decomposition didn't fail
  if (!x_chunks || !y_chunks) {
    die(__LINE__, __FILE__, "Failed to decompose the field with given parameters.\n");
  }

  settings.grid_x_chunks = x_chunks;
  settings.grid_y_chunks = y_chunks;

  // Initialise a cartesian topology given the number of ranks calculated along X and Y axis
  initialise_cart_topology(settings.grid_x_chunks, settings.grid_y_chunks, settings);

  int dx = settings.grid_x_cells / x_chunks;
  int dy = settings.grid_y_cells / y_chunks;

  int mod_x = settings.grid_x_cells % x_chunks;
  int mod_y = settings.grid_y_cells % y_chunks;
  int add_x_prev = 0;
  int add_y_prev = 0;

  // Compute the full decomposition on all ranks
  for (int yy = 0; yy < settings.grid_y_chunks; ++yy) {
    int add_y = (yy < mod_y);

    for (int xx = 0; xx < settings.grid_x_chunks; ++xx) {
      int add_x = (xx < mod_x);

      if (xx == settings.cart_coords[X_AXIS] && yy == settings.cart_coords[Y_AXIS]) {
        // [0] because forked version of TeaLeaf does not allow more than 1 chunk per rank !
        initialise_chunk(&(chunks[0]), settings, dx + add_x, dy + add_y);

        // Set up the mesh ranges
        chunks[0].left = xx * dx + add_x_prev;
        chunks[0].right = chunks[0].left + dx + add_x;
        chunks[0].bottom = yy * dy + add_y_prev;
        chunks[0].top = chunks[0].bottom + dy + add_y;
      }

      // If chunks rounded up, maintain relative location
      add_x_prev += add_x;
    }
    add_x_prev = 0;
    add_y_prev += add_y;
  }
}

void initialise_model_info(Settings &settings) { run_model_info(settings); }

// Initialise settings from input file
void initialise_application(Chunk **chunks, Settings &settings, State *states) {

  *chunks = (Chunk *)malloc(sizeof(Chunk) * settings.num_chunks_per_rank);

  decompose_field(settings, *chunks);
  kernel_initialise_driver(*chunks, settings);
  set_chunk_data_driver(*chunks, settings);
  set_chunk_state_driver(*chunks, settings, states);

  // Prime the initial halo data
  reset_fields_to_exchange(settings);
  settings.fields_to_exchange[FIELD_DENSITY] = true; // start.f90:111
  settings.fields_to_exchange[FIELD_ENERGY0] = true; // start.f90:112
  settings.fields_to_exchange[FIELD_ENERGY1] = true; // start.f90:113
  halo_update_driver(*chunks, settings, 2);

  store_energy_driver(*chunks, settings);
}
