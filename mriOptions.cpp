#include "mriOptions.h"
#include "mriThresholdCriteria.h"
#include "mriConstants.h"
#include "mriUtils.h"
#include "mriException.h"

#include <stdlib.h>
#include <getopt.h>

MRIOptions::MRIOptions(){
  // Set Default Values of the parameters
  runMode = rmHELP;
  // File Names
  inputFileName = "";
  outputFileName = "";
  statFileName = "";
  generateCommandFile = false;
  useCommandFile = false;
  commandFileName = "";
  // Parameters
  itTol = 1.0e-3;
  maxIt = 2000;
  thresholdType = kNoQuantity;
  thresholdValue = 0.0;
  // Save Initial Velocities
  saveInitialVel = false;
  saveExpansionCoeffs = false;
  // Apply Filter
  applySMPFilter = false;
  applyBCFilter = false;
  useConstantPatterns = true;
  // Export Format Type
  inputFormatType = itFILEVTK;
  outputFormatType = otFILEVTK;
  // Default: Process Single Scan
  sequenceFileName = "";
  // Post processing
  evalPopVortexCriteria = false;
  evalSMPVortexCriterion = false;
  evalPressure = false;
  // Export to Poisson
  exportToPoisson = false;
}

// =========================
// GET COMMANDLINE ARGUMENTS
// =========================
int MRIOptions::getCommadLineOptions(int argc, char **argv){
  int c;
  string threshString;
  printf("--- COMMAND OPTIONS ECHO\n");
  while (1){
    static struct option long_options[] =
    { {"input",     required_argument, 0, 'i'},
      {"output",    required_argument, 0, 'o'},
      {"command",   required_argument, 0, 'c'},
      {"filter",        no_argument, 0, 1},
      {"bcfilter",        no_argument, 0, 2},
      {"iterationTol",  required_argument, 0, 3},
      {"maxIterations", required_argument, 0, 4},
      {"thresholdQty",  required_argument, 0, 5},
      {"thresholdType", required_argument, 0, 6},
      {"thresholdValue",required_argument, 0, 7},
      {"iformat",    required_argument, 0, 8},
      {"oformat",    required_argument, 0, 9},
      {"sequence",    required_argument, 0, 10},
      {"noConstPatterns",    no_argument, 0, 11},
      {"evalPOPVortex",    no_argument, 0, 12},
      {"evalSMPVortex",    no_argument, 0, 13},
      {"saveInitialVel",    no_argument, 0, 14},
      {"saveExpansionCoeffs",    no_argument, 0, 15},
      {"poisson",    no_argument, 0, 16},
      {"normal",        no_argument, 0, 22},
      {"writexp",   no_argument, 0, 23},
      {"test",      no_argument, 0, 24},
      {"testMPI",   no_argument, 0, 25},
      {0, 0, 0, 0}
    };

    /* getopt_long stores the option index here. */
    int option_index = 0;    

    c = getopt_long (argc, argv, "c:i:o:g:",
                     long_options, &option_index);

    /* Detect the end of the options.*/
    if (c == -1){
      break;
    }    
    // Check which option was selected
    switch (c){
      case 0:
        /* If this option sets a flag, do nothing else now. */
        if (long_options[option_index].flag != 0){
          break;
        }
        printf ("option %s", long_options[option_index].name);
        if (optarg){
          printf (" with arg %s", optarg);
        }
        printf ("\n");
        break;
      case 'i':
        // Set input file name
        inputFileName = optarg;
        printf("Input File: %s\n",inputFileName.c_str());
        break;
      case 'o':
        // Set input file name
        outputFileName = optarg;
        printf("Output File: %s\n",outputFileName.c_str());
        break;
      case 'c':
        // Set input file name
        useCommandFile = true;
        commandFileName = optarg;
        printf("COMMAND FILE MODE\n");
        printf("Command File: %s\n",commandFileName.c_str());
        break;
      case 'g':
        // Set input file name
        generateCommandFile = true;
        commandFileName = optarg;
        printf("COMMAND FILE GENERATION MODE\n");
        printf("Command File: %s\n",commandFileName.c_str());
        break;
      case 1:
        applySMPFilter = true;
        printf("SMP Filter Enabled\n");
        break;
      case 2:
        applyBCFilter = true;
        printf("Boundary Filter Enabled\n");
        break;
      case 3:
        itTol = atof(optarg);
        printf("Iteration Tolerance set to %e\n",itTol);
        break;
      case 4:
        maxIt = atoi(optarg);
        printf("Maximum number of iterations set to %d\n",maxIt);
        break;
      case 5:
        thresholdQty = atoi(optarg);
        threshString = MRIUtils::getThresholdQtyString(thresholdQty);
        printf("Threshold quantity set: %s\n",threshString.c_str());
        break;
      case 6:
        thresholdType = atoi(optarg);
        threshString = MRIUtils::getThresholdTypeString(thresholdType);
        printf("Threshold type set: %s\n",threshString.c_str());
        break;
      case 7:
        thresholdValue = atof(optarg);
        printf("Threshold value: %f\n",thresholdValue);
        break;
      case 8:
        inputFormatType = atoi(optarg);
        printf("Input File Format: %s\n",optarg);
        break;
      case 9:
        outputFormatType = atoi(optarg);
        printf("Output File Format: %s\n",optarg);
        break;
      case 10:
        sequenceFileName = optarg;
        // Read Sequence file names
        printf("Output File Format: %s\n",optarg);
        break;
      case 11:
        useConstantPatterns = false;
        // Read Sequence file names
        printf("Constant Patterns Disabled in BC Filter\n");
        break;
      case 12:
        evalPopVortexCriteria = true;
        printf("Evaluating Vortex Criteria\n");
        break;
      case 13:
        evalSMPVortexCriterion = true;
        printf("Evaluating SMP Vortex Criteria\n");
        break;
      case 14:
        saveInitialVel = true;
        printf("Saving Initial Velocities To Output\n");
        break;
      case 15:
        saveExpansionCoeffs = true;
        printf("Saving Expansion Coefficients\n");
        break;
      case 16:
        exportToPoisson = true;
        printf("Exporting FE Files for Poisson Solver\n");
        break;
      case 22:
        runMode = rmNORMAL;
        // Read Sequence file names
        printf("Running in NORMAL mode\n");
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;
      default:
        abort ();
      }
    }
    /* Print any remaining command line arguments (not options). */
    /*if (optind < argc){
      printf ("non-option ARGV-elements: ");
      while (optind < argc){
        printf ("%s ", argv[optind++]);
      }
      putchar ('\n');
    }*/
  printf("---\n");
  printf("\n");
  return 0;
}

// ======================================
// FINALIZE OPTIONS: PROCESS INPUT PARAMS
// ======================================
void MRIOptions::finalize(){
  // CREATE THRESHOLD OBJECT
  thresholdCriteria = new MRIThresholdCriteria(thresholdQty,thresholdType,thresholdValue);
  // FILL FILE LIST SEQUENCE WITH SINGLE FILE
  if(sequenceFileName == ""){
    sequenceFileList.push_back(inputFileName);
  }else{
    // READ FROM SEQUENCE LIST FILE
  }
}

// =============================
// GET OPTIONS FROM COMMAND FILE
// =============================
int MRIOptions::getOptionsFromCommandFile(string commandFile){
  // Write Message
  printf("Reading Command file: %s\n",commandFile.c_str());

  // Declare input File
  std::ifstream infile;
  infile.open(commandFile);

  // Declare
  vector<string> tokenizedString;

  // Read Data From File
  std::string buffer;
  while (std::getline(infile,buffer)){
    // Trim String
    boost::trim(buffer);
    // Tokenize String
    boost::split(tokenizedString, buffer, boost::is_any_of(":"), boost::token_compress_on);
    // CHECK THE ELEMENT TYPE
    if(boost::to_upper_copy(tokenizedString[0]) == std::string("RUNMODE")){
      // READ RUN MODE
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("NORMAL")){
        runMode =rmNORMAL;
      }else{
        throw MRIException("ERROR: Invalid Run Mode.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("INPUTFILE")){
      try{
        inputFileName = tokenizedString[1];
      }catch(...){
        throw MRIException("ERROR: Invalid Input File.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("OUTPUTFILE")){
      try{
        outputFileName = tokenizedString[1];
      }catch(...){
        throw MRIException("ERROR: Invalid Output File.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("STATFILE")){
      try{
        statFileName = tokenizedString[1];
      }catch(...){
        throw MRIException("ERROR: Invalid Statistics File.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("SMPITERATIONTOLERANCE")){
      try{
        itTol = atof(tokenizedString[1].c_str());
      }catch(...){
        throw MRIException("ERROR: Invalid SMP Tolerance.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("SMPMAXITERATIONS")){
      try{
        maxIt = atoi(tokenizedString[1].c_str());
      }catch(...){
        throw MRIException("ERROR: Invalid Max number of SMP Iterations.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("THRESHOLDQTY")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("POSX")){
        thresholdQty = kQtyPositionX;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("POSY")){
        thresholdQty = kQtyPositionY;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("POSZ")){
        thresholdQty = kQtyPositionZ;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("CONCENTRATION")){
        thresholdQty = kQtyConcentration;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("VELX")){
        thresholdQty = kQtyVelocityX;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("VELY")){
        thresholdQty = kQtyVelocityY;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("VELZ")){
        thresholdQty = kQtyVelocityZ;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("VELMOD")){
        thresholdQty = kQtyVelModule;
      }else{
        throw MRIException("ERROR: Invalid Threshold Quantity.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("THRESHOLDTYPE")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("LT")){
        thresholdType = kCriterionLessThen;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("GT")){
        thresholdType = kCriterionGreaterThen;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("ABSLT")){
        thresholdType = kCriterionABSLessThen;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("ABSGT")){
        thresholdType = kCriterionABSGreaterThen;
      }else{
        throw MRIException("ERROR: Invalid Threshold Type.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("THRESHOLDVALUE")){
      try{
        thresholdValue = atof(tokenizedString[1].c_str());
      }catch(...){
        throw MRIException("ERROR: Invalid Threshold Value.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("SAVEINITIALVELOCITIES")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        saveInitialVel = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        saveInitialVel = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for saveInitialVel.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("SAVEEXPANSIONCOEFFS")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        saveExpansionCoeffs = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        saveExpansionCoeffs = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for saveExpansionCoeffs.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("INPUTTYPE")){
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("OUTPUTTYPE")){
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("USESMPFILTER")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        applySMPFilter = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        applySMPFilter = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for applySMPFilter.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("USEBCFILTER")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        applyBCFilter = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        applyBCFilter = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for applyBCFilter.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("USECONSTANTPATTERNS")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        useConstantPatterns = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        useConstantPatterns = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for useConstantPatterns.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("SEQUENCEFILENAME")){
      try{
        sequenceFileName = tokenizedString[1];
      }catch(...){
        throw MRIException("ERROR: Invalid Sequence File Name.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("EVALVORTEXCRITERIA")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        evalPopVortexCriteria = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        evalPopVortexCriteria = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for evalPopVortexCriteria.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("EVALSMPVORTEXCRITERIA")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        evalSMPVortexCriterion = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        evalSMPVortexCriterion = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for evalSMPVortexCriterion.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("EVALPRESSURE")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        evalPressure = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        evalPressure = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for evalPressure.\n");
      }
    }else if(boost::to_upper_copy(tokenizedString[0]) == std::string("EXPORTTOPOISSON")){
      if(boost::to_upper_copy(tokenizedString[1]) == std::string("TRUE")){
        exportToPoisson = true;
      }else if(boost::to_upper_copy(tokenizedString[1]) == std::string("FALSE")){
        exportToPoisson = false;
      }else{
        throw MRIException("ERROR: Invalid logical value for exportToPoisson.\n");
      }
    }
  }
  // Close File
  infile.close();
}

// ============================
// WRITE COMMAND FILE PROTOTYPE
// ============================
int writeCommandFilePrototype(string commandFile){
    // File Names
    inputFileName = "";
    outputFileName = "";
    statFileName = "";
    // Parameters
    itTol = 1.0e-3;
    maxIt = 2000;
    thresholdType = kNoQuantity;
    thresholdValue = 0.0;
    // Save Initial Velocities
    saveInitialVel = false;
    saveExpansionCoeffs = false;
    // Apply Filter
    applySMPFilter = false;
    applyBCFilter = false;
    useConstantPatterns = true;
    // Export Format Type
    inputFormatType = itFILEVTK;
    outputFormatType = otFILEVTK;
    // Default: Process Single Scan
    sequenceFileName = "";
    // Post processing
    evalPopVortexCriteria = false;
    evalSMPVortexCriterion = false;
    evalPressure = false;
    // Export to Poisson
    exportToPoisson = false;

}

