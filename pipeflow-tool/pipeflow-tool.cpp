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
#include <hemocell.h>
#include <helper/voxelizeDomain.h>
#include "rbcHighOrderModel.h"
#include "pltSimpleModel.h"
#include "cellInfo.h"
#include "fluidInfo.h"
#include "particleInfo.h"
#include "writeCellInfoCSV.h"
#include <fenv.h>

#include "palabos3D.h"
#include "palabos3D.hh"

#ifdef SCOREP_USER_ENABLE
#include <scorep/SCOREP_User.h>
#else // SCOREP_USER_ENABLE

/* **************************************************************************************
 * Empty macros, if user instrumentation is disabled
 * *************************************************************************************/
#define SCOREP_USER_REGION_BEGIN(handle, name, type) hlog << "SCOREP USER not defined" << endl;
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

#include <mytool.h>

float perf_model(float V, float RBCs){
    RBCs = RBCs / 672;
    float fluid = (0.007965768126115606 + V * 2.5082866244023706 * std::pow(10,-7)) + 
                  (-0.0002188578044320071 + V * 2.1235687904379402 * std::pow(10,-8));

    float RBC = (9.24827169270812*std::pow(10,-5) + RBCs * 3.599652199729481 * std::pow(10,-5)) +
                (0.0005929407189597003 + RBCs * 8.153308088650628 * std::pow(10, -5)) +
                (-4.432855061471974 * std::pow(10,-5) *  RBCs * 2.826877556655128 * std::pow(10, -5)) +
                (2.095287766730597 * std::pow(10,-5) + RBCs * 7.4864373485193265 * std::pow(10,-6)) + 
                (0.0012118130950354064 + RBCs * 0.00025403341035855966) + 
                (0.0003094544304245024 + RBCs * 4.183950985174891 * std::pow(10, -5));

    return 4*RBC + fluid;
}

int main(int argc, char *argv[]) {
  if(argc < 2) {
      std::cout << "Usage: " << argv[0] << " <configuration.xml>" << endl;
    return -1;
  }

  HemoCell hemocell(argv[1], argc, argv);
  Config * cfg = hemocell.cfg;

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

  bool enableTool = 0;
  try {
    enableTool = (*cfg)["benchmark"]["enableTool"].read<int>();
    hlog << "Enable Tool: " << enableTool << endl;
  }
  catch (...) {}


  hlogfile << "(PipeFlow) (Geometry) reading and voxelizing STL file " << (*cfg)["domain"]["geometry"].read<string>() << endl;

  std::auto_ptr<MultiScalarField3D<int>> flagMatrix;
  std::auto_ptr<VoxelizedDomain3D<T>> voxelizedDomain;
  getFlagMatrixFromSTL((*cfg)["domain"]["geometry"].read<string>(),
                       (*cfg)["domain"]["fluidEnvelope"].read<int>(),
                       (*cfg)["domain"]["refDirN"].read<int>(),
                       (*cfg)["domain"]["refDir"].read<int>(),
                       voxelizedDomain, flagMatrix,
                       (*cfg)["domain"]["blockSize"].read<int>(),
                       (*cfg)["domain"]["particleEnvelope"].read<int>());

  param::lbm_pipe_parameters((*cfg),flagMatrix.get());
  param::printParameters();

  hemocell.lattice = new MultiBlockLattice3D<T, DESCRIPTOR>(
            voxelizedDomain.get()->getMultiBlockManagement(),
            defaultMultiBlockPolicy3D().getBlockCommunicator(),
            defaultMultiBlockPolicy3D().getCombinedStatistics(),
            defaultMultiBlockPolicy3D().getMultiCellAccess<T, DESCRIPTOR>(),
            new GuoExternalForceBGKdynamics<T, DESCRIPTOR>(1.0/param::tau));

  defineDynamics(*hemocell.lattice, *flagMatrix.get(), (*hemocell.lattice).getBoundingBox(), new BounceBack<T, DESCRIPTOR>(1.), 0);

  hemocell.lattice->toggleInternalStatistics(false);
  hemocell.lattice->periodicity().toggleAll(false);
  hemocell.latticeEquilibrium(1.,plb::Array<T, 3>(0.,0.,0.));

  //Driving Force
  T poiseuilleForce =  8 * param::nu_lbm * (param::u_lbm_max * 0.5) / param::pipe_radius / param::pipe_radius;

  hemocell.lattice->initialize();

  //Adding all the cells
  hemocell.initializeCellfield();

  hemocell.addCellType<RbcHighOrderModel>("RBC", RBC_FROM_SPHERE);
  hemocell.setMaterialTimeScaleSeparation("RBC", (*cfg)["ibm"]["stepMaterialEvery"].read<int>());
  hemocell.setInitialMinimumDistanceFromSolid("RBC", 0.5); //Micrometer! not LU

  hemocell.addCellType<PltSimpleModel>("PLT", ELLIPSOID_FROM_SPHERE);
  hemocell.setMaterialTimeScaleSeparation("PLT", (*cfg)["ibm"]["stepMaterialEvery"].read<int>());

  hemocell.setParticleVelocityUpdateTimeScaleSeparation((*cfg)["ibm"]["stepParticleEvery"].read<int>());

  //hemocell.setRepulsion((*cfg)["domain"]["kRep"].read<T>(), (*cfg)["domain"]["RepCutoff"].read<T>());
  //hemocell.setRepulsionTimeScaleSeperation((*cfg)["ibm"]["stepMaterialEvery"].read<int>());

  vector<int> outputs = {OUTPUT_POSITION,OUTPUT_TRIANGLES,OUTPUT_FORCE,OUTPUT_FORCE_VOLUME,OUTPUT_FORCE_BENDING,OUTPUT_FORCE_LINK,OUTPUT_FORCE_AREA,OUTPUT_FORCE_VISC};
  hemocell.setOutputs("RBC", outputs);
  hemocell.setOutputs("PLT", outputs);

  outputs = {OUTPUT_VELOCITY,OUTPUT_DENSITY,OUTPUT_FORCE};
  hemocell.setFluidOutputs(outputs);

  // Turn on periodicity in the X direction
  hemocell.setSystemPeriodicity(0, true);

  // Enable boundary particles
  //hemocell.enableBoundaryParticles((*cfg)["domain"]["kRep"].read<T>(), (*cfg)["domain"]["BRepCutoff"].read<T>(),(*cfg)["ibm"]["stepMaterialEvery"].read<int>());

  //loading the cellfield
  if (not cfg->checkpointed) {
    hemocell.loadParticles();
    WRITE_OUTPUT()
  } else {
    hemocell.loadCheckPoint();
  }

  //Restructure atomic blocks on processors when possible
  //hemocell.doRestructure(false); // cause errors

  if (hemocell.iter == 0) {
    hlog << "(PipeFlow) fresh start: warming up cell-free fluid domain for "  << (*cfg)["parameters"]["warmup"].read<plint>() << " iterations..." << endl;
	setExternalVector(*hemocell.lattice, (*hemocell.lattice).getBoundingBox(),
                    DESCRIPTOR<T>::ExternalField::forceBeginsAt,
                    plb::Array<T, DESCRIPTOR<T>::d>(poiseuilleForce, 0.0, 0.0));
    for (plint itrt = 0; itrt < (*cfg)["parameters"]["warmup"].read<plint>(); ++itrt) {
      hemocell.lattice->collideAndStream();
    }
  }

  unsigned int tmax = (*cfg)["sim"]["tmax"].read<unsigned int>();
  unsigned int tmeas = (*cfg)["sim"]["tmeas"].read<unsigned int>();
  unsigned int tcheckpoint = (*cfg)["sim"]["tcheckpoint"].read<unsigned int>();
  unsigned int tcsv = (*cfg)["sim"]["tcsv"].read<unsigned int>();

  hlog << "(PipeFlow) Starting simulation..." << endl;

  SCOREP_USER_REGION_DEFINE(my_region)
  SCOREP_USER_REGION_BEGIN(my_region, "Tool-phase", SCOREP_USER_REGION_TYPE_DYNAMIC)

  while (hemocell.iter < tmax ) {

    if ((hemocell.iter % 1000 == 0) && hemocell.iter != 0)
    {
      SCOREP_USER_REGION_END(my_region)
      SCOREP_USER_REGION_BEGIN(my_region, "Tool-phase", SCOREP_USER_REGION_TYPE_DYNAMIC)

      if (enableTool) {

          hlog << "using the tool" << endl;

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

          // Get the rank of the process
          int world_rank;
          MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

          tool::TS ts(world_rank);

          float wl = perf_model((float)size, (float)RBCs);

          // cout << "size: " << size << " RBCs: " << RBCs << " model: " << wl << endl;
          
          ts.exec(wl);

          hlog << "Tool done" << endl;
      }
    }

    hemocell.iterate();

    //Set driving force as required after each iteration
    setExternalVector(*hemocell.lattice, hemocell.lattice->getBoundingBox(),
                DESCRIPTOR<T>::ExternalField::forceBeginsAt,
                plb::Array<T, DESCRIPTOR<T>::d>(poiseuilleForce, 0.0, 0.0));

    if (hemocell.iter % tmeas == 0) {
        hlog << "(main) Stats. @ " <<  hemocell.iter << " (" << hemocell.iter * param::dt << " s):" << endl;
        hlog << "\t # of cells: " << CellInformationFunctionals::getTotalNumberOfCells(&hemocell);
        hlog << " | # of RBC: " << CellInformationFunctionals::getNumberOfCellsFromType(&hemocell, "RBC");
        hlog << ", PLT: " << CellInformationFunctionals::getNumberOfCellsFromType(&hemocell, "PLT") << endl;
        FluidStatistics finfo = FluidInfo::calculateVelocityStatistics(&hemocell); T toMpS = param::dx / param::dt;
        hlog << "\t Velocity  -  max.: " << finfo.max * toMpS << " m/s, mean: " << finfo.avg * toMpS<< " m/s, rel. app. viscosity: " << (param::u_lbm_max*0.5) / finfo.avg << endl;
        ParticleStatistics pinfo = ParticleInfo::calculateForceStatistics(&hemocell); T topN = param::df * 1.0e12;
        hlog << "\t Force  -  min.: " << pinfo.min * topN << " pN, max.: " << pinfo.max * topN << " pN (" << pinfo.max << " lf), mean: " << pinfo.avg * topN << " pN" << endl;

        // Additional useful stats, if needed
        //finfo = FluidInfo::calculateForceStatistics(&hemocell);
        //Set force as required after this function;
        // setExternalVector(*hemocell.lattice, hemocell.lattice->getBoundingBox(),
        //           DESCRIPTOR<T>::ExternalField::forceBeginsAt,
        //           hemo::Array<T, DESCRIPTOR<T>::d>(poiseuilleForce, 0.0, 0.0));
        // pcout << "Fluid force, Minimum: " << finfo.min << " Maximum: " << finfo.max << " Average: " << finfo.avg << endl;
        // ParticleStatistics pinfo = ParticleInfo::calculateVelocityStatistics(&hemocell);
        // pcout << "Particle velocity, Minimum: " << pinfo.min << " Maximum: " << pinfo.max << " Average: " << pinfo.avg << endl;
        WRITE_OUTPUT()
    }



    if (hemocell.iter % tcsv == 0) {
      hlog << "Saving simple mean cell values to CSV at timestep " << hemocell.iter << endl;
      writeCellInfo_CSV(hemocell);
    }

    if (hemocell.iter % tcheckpoint == 0) {
       hemocell.saveCheckPoint();
    }
  }

  SCOREP_USER_REGION_END(my_region)
  WRITE_OUTPUT()

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

  hlog << "(main) Simulation finished :) " << endl;

  return 0;
}
