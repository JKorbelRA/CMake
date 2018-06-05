/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalIarGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalIarGenerator.h"
#include "cmMakefile.h"

cmLocalIarGenerator::cmLocalIarGenerator(cmGlobalGenerator* gg,
                                         cmMakefile* mf)
  : cmLocalGenerator(gg, mf)
{
}

cmLocalIarGenerator::~cmLocalIarGenerator()
{
}

void cmLocalIarGenerator::Generate()
{
  std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();

  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); ++l) {
    switch((*l)->GetType())
    {
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY:
        {
            cmGlobalIarGenerator* globIarGen = static_cast<cmGlobalIarGenerator*>(this->GlobalGenerator);
            globIarGen->ConvertTargetToProject(*((*l)->Target),*l);
            break;
        }
        default:
            break;
    }
  }
}
