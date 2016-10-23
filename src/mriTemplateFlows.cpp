# include "mriScan.h"

using namespace std;

// STAGNATION FLOW SOLUTION
void MRIScan::assignStagnationFlowSignature(MRIDirection dir){
  double bConst = -1.0;
  double xCoord,yCoord,zCoord;
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    // Set to Zero
    cells[loopA].concentration = 0.0;
    switch(dir){
      case kdirX:
        yCoord = topology->cellLocations[loopA][1] - topology->domainSizeMin[1];
        zCoord = topology->cellLocations[loopA][2] - 0.5*(topology->domainSizeMin[2]+topology->domainSizeMax[2]);
        cells[loopA].velocity[0] = 0.0;
        cells[loopA].velocity[1] = -bConst * yCoord;
        cells[loopA].velocity[2] = bConst * zCoord;
        break;
      case kdirY:
        xCoord = topology->cellLocations[loopA][0] - topology->domainSizeMin[0];
        zCoord = topology->cellLocations[loopA][2]- 0.5 * (topology->domainSizeMin[2] + topology->domainSizeMax[2]);
        cells[loopA].velocity[0] = bConst * xCoord;
        cells[loopA].velocity[1] = 0.0;
        cells[loopA].velocity[2] = -bConst * zCoord;
        break;
      case kdirZ:
        xCoord = topology->cellLocations[loopA][0] - topology->domainSizeMin[0];
        yCoord = topology->cellLocations[loopA][1] - 0.5 * (topology->domainSizeMin[1] + topology->domainSizeMax[1]);
        cells[loopA].velocity[0] = bConst * xCoord;
        cells[loopA].velocity[1] = -bConst * yCoord;
        cells[loopA].velocity[2] = 0.0;
        break;
    }
    cells[loopA].concentration = 1.0;
  }
}

// EVAL TANGENT DIRECTION
void evalTangentDirection(MRIDirection dir, const MRIDoubleVec& radialVector, MRIDoubleVec& tangVector){
  // Create Axial Direction
  MRIDoubleVec axialVec(3,0.0);
  switch(dir){
    case kdirX:
      axialVec[0] = 1.0;
      axialVec[1] = 0.0;
      axialVec[2] = 0.0;
      break;
    case kdirY:
      axialVec[0] = 0.0;
      axialVec[1] = 1.0;
      axialVec[2] = 0.0;
      break;
    case kdirZ:
      axialVec[0] = 0.0;
      axialVec[1] = 0.0;
      axialVec[2] = 1.0;
      break;    
  }
  // Perform External Product
  MRIUtils::do3DExternalProduct(axialVec,radialVector,tangVector);
  // Normalize Result
  MRIUtils::normalize3DVector(tangVector);
}

// ASSIGN CYLINDRICAL VORTEX FLOW 
void MRIScan::assignCylindricalFlowSignature(MRIDirection dir){
  // Set Parameters
  // Get Min Dimension
  double minDist = min((topology->domainSizeMax[0]-topology->domainSizeMin[0]),
                   min((topology->domainSizeMax[1]-topology->domainSizeMin[1]),
                       (topology->domainSizeMax[2]-topology->domainSizeMin[2])));
  // Set the Minimum and Maximum Radius
  const double minRadius = minDist * 0.1;
  const double maxRadius = minDist * 0.4;
  const double velMod = 10.0;
  // Init Coords
  double currRadius = 0.0;
  MRIDoubleVec radialVector(3,0.0);
  MRIDoubleVec tangVector(3,0.0);
  // Find the Centre Of the Domain
  MRIDoubleVec centrePoint(3,0.0);
  centrePoint[0] = 0.5 * (topology->domainSizeMax[0] + topology->domainSizeMin[0]);
  centrePoint[1] = 0.5 * (topology->domainSizeMax[1] + topology->domainSizeMin[1]);
  centrePoint[2] = 0.5 * (topology->domainSizeMax[2] + topology->domainSizeMin[2]);
  // Assign Cell Velocities
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    switch(dir){
      case kdirX:
        // Get Current Radius
        radialVector[0] = 0.0;
        radialVector[1] = topology->cellLocations[loopA][1] - centrePoint[1];
        radialVector[2] = topology->cellLocations[loopA][2] - centrePoint[2];
        break;
      case kdirY:
        // Get Current Radius
        radialVector[0] = topology->cellLocations[loopA][0] - centrePoint[0];
        radialVector[1] = 0.0;
        radialVector[2] = topology->cellLocations[loopA][2] - centrePoint[2];      
        break;
      case kdirZ:
        // Get Current Radius
        radialVector[0] = topology->cellLocations[loopA][0] - centrePoint[0];
        radialVector[1] = topology->cellLocations[loopA][1] - centrePoint[1];
        radialVector[2] = 0.0;
        break;
    }
    // Normalize Radial Direction
    currRadius = MRIUtils::do3DEucNorm(radialVector);
    MRIUtils::normalize3DVector(radialVector);
    // Eval Tangential Direction
    evalTangentDirection(dir,radialVector,tangVector);
    // Set Velocities    
    if ((currRadius>=minRadius)&&(currRadius<=maxRadius)){
      cells[loopA].velocity[0] = tangVector[0]*velMod;
      cells[loopA].velocity[1] = tangVector[1]*velMod;
      cells[loopA].velocity[2] = tangVector[2]*velMod;
      cells[loopA].concentration = 1.0;
    }else{
      cells[loopA].concentration = 0.0;
    }
  }  
}

// ASSIGN SPHERICAL HILL VORTEX FLOW 
void MRIScan::assignSphericalFlowSignature(MRIDirection dir){
  // Set Parameters
  double minDist = min(topology->domainSizeMax[0]-topology->domainSizeMin[0],
                   min(topology->domainSizeMax[1]-topology->domainSizeMin[1],
                       topology->domainSizeMax[2]-topology->domainSizeMin[2]));
  const double CONST_U0 = 0.1;
  const double CONST_A = minDist * 0.4;
  // Init Local Coords
  double currentX = 0.0;
  double currentY = 0.0;
  double currentZ = 0.0;
  double localR = 0.0;
  double localZ = 0.0; 
  double axialComponentIn = 0.0;
  double radialComponentIn = 0.0;
  double axialComponentOut = 0.0;
  double radialComponentOut = 0.0;  
  // Allocate Radial and Axial Vectors
  MRIDoubleVec axialVec(3,0.0);
  MRIDoubleVec radialVec(3,0.0);
  // Find the Centre Of the Domain
  MRIDoubleVec centrePoint(3,0.0);
  centrePoint[0] = 0.5 * (topology->domainSizeMax[0]+topology->domainSizeMin[0]);
  centrePoint[1] = 0.5 * (topology->domainSizeMax[1]+topology->domainSizeMin[1]);
  centrePoint[2] = 0.5 * (topology->domainSizeMax[2]+topology->domainSizeMin[2]);
  // Assign Cell Velocities
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    // Get The Three Coordinates
    currentX = topology->cellLocations[loopA][0] - centrePoint[0];
    currentY = topology->cellLocations[loopA][1] - centrePoint[1];
    currentZ = topology->cellLocations[loopA][2] - centrePoint[2];
    // Set a Zero Concentration
    cells[loopA].concentration = 0.0;
    switch(dir){
      case kdirX:
        // Set Local Coordinates
        //localR = sqrt(currentX*currentX + currentY*currentY + currentZ*currentZ);
        localR = sqrt(currentY * currentY + currentZ * currentZ);
        localZ = currentX;
        // Build Axial Vector
        axialVec[0] = 1.0;
        axialVec[1] = 0.0;
        axialVec[2] = 0.0;
         // Build Radial Vector
        radialVec[0] = 0.0;
        radialVec[1] = currentY;
        radialVec[2] = currentZ;
        MRIUtils::normalize3DVector(radialVec);
        break;
      case kdirY:
        // Set Local Coordinates
        localR = sqrt(currentX*currentX + currentY*currentY + currentZ*currentZ);
        localZ = currentY;                
        // Build Axial Vector
        axialVec[0] = 0.0;
        axialVec[1] = 1.0;
        axialVec[2] = 0.0;        
         // Build Radial Vector
        radialVec[0] = currentX;
        radialVec[1] = 0.0;
        radialVec[2] = currentZ;  
        MRIUtils::normalize3DVector(radialVec);      
        break;
      case kdirZ:
        // Set Local Coordinates
        localR = sqrt(currentX*currentX + currentY*currentY + currentZ*currentZ);
        localZ = currentZ;
        // Build Axial Vector
        axialVec[0] = 0.0;
        axialVec[1] = 0.0;
        axialVec[2] = 1.0;      
         // Build Radial Vector
        radialVec[0] = currentX;
        radialVec[1] = currentY;
        radialVec[2] = 0.0;
        MRIUtils::normalize3DVector(radialVec);
        break;
    }
    // Assign Concentration
    cells[loopA].concentration = 1.0;
    // Normalize Radial Direction
    axialComponentIn = (3.0/2.0)*CONST_U0*(1.0-((2.0*localR*localR+localZ*localZ)/(CONST_A*CONST_A)));
    radialComponentIn = (3.0/2.0)*CONST_U0*((localR*localZ)/(CONST_A*CONST_A));
    axialComponentOut = CONST_U0*(pow(((CONST_A*CONST_A)/(localR*localR+localZ*localZ)),2.5)*((2.0*localZ*localZ-localR*localR)/(2.0*CONST_A*CONST_A))-1.0);
    radialComponentOut = (3.0/2.0)*CONST_U0*((localR*localZ)/(CONST_A*CONST_A))*pow(((CONST_A*CONST_A)/(localR*localR+localZ*localZ)),2.5);
    // Set The Vector Components
    if (localR*localR+localZ*localZ<=CONST_A*CONST_A){
      cells[loopA].velocity[0] = axialComponentIn * axialVec[0] + radialComponentIn * radialVec[0];
      cells[loopA].velocity[1] = axialComponentIn * axialVec[1] + radialComponentIn * radialVec[1];
      cells[loopA].velocity[2] = axialComponentIn * axialVec[2] + radialComponentIn * radialVec[2];
    }else{
      cells[loopA].velocity[0] = 0.0;
      cells[loopA].velocity[1] = 0.0;
      cells[loopA].velocity[2] = 0.0;
      //cellPoints[loopA].velocity[0] = axialComponentOut * axialVec[0] + radialComponentOut * radialVec[0];
      //cellPoints[loopA].velocity[1] = axialComponentOut * axialVec[1] + radialComponentOut * radialVec[1];
      //cellPoints[loopA].velocity[2] = axialComponentOut * axialVec[2] + radialComponentOut * radialVec[2];     
    }
  }
}

// ASSIGN SPHERICAL VORTEX FLOW 
void MRIScan::assignToroidalVortexFlowSignature(){
  // Set Parameters
  const double CONST_A = 5.0;
  const double CONST_L = 1.3*1.3;
  // Init Local Coords
  double currentX = 0.0;
  double currentY = 0.0;
  double currentZ = 0.0;
  double radius = 0.0;
  double currModulus = 0.0;
  // Allocate Radial and Axial Vectors
  MRIDoubleVec axialVec(3,0.0);
  MRIDoubleVec radialVec(3,0.0); 
  MRIDoubleVec tangVector(3,0.0);
  MRIDoubleVec inclVector(3,0.0);
  MRIDoubleVec velVector(3,0.0);
  // Find the Centre Of the Domain
  MRIDoubleVec centrePoint(3,0.0);
  centrePoint[0] = 0.5 * (topology->domainSizeMax[0]+topology->domainSizeMin[0]);
  centrePoint[1] = 0.5 * (topology->domainSizeMax[1]+topology->domainSizeMin[1]);
  centrePoint[2] = 0.5 * (topology->domainSizeMax[2]+topology->domainSizeMin[2]);
  // Assign Cell Velocities
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    // Get The Three Coordinates
    currentX = topology->cellLocations[loopA][0] - centrePoint[0];
    currentY = topology->cellLocations[loopA][1] - centrePoint[1];
    currentZ = topology->cellLocations[loopA][2] - centrePoint[2];
    
    // Set a Zero Concentration
    cells[loopA].concentration = 1.0;
    
    // Build Axial Vector
    axialVec[0] = 1.0;
    axialVec[1] = 0.0;
    axialVec[2] = 0.0;      
    
    // Find The Projection on the YZ Plane
    radialVec[0] = 0.0;
    radialVec[1] = currentY;
    radialVec[2] = currentZ;
    MRIUtils::normalize3DVector(radialVec);
    for(int loopB=0;loopB<kNumberOfDimensions;loopB++){
        radialVec[loopB] *=  CONST_A;
    }
    
    // Find The Tangent Vector
    MRIUtils::do3DExternalProduct(radialVec,axialVec,tangVector);
    MRIUtils::normalize3DVector(tangVector);
    
    // Find The Inclined Vector
    inclVector[0] = currentX - radialVec[0];
    inclVector[1] = currentY - radialVec[1];
    inclVector[2] = currentZ - radialVec[2];
    radius = MRIUtils::do3DEucNorm(inclVector);
    MRIUtils::normalize3DVector(inclVector);
    
    // Eval Vel Vector
    MRIUtils::do3DExternalProduct(inclVector,tangVector,velVector);
    MRIUtils::normalize3DVector(velVector);
    
    // Eval Current Modulus
    currModulus = (8.0*radius/CONST_L)*exp(-(radius/CONST_L));
      
    // Normalize Radial Direction
    cells[loopA].velocity[0] = currModulus * velVector[0];
    cells[loopA].velocity[1] = currModulus * velVector[1];
    cells[loopA].velocity[2] = currModulus * velVector[2];
  }
}

// ASSIGN CONSTANT FLOW
void MRIScan::assignConstantSignature(MRIDirection dir){
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    switch(dir){
      case kdirX:
        cells[loopA].velocity[0] = 1.0;
        cells[loopA].velocity[1] = 0.0;
        cells[loopA].velocity[2] = 0.0;
        break;
      case kdirY:
        cells[loopA].velocity[0] = 0.0;
        cells[loopA].velocity[1] = 1.0;
        cells[loopA].velocity[2] = 0.0;
        break;
      case kdirZ:
        cells[loopA].velocity[0] = 0.0;
        cells[loopA].velocity[1] = 0.0;
        cells[loopA].velocity[2] = 1.0;
        break;
    }
  }
}

// ====================
// TAYLOR FLOW VORTEX
// ====================
void MRIScan::assignTaylorVortexSignature(MRIDirection dir){
  
  MRIDoubleVec centrePoint(3,0.0);

  centrePoint[0] = 0.5 * (topology->domainSizeMax[0] + topology->domainSizeMin[0]);
  centrePoint[1] = 0.5 * (topology->domainSizeMax[1] + topology->domainSizeMin[1]);
  centrePoint[2] = 0.5 * (topology->domainSizeMax[2] + topology->domainSizeMin[2]);
  
  // Loop on cell
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    switch(dir){
      case kdirX:
        cells[loopA].velocity[0] = 0.0;
        cells[loopA].velocity[1] = topology->cellLocations[loopA][2]-centrePoint[2];
        cells[loopA].velocity[2] = -(topology->cellLocations[loopA][1]-centrePoint[1]);
        break;
      case kdirY:
        cells[loopA].velocity[0] = topology->cellLocations[loopA][2]-centrePoint[2];
        cells[loopA].velocity[1] = 0.0;
        cells[loopA].velocity[2] = -(topology->cellLocations[loopA][0]-centrePoint[0]);
        break;
      case kdirZ:
        cells[loopA].velocity[0] = topology->cellLocations[loopA][1]-centrePoint[1];
        cells[loopA].velocity[1] = -(topology->cellLocations[loopA][0]-centrePoint[0]);
        cells[loopA].velocity[2] = 0.0;
        break;
    }
    cells[loopA].concentration = 1.0;
  }
}


// SET VELOCITIES TO ZERO
void MRIScan::assignZeroVelocities(){
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    cells[loopA].velocity[0] = 0.0;
    cells[loopA].velocity[1] = 0.0;
    cells[loopA].velocity[2] = 0.0;
  }
}

// Assign Constant Flow With Step
void MRIScan::assignConstantFlowWithStep(){
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    if ((topology->cellLocations[loopA][0] > (0.5*(topology->domainSizeMin[0] + topology->domainSizeMax[0])))&&
       (topology->cellLocations[loopA][1] > (0.5*(topology->domainSizeMin[1] + topology->domainSizeMax[1])))){
      // Assign Constant Velocity
      cells[loopA].concentration = 0.0;
      cells[loopA].velocity[0] = 0.0;
      cells[loopA].velocity[1] = 0.0;
      cells[loopA].velocity[2] = 0.0;         
    }else{
      // Assign Constant Velocity
      cells[loopA].concentration = 10.0;
      cells[loopA].velocity[0] = 1.0;
      cells[loopA].velocity[1] = 0.0;
      cells[loopA].velocity[2] = 0.0;
    }
  }
}

// Assign Standard Gaussian Random Velocities on the Three Separated Components
void MRIScan::assignRandomStandardGaussianFlow(){
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    // Assign Constant Velocity
    cells[loopA].concentration = 10.0;
    cells[loopA].velocity[0] = 1.0;
    cells[loopA].velocity[1] = 0.0;
    cells[loopA].velocity[2] = 0.0;
  }
}

// =====================
// ASSIGN POISEILLE FLOW
// =====================
void MRIScan::assignPoiseilleSignature(MRIDirection dir){
  double currentVelocity = 0.0;
  double conc = 0.0;
  // SET CENTER POINT
  double centerPoint[3];
  centerPoint[0] = 0.5*(topology->domainSizeMax[0] + topology->domainSizeMin[0]);
  centerPoint[1] = 0.5*(topology->domainSizeMax[1] + topology->domainSizeMin[1]);
  centerPoint[2] = 0.5*(topology->domainSizeMax[2] + topology->domainSizeMin[2]);
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    double currentDistance = 0.0;
    double totalDistance = 0.0;
    // Set to Zero
    cells[loopA].velocity[0] = 0.0;
    cells[loopA].velocity[1] = 0.0;
    cells[loopA].velocity[2] = 0.0;
    cells[loopA].concentration = 0.0;
    switch(dir){
      case kdirX:
        currentDistance = sqrt((topology->cellLocations[loopA][1] - centerPoint[1])*(topology->cellLocations[loopA][1] - centerPoint[1]) +
                               (topology->cellLocations[loopA][2] - centerPoint[2])*(topology->cellLocations[loopA][2] - centerPoint[2]));
        //totalDistance = 0.5*min(0.5*(domainSizeMax[1]-domainSizeMin[1]),0.5*(domainSizeMax[2]-domainSizeMin[2]));
        //totalDistance = 0.00855;
        totalDistance = 0.01;
        break;
      case kdirY:
        currentDistance = sqrt((topology->cellLocations[loopA][0] - centerPoint[0])*(topology->cellLocations[loopA][0] - centerPoint[0]) +
                               (topology->cellLocations[loopA][2] - centerPoint[2])*(topology->cellLocations[loopA][2] - centerPoint[2]));
        //totalDistance = 0.5*min(0.5*(domainSizeMax[0]-domainSizeMin[0]),0.5*(domainSizeMax[2]-domainSizeMin[2]));
        //totalDistance = 0.00855;
        totalDistance = 0.01;
        break;
      case kdirZ:
        currentDistance = sqrt((topology->cellLocations[loopA][0] - centerPoint[0])*(topology->cellLocations[loopA][0] - centerPoint[0]) +
                               (topology->cellLocations[loopA][1] - centerPoint[1])*(topology->cellLocations[loopA][1] - centerPoint[1]));
        //totalDistance = 0.5*min(0.5*(domainSizeMax[0]-domainSizeMin[0]),0.5*(domainSizeMax[1]-domainSizeMin[1]));
        //totalDistance = 0.00855;
        totalDistance = 0.01;
        break;
    }
    //double peakVel = 0.22938;
    double peakVel = 0.4265;
    // Apply a threshold
    if(currentDistance<totalDistance){
      currentVelocity = -(peakVel/(totalDistance*totalDistance))*(currentDistance*currentDistance) + peakVel;
      conc = 1.0;
    }else{
      currentVelocity = 0.0;
      conc = 1.0e-10;
    }
    // Assign Velocity
    cells[loopA].concentration = conc;
    switch(dir){
      case kdirX: 
        cells[loopA].velocity[0] = currentVelocity;
        break;
      case kdirY: 
        cells[loopA].velocity[1] = currentVelocity;
        break;
      case kdirZ: 
        cells[loopA].velocity[2] = currentVelocity;
        break;
    }
  }
}

// ASSIGN CONCENTRATIONS AND VELOCITIES
void MRIScan::assignVelocitySignature(MRIDirection dir, MRISamples sample, double currTime){
  switch(sample){
    case kZeroVelocity:
      assignZeroVelocities();
      break;
    case kConstantFlow:
      assignConstantSignature(dir);
      break;    
    case kPoiseilleFlow:
      assignPoiseilleSignature(dir);
      break;
    case kStagnationFlow:
      assignStagnationFlowSignature(dir);
      break;
    case kCylindricalVortex:
      assignCylindricalFlowSignature(dir);
      break;    
    case kSphericalVortex:
      assignSphericalFlowSignature(dir);
      break;   
    case kToroidalVortex:
      assignToroidalVortexFlowSignature();
      break;  
    case kTransientFlow:
      assignTimeDependentPoiseilleSignature(2*3.1415,8.0,1.0e-3,currTime,1.0);
      break;
    case kConstantFlowWithStep:
      assignConstantFlowWithStep();
      break;
    case kTaylorVortex:
      assignTaylorVortexSignature(dir);
      break;
  }
}

// CREATE SAMPLE FLOWS
void MRIScan::createFromTemplate(MRISamples sampleType,const MRIDoubleVec& params){

  // Store Parameter Values
  int sizeX = int(params[0]);
  int sizeY = int(params[1]);
  int sizeZ = int(params[2]);
  double distX = params[3];
  double distY = params[4];
  double distZ = params[5];
  double currTime = params[6];

  // Template Orientation
  int dir = 0;
  int direction = int(params[7]);
  if(direction == 0){
    dir = kdirX;
  }else if(direction == 1){
    dir = kdirY;
  }else if(direction == 2){
    dir = kdirZ;
  }else{
    throw MRIException("ERROR: Invalid template direction in CreateSampleCase.\n");
  }

  // Assign Concentrations and Velocities
  assignVelocitySignature(dir,sampleType,currTime);

  // Find Velocity Modulus
  maxVelModule = 0.0;
  double currentMod = 0.0;
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    currentMod = sqrt((cells[loopA].velocity[0])*(cells[loopA].velocity[0])+
                      (cells[loopA].velocity[1])*(cells[loopA].velocity[1])+
                      (cells[loopA].velocity[2])*(cells[loopA].velocity[2]));
    if(currentMod>maxVelModule) maxVelModule = currentMod;
  }
}

// ASSIGN TIME DEPENDENT FLOW
void MRIScan::assignTimeDependentPoiseilleSignature(double omega, double radius, double viscosity, double currtime, double maxVel){
  // Eval omegaMod
  double omegaMod = ((omega*radius*radius)/viscosity);
  double relCoordX = 0.0;
  double relCoordY = 0.0;
  double relCoordZ = 0.0;
  double localRadius = 0.0;
  double normRadius = 0.0;
  double bValue = 0.0;
  // Get center point of domain
  double centrePoint[3] = {0.0};
  centrePoint[0] = 0.5 * (topology->domainSizeMax[0] + topology->domainSizeMin[0]);
  centrePoint[1] = 0.5 * (topology->domainSizeMax[1] + topology->domainSizeMin[1]);
  centrePoint[2] = 0.5 * (topology->domainSizeMax[2] + topology->domainSizeMin[2]);
  // Loop Through the Points
  for(int loopA=0;loopA<topology->totalCells;loopA++){
    //Get Radius
    relCoordX = topology->cellLocations[loopA][0] - centrePoint[0];
    relCoordY = topology->cellLocations[loopA][1] - centrePoint[1];
    relCoordZ = topology->cellLocations[loopA][2] - centrePoint[2];
    localRadius = sqrt(relCoordY*relCoordY+relCoordZ*relCoordZ);
    normRadius = (localRadius/radius);
    bValue = (1.0 - normRadius)*sqrt(omegaMod/2.0);
    // Assign Velocity
    double peakVel = 4.0;
    if (normRadius<=1.0){
      if (normRadius>kMathZero){
        cells[loopA].velocity[0] = maxVel*((peakVel/omegaMod)*(sin(omega*currtime)-((exp(-bValue))/(sqrt(normRadius)))*sin(omega*currtime-bValue)));
      }else{
        cells[loopA].velocity[0] = maxVel*((peakVel/omegaMod)*(sin(omega*currtime)));
      }
      cells[loopA].velocity[1] = 0.0;
      cells[loopA].velocity[2] = 0.0;
      cells[loopA].concentration = 1.0;
    }else{
      cells[loopA].velocity[0] = 0.0;
      cells[loopA].velocity[1] = 0.0;
      cells[loopA].velocity[2] = 0.0;
      cells[loopA].concentration = 0.0;
    }
  }
}
