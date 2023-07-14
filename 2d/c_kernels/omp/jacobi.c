#include <stdlib.h>
#include <math.h>
#include "../../shared.h"

/*
 *		JACOBI SOLVER KERNEL
 */

// Initialises the Jacobi solver
void jacobi_init(
        const int x,
        const int y,
        const int halo_depth,
        const int coefficient,
        double rx,
        double ry,
        double* density,
        double* energy,
        double* u0,
        double* u,
        double* kx,
        double* ky)
{
    if(coefficient < CONDUCTIVITY && coefficient < RECIP_CONDUCTIVITY)
    {
        die(__LINE__, __FILE__, "Coefficient %d is not valid.\n", coefficient);
    }

#ifdef OMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(2)
#else
#pragma omp parallel for
#endif
        for(int jj = 1; jj < y-1; ++jj)
        {
            for(int kk = 1; kk < x-1; ++kk)
            {
                const int index = kk + jj*x;
                double temp = energy[index]*density[index];
                u0[index] = temp;
                u[index] = temp;
            }
        }

#ifdef OMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(2)
#else
#pragma omp parallel for
#endif
        for(int jj = halo_depth; jj < y-1; ++jj)
        {
            for(int kk = halo_depth; kk < x-1; ++kk)
            {
                const int index = kk + jj*x;
                double densityCentre = (coefficient == CONDUCTIVITY) 
                    ? density[index] : 1.0/density[index];
                double densityLeft = (coefficient == CONDUCTIVITY) 
                    ? density[index-1] : 1.0/density[index-1];
                double densityDown = (coefficient == CONDUCTIVITY) 
                    ? density[index-x] : 1.0/density[index-x];

                kx[index] = rx*(densityLeft+densityCentre)/(2.0*densityLeft*densityCentre);
                ky[index] = ry*(densityDown+densityCentre)/(2.0*densityDown*densityCentre);
            }
    }
}

// The main Jacobi solve step
void jacobi_iterate(
        const int x,
        const int y,
        const int halo_depth,
        double* error,
        double* kx,
        double* ky,
        double* u0,
        double* u,
        double* r)
{
#ifdef OMP_TARGET
#pragma omp target teams distribute parallel for simd collapse(2)
#else
#pragma omp parallel for
#endif
        for(int jj = 0; jj < y; ++jj)
        {
            for(int kk = 0; kk < x; ++kk)
            {
                const int index = kk + jj*x;
                r[index] = u[index];	
            }
        }

    double err=0.0;

#ifdef OMP_TARGET
#pragma omp target teams distribute parallel for simd reduction(+: err) collapse(2)
#else
#pragma omp parallel for reduction(+: err)
#endif
        for(int jj = halo_depth; jj < y-halo_depth; ++jj)
        {
            for(int kk = halo_depth; kk < x-halo_depth; ++kk)
            {
                const int index = kk + jj*x;
                u[index] = (u0[index] 
                        + (kx[index+1]*r[index+1] + kx[index]*r[index-1])
                        + (ky[index+x]*r[index+x] + ky[index]*r[index-x]))
                    / (1.0 + (kx[index]+kx[index+1])
                            + (ky[index]+ky[index+x]));

                err += fabs(u[index]-r[index]);
            }
    }

    *error = err;
}

