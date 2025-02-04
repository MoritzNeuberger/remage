#ifndef _RMG_OUTPUT_SCHEME_HH_
#define _RMG_OUTPUT_SCHEME_HH_

#include <map>
#include <string>

#include "G4AnalysisManager.hh"

#include "fmt/format.h"

class G4Event;
class RMGVOutputScheme {

  public:

    inline RMGVOutputScheme(G4AnalysisManager* ana_man) { this->AssignOutputNames(ana_man); }
    virtual inline void clear(){};
    virtual inline void AssignOutputNames(G4AnalysisManager*){};
    virtual inline void EndOfEventAction(const G4Event*){};
    inline std::string GetNtupleName(int det_uid) { return fmt::format("det{:03}", det_uid); }
};

#endif

// vim: tabstop=2 shiftwidth=2 expandtab
