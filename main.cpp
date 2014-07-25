#include <iostream>

#include "mriScan.h"
#include "mriStructuredScan.h"
#include "mriSequence.h"
#include "mriUtils.h"
#include "mriStatistics.h"
#include "mriConstants.h"
#include "mriProgramOptions.h"
#include "schMessages.h"
#include "mpi.h"
#include "mriCommunicator.h"

#include <boost/random.hpp>

// ======================================
// Read Scan Sequence from Raw Data Files
// ======================================
void ConvertDICOMToVTK(std::string inFileName,std::string outfileName){

  // Add File to Sequence
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);

  // Read from Raw Binary File
  MyMRIScan->ReadRAWFileSequence(inFileName);

  // Export to VTK
  MyMRIScan->ExportToVTK("testVTK.vtk");

}

// ============================
// CONVERT TECPLOT ASCII TO VTK
// ============================
void ConvertTECPLOToVTK(std::string inFileName,std::string outfileName){

  // Add File to Sequence
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);

  // Read from Raw Binary File
  MyMRIScan->ReadPltFile(inFileName,true);

  WriteSchMessage(std::string("ECCOLO\n"));
  WriteSchMessage(std::to_string(MyMRIScan->cellPoints[300].velocity[1]) + std::string("\n"));

  // Export to VTK
  MyMRIScan->ExportToVTK(outfileName.c_str());
  //MyMRIScan->ExportToTECPLOT(outfileName.c_str(),true);

}

// ======================
// SOLVE POISSON EQUATION
// ======================
void SolvePoissonEquation(mriCommunicator* comm){
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->SolvePoissonEquation(comm);
}


// =====================================
// PROCESS SEQUENCE TO PRODUCE PRESSURES
// =====================================
void EvalSequencePressure(std::string inFileName, std::string outfileName){

  // Create New Sequence
  MRISequence* MyMRISequence = new MRISequence(true/*Cyclic Sequence*/);

  // Read From VOL Files
  MyMRISequence->ReadFromVolSequence(inFileName);

  // Apply MP Filter to all Scan Separately
  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(1.0e-4,2000);
  options->useBCFilter = false;
  options->useConstantPatterns = true;
  options->thresholdCriteria = new MRIThresholdCriteria(kCriterionLessThen,kQtyConcentration,500.0);

  // PRELIMINARY THRESHOLDING
  MyMRISequence->ApplyThresholding(options->thresholdCriteria);

  // APPLY FULL FILTER
  MyMRISequence->ApplySMPFilter(options);

  // Apply Boundary Condition Filter to all scans
  options->useBCFilter = true;
  // APPLY FILTER
  MyMRISequence->ApplySMPFilter(options);

  // Apply Final Threshold
  MyMRISequence->ApplyThresholding(options->thresholdCriteria);

  // Compute Pressure Gradient
  MyMRISequence->ComputePressureGradients();

  // Compute Relative Pressure
  MyMRISequence->ComputeRelativePressure(false);

  // Export Sequence To VOL File
  MyMRISequence->ExportToVOL(outfileName);
}

// ====================================
// PROCESS SIGNATURE FLOW FOR PRESSURES
// ====================================
void EvalPressureFromSignatureFlow(std::string inFileName,std::string outfileName){

  // Set Parameters
  int totalSlides = 1;
  double currentTime = 0.0;
  double timeIncrement = 0.1;

  // Create New Sequence
  MRISequence* MyMRISequence = new MRISequence(true/*Cyclic Sequence*/);

  // Fill Seguence
  currentTime = 0.0;
  for(int loopA=0;loopA<totalSlides;loopA++){
    MRIStructuredScan* MyMRIScan = new MRIStructuredScan(currentTime);
    MyMRIScan->CreateSampleCase(kStagnationFlow,20,80,80,0.1,0.1,0.1,currentTime,kdirX);
    //MyMRIScan->CreateSampleCase(kTransientFlow,40,60,80,0.01,0.01,0.01,currentTime,kdirX);
    MyMRISequence->AddScan(MyMRIScan);
    currentTime += timeIncrement;
  }

  // Read Plt File
  //MyMRIScan->ReadPltFile(inFileName,true);

  // Generate Sample Case
  // kPoiseilleFlow, kStagnationFlow, kCylindricalVortex,kSphericalVortex
  //MyMRIScan->CreateSampleCase(kCylindricalVortex,20,50,50,1.0,1.0,1.0,kdirX);
  //MyMRIScan->CreateSampleCase(kSphericalVortex,60,60,60,0.5,0.5,0.5,kdirX);
  //MyMRIScan->CreateSampleCase(kToroidalVortex,40,60,80,1.0,1.0,1.0,kdirX);
  //MyMRIScan->CreateSampleCase(kTransientFlow,40,60,80,0.01,0.01,0.01,0.0,kdirX);

  // TEST: USE VOL FILES
  //MyMRIScan->ReadScanFromVOLFiles(std::string("volDataAn.vol"),std::string("volDataAn.vol"),std::string("volDataAn.vol"),std::string("volDataAn.vol"));

  // USE DIVERGENCE SMOOTHING FILTER
  //MyMRIScan->ApplySmoothingFilter();

  // SET OPTIONS AND THRESHOLD
  MRIOptions Options(1.0e-4,2000);
  bool useBCFilter = false;
  bool useConstantPatterns = true;
  MRIThresholdCriteria criteria(kCriterionLessThen,kQtyConcentration,1000.0);

  // APPLY FILTER
  //MyMRIScan->PerformPhysicsFiltering(Options,useBCFilter,useConstantPatterns,criteria);

  // Update Velocities
  //MyMRIScan->UpdateVelocities();

  // =========================
  // Compute Pressure Gradient
  // =========================
  MyMRISequence->ComputePressureGradients();

  // =========================
  // Compute Relative Pressure
  // =========================
  MyMRISequence->ComputeRelativePressure(false);

  // =========================
  // Select the resulting File
  // =========================
  MyMRISequence->ExportToTECPLOT(outfileName);
}

// ===================
// PROCESS SINGLE SCAN
// ===================
void ProcessSingleScan(std::string inFileName,std::string outfileName,double itTol, int maxIt, std::string thresholdTypeString,double thresholdValue){

  // Create New Sequence
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // Add File to Sequence
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName, true);
  MyMRISequence->AddScan(MyMRIScan);

  // Echo Inserted Parameters
  WriteSchMessage(std::string("--------------------------------------------\n"));
  WriteSchMessage(std::string("MP Iteration tolerance Value: ")+MRIUtils::FloatToStr(itTol)+"\n");
  WriteSchMessage(std::string("Threshold Value             : ")+MRIUtils::FloatToStr(thresholdValue)+"\n");
  WriteSchMessage(std::string("--------------------------------------------\n"));

  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(itTol,maxIt);
  options->useBCFilter = false;
  options->useConstantPatterns = true;
  int thresholdType = 0;
  if (thresholdTypeString == "conc"){
    thresholdType = kQtyConcentration;
  }else{
    thresholdType = kQtyVelModule;
  }
  options->thresholdCriteria = new MRIThresholdCriteria(kCriterionABSLessThen,thresholdType,thresholdValue);

  // APPLY FULL FILTER
  MyMRISequence->ApplySMPFilter(options);

  // APPLY BOUNDARY CONDITION FILTER
  options->useBCFilter = true;
  MyMRISequence->ApplySMPFilter(options);

  // SAVE EXPANSION TO FILE
  MyMRISequence->GetScan(0)->WriteExpansionFile(std::string("ExpansionFile.dat"));

  // APPLY FINAL THRESHOLD
  MyMRISequence->ApplyThresholding(options->thresholdCriteria);

  // Evaluate Statistics
  //MyMRISequence->EvalStatistics();

  // Compute Pressure Gradient
  //MyMRISequence->ComputePressureGradients();

  // Compute Relative Pressure
  //MyMRISequence->ComputeRelativePressure(false);

  // EXPORT FILE
  //MyMRISequence->ExportToTECPLOT(outfileName);
  MyMRISequence->ExportToVTK(outfileName);
}

// =============================
// EVAL STATISTICS BETWEEN SCANS
// =============================
void ComputeScanStatistics(std::string firstFileName,std::string secondFileName,std::string statFileName){

    // Init File Names
    std::string statFileNameFirst = statFileName+"_First.dat";
    std::string statFileNameSecond = statFileName+"_Second.dat";
    std::string statFileNameDiff = statFileName+"_Diff.dat";

    // Read the File in a Sequence
    // Create New Sequence
    MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

    // Add First File to Sequence
    MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
    MyMRIScan->ReadPltFile(firstFileName, true);
    MyMRISequence->AddScan(MyMRIScan);

    // Add Second File to Sequence
    MyMRIScan = new MRIStructuredScan(1.0);
    MyMRIScan->ReadPltFile(secondFileName, true);
    MyMRISequence->AddScan(MyMRIScan);

    // Create Statistics
    bool useBox = false;
    int numberOfBins = 301;
    double* limitBox = new double[6];
    // Set Limits
    limitBox[0] = MyMRISequence->GetScan(0)->domainSizeMin[0];
    limitBox[1] = MyMRISequence->GetScan(0)->domainSizeMax[0];
    limitBox[2] = MyMRISequence->GetScan(0)->domainSizeMin[1];
    limitBox[3] = MyMRISequence->GetScan(0)->domainSizeMax[1];
    limitBox[4] = MyMRISequence->GetScan(0)->domainSizeMin[2];
    limitBox[5] = MyMRISequence->GetScan(0)->domainSizeMax[2];
    // Apply Factors to limitBox
    double xFactor = 1.0;
    double yFactor = 1.0;
    double zFactor = 1.0;
    MRIUtils::ApplylimitBoxFactors(xFactor,yFactor,zFactor,limitBox);
    // Allocate Bin Arrays
    double* binCenters = new double[numberOfBins];
    double* binValues =  new double[numberOfBins];
    // Eval Single PDFs
    // FIRST
    MRIStatistics::EvalScanPDF(MyMRISequence->GetScan(0),kQtyVelModule,numberOfBins,useBox,limitBox,binCenters,binValues);
    MRIUtils::PrintBinArrayToFile(statFileNameFirst,numberOfBins,binCenters,binValues);

    // SECOND
    MRIStatistics::EvalScanPDF(MyMRISequence->GetScan(1),kQtyVelModule,numberOfBins,useBox,limitBox,binCenters,binValues);
    MRIUtils::PrintBinArrayToFile(statFileNameSecond,numberOfBins,binCenters,binValues);

    // DIFFERENCE
    MRIStatistics::EvalScanDifferencePDF(MyMRISequence,1,0,kQtyVelModule,numberOfBins,useBox,limitBox,binCenters,binValues);
    MRIUtils::PrintBinArrayToFile(statFileNameDiff,numberOfBins,binCenters,binValues);

    // DEALLOCATE
    delete [] binCenters;
    delete [] binValues;
}

// =============================
// EXPLICITLY EVAL SCAN MATRICES
// =============================
void ComputeScanMatrices(){
  // VAR
  int totalERows = 0;
  int totalECols = 0;
  double** EMat = NULL;
  int totalDRows = 0;
  int totalDCols = 0;
  double** DMat = NULL;
  int totalStarRows = 0;
  int totalStarCols = 0;
  double** StarMatrix = NULL;

  // Print Matrices for Matlab Analysis Of Variance
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  // ISOTROPIC
  MyMRIScan->CreateSampleCase(kConstantFlow,5,5,5,1.0,1.0,1.0,0.0,kdirX);
  // ANISOTROPIC
  //MyMRIScan->CreateSampleCase(kConstantFlow,5,5,5,1.0,2.0,3.0,0.0,kdirX);
  // Assemble All Matrices
  // Assemble Encoding
  MyMRIScan->AssembleEncodingMatrix(totalERows,totalECols,EMat);
  MRIUtils::PrintMatrixToFile("EncodingMat.dat",totalERows,totalECols,EMat);
  // Assemble Decoding
  MyMRIScan->AssembleDecodingMatrix(totalDRows,totalDCols,DMat);
  MRIUtils::PrintMatrixToFile("DecodingMat.dat",totalDRows,totalDCols,DMat);
  // Assemble Star Matrix
  MyMRIScan->AssembleStarMatrix(totalStarRows,totalStarCols,StarMatrix);
  MRIUtils::PrintMatrixToFile("StarMat.dat",totalStarRows,totalStarCols,StarMatrix);

  // Deallocate Matrices
  for(int loopA=0;loopA<totalERows;loopA++){
    delete [] EMat[loopA];
  }
  for(int loopA=0;loopA<totalDRows;loopA++){
    delete [] DMat[loopA];
  }
  for(int loopA=0;loopA<totalStarRows;loopA++){
    delete [] StarMatrix[loopA];
  }
  delete [] EMat;
  delete [] DMat;
  delete [] StarMatrix;
}

// ================
// READ FACE FLUXES
// ================
void ShowFaceFluxPatterns(std::string faceFluxFileName, std::string outFileName){
  // NEW SCAN
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);

  // ISOTROPIC
  MyMRIScan->CreateSampleCase(kConstantFlow,5,5,5,1.0,1.0,1.0,0.0,kdirX);

  // ANISOTROPIC
  //MyMRIScan->CreateSampleCase(kConstantFlow,5,5,5,1.0,2.0,3.0,0.0,kdirX);

  // READ FACE FLUXES FROM FILE
  int totalRows = 0;
  int totalCols = 0;
  std::vector<std::vector<double> > faceFluxMat;
  MRIUtils::ReadMatrixFromFile(faceFluxFileName,totalRows,totalCols,faceFluxMat);

  // COPY THE INTERESTING COLUMN
  double faceFluxVec[totalRows];
  for(int loop0=0;loop0<100;loop0++){
    int selectedCol = loop0;
    for(int loopA=0;loopA<totalRows;loopA++){
      faceFluxVec[loopA] = faceFluxMat[loopA][selectedCol];
    }

    // TRASFORM FACE FLUXES IN VELOCITIES
    MyMRIScan->RecoverCellVelocitiesRT0(false,faceFluxVec);
    // UPDATE VELOCITIES
    MyMRIScan->UpdateVelocities();

    // EXPORT TO VTK
    MyMRIScan->ExportToVTK(outFileName + "_" + MRIUtils::IntToStr(loop0) + ".vtk");
  }
}

// ===========================
// Test Expansion Coefficients
// ===========================
void TEST_ExpansionCoefficients(std::string inFileName){

  // Create New Sequence
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // Add File to Sequence
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName, true);
  MyMRISequence->AddScan(MyMRIScan);

  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(1.5e-2,2000);
  bool useBCFilter = false;
  bool useConstantPatterns = true;
  int thresholdType = 0;
  std::string thresholdTypeString("Conc");
  double thresholdValue = 0.0;
  if (thresholdTypeString == "Conc"){
    thresholdType = kQtyConcentration;
  }else{
    thresholdType = kQtyVelModule;
  }
  MRIThresholdCriteria criteria(kCriterionLessThen,thresholdType,thresholdValue);

  // APPLY FULL FILTER
  MyMRISequence->ApplySMPFilter(options);

  // GET FIRST SCAN
  MRIScan* firstMRIScan = new MRIScan(0.0);
  firstMRIScan = MyMRISequence->GetScan(0);

  // THRESHOLDING ON EXPANSION COEFFICIENTS
  firstMRIScan->expansion->ApplyVortexThreshold(kSoftThreshold,0.5);
  //firstMRIScan->expansion->WriteToFile("Expansion.dat");

  // REBUILD FROM EXPANSION
  MRISequence* ReconstructedSequence = new MRISequence(MyMRISequence);
  MRIScan* currScan = nullptr;
  for(int loopA=0;loopA<ReconstructedSequence->GetTotalScans();loopA++){
    currScan = ReconstructedSequence->GetScan(loopA);
    currScan->RebuildFromExpansion(firstMRIScan->expansion,false);
  }

  // COMPARE THE TWO SCANS
  double currDiffNorm = 0.0;
  MRIScan* currScan1 = nullptr;
  MRIScan* currScan2 = nullptr;
  for(int loopA=0;loopA<MyMRISequence->GetTotalScans();loopA++){
    currScan1 = MyMRISequence->GetScan(loopA);
    currScan2 = ReconstructedSequence->GetScan(loopA);
    // Get Difference Norm
    currDiffNorm += currScan1->GetDiffNorm(currScan2);
  }

  // WRITE OUTPUT FILES TO VTK
  MyMRISequence->ExportToVTK("FilteredSeq.vtk");
  ReconstructedSequence->ExportToVTK("ReconstructedSeq.vtk");

  // PRINT DIFFERENCE NORM
  WriteSchMessage("Difference Norm " + MRIUtils::FloatToStr(currDiffNorm) + "\n");
}

// ==================
// PRINT THRESHOLDING
// ==================
void TEST02_PrintThresholdingToVTK(std::string inFileName){

  // Create New Sequence
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);
  MRISequence* MyRECSequence = new MRISequence(false/*Cyclic Sequence*/);

  // Add File to Sequence
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName, true);
  MyMRISequence->AddScan(MyMRIScan);

  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(1.0e-4,2000);
  bool useBCFilter = false;
  bool useConstantPatterns = true;
  int thresholdType = 0;
  std::string thresholdTypeString("Conc");
  double thresholdValue = 0.0;
  if (thresholdTypeString == "Conc"){
    thresholdType = kQtyConcentration;
  }else{
    thresholdType = kQtyVelModule;
  }
  MRIThresholdCriteria criteria(kCriterionLessThen,thresholdType,thresholdValue);

  // APPLY FULL FILTER
  MyMRISequence->ApplySMPFilter(options);

  // APPLY SUCCESSIVE THRESHOLD
  for(int loopA=0;loopA<5;loopA++){
    WriteSchMessage("Applying Threshold " + MRIUtils::IntToStr(loopA) + "\n");
    // CREATE NEW EXPANSION
    MRIExpansion* currExp = new MRIExpansion(MyMRISequence->GetScan(0)->expansion);
    // APPLY THRESHOLD
    currExp->ApplyVortexThreshold(kSoftThreshold,(0.95/4.0)*(loopA));
    // WRITE EXPANSION TO FILE
    currExp->WriteToFile("Expansion_" + MRIUtils::IntToStr(loopA) + "\n");
    // GET A SCAN FROM ORIGINAL SEQUENCE
    MRIScan* myScan = new MRIScan(*MyMRISequence->GetScan(0));
    myScan->RebuildFromExpansion(currExp,false);
    // ADD SCAN TO RECONSTRUCTION
    MyRECSequence->AddScan(myScan);
  }

  // WRITE OUTPUT FILES TO VTK
  MyMRISequence->ExportToVTK("FilteredSeq");
  MyRECSequence->ExportToVTK("ReconstructedSeq");
}

// ======================
// EVAL REYNOLDS STRESSES
// ======================
void TEST03_EvalReynoldsStresses(std::string inFileName){

  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);
  MRISequence* MyRECSequence = new MRISequence(false/*Cyclic Sequence*/);

  // ADD FILE TO SEQUENCE
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName, true);
  MyMRISequence->AddScan(MyMRIScan);

  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(5.0e-1,2000);
  options->useBCFilter = false;
  options->useConstantPatterns = true;
  int thresholdType = 0;
  std::string thresholdTypeString("Conc");
  double thresholdValue = 0.0;
  if (thresholdTypeString == "Conc"){
    thresholdType = kQtyConcentration;
  }else{
    thresholdType = kQtyVelModule;
  }
  options->thresholdCriteria = new MRIThresholdCriteria(kCriterionLessThen,thresholdType,thresholdValue);

  // APPLY FULL FILTER
  MyMRISequence->ApplySMPFilter(options);

  WriteSchMessage("Filter Applied!\n");

  // APPLY SUCCESSIVE THRESHOLD
  for(int loopA=0;loopA<2;loopA++){

    // WRITE MESSAGE
    WriteSchMessage("Applying Threshold " + MRIUtils::IntToStr(loopA) + "\n");

    // CREATE NEW EXPANSION
    MRIExpansion* currExp = new MRIExpansion(MyMRISequence->GetScan(0)->expansion);

    // APPLY THRESHOLD
    currExp->ApplyVortexThreshold(kHardThresold,(0.99/1.0)*(loopA));

    // WRITE EXPANSION TO FILE
    currExp->WriteToFile("Expansion_" + MRIUtils::IntToStr(loopA) + "\n");

    // GET A SCAN FROM ORIGINAL SEQUENCE
    MRIScan* myScan = new MRIScan(*MyMRISequence->GetScan(0));
    myScan->RebuildFromExpansion(currExp,false);

    // EVAL REYNOLDS STRESSES
    WriteSchMessage("Evaluating Reynolds Stresses...");
    myScan->EvalReynoldsStressComponent();
    WriteSchMessage("Done.\n");

    // ADD SCAN TO RECONSTRUCTION
    MyRECSequence->AddScan(myScan);
  }

  // WRITE OUTPUT FILES TO VTK
  MyRECSequence->ExportToVTK("ReynoldsStressSequence");
}

// ===========================================
// EVAL REYNOLDS STRESSES FROM EXPANSION FILES
// ===========================================
void EvalPressureFromExpansion(mriProgramOptions* options){

  // SET PARAMETERS
  bool applyThreshold = true;
  double thresholdRatio = 0.0;
  bool doPressureSmoothing = true;

  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // ADD FILE TO SEQUENCE
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  //MyMRIScan->ReadFromExpansionFile(inFileName,applyThreshold,thresholdRatio);
  MyMRIScan->ReadPltFile(options->inputFileName,true);
  MyMRISequence->AddScan(MyMRIScan);
  // APPLY MEDIAN FILTER TO VELOCITIES
  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyVelocityX,1);
  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyVelocityY,1);
  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyVelocityZ,1);

  //MyMRISequence->GetScan(0)->ThresholdQuantity(kQtyVelocityZ,1.0e10);

  // EVAL REYNOLDS STRESSES AND PRESSURE GRADIENTS
  MyMRISequence->ComputePressureGradients();

  // APPLY MEDIAN FILTER TO PRESSURE GRADIENT COMPONENTS
  // JET FILIPPO
  //MyMRISequence->GetScan(0)->ThresholdQuantity(kQtyPressGradientMod,2500.0);
  //MyMRISequence->GetScan(0)->ThresholdQuantity(kQtyPressGradientMod,1200.0);
  //MyMRISequence->GetScan(0)->ThresholdQuantity(kQtyPressGradientMod,200.0);

  //MyMRISequence->GetScan(0)->ThresholdQuantity(kQtyPressGradientMod,50000.0);

  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyPressGradientX,5);
  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyPressGradientY,5);
  //MyMRISequence->GetScan(0)->ApplyMedianFilter(kQtyPressGradientZ,5);

  // EVAL LOCATIONS OF NOISY POINTS
  MyMRISequence->GetScan(0)->EvalNoisyPressureGradientPoints();

  // EVAL RELATIVE PRESSURE
  MyMRISequence->ComputeRelativePressure(doPressureSmoothing);

  if(options->exportFormat == efTECPLOT){
    // WRITE OUTPUT FILES TO TECPLOT
    MyMRISequence->ExportToTECPLOT(options->outputFileName);
  }else{
    // WRITE OUTPUT FILES TO VTK
    MyMRISequence->ExportToVTK(options->outputFileName);
  }
}

// =========================================
// EVAL PRESSURES FROM EXPANSION COFFICIENTS
// =========================================
void EvalConcentrationGradient(std::string inFileName,std::string outFileName){

  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  bool doPressureSmoothing = false;

  // ADD FILE TO SEQUENCE
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName,true);
  MyMRIScan->ScalePositions(0.0058);
  MyMRISequence->AddScan(MyMRIScan);

  // EVAL REYNOLDS STRESSES AND PRESSURE GRADIENTS
  MyMRISequence->GetScan(0)->ComputeQuantityGradient(kQtyConcentration);

  // EVAL RELATIVE PRESSURE
  MyMRISequence->ComputeRelativePressure(doPressureSmoothing);

  // WRITE OUTPUT FILES TO VTK
  MyMRISequence->ExportToVTK(outFileName);

}



// ===================
// PERFORM RANDOM TEST
// ===================
void PerformRandomTest(){
  // Set parameters
  int numberMC = 500;
  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(1.0e-10,2000);
  bool useBCFilter = false;
  bool useConstantPatterns = true;
  MRIThresholdCriteria criteria(kCriterionLessThen,kQtyConcentration,1.0);

  // Generating Random Numbers
  boost::variate_generator<boost::mt19937, boost::normal_distribution<> > generator(boost::mt19937(time(0)),boost::normal_distribution<>());

  // Declare Scan
  MRIStructuredScan* MyMRIScan;

  // Monte Carlo Loops
  for(int loopA=0;loopA<numberMC;loopA++){

    // Generate Random Velocity Field - Standard Gaussian Distribution
    MyMRIScan = new MRIStructuredScan(0.0);
    MyMRIScan->CreateSampleCase(kZeroVelocity,5,5,5,1.0,1.0,1.0,0.0,kdirX);

    // Assign Random Component
    MyMRIScan->AssignRandomComponent(kdirX,generator);
    MyMRIScan->AssignRandomComponent(kdirY,generator);
    MyMRIScan->AssignRandomComponent(kdirZ,generator);

    // Write Generated Velocities To File
    MyMRIScan->ExportVelocitiesToFile("InputVelocities.dat",true);

    // Filter Velocities - NO GLOBAL ATOMS
    MyMRIScan->applySMPFilter(options);
    MyMRIScan->UpdateVelocities();

    // Write Resultant Velocities
    MyMRIScan->ExportVelocitiesToFile("OutputVelocities.dat",true);

    // Deallocate
    delete MyMRIScan;
    MyMRIScan = nullptr;
  }
}

// ================================
// CROP DICOM AND EVALUATE PRESSURE
// ================================
void CropAndComputeVol(std::string inFileName,std::string outfileName){
  // ================
  // ANALYZE SEQUENCE
  // ================

  // -------------------
  // Create New Sequence
  // -------------------
  MRISequence* MyMRISequence = new MRISequence(true/*Cyclic Sequence*/);

  // -------------------
  // Read From VOL Files
  // -------------------
  MyMRISequence->ReadFromVolSequence(inFileName);

  // ----------------
  // Create Limit Box
  // ----------------
  double limitBox[6] = {0.0};
  limitBox[0] = 135.0;
  limitBox[1] = 160.0;
  limitBox[2] = 130.0;
  limitBox[3] = 165.0;
  limitBox[4] = MyMRISequence->GetScan(0)->domainSizeMin[2];
  limitBox[5] = MyMRISequence->GetScan(0)->domainSizeMax[2];

  // ---------------
  // Reduce Sequence
  // ---------------
  MyMRISequence->Crop(limitBox);

  // ----------------
  // Scale Velocities
  // ----------------
  MyMRISequence->ScaleVelocities(0.001);
  MyMRISequence->ScalePositions(0.001);

  // SET OPTIONS AND THRESHOLD
  MRIOptions* options = new MRIOptions(1.0e-4,2000);
  options->useBCFilter = false;
  options->useConstantPatterns = true;
  options->thresholdCriteria = new MRIThresholdCriteria(kCriterionLessThen,kQtyConcentration,500.0);

  // ------------------------
  // PRELIMINARY THRESHOLDING
  // ------------------------
  MyMRISequence->ApplyThresholding(options->thresholdCriteria);

  // -----------------
  // APPLY FULL FILTER
  // -----------------
  MyMRISequence->ApplySMPFilter(options);

  // --------------------------------------------
  // Apply Boundary Condition Filter to all scans
  // --------------------------------------------
  options->useBCFilter = true;
  // APPLY FILTER
  MyMRISequence->ApplySMPFilter(options);

  // ---------------------
  // Apply Final Threshold
  // ---------------------
  MyMRISequence->ApplyThresholding(options->thresholdCriteria);

  // -------------------------
  // Compute Pressure Gradient
  // -------------------------
  MyMRISequence->ComputePressureGradients();

  // -------------------------
  // Compute Relative Pressure
  // -------------------------
  MyMRISequence->ComputeRelativePressure(false);

  // ---------------------------
  // Export Sequence To VOL File
  // ---------------------------
  MyMRISequence->ExportToTECPLOT(outfileName.c_str());
}

// =========================
// PERFORM STREAMLINE TEST 1
// =========================
void PerformStreamlineTest1(int intValue,std::string inFileName,std::string outfileName){

  // Set Default Options For Streamlines
  // Re   Xmin  Xmax Ymin  Ymax Zmin  Zmax
  // 80   -18.0 15.0 -12.0 21.0 -78.0 75.0
  // 145  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 190  -14.0 19.0 -3.0  30.0 -78.0 75.0
  // 240  -14.0 19.0 -3.0  30.0 -78.0 75.0
  // 290  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 458  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 1390 -15.0 18.0 -12.0 21.0 -78.0 75.0
  // 2740 -15.0 18.0 -12.0 21.0 -78.0 75.0
  MRIStreamlineOptions slOptions(0.0,0.0,0.0,0.0,0.0,0.0);
  switch (intValue){
    case 0:
      // Re 80
      slOptions.setLimits(-18.0,15.0,-12.0,21.0,-78.0,75.0);
      break;
    case 1:
      // Re 145
      slOptions.setLimits(-12.0,21.0,-22.0,11.0,-78.0,75.0);
      break;
    case 2:
      // Re 190
      slOptions.setLimits(-14.0,19.0,-3.0,30.0,-78.0,75.0);
      break;
    case 3:
      // Re 240
      slOptions.setLimits(-14.0,19.0,-3.0,30.0,-78.0,75.0);
      break;
    case 4:
      // Re 290
      slOptions.setLimits(-12.0,21.0,-22.0,11.0,-78.0,75.0);
      break;
    case 5:
      // Re 458
      slOptions.setLimits(-12.0,21.0,-22.0,11.0,-78.0,75.0);
      break;
    case 6:
      // Re 1390
      slOptions.setLimits(-15.0,18.0,-12.0,21.0,-78.0,75.0);
      break;
    case 7:
      // Re 2740
      slOptions.setLimits(-15.0,18.0,-12.0,21.0,-78.0,75.0);
      break;
  }

  // Get First Scan
  MRIStructuredScan* myScan = new MRIStructuredScan(0.0);

  // Read From File
  //myScan->ReadPltFile(inFileName,true);

  // Export to VTK
  //myScan->ExportToVTK(outfileName+"_Model.vtk");

  // Compute Streamlines
  std::vector<MRIStreamline*> streamlines;
  //myScan->ComputeStreamlines(outfileName+"_grid.dat",slOptions,streamlines);

  // Read Streamlines from File
  MRIUtils::ReadStreamlinesFromLegacyVTK(inFileName,streamlines);

  // Print Streamlines To File
  //MRIUtils::PrintStreamlinesToVTK(streamlines,outfileName+"_DebugStreamlines.vtk");

  // Eval Streamlines Statistics
  myScan->EvalStreamLineStatistics(outfileName,kdirZ,slOptions,streamlines);
}

// =========================
// PERFORM STREAMLINE TEST 2
// =========================
void PerformStreamlineTest2(std::string inFileName,std::string outfileName){

  // Set Default Options For Streamlines
  // Re   Xmin  Xmax Ymin  Ymax Zmin  Zmax
  // 80   -18.0 15.0 -12.0 21.0 -78.0 75.0
  // 145  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 190  -14.0 19.0 -3.0  30.0 -78.0 75.0
  // 240  -14.0 19.0 -3.0  30.0 -78.0 75.0
  // 290  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 458  -12.0 21.0 -22.0 11.0 -78.0 75.0
  // 1390 -15.0 18.0 -12.0 21.0 -78.0 75.0
  // 2740 -15.0 18.0 -12.0 21.0 -78.0 75.0
  MRIStreamlineOptions slOptions(0.0,0.0,0.0,0.0,0.0,0.0);
  slOptions.setLimits(-18.0,15.0,-12.0,21.0,-78.0,75.0);

  // Get First Scan
  MRIStructuredScan* myScan = new MRIStructuredScan(0.0);

  // Read From File
  myScan->ReadPltFile(inFileName,true);

  // Set Velocities constant in Z
  for(int loopA=0;loopA<myScan->totalCellPoints;loopA++){
    myScan->cellPoints[loopA].velocity[0] = 0.01;
    myScan->cellPoints[loopA].velocity[1] = 0.01;
    myScan->cellPoints[loopA].velocity[2] = 1.0;
  }

  // Export to VTK
  myScan->ExportToVTK(outfileName+"_Model.vtk");

  // Compute Streamlines
  std::vector<MRIStreamline*> streamlines;
  myScan->ComputeStreamlines(outfileName+"_grid.dat",slOptions,streamlines);

  // Read Streamlines from File
  //MRIUtils::ReadStreamlinesFromLegacyVTK(inFileName,streamlines);

  // Print Streamlines To File
  MRIUtils::PrintStreamlinesToVTK(streamlines,outfileName+"_DebugStreamlines.vtk");

  // Eval Streamlines Statistics
  myScan->EvalStreamLineStatistics(outfileName,kdirZ,slOptions,streamlines);
}

// ========================================
// BUILD FROM COEFFICIENTS AND WRITE TO PLT
// ========================================
void BuildFromCoeffs(std::string coeffFileName,std::string plotOut,bool performThreshold,int thresholdType,double threshold){

  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // ADD FILE TO SEQUENCE
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadFromExpansionFile(coeffFileName,performThreshold,thresholdType,threshold);
  MyMRISequence->AddScan(MyMRIScan);

  // APPLY THRESHOLDING
  //MRIThresholdCriteria criteria(kCriterionLessThen,kQtyConcentration,1000.0);
  //MyMRISequence->ApplyThresholding(criteria);

  // EXPORT TO PLT FILE
  //MyMRISequence->ExportToTECPLOT(plotOut);
  MyMRISequence->ExportToVTK(plotOut);
}

// ============================
// EVAL VARIOUS VORTEX CRITERIA
// ============================
void EvalVortexCriteria(std::string inFileName,std::string outFileName){
  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // ADD FILE TO SEQUENCE
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);
  MyMRIScan->ReadPltFile(inFileName,true);
  MyMRISequence->AddScan(MyMRIScan);

  // EVAL VORTEX CRITERIA
  // MyMRISequence->GetScan(0)->EvalVortexCriteria();
  // MyMRISequence->GetScan(0)->EvalVorticity();
  MyMRISequence->GetScan(0)->EvalEnstrophy();


  // WRITE OUTPUT FILES TO VTK
  MyMRISequence->ExportToVTK(outFileName);
}

// ===========================================
// WRITE SPATIAL DISTRIBUTIONS OF COEFFICIENTS
// ===========================================
void WriteSpatialExpansion(std::string expFileName,std::string outFileName){
  // CREATE NEW SEQUENCES
  MRISequence* MyMRISequence = new MRISequence(false/*Cyclic Sequence*/);

  // CREATE NEW SCAN
  MRIStructuredScan* MyMRIScan = new MRIStructuredScan(0.0);

  // READ FROM EXPANSION COEFFICIENTS
  MyMRIScan->ReadFromExpansionFile(expFileName,false,kSoftThreshold,0.0);

  // ADD TO SEQUENCE
  MyMRISequence->AddScan(MyMRIScan);

  // SPATIALLY EVALUATE VORTEX COEFFICIENTS
  MyMRISequence->GetScan(0)->EvalSMPVortexCriteria(MyMRISequence->GetScan(0)->expansion);

  // EXPORT TO VTK
  MyMRISequence->ExportToVTK(outFileName);
}

// ============
// MAIN PROGRAM
// ============
int main(int argc, char **argv){

  // Init MRI Communicator
  mriCommunicator* comm = new mriCommunicator();

  // Initialize MPI
  MPI::Init();
  int rank; int nproc;
  comm->mpiComm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm->mpiComm, &comm->currProc);
  MPI_Comm_size(comm->mpiComm, &comm->totProc);

  //  Declare
  int val = 0;
  mriProgramOptions* options = new mriProgramOptions();

  // WRITE PROGRAM HEADER - ONLY MASTER NODE
  if(comm->currProc == 0){
    WriteHeader();
  }

  // Get Commandline Options
  int res = options->getCommadLineOptions(argc,argv);
  if(res != 0){
    return -1;
  }

  try{
    // Normal Model Running
    switch(options->runMode){
      case rmEVALSEQUENCEPRESSURE:
        // Eval Pressure From Velocity Sequence
        EvalSequencePressure(options->inputFileName,options->outputFileName);
        break;
      case rmEVALPRESSUREFROMSIGNATUREFLOW:
        // Eval Pressure for signature Flow Cases
        EvalPressureFromSignatureFlow(options->inputFileName,options->outputFileName);
        break;
      case rmPROCESSSINGLESCAN:
      {
        // Get Parameters
        double itTol = atof(argv[4]);
        int maxIt = atoi(argv[5]);
        std::string thresholdTypeString(argv[6]);
        double thresholdValue = atof(argv[7]);
        // Process Single Scan
        ProcessSingleScan(options->inputFileName,
                          options->outputFileName,
                          options->itTol,
                          options->maxIt,
                          options->thresholdType,
                          options->thresholdValue);
        break;
      }
      case rmPLTTOVTK:
      {
        // Convert to VTK
        ConvertTECPLOToVTK(options->inputFileName,options->outputFileName);
        break;
      }
      case rmEVALSCANSTATISTICS:
      {
        // COMPUTE SCAN STATISTICS
        ComputeScanStatistics(options->inputFileName,
                              options->outputFileName,
                              options->statFileName);
        break;
      }
      case rmCOMUTESCANMATRICES:
        // COMPUTE SCAN MATRICES
        ComputeScanMatrices();
        break;
      case rmPERFORMRANDOMTEST:
        // PERFORM RANDOM TEST
        PerformRandomTest();
        break;
      case rmCROPANDCOMPUTEVOLUME:
        // CROP AND REDUCE VOL FILE
        CropAndComputeVol(options->inputFileName,options->outputFileName);
        break;
      case rmSTREAMLINETEST1:
      {
        // GET FILE NAME
        int intValue = atoi(argv[2]);
        // PERFORM STREAMLINE TEST 1
        PerformStreamlineTest1(intValue,
                               options->inputFileName,
                               options->outputFileName);
        break;
      }
      case rmSTREAMLINETEST2:
        // PERFORM STREAMLINE TEST 2
        PerformStreamlineTest2(options->inputFileName,options->outputFileName);
        break;
      case rmPRINTTHRESHOLDINGTOVTK:
        // Test Expansion Coefficients
        // TEST_ExpansionCoefficients(inFileName);
        TEST02_PrintThresholdingToVTK(options->inputFileName);
        break;
      case rmEVALREYNOLDSSTRESSES:
        // Test Expansion Coefficients
        TEST03_EvalReynoldsStresses(options->inputFileName);
        break;
      case rmSHOWFACEFLUXPATTERS:
        // READ FACE FLUXES FROM FILE AND EXPORT TO VTK
        ShowFaceFluxPatterns(options->inputFileName,options->outputFileName);
        break;
      case rmBUILDFROMCOEFFICIENTS:
      {
        // Get File Names
        std::string plotOut(argv[3]);
        double threshold = atof(argv[4]);
        // READ FROM COEFFICIENT FILE AND EXPORT TO PLT
        BuildFromCoeffs(options->inputFileName,plotOut,true,kSoftThreshold,threshold);
        break;
      }
      case rmEVALPRESSURE:
        // EVAL PRESSURES FROM EXPANSION COFFICIENTS
        EvalPressureFromExpansion(options);
        break;
      case rmEVALCONCGRADIENT:
        // Eval Concetrantion Gradient
        EvalConcentrationGradient(options->inputFileName,options->outputFileName);
        break;
      case rmEVALVORTEXCRITERIA:
        // Eval Vortex Criterion
        EvalVortexCriteria(options->inputFileName,options->outputFileName);
        break;
      case rmWRITESPATIALEXPANSION:
        // EVAL Spatial Expansion Coefficients Distribution
        WriteSpatialExpansion(options->inputFileName,options->outputFileName);
        break;
      case rmSOLVEPOISSON:
        // Solve Poisson Equation to Compute the pressures
        SolvePoissonEquation(comm);
        break;
      default:
        // Invalid Switch
        std::string currMsgs("Error: Invalid Switch. Terminate.\n");
        WriteSchMessage(currMsgs);
        return (1);
        break;
    }
  }catch (std::exception& ex){
    WriteSchMessage(std::string(ex.what()));
    WriteSchMessage(std::string("\n"));
    WriteSchMessage(std::string("Program Terminated.\n"));
    return -1;
  }
  WriteSchMessage(std::string("\n"));
  WriteSchMessage(std::string("Program Completed.\n"));
  // Finalize MPI
  MPI::Finalize();
  return val;
}

