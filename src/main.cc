// Alta Fang, 2017
//
// Solve Poisson's equation in 3D using PETSc.
// 
// To run on two processors, execute  for example:
//    mpiexec -np 2 ./main

#include <petscsys.h>
#include "field.hpp"
#include "nonlocal_field.hpp"
#include "input_tools.hpp"
#include "linear_solver.hpp"
#include <math.h>

// namespace for the constants from the input file
namespace model
{
    int NX;
    int NY;
    int NZ;
    
    double PHI_APPLIED;
}

void setup_params(void)
{
    // Read input parameters from text file called "input.txt"
    std::map<std::string, std::string> params;
    readParameters(params);
    
    unpack(params, "NX", model::NX);
    unpack(params, "NY", model::NY);
    unpack(params, "NZ", model::NZ);
    
    unpack(params, "PHI_APPLIED", model::PHI_APPLIED);
}

int main(int argc,char **args)
{
    PetscInitialize(&argc,&args,NULL,NULL);
    
    // Unpack the input parameters.
    setup_params();
    
    // Set up the distributed array used for all fields.
    DM da;
    // Note: DM_BOUNDARY_MIRROR is not yet implemented in 3D so have to manually fill those cells
    DMDACreate3d(PETSC_COMM_WORLD, DM_BOUNDARY_GHOSTED, DM_BOUNDARY_GHOSTED, DM_BOUNDARY_GHOSTED, DMDA_STENCIL_STAR, \
                 model::NX, model::NY, model::NZ, 1, 1, PETSC_DECIDE, 1, 1, PETSC_NULL, PETSC_NULL, PETSC_NULL, &da);
    
    BC bc_phi;
    bc_phi.upper_BC_type = 1;
    bc_phi.upper_BC_val = model::PHI_APPLIED;
    bc_phi.lower_BC_type = 1;
    bc_phi.lower_BC_val = 0.;
    NonLocalField *phi = new NonLocalField(&da, &bc_phi);
    PetscObjectSetName((PetscObject)phi->global_vec, "phi");
    
    BC bc_sigma;
    bc_sigma.upper_BC_type = 0;
    bc_sigma.upper_BC_val = 0.;
    bc_sigma.lower_BC_type = 0;
    bc_sigma.lower_BC_val = 0.;
    NonLocalField *sigma = new NonLocalField(&da, &bc_sigma);
    PetscObjectSetName((PetscObject)sigma->global_vec, "sigma");
    
    // Read sigma in from hdf5 file
    sigma->read_from_file("sigma", 0);
    sigma->send_global_to_local();

    // Get coords of where this thread is in the global domain.
    int xs, ys, zs, xm, ym, zm, i, j, k;
    DMDAGetCorners(da, &xs, &ys, &zs, &xm, &ym, &zm);

    // Fill in initial values.
    for (i = zs; i < zs+zm; i++)
    {
        for (j = ys; j < ys+ym; j++)
        {
            for (k = xs; k < xs+xm; k++)
            {
                phi->global_array[i][j][k] = 0.;
            }
        }
    }
    
    // Create the objects necessary for solving
    LinearSolver *linearsolver = new LinearSolver(&da, phi, sigma);
    
    // Make ghost rows available
    sigma->send_global_to_local();
    phi->send_global_to_local();
    
    // Do matrix solve to get the potential phi.
    linearsolver->run_solver();
    
    // Save the solution to a file
    phi->write_to_file("phi", 0);
    
    // Cleanup
    delete phi;
    delete sigma;
    
    delete linearsolver;
    
    DMDestroy(&da);
    
    PetscFinalize();
    
    return 0;
}
