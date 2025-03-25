#!/usr/bin/env python3

import atlas4py as atlas

# ------------------------------------------------------------------------
# Functions
# ------------------------------------------------------------------------

class Grib:
   def read_from_file(self, path):
      import eccodes
      grib_file = open(path,'rb')
      grib_handle = eccodes.codes_grib_new_from_file(grib_file)
      if grib_handle is None: 
         raise RuntimeError('Could not read grib message from file '+path)

      self.type = eccodes.codes_get(grib_handle,'gridType')
      if self.type == 'sh':
         self.spectral_truncation = eccodes.codes_get(grib_handle,'J')
         self.gaussian_number = self.spectral_truncation + 1
         self.grid_name = "O"+str(self.gaussian_number)
      elif self.type == 'regular_gg' or self.type == 'reduced_gg':
         self.grid_name = eccodes.codes_get(grib_handle,'gridName')
         self.gaussian_number = eccodes.codes_get(grib_handle,'N')
         self.spectral_truncation = self.gaussian_number - 1 # cubic
      else:
         raise RuntimeError('Unsupported grid type '+self.type)

      self.name = eccodes.codes_get(grib_handle,'shortName')
      self.values = eccodes.codes_get_array(grib_handle,'values')

      grib_file.close()

def plot_spectrum(field_sp):
   import math
   import numpy as np
   fs = atlas.functionspace.Spectral(field_sp.functionspace)
   field_glb = fs.create_field_global(np.float64)
   fs.gather(field_sp, field_glb)

   T = fs.truncation
   if fs.part == 0:
      sp = atlas.make_view(field_glb)
      spectrum = np.empty(T+1)
      for n in range(T+1):
         v = 0.
         for m in range(n+1):
            idx = 2 * (n + 1) + m * (2 * T + 1 - m) - 1
            v += sp[idx] * sp[idx] + sp[idx - 1] * sp[idx - 1]
         spectrum[n] = math.sqrt(v)

      import matplotlib.pyplot as plt
      wavenumber = np.linspace(0, T, len(spectrum))

      # Create the plot
      plt.plot(wavenumber, spectrum)

      # Set logarithmic scale for the y-axis
      plt.yscale('log')

      # Add labels and title
      plt.xlabel('wave number')
      plt.ylabel('spectrum')
      plt.title(field_sp.name)

      print("writing file transform-spectrum.png")
      plt.savefig('transform-spectrum.png')   # save the figure to file

def plot_gridpoint(grid,field_gp):
   # Visualisation of gridpoint field with gmsh
   fs = field_gp.functionspace
   mesh = atlas.MeshGenerator(type='structured',three_dimensional=(fs.nb_parts==1)).generate(grid)
   if fs.part == 0:
      print('writing file transform-mesh.msh')
   atlas.Gmsh("transform-mesh.msh", coordinates='xyz').write(mesh).write(field_gp)

# ------------------------------------------------------------------------
# Start here
# ------------------------------------------------------------------------

def run(grib_file_path):
  atlas.initialize() # Required

  assert atlas.Trans.has_backend("ectrans")

  grib = Grib()
  grib.read_from_file(grib_file_path)

  grid = atlas.Grid(grib.grid_name)
  truncation = grib.spectral_truncation

  atlas.Trans.backend("ectrans")
  partitioner = atlas.Partitioner("ectrans")

  # Create function spaces
  fs_gp = atlas.functionspace.StructuredColumns(grid, partitioner, halo=0)
  fs_sp = atlas.functionspace.Spectral(truncation)

  trans = atlas.Trans(grid, truncation)

  # Create fields (already distributed)
  field_sp = fs_sp.create_field(name=grib.name)
  field_gp = fs_gp.create_field(name=grib.name)

  if grib.type == 'sh':
     field_sp_glb = fs_sp.create_field_global(name=grib.name)
     if fs_sp.part == 0:
        sp = atlas.make_view(field_sp_glb)
        sp[:len(grib.values)] = grib.values[:]
     fs_sp.scatter(field_sp_glb,field_sp)
     trans.invtrans(field_sp, field_gp)

  else:
     field_gp_glb = fs_gp.create_field_global(name=grib.name)
     if fs_gp.part == 0:
       gp = atlas.make_view(field_gp_glb)
       gp[:len(grib.values)] = grib.values[:]
     fs_gp.scatter(field_gp_glb,field_gp)
     trans.dirtrans(field_gp, field_sp)

  plot_gridpoint(grid,field_gp)
  plot_spectrum(field_sp)

  atlas.finalize()


def main():
    import sys
    grib_file_path = sys.argv[1]
    run(grib_file_path)

if __name__ == '__main__':
    main()
# ------------------------------------------------------------------------
