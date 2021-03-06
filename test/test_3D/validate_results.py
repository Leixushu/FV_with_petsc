# Example script for reading results
import numpy
import h5py
from matplotlib import pyplot
import subprocess
import numpy.testing

# XXX Assumes that all testing scripts are run from within their respective directories!
import sys
sys.path.insert(0, '../')
from tools import *

def validate_results(path = "", plot=True):

    # Read parameters from the input file
    param_dict = extract_params(path)

    NX = int(param_dict['NX'])
    NY = int(param_dict['NY'])
    NZ = int(param_dict['NZ'])

    dims = [NX, NY, NZ]

    phi = read_hdf5(path + "phi.h5", dims)

    # Check that it is linear
    theoretical_1D = numpy.linspace(1, 0, NX+2)[1:-1]
    theoretical_3D = numpy.broadcast_to(theoretical_1D.reshape(1,1,NX), (NZ, NY, NX))
    # Assert if error is too large
    try:
        rtol = 1.e-3
        numpy.testing.assert_allclose(phi, theoretical_3D, rtol=rtol)
        # If no exception raised so far, then it passed
        print "Ok: solution is within a factor of %f of theoretical." % rtol
    except Exception as e:
        print e

    if plot:
        pyplot.imshow(phi[NZ/2,:,:])
        pyplot.savefig("slice2D_half.png")
        pyplot.show()
        
        # Check that it is linear
        pyplot.plot(range(NX), theoretical_1D, 'r')
        pyplot.plot(range(NX), phi[NZ/2,NY/2,:], 'k-o')
        pyplot.savefig("slice1D_middle.png")
        pyplot.show()

if __name__ == '__main__':
    validate_results(plot=False)

