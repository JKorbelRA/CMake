/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmBinUtilsWindowsPELinker_h
#define cmBinUtilsWindowsPELinker_h

#include "cmBinUtilsLinker.h"
#include "cmBinUtilsWindowsPEGetRuntimeDependenciesTool.h"
#include "cmStateTypes.h"

#include <memory> // IWYU pragma: keep
#include <string>

class cmRuntimeDependencyArchive;

class cmBinUtilsWindowsPELinker : public cmBinUtilsLinker
{
public:
  cmBinUtilsWindowsPELinker(cmRuntimeDependencyArchive* archive);

  bool Prepare() override;

  bool ScanDependencies(std::string const& file,
                        cmStateEnums::TargetType type) override;

private:
  std::unique_ptr<cmBinUtilsWindowsPEGetRuntimeDependenciesTool> Tool;

  bool ResolveDependency(std::string const& name, std::string const& origin,
                         std::string& path, bool& resolved);
};

#endif // cmBinUtilsWindowsPELinker_h
