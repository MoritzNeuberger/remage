#include "MGProcessesMessenger.hh"
#include "MGProcessesList.hh"
#include "MGUIcmdStepLimit.hh"
#include "MGLogger.hh"
// #include "MGManager.hh"
// #include "MGManagerDetectorConstruction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "globals.hh"
// #include "G4ProcessManager.hh"
// #include "G4StepLimiter.hh"

MGProcessesMessenger::MGProcessesMessenger(MGProcessesList *plist) :
  fProcessesList(plist) {

  fMGProcessesDir = new G4UIdirectory("/MG/processes/");
  fMGProcessesDir->SetGuidance("UI commands to control the energy realm of the simulation");

  fMGProcessesChoiceCmd = new G4UIcmdWithAString("/MG/processes/realm", this);
  fMGProcessesChoiceCmd->SetGuidance("Select the simulation realm");
  fMGProcessesChoiceCmd->SetParameterName("simRealm", false);
  fMGProcessesChoiceCmd->SetCandidates("BBdecay DarkMatter CosmicRays OpticalPhoton");
  fMGProcessesChoiceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fOpticalProcessesCmd = new G4UIcmdWithABool("/MG/processes/optical", this);
  fOpticalProcessesCmd->SetGuidance("Switch on/off optical processes");
  fOpticalProcessesCmd->SetGuidance("(default: false)");
  fOpticalProcessesCmd->AvailableForStates(G4State_PreInit);

  fOpticalOnlyCmd = new G4UIcmdWithABool("/MG/processes/opticalOnly", this);
  fOpticalOnlyCmd->SetGuidance("Only use optical processes");
  fOpticalOnlyCmd->SetGuidance("(default: false)");
  fOpticalOnlyCmd->AvailableForStates(G4State_PreInit);

  fLowEnergyProcessesCmd = new G4UIcmdWithABool("/MG/processes/lowenergy", this);
  fLowEnergyProcessesCmd->SetGuidance("Set on/off the LowE EM processes");
  fLowEnergyProcessesCmd->SetGuidance("(default: true)");
  fLowEnergyProcessesCmd->AvailableForStates(G4State_PreInit);

  fLowEnergyProcessesOptionCmd = new G4UIcmdWithAnInteger("/MG/processes/lowenergyOption", this);
  fLowEnergyProcessesOptionCmd->SetGuidance("Switch between physics list options when lowenergy flag enabled");
  fLowEnergyProcessesOptionCmd->SetGuidance("see https://geant4.web.cern.ch/node/1731#Ex");
  fLowEnergyProcessesOptionCmd->SetGuidance("0 (default) Livermore");
  fLowEnergyProcessesOptionCmd->SetGuidance("1           EmOption1");
  fLowEnergyProcessesOptionCmd->SetGuidance("2           EmOption2");
  fLowEnergyProcessesOptionCmd->SetGuidance("3           EmOption3");
  fLowEnergyProcessesOptionCmd->SetGuidance("4           EmOption4");
  fLowEnergyProcessesOptionCmd->SetGuidance("5           Penelope");
  fLowEnergyProcessesOptionCmd->SetGuidance("6           LivermorePolarized");
  fLowEnergyProcessesOptionCmd->AvailableForStates(G4State_PreInit);
  fLowEnergyProcessesOptionCmd->SetDefaultValue(0);

  // FIXME: uncomment this
  // fStepLimitCmd = new MGUIcmdStepLimit("/MG/processes/setStepLimit", this);
  // fGetStepLimitCmd = new G4UIcmdWithoutParameter("/MG/processes/GetStepLimit", this);
  // fGetStepLimitCmd->SetGuidance("Prints step limits for all particles");

  fAngCorrCmd = new G4UIcmdWithAnInteger("/MG/processes/useAngCorr", this);
  fAngCorrCmd->SetGuidance("Turn on angular correlations and optionally set max two J to something other than 20");
  fAngCorrCmd->SetGuidance("Must be called before /run/initialize");
  fAngCorrCmd->SetParameterName("maxTwoJ", true);
  fAngCorrCmd->SetDefaultValue(20);
  fAngCorrCmd->AvailableForStates(G4State_PreInit);

  fStoreICLevelData = new G4UIcmdWithABool("/MG/processes/storeICLevelData", this);
  fStoreICLevelData->SetGuidance("Enable storage of internal conversion level data. This has to be enabled manually due to the order in which processes are initialized");
  fStoreICLevelData->SetGuidance("(default: true)");
  fStoreICLevelData->SetDefaultValue(true);
  fStoreICLevelData->AvailableForStates(G4State_PreInit);
}

MGProcessesMessenger::~MGProcessesMessenger() {
  delete fMGProcessesChoiceCmd;
  delete fMGProcessesDir;
  delete fOpticalProcessesCmd;
  delete fOpticalOnlyCmd;
  delete fLowEnergyProcessesCmd;
  delete fLowEnergyProcessesOptionCmd;
  // delete fStepLimitCmd;
  // delete fGetStepLimitCmd;
  delete fAngCorrCmd;
  delete fStoreICLevelData;
}

void MGProcessesMessenger::SetNewValue(G4UIcommand *cmd, G4String new_val) {

  if (cmd == fMGProcessesChoiceCmd) fProcessesList->SetRealm(new_val);
  else if (cmd == fOpticalProcessesCmd) {
    fProcessesList->SetOpticalFlag(fOpticalProcessesCmd->GetNewBoolValue(new_val));
  }
  else if (cmd == fOpticalOnlyCmd) {
    fProcessesList->SetOpticalPhysicsOnly(fOpticalOnlyCmd->GetNewBoolValue(new_val));
  }
  else if (cmd == fLowEnergyProcessesCmd) {
    fProcessesList->SetLowEnergyFlag(fLowEnergyProcessesCmd->GetNewBoolValue(new_val));
  }
  else if (cmd == fLowEnergyProcessesOptionCmd) {
    fProcessesList->SetLowEnergyOption(fLowEnergyProcessesOptionCmd->GetNewIntValue(new_val));
  }
  // else if (cmd == fStepLimitCmd) {
  //   G4String particle_name = fStepLimitCmd->GetParticleName(new_val);
  //   fProcessesList->LimitStepForParticle(particle_name);
  //   MGLog(trace) << "Limit step for " << particle_name << endlog;

  //   G4String volume_name = fStepLimitCmd->GetVolumeName(new_val);
  //   G4double max_step = fStepLimitCmd->GetStepSize(new_val);
  //   MGManager::GetMGManager()->GetManagerDetectorConstruction()->SetMaxStepLimit(volume_name, max_step);
  // }
  // else if (cmd == fGetStepLimitCmd) fProcessesList->GetStepLimits();
  else if (cmd == fAngCorrCmd) {
    fProcessesList->SetUseAngCorr(fAngCorrCmd->GetNewIntValue(new_val));
  }
  else if (cmd == fStoreICLevelData) {
    fProcessesList->SetStoreICLevelData(fStoreICLevelData->GetNewBoolValue(new_val));
  }
}

// vim: shiftwidth=2 tabstop=2 expandtab
