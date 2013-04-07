#include <math.h>
#include "mriCell.h"
#include "mriConstants.h"
#include "mriException.h"

MRICell::MRICell()
{
  // Initialize Cell
  concentration = 0.0;
  relPressure = 0.0;
  for(int loopA=0;loopA<kNumberOfDimensions;loopA++){
    velocity[loopA] = 0.0;
    position[loopA] = 0.0;
    filteredVel[loopA] = 0.0;
    pressGrad[loopA] = 0.0;    
  }
}

MRICell::~MRICell()
{
}

// Get Value for integer
double MRICell::getQuantity(int qtyID){
  switch(qtyID){
    case kQtyPositionX:
      return position[0];
      break;
    case kQtyPositionY:
      return position[1];
      break;
    case kQtyPositionZ:
      return position[2];
      break;
    case kQtyConcentration:
      return concentration;
      break;
    case kQtyVelocityX:
      return velocity[0];
      break;
    case kQtyVelocityY:
      return velocity[1];
      break;
    case kQtyVelocityZ:
      return velocity[2];
      break;
    case kQtyPressGradientX:
      return pressGrad[0];
      break;
    case kQtyPressGradientY:
      return pressGrad[1];
      break;
    case kQtyPressGradientZ:
      return pressGrad[2];
      break;
    case kQtyRelPressure:
      return relPressure;
      break;
    case kQtyVelModule:
      return sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1] + velocity[2]*velocity[2]);
      break;
    default:
      throw MRIStatisticsException("Error: Invalid quantity.\n");
      break;
  }
}


