/*
This file is part of the HemoCell library

HemoCell is developed and maintained by the Computational Science Lab
in the University of Amsterdam. Any questions or remarks regarding this library
can be sent to: info@hemocell.eu

When using the HemoCell library in scientific work please cite the
corresponding paper: https://doi.org/10.3389/fphys.2017.00563

The HemoCell library is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "cellInfo.h"
#include "fluidInfo.h"
#include "hemocell.h"
#include "particleInfo.h"
#include "pltSimpleModel.h"
#include "rbcHighOrderModel.h"
#include <fenv.h>

#include "palabos3D.h"
#include "palabos3D.hh"

#ifdef SCOREP_USER_ENABLE
#include <scorep/SCOREP_User.h>
#else // SCOREP_USER_ENABLE

/* **************************************************************************************
 * Empty macros, if user instrumentation is disabled
 * *************************************************************************************/
#define SCOREP_USER_REGION_BEGIN(handle, name, type)
#define SCOREP_USER_REGION_END(handle)
#define SCOREP_USER_REGION_DEFINE(handle)
#define SCOREP_USER_REGION_TYPE_DYNAMIC 1

#endif // SCOREP_USER_ENABLE

#define WRITE_OUTPUT()      \
  if (writeOutput)          \
  {                         \
    hemocell.writeOutput(); \
  }

typedef double T;

using namespace hemo;

map<plint, plint> BlockToMpi;

void initializeLattice(HemoCell hemocell, Config *cfg)
{
}

void createBlocks(SparseBlockStructure3D *sb,
                  plint ox, plint oy, plint oz, 
                  plint nx, plint ny, plint nz, 
                  plint numBlocksX, plint numBlocksY, plint numBlocksZ){

  plint posX = ox;
  plint lx = nx / numBlocksX;
  plint ly = ny / numBlocksY;
  plint lz = nz / numBlocksZ;
  for (plint iBlockX=0; iBlockX<numBlocksX; ++iBlockX) {
    if (iBlockX < nx % numBlocksX) ++lx;
    plint posY = oy;
    for (plint iBlockY=0; iBlockY<numBlocksY; ++iBlockY) {
      if (iBlockY < ny % numBlocksY) ++ly;
      plint posZ = oz;
      for (plint iBlockZ=0; iBlockZ<numBlocksZ; ++iBlockZ) {
        if (iBlockZ < nz % numBlocksZ) ++lz;
        sb->addBlock (
                Box3D(posX, posX+lx-1, posY, posY+ly-1, posZ, posZ+lz-1),
                sb->nextIncrementalId() );
        posZ += lz;
      }
      posY += ly;
    }
    posX += lx;
  }

}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <configuration.xml>" << endl;
    return -1;
  }

  HemoCell hemocell(argv[1], argc, argv);
  Config *cfg = hemocell.cfg;

  int bin_size = 0;
  try {
    bin_size = (*cfg)["benchmark"]["binSize"].read<int>();
    hlog << "bin size : " << bin_size << endl;
  }
  catch (...) {
    bin_size = (*cfg)["sim"]["tmax"].read<int>() + 1;
  }

  bool writeOutput = 1;
  try {
    writeOutput = (*cfg)["benchmark"]["writeOutput"].read<int>();
  }
  catch (...) {}

  // number of cells along each axis
  int nx, ny, nz;
  nx = (*cfg)["domain"]["nx"].read<int>();
  ny = (*cfg)["domain"]["ny"].read<int>();
  nz = (*cfg)["domain"]["nz"].read<int>();

  hlog << "(unbounded) (Parameters) calculating flow parameters" << endl;
  param::lbm_shear_parameters((*cfg), nz);
  param::printParameters();

  hlog << "(unbounded) (Fluid) Initializing Palabos Fluid Field" << endl;

  /* ------------------------------ FLI Fluid ------------------------------------*/

  plint nProcs = global::mpi().getSize();
  plint nBlocks = nProcs;

  for (int i = nBlocks; i < nBlocks; i++) {
    BlockToMpi[i] = i;
  }
  ExplicitThreadAttribution *eta = new ExplicitThreadAttribution(BlockToMpi);

  MultiBlockManagement3D management = defaultMultiBlockPolicy3D().getMultiBlockManagement(nx, ny, nz, (*cfg)["domain"]["fluidEnvelope"].read<int>());

  Box3D const &domain = management.getBoundingBox();
  std::vector<plint> repartition = algorithm::evenRepartition(nBlocks, 3);
  std::vector<plint> newRepartition(3);
  if (domain.getNx() > domain.getNy())
  { // nx>ny
    if (domain.getNx() > domain.getNz()) { // nx>nz
      newRepartition[0] = repartition[0];
      if (domain.getNy() > domain.getNz()) { // ny>nz
        newRepartition[1] = repartition[1];
        newRepartition[2] = repartition[2];
      }
      else { // nz>ny
        newRepartition[1] = repartition[2];
        newRepartition[2] = repartition[1];
      }
    }
    else { // nz>nx
      newRepartition[2] = repartition[0];
      newRepartition[1] = repartition[2];
      newRepartition[0] = repartition[1];
    }
  }
  else { // ny>nx
    if (domain.getNy() > domain.getNz()) { // ny>nz
      newRepartition[1] = repartition[0];
      if (domain.getNx() > domain.getNz()) { // nx>nz
        newRepartition[0] = repartition[1];
        newRepartition[2] = repartition[2];
      }
      else { // nz>nx
        newRepartition[0] = repartition[2];
        newRepartition[2] = repartition[1];
      }
    }
    else { // nz>ny
      newRepartition[2] = repartition[0];
      newRepartition[1] = repartition[1];
      newRepartition[0] = repartition[2];
    }
  }

  SparseBlockStructure3D sb(domain);

  float FLIfluid = 0.0;
  try { FLIfluid = (*cfg)["benchmark"]["FLIfluid"].read<float>(); }
  catch (...) {};
 
  /* If a FLIfluid is set, we increase the size of the of blocks with x=0 to match requested fli.  */
  if (FLIfluid != 0.0){
    plint nSizeX = (nx / newRepartition[0]) * (FLIfluid + 1);
    createBlocks(&sb, 0, 0, 0, nSizeX, ny, nz, 1, newRepartition[1], newRepartition[2]);
    createBlocks(&sb, nSizeX, 0, 0, nx - nSizeX, ny, nz, newRepartition[0] - 1, newRepartition[1], newRepartition[2]);
  } else {
    createBlocks(&sb, 0, 0, 0, nx, ny, nz, newRepartition[0], newRepartition[1], newRepartition[2]);
  }

  MultiBlockManagement3D *domain_lattice_management = new MultiBlockManagement3D(sb, eta, management.getEnvelopeWidth(), management.getRefinementLevel());

  hemocell.lattice = new MultiBlockLattice3D<T, DESCRIPTOR>(*domain_lattice_management,
                                                            defaultMultiBlockPolicy3D().getBlockCommunicator(),
                                                            defaultMultiBlockPolicy3D().getCombinedStatistics(),
                                                            defaultMultiBlockPolicy3D().getMultiCellAccess<T, DESCRIPTOR>(),
                                                            new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0 / param::tau));

  /* ------------------------------ END FLI Fluid ------------------------------------*/

  OnLatticeBoundaryCondition3D<T, DESCRIPTOR> *boundaryCondition = createLocalBoundaryCondition3D<T, DESCRIPTOR>();

  hemocell.lattice->toggleInternalStatistics(false);

  // extract sides of the rectangular domain for assignment of boundary
  // conditions along the outer planes of the domain
  Box3D top = Box3D(0, nx - 1, 0, ny - 1, nz - 1, nz - 1);
  Box3D bottom = Box3D(0, nx - 1, 0, ny - 1, 0, 0);
  Box3D front = Box3D(0, nx - 1, 0, 0, 0, nz - 1);
  Box3D back = Box3D(0, nx - 1, ny - 1, ny - 1, 0, nz - 1);

  Box3D left = Box3D(0, 0, 0, ny - 1, 0, nz - 1);
  Box3D right = Box3D(nx - 1, nx - 1, 0, ny - 1, 0, nz - 1);

  // all directions have periodicity
  hemocell.lattice->periodicity().toggleAll(false);

  // bounce back conditions along the front and back of the domain
  defineDynamics(*hemocell.lattice, front, new BounceBack<T, DESCRIPTOR>);
  defineDynamics(*hemocell.lattice, back, new BounceBack<T, DESCRIPTOR>);

  defineDynamics(*hemocell.lattice, left, new BounceBack<T, DESCRIPTOR>);
  defineDynamics(*hemocell.lattice, right, new BounceBack<T, DESCRIPTOR>);

  defineDynamics(*hemocell.lattice, top, new BounceBack<T, DESCRIPTOR>);
  defineDynamics(*hemocell.lattice, bottom, new BounceBack<T, DESCRIPTOR>);

  // define shear velocity along top/bottom planes (z axis)
  // shear velocity given by `height * shear rate / 2`
  // T vHalf = (nz-1)*param::shearrate_lbm*0.5;
  T vHalf = 0;

  hemocell.latticeEquilibrium(1., plb::Array<T, 3>(0.0, 0.0, 0.0));

  // report basic information regarding the current multi-block configuration
  hlog << getMultiBlockInfo(*hemocell.lattice) << endl;

  // initialise the lattic
  hemocell.lattice->initialize();

  // initialise the cells
  hemocell.initializeCellfield();

  // the desired RBC type
  hemocell.addCellType<RbcHighOrderModel>("RBC", RBC_FROM_SPHERE);

  // define update increments
  hemocell.setMaterialTimeScaleSeparation(
      "RBC", (*cfg)["ibm"]["stepMaterialEvery"].read<int>());
  hemocell.setParticleVelocityUpdateTimeScaleSeparation(
      (*cfg)["ibm"]["stepParticleEvery"].read<int>());

  // hemocell output fields
  vector<int> outputs = {OUTPUT_POSITION, OUTPUT_TRIANGLES};
  hemocell.setOutputs("RBC", outputs);

  // LBM fluid output fields
  outputs = {OUTPUT_VELOCITY};
  hemocell.setFluidOutputs(outputs);

  // loading the cellfield
  if (not cfg->checkpointed)
  {
    hemocell.loadParticles();
    WRITE_OUTPUT();
  }
  else
  {
    hemocell.loadCheckPoint();
  }

  if (hemocell.iter == 0)
  {
    hlog << "(unbounded) fresh start: warming up cell-free fluid domain for "
         << (*cfg)["parameters"]["warmup"].read<plint>() << " iterations..."
         << endl;
    for (plint itrt = 0; itrt < (*cfg)["parameters"]["warmup"].read<plint>();
         ++itrt)
    {
      hemocell.lattice->collideAndStream();
    }
  }

  unsigned int tmax = (*cfg)["sim"]["tmax"].read<unsigned int>();
  unsigned int tmeas = (*cfg)["sim"]["tmeas"].read<unsigned int>();

  hlog << "(unbounded) Starting simulation..." << endl;
  int ncells =
      CellInformationFunctionals::getNumberOfCellsFromType(&hemocell, "RBC");
  hlog << " | RBC Volume ratio [x100%]: "
       << ncells * 77.0 * 100 / (nx * ny * nz) << endl;
  hlog << "(main)   nCells (global) = " << ncells << endl;

  SCOREP_USER_REGION_DEFINE(my_region)
  SCOREP_USER_REGION_BEGIN(my_region, "iteration-bin", SCOREP_USER_REGION_TYPE_DYNAMIC)

  while (hemocell.iter < tmax)
  {
    hemocell.iterate();

    if (hemocell.iter % bin_size == 0 && hemocell.iter != 0)
    {
      SCOREP_USER_REGION_END(my_region)
      SCOREP_USER_REGION_BEGIN(my_region, "iteration-bin", SCOREP_USER_REGION_TYPE_DYNAMIC)
    }

    if (hemocell.iter % tmeas == 0)
    {
      hlog << "(main) Stats. @ " << hemocell.iter << " ("
           << hemocell.iter * param::dt << " s):" << endl;
      hlog << "\t # of cells: "
           << CellInformationFunctionals::getTotalNumberOfCells(&hemocell);
      hlog << " | # of RBC: "
           << CellInformationFunctionals::getNumberOfCellsFromType(&hemocell,
                                                                   "RBC");
      FluidStatistics finfo = FluidInfo::calculateVelocityStatistics(&hemocell);
      double toMpS = param::dx / param::dt;
      hlog << "\t Velocity  -  max.: " << finfo.max * toMpS
           << " m/s, mean: " << finfo.avg * toMpS
           << " m/s, rel. app. viscosity: "
           << (param::u_lbm_max * 0.5) / finfo.avg << endl;
      WRITE_OUTPUT()
    }
  }

  SCOREP_USER_REGION_END(my_region)

  /*
   * Outputs the neighbouring blocks for all processes
   *
   * It uses the overlap function, based on the fact that the domain is extended to include the edge lattitice points.
   * Not very efficiant, but should not have to be run often so probably not a big problem (yet).
   */
  std::vector<PeriodicOverlap3D> overlapsP = hemocell.lattice->getMultiBlockManagement().getLocalInfo().getPeriodicOverlaps();
  std::vector<Overlap3D> overlaps = hemocell.lattice->getMultiBlockManagement().getLocalInfo().getNormalOverlaps();

  std::stringstream strings;

  strings << "(";
  for (int tmpi = 0; tmpi < (int)hemocell.lattice->getMultiBlockManagement().getLocalInfo().getBlocks().size(); tmpi++)
    strings << hemocell.lattice->getMultiBlockManagement().getLocalInfo().getBlocks()[tmpi];
  strings << ") ";

  for (int tmpi = 0; tmpi < (int)overlaps.size(); tmpi++)
  {
    bool self = false;

    for (int tmpj = 0; tmpj < (int)hemocell.lattice->getMultiBlockManagement().getLocalInfo().getBlocks().size(); tmpj++)
    {
      if (overlaps[tmpi].getOriginalId() == hemocell.lattice->getMultiBlockManagement().getLocalInfo().getBlocks()[tmpj])
      {
        self = true;
        break;
      }
    }

    if (self)
      strings << " " << overlaps[tmpi].getOverlapId();
  }
  hemo::global.statistics.addMetric("neighbours", strings.str());

  int RBCs, size;
  RBCs = size = 0;

  for (const plint &blockId : hemocell.lattice->getMultiBlockManagement().getLocalInfo().getBlocks())
  {
    // Warning: Time measurements will be inaccurate
    RBCs += (float)hemocell.cellfields->immersedParticles->getComponent(blockId).particles.size();
    int Nx = hemocell.lattice->getComponent(blockId).getNx();
    int Ny = hemocell.lattice->getComponent(blockId).getNy();
    int Nz = hemocell.lattice->getComponent(blockId).getNz();
    size += Nx * Ny * Nz;
  }

  hemo::global.statistics.addMetric("RBCs", std::to_string(RBCs));
  hemo::global.statistics.addMetric("Atomic Block Size", std::to_string(size));

  hemo::global.statistics.printStatistics();
  hemo::global.statistics.outputStatistics(128);

  WRITE_OUTPUT()

  hlog << "(main) Simulation finished :) " << endl;
  return 0;
}
