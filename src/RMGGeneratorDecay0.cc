#include "RMGGeneratorDecay0.hh"
#include "RMGLog.hh"
#include "RMGManager.hh"

RMGGeneratorDecay0::RMGGeneratorDecay0(RMGVVertexGenerator* prim_gen) : RMGVGenerator("Decay0") {

  if (!RMGManager::Instance()->IsExecSequential())
    RMGLog::Out(RMGLog::fatal, "BxDecay0 is not thread-safe (yet). Re-run in sequential mode.");

  if (!prim_gen) RMGLog::OutDev(RMGLog::fatal, "Primary position generator is nullptr");

  fDecay0G4Generator = std::make_unique<bxdecay0_g4::PrimaryGeneratorAction>();
  // NOTE: BxDecay0's primary generator action will own the pointer
  fDecay0G4Generator->SetVertexGenerator(prim_gen);
}

void RMGGeneratorDecay0::GeneratePrimariesKinematics(G4Event* event) {
  fDecay0G4Generator->GeneratePrimaries(event);
}

// vim: tabstop=2 shiftwidth=2 expandtab
