/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalIarGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmSourceFile.h"
#include "cmGlobalIarGenerator.h"
#include "cmMakefile.h"
#include "cmStringAlgorithms.h"

cmLocalIarGenerator::cmLocalIarGenerator(cmGlobalGenerator* gg,
                                         cmMakefile* mf)
  : cmLocalGenerator(gg, mf)
{
}

cmLocalIarGenerator::~cmLocalIarGenerator() = default;

void cmLocalIarGenerator::Generate()
{
  // Collect all targets from all generators.
  for (cmGeneratorTarget* gt :
       this->GlobalGenerator->GetLocalGeneratorTargetsInOrder(this)) {
    if (!gt->IsInBuildSystem()) {
      continue;
    }

    switch (gt->GetType()) {
      case cmStateEnums::EXECUTABLE:
      case cmStateEnums::STATIC_LIBRARY:
      case cmStateEnums::SHARED_LIBRARY:
      case cmStateEnums::MODULE_LIBRARY:
      case cmStateEnums::OBJECT_LIBRARY: {
        cmGlobalIarGenerator* globIarGen =
          static_cast<cmGlobalIarGenerator*>(this->GlobalGenerator);
        globIarGen->ConvertTargetToProject(*(gt->Target), gt);
        break;
      }
      default:
        break;
    }
  }
}

std::string cmLocalIarGenerator::GetTargetDirectory(
  cmGeneratorTarget const* target) const
{
  std::string dir = cmStrCat(target->GetName(), ".dir");
  return dir;
}

void cmLocalIarGenerator::ComputeObjectFilenames(
  std::map<cmSourceFile const*, std::string>& mapping,
  cmGeneratorTarget const* gt)
{
  std::string dir_max = cmStrCat(this->GetCurrentBinaryDirectory(), '/',
                                 this->GetTargetDirectory(gt), '/',
                                 this->GlobalGenerator->GetCMakeCFGIntDir(), '/');

  // Count the number of object files with each name.  Note that
  // filesystem may not be case sensitive.
  std::map<std::string, int> counts;

  for (auto const& si : mapping) {
    cmSourceFile const* sf = si.first;
    std::string objectNameLower = cmStrCat(
      cmSystemTools::LowerCase(
        cmSystemTools::GetFilenameWithoutLastExtension(sf->GetFullPath())),
      this->GlobalGenerator->GetLanguageOutputExtension(*sf));
    counts[objectNameLower] += 1;
  }

  // For all source files producing duplicate names we need unique
  // object name computation.
  for (auto& si : mapping) {
    cmSourceFile const* sf = si.first;
    std::string objectName = cmStrCat(
      cmSystemTools::GetFilenameWithoutLastExtension(sf->GetFullPath()),
      this->GlobalGenerator->GetLanguageOutputExtension(*sf));

    if (counts[cmSystemTools::LowerCase(objectName)] > 1) {
      const_cast<cmGeneratorTarget*>(gt)->AddExplicitObjectName(sf);
      bool keptSourceExtension;
      objectName = this->GetObjectFileNameWithoutTarget(*sf, dir_max,
                                                        &keptSourceExtension);
      cmsys::SystemTools::ReplaceString(objectName, "/", "_");
    }
    si.second = objectName;
  }
}
