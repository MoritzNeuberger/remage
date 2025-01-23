// Copyright (C) 2022 Luigi Pertoldi <gipert@pm.me>
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <filesystem>
#include <random>

#include "RMGVertexFromFile.hh"
namespace fs = std::filesystem;

#include "G4AnalysisUtilities.hh"
#include "G4RootAnalysisReader.hh"
#include "G4Threading.hh"
#include "G4VAnalysisReader.hh"
#if RMG_HAS_HDF5
#include "G4Hdf5AnalysisReader.hh"
#endif
#include "G4CsvAnalysisReader.hh"
#include "G4XmlAnalysisReader.hh"

#if RMG_HAS_HDF5
#include "RMGConvertLH5.hh"
#endif

#include "RMGLog.hh"

G4Mutex RMGAnalysisReader::fMutex = G4MUTEX_INITIALIZER;

RMGAnalysisReader::Access RMGAnalysisReader::OpenFile(std::string& file_name,
    std::string ntuple_dir_name, std::string ntuple_name) {

  // create an invalid lock to return on error.
  G4AutoLock invalid_lock(&fMutex, std::defer_lock);
  invalid_lock.release();
  auto invalid_access = RMGAnalysisReader::Access(std::move(invalid_lock), nullptr, -1);

  // reader initialization should only happen on the master thread (otherwise it will fail).
  if (!G4Threading::IsMasterThread()) {
    RMGLog::OutDev(RMGLog::fatal, "can only be used on the master thread");
    std::abort();
  }

  G4AutoLock lock(&fMutex);

  if (fReader) return std::move(invalid_access);

  fFileIsTemp = false;

  if (!fs::exists(fs::path(file_name))) {
    RMGLog::Out(RMGLog::error, "input file ", file_name, " does not exist");
    return std::move(invalid_access);
  }

  // NOTE: GetExtension() returns a default extension if there is no file extension
  auto ext = G4Analysis::GetExtension(file_name);
  if (ext == "root") {
    fReader = G4RootAnalysisReader::Instance();
  } else if (ext == "hdf5") {
#if RMG_HAS_HDF5
    fReader = G4Hdf5AnalysisReader::Instance();
#else
    RMGLog::Out(RMGLog::fatal,
        "HDF5 input not available, please recompile Geant4 with HDF5 support");
#endif
  } else if (ext == "lh5") {
#if RMG_HAS_HDF5
    std::uniform_int_distribution<int> dist(10000, 99999);
    std::random_device rd;
    auto path = fs::path(file_name);
    std::string new_fn =
        ".rmg-vtx-" + std::to_string(dist(rd)) + "." + path.stem().string() + ".hdf5";

    std::error_code ec;
    if (!fs::copy_file(path, fs::path(new_fn)), ec) {
      RMGLog::Out(RMGLog::error, "copy of input file ", file_name, " failed. Does it exist? (",
          ec.message(), ")");
      return std::move(invalid_access);
    }

    if (!RMGConvertLH5::ConvertFromLH5(new_fn, ntuple_dir_name, false)) {
      RMGLog::Out(RMGLog::error, "Conversion of input file ", new_fn,
          " to LH5 failed. Data is potentially corrupted.");
      return std::move(invalid_access);
    }

    file_name = new_fn;
    fReader = G4Hdf5AnalysisReader::Instance();
    fFileIsTemp = true;
#else
    RMGLog::Out(RMGLog::fatal,
        "HDF5 input not available, please recompile Geant4 with HDF5 support");
#endif
  } else if (ext == "csv") {
    fReader = G4CsvAnalysisReader::Instance();
  } else if (ext == "xml") {
    fReader = G4XmlAnalysisReader::Instance();
  } else {
    RMGLog::OutFormat(RMGLog::fatal, "File Extension '.{}' not recognized!", ext);
  }

  if (RMGLog::GetLogLevel() <= RMGLog::debug) fReader->SetVerboseLevel(10);

  fFileName = file_name;

  fNtupleId = fReader->GetNtuple(ntuple_name, file_name, ntuple_dir_name);
  if (fNtupleId < 0) {
    RMGLog::Out(RMGLog::error, "Ntuple named '", ntuple_name, "' could not be found in input file!");
    return std::move(invalid_access);
  }
  return {std::move(lock), fReader, fNtupleId};
}

void RMGAnalysisReader::CloseFile() {
  if (!G4Threading::IsMasterThread()) {
    RMGLog::OutDev(RMGLog::fatal, "can only be used on the master thread");
    return;
  }

  G4AutoLock lock(&fMutex);

  if (!fReader) return;

  // fReader is a thread-local singleton. Do not delete it here, otherwise geant4 will to delete it
  // again. also do not close files, as there is no way to close a specific file only.
  fReader = nullptr;
  if (fFileIsTemp) {
    std::error_code ec;
    fs::remove(fs::path(fFileName), ec);
  }
  fFileName = "";
  fFileIsTemp = false;
  fNtupleId = -1;
}

RMGAnalysisReader::Access RMGAnalysisReader::GetLockedReader() const {

  G4AutoLock lock(&fMutex);

  return {std::move(lock), fReader, fNtupleId};
}
