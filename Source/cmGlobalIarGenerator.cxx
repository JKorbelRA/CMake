/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2015 Rockwell Automation Technologies, Inc.
  Copyright 2015 Jakub Korbel (jkorbel@ra.rockwell.com).

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "cmake.h"
#include <cm/memory>
#include <cm/string>
#include <cmext/algorithm>
#include <cmext/memory>

#include "cmGlobalIarGenerator.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmLocalIarGenerator.h"

#include "cmTimestamp.h"
#include "cmMakefile.h"
#include "cmGeneratedFileStream.h"
#include "cmTarget.h"
#include "cmGeneratorTarget.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmCustomCommand.h"
#include "cmLinkLineComputer.h"
#include "cmComputeLinkInformation.h"
#include "cmValue.h"

#include "cmsys/Glob.hxx"

#include <filesystem>
namespace fs = std::filesystem;
#ifdef __linux__
char const* cmGlobalIarGenerator::DEFAULT_BUILD_PROGRAM = "IarBuild";
#elif defined(_WIN32)
const char* cmGlobalIarGenerator::DEFAULT_BUILD_PROGRAM = "IarBuild.exe";
#endif
const char* cmGlobalIarGenerator::CHECK_BUILD_SYSTEM_TARGET = "RERUN_CMAKE";

/// @brief XML Declaration.
char const* cmGlobalIarGenerator::XML_DECL =
  "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";

char const* cmGlobalIarGenerator::XML_DECL_V9 =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

const char* cmGlobalIarGenerator::PROJ_FILE_EXT = ".ewp";
const char* cmGlobalIarGenerator::WS_FILE_EXT = ".eww";

#ifdef __linux__
char const* cmGlobalIarGenerator::DEFAULT_MAKE_PROGRAM = "IarBuild";
#elif defined(_WIN32)
const char* cmGlobalIarGenerator::DEFAULT_MAKE_PROGRAM = "IarBuild.exe";
#endif


static const char* LEVELS[11] = { "",
                                  "    ",
                                  "        ",
                                  "            ",
                                  "                ",
                                  "                    ",
                                  "                        ",
                                  "                            ",
                                  "                                ",
                                  "                                    ",
                                  "                                        " };

const char* cmGlobalIarGenerator::MULTIOPTS_COMPILER[13] = {
  "--dependencies",
                                     "--diagnostics_tables",
                                     "--dlib_config",
                                     "-f",
                                     "-l",
                                     "--output",
                                     "-o",
                                     "--predef_macros",
                                     "--preinclude",
                                     "--preprocess",
                                     "--public_equ",
                                     "--section",
                                     "--system_include_dir" };
const char* cmGlobalIarGenerator::MULTIOPTS_LINKER[24] = {
  "--call_graph",
                                "--config",
                                "--config_def",
                                "--config_search",
                                "--cpp_init_routine",
                                "--define_symbol",
                                "--dependencies",
                                "--diagnostics_tables",
                                "--entry",
                                "--export_builtin_config",
                                "--extra_init",
                                "-f",
                                "--image_input",
                                "--keep",
                                "--log",
                                "--log_file",
                                "--map",
                                "--output",
                                "-o",
                                "--place_holder",
                                "--redirect",
                                "--search",
                                "--stack_usage_control",
                                "--whole_archive" };

/// @brief Global configuration of the project (it should be visible
/// from everywhere).
cmGlobalIarGenerator::GlobalCmakeCfg cmGlobalIarGenerator::GLOBALCFG =
    cmGlobalIarGenerator::GlobalCmakeCfg();



std::string getLvl(unsigned int level)
{
  if (level < 11)
    {
    return LEVELS[level];
    }
  else
    {
    std::string result = LEVELS[10];
    for (unsigned int i = 0; i < level - 10; ++i)
      {
      result += LEVELS[1];
      }
    return result;
    }
}


//----------------------------------------------------------------------------
cmGlobalIarGenerator::cmGlobalIarGenerator(cmake* cm)
  : cmGlobalGenerator(cm)
{
  cm->GetState()->SetIarIDE(true);
}

cmGlobalIarGenerator::~cmGlobalIarGenerator()
{
}

void cmGlobalIarGenerator::AppendDirectoryForConfig(
  const std::string& prefix, const std::string& config,
  const std::string& suffix, std::string& dir)
{
  if (!config.empty()) {
    dir += prefix;
    dir += config;
    dir += suffix;
  }
}

std::unique_ptr<cmLocalGenerator> cmGlobalIarGenerator::CreateLocalGenerator(
  cmMakefile* mf)
{
  return std::unique_ptr<cmLocalGenerator>(
    cm::make_unique<cmLocalIarGenerator>(this, mf));
}


cmDocumentationEntry
  cmGlobalIarGenerator::GetDocumentation(void)
{
  cmDocumentationEntry entry{ GetActualName(), "Generates IAR Embedded Workbench files (experimental, work-in-progress)." };
  return entry;
}

enum XmlParseState
{
  EXP_LT = 0,
  EXP_GT = 1,
  EXP_NOPAIR_GT = 2,
  EXP_PAIR_LT = 3,
  EXP_PAIR_GT = 4,
  EXP_NOPAIR_QM = 5
};


void cmGlobalIarGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
      // Load the settings only once.
      GLOBALCFG.iarPath = mf->GetSafeDefinition("IAR_INSTALL_DIR");
      std::string wbVer = mf->GetSafeDefinition("IAR_WORKBENCH_VERSION");
      // Load the settings only once.
      GLOBALCFG.iarArmPath = mf->GetSafeDefinition("IAR_TOOLKIT_DIR");
      GLOBALCFG.buildType = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");

      // todo: remove eventually, this should be set from the platform file.
      mf->AddCacheDefinition("CMAKE_STATIC_LIBRARY_PREFIX", "",
                             "",
                             cmStateEnums::CacheEntryType::STRING);

      if (GLOBALCFG.iarPath.empty() && wbVer.empty()) {
        mf->AddCacheDefinition("IAR_INSTALL_DIR", "",
                               "IAR Workbench Installation Path",
                               cmStateEnums::CacheEntryType::PATH);
        mf->AddCacheDefinition("IAR_WORKBENCH_VERSION", "",
                               "IAR Workbench Version",
                               cmStateEnums::CacheEntryType::PATH);

          cmSystemTools::Message("IAR_INSTALL_DIR neither IAR_WORKBENCH_VERSION (obsolete) is set. Please, pre-set one of those.");
        this->cmGlobalGenerator::EnableLanguage(l, mf, optional);
          return;
      }


      this->cmGlobalGenerator::EnableLanguage(l, mf, optional);

      if (!wbVer.empty())
      {
          return;
      }

      // Get platform version dynamically:
      std::string platformVersionsName =
        GLOBALCFG.iarPath + "/common/config/PlatformVersions.xml";
      
      FILE* pPlatformVersions = fopen(platformVersionsName.c_str(), "r");

      std::string pvcontent = "";

      if (pPlatformVersions != NULL)
      {
        pvcontent.reserve(1024);
        char buffer[513];
        while (!feof(pPlatformVersions))
        {
          size_t readBytes = fread(buffer, 1, sizeof(buffer)-1, pPlatformVersions);
          buffer[readBytes] = '\0';
          pvcontent += buffer;
        }

   
        XmlParseState state = EXP_LT;

        // String index.
        size_t i = 0;
        // Var or val?
        bool var = false;

        std::string curVarName;
        curVarName.reserve(128);
        std::string curVal;
        curVal.reserve(128);

        bool error = false;
        std::string iarPlatformVersion = "";
        size_t len = pvcontent.length();

        while (!error && i < len)
        {
            // Load current char.
          char c = pvcontent[i];
            switch (state) {
            case EXP_LT:
                // Expedcting <.
              if (c == '<') {
                  // Found! Let's expect
                  curVarName = "";
                  curVal = "";
                  state = EXP_GT;
              } else if (isspace(c)) {
                  // Skip
              } else {
                  // Only whitespaces allowed outside tags.
                error = true;
              }

                break;
            case EXP_GT:
              // Expecting >. Loading all text into buffer (a var).
              if (c == '>') {
                  // Found, we should have a var name.
                state = EXP_PAIR_LT;
                error = curVarName.empty();
                var = true;
              } else if (c == '/') {
                // This is not a pair tag and it does not have a value.
                state = EXP_NOPAIR_GT;
              } else if (c == '?') {
                // First tag.
                state = EXP_NOPAIR_QM;
              } else {
                  // Load everything else into variable name string.
                curVarName += c;
              }
              break;
            case EXP_NOPAIR_QM:
              if (c == '?') {
                state = EXP_NOPAIR_GT;
              } else {
                // Load everything else into variable name string.
                curVarName += c;
              }
              break;
            case EXP_NOPAIR_GT:
              // Expecting > after / for no-pair tags.
              if (c == '>') {
                // Got it. Reset the SM to the initial state.
                state = EXP_LT;
              } else if (isspace(c)) {
                // Skip
              } else {
                error = true;
              }
              break;
            case EXP_PAIR_LT:

              // Expecting <. Loading all text into buffer (a val).
              if (c == '<' && (i + 1 < len) && pvcontent[i + 1] == '/') {
                if (!curVal.empty() && isspace(curVal.back())) {
                  curVal = curVal.substr(0, curVal.length() - 1);
                }
                  // A pair tag reached. We have a var and a val now.
                if (curVarName == "number") {
                    iarPlatformVersion += curVal;
                } else if (curVarName == "release") {
                  iarPlatformVersion += std::string(".") + curVal;
                  i = len - 1;
                }

                state = EXP_PAIR_GT;
              } else if (c == '<') {
                  // We have a new tag!
                  // todo: won't work if the tag has a space before /.
                curVarName = "";
                curVal = "";
                state = EXP_GT;
              } else {
                if (isspace(c)) {
                  if (!curVal.empty() && isspace(curVal.back())) {
                    curVal += c;
                  }
                } else {
                  curVal += c;
                }
              }
              break;
            case EXP_PAIR_GT:
              // Expecting >. Skipping everything.
              if (c == '>') {
                // A pair tag end reached. Reset the SM to the initial state.
                state = EXP_LT;
              }
              break;
            }

            i++;
        }

        if (!error) {
          GLOBALCFG.wbVersion = iarPlatformVersion;

          sscanf(iarPlatformVersion.c_str(), "%u.%u.%u",
                 &GLOBALCFG.wbVersionMajor, &GLOBALCFG.wbVersionMinor,
                 &GLOBALCFG.wbVersionPatch);

          
          // TODO COMMENT
          //cmSystemTools::Message(std::string("IAR Generator: Detected IAR Version ") +
          //                       std::to_string(GLOBALCFG.wbVersionMajor) +
          //                                 "." +
          //                                 std::to_string(GLOBALCFG.wbVersionMinor) +
          //              "." + std::to_string(GLOBALCFG.wbVersionPatch));
          // END TODO

          mf->AddCacheDefinition("IAR_WORKBENCH_VERSION", iarPlatformVersion.c_str(),
                                 "IAR Workbench Platform Version calculated from common/config/PlatformVersions.xml file",
                                 cmStateEnums::CacheEntryType::STRING);
          mf->AddCacheDefinition("IAR_WORKBENCH_VERSION_MAJOR",
                                 std::to_string(GLOBALCFG.wbVersionMajor).c_str(),
                                 "IAR Workbench Platform Version - Major",
                                 cmStateEnums::CacheEntryType::STRING);
          mf->AddCacheDefinition(
            "IAR_WORKBENCH_VERSION_MINOR",
                            std::to_string(GLOBALCFG.wbVersionMinor).c_str(),
                            "IAR Workbench Platform Version - Minor",
                            cmStateEnums::CacheEntryType::STRING);
          mf->AddCacheDefinition(
            "IAR_WORKBENCH_VERSION_PATCH",
                            std::to_string(GLOBALCFG.wbVersionPatch).c_str(),
                            "IAR Workbench Platform Version - Patch",
                            cmStateEnums::CacheEntryType::STRING);
        }

        fclose(pPlatformVersions);
      }
}

std::string cmGlobalIarGenerator::FindIarBuildCommand()
{
    std::string commonBin = GLOBALCFG.iarPath + "/common/bin";
    std::vector<std::string> userPaths;
    userPaths.push_back(commonBin);

    // TODO COMMENT
    // cmSystemTools::Message(std::string("USER PATH: ") + commonBin);
    // END TODO

    std::string makeProgram =
      cmSystemTools::FindProgram(DEFAULT_MAKE_PROGRAM, userPaths);
    if (makeProgram.empty()) {
      makeProgram = commonBin + "/" + DEFAULT_MAKE_PROGRAM;
    }

    return makeProgram;
}


bool cmGlobalIarGenerator::FindMakeProgram(cmMakefile* mf)
{
  // The GHS generator knows how to lookup its build tool
  // directly instead of needing a helper module to do it, so we
  // do not actually need to put CMAKE_MAKE_PROGRAM into the cache.
  if (cmIsOff(mf->GetDefinition("CMAKE_MAKE_PROGRAM"))) {
    mf->AddDefinition("CMAKE_MAKE_PROGRAM",
                      this->FindIarBuildCommand().c_str());
  }

  return true;
}

std::vector<cmGlobalGenerator::GeneratedMakeCommand>
cmGlobalIarGenerator::GenerateBuildCommand(
  std::string const& makeProgram,
                     std::string const& projectName,
                     std::string const& projectDir,
                     std::vector<std::string> const& targetNames,
                     std::string const& config, int jobs, bool verbose,
                     cmBuildOptions buildOptions,
                     std::vector<std::string> const& makeOptions,
                     BuildTryCompile /*isInTryCompile*/)
{
  cmGlobalGenerator::GeneratedMakeCommand makeCommand = {};

  makeCommand.Add(
    this->SelectMakeProgram(makeProgram, this->FindIarBuildCommand()));

  makeCommand.Add(makeOptions.begin(), makeOptions.end());

  if (!targetNames.empty()) {
    if (std::find(targetNames.begin(), targetNames.end(), "clean") !=
        targetNames.end()) {
      makeCommand.Add("-clean");
    } else {
      for (const auto& tname : targetNames) {
        if (!tname.empty()) {
          makeCommand.Add(tname + ".ewp");
        }
      }
    }
  }

  makeCommand.Add("-build");

  std::string buildType = GLOBALCFG.buildType;
  if (GLOBALCFG.buildType.empty()) {
    buildType = "empty";
  }

  makeCommand.Add(buildType);

  if (jobs > 1) {
    makeCommand.Add("-parallel");
    makeCommand.Add(std::to_string(jobs));
  }


  // TODO COMMENT
  // cmSystemTools::Message(makeCommand.Printable());
  // END TODO

  return { makeCommand };
}

bool cmGlobalIarGenerator::SetGeneratorPlatform(std::string const& p,
                                                cmMakefile* mf)
{
    /* Use the value from `-A` or use `arm` */
    std::string arch = "arm";
    if (!cmIsOff(p)) {
      arch = p;
    }

    /* update the primary target name*/
    mf->AddDefinition("CMAKE_SYSTEM_PROCESSOR", arch);

  return true;
}

void cmGlobalIarGenerator::ComputeTargetObjectDirectory(cmGeneratorTarget* gt) const
{
  // Compute full path to object file directory for this target.
  std::string dir =
    cmStrCat(gt->LocalGenerator->GetCurrentBinaryDirectory(), '/',
             gt->LocalGenerator->GetTargetDirectory(
               gt, cmStateEnums::IntermediateDirKind::ObjectFiles),
             '/', this->GetCMakeCFGIntDir(), '/');
/*
  // A nasty haxx. IAR builds binaries into CMAKE_BUILD_TYPE folder. And we need
  // to set this early.
  if (!gt->Target->GetProperty("OUTPUT_NAME")) {
    gt->Target->SetProperty("OUTPUT_NAME",
                            std::string(this->GetCMakeCFGIntDir()) + "/" +
                              gt->GetName());
  }*/

  gt->ObjectDirectory = dir;
}

bool cmGlobalIarGenerator::AddCheckTarget()
{
  // Skip the target if no regeneration is to be done.
  if (this->GlobalSettingIsOn("CMAKE_SUPPRESS_REGENERATION")) {
    return false;
  }

  // Get the generators.
  std::vector<std::unique_ptr<cmLocalGenerator>> const& generators =
    this->LocalGenerators;
  auto& lg =
    cm::static_reference_cast<cmLocalIarGenerator>(generators[0]);

  // The name of the output file for the custom command.
  this->StampFile = lg.GetBinaryDirectory() + std::string("/CMakeFiles/") +
    CHECK_BUILD_SYSTEM_TARGET;

  // Add a custom rule to re-run CMake if any input files changed.
  {
    // Collect the input files used to generate all targets in this
    // project.
    std::vector<std::string> listFiles;
    for (const auto& gen : generators) {
      cm::append(listFiles, gen->GetMakefile()->GetListFiles());
    }

    // Add the cache file.
    listFiles.push_back(cmStrCat(
      this->GetCMakeInstance()->GetHomeOutputDirectory(), "/CMakeCache.txt"));

    // Print not implemented warning.
    if (this->GetCMakeInstance()->DoWriteGlobVerifyTarget()) {
      std::ostringstream msg;
      msg << "Any pre-check scripts, such as those generated for file(GLOB "
             "CONFIGURE_DEPENDS), will not be run by gbuild.";
      this->GetCMakeInstance()->IssueMessage(MessageType::AUTHOR_WARNING,
                                             msg.str());
    }

    // Sort the list of input files and remove duplicates.
    std::sort(listFiles.begin(), listFiles.end(), std::less<std::string>());
    auto newEnd = std::unique(listFiles.begin(), listFiles.end());
    listFiles.erase(newEnd, listFiles.end());

    // Create a rule to re-run CMake and create output file.
    cmCustomCommandLines commandLines;
    commandLines.emplace_back(
      cmMakeCommandLine({ cmSystemTools::GetCMakeCommand(), "-E", "rm", "-f",
                          this->StampFile }));
    std::string argS = cmStrCat("-S", lg.GetSourceDirectory());
    std::string argB = cmStrCat("-B", lg.GetBinaryDirectory());
    commandLines.emplace_back(
      cmMakeCommandLine({ cmSystemTools::GetCMakeCommand(), argS, argB }));
    commandLines.emplace_back(cmMakeCommandLine(
      { cmSystemTools::GetCMakeCommand(), "-E", "touch", this->StampFile }));

    /* Create the target(Exclude from ALL_BUILD).
     *
     * The build tool, currently, does not support rereading the project files
     * if they get updated. So do not run this target as part of ALL_BUILD.
     */
    auto cc = cm::make_unique<cmCustomCommand>();
    cmTarget* tgt =
      lg.AddUtilityCommand(CHECK_BUILD_SYSTEM_TARGET, true, std::move(cc));
    auto ptr = cm::make_unique<cmGeneratorTarget>(tgt, &lg);
    auto* gt = ptr.get();
    lg.AddGeneratorTarget(std::move(ptr));

    // Add the rule.
    cc = cm::make_unique<cmCustomCommand>();
    cc->SetOutputs(this->StampFile);
    cc->SetDepends(listFiles);
    cc->SetCommandLines(commandLines);
    cc->SetComment("Checking Build System");
    //cc->SetCMP0116Status(cmPolicies::NEW);
    cc->SetEscapeOldStyle(false);
    cc->SetStdPipesUTF8(true);

    if (cmSourceFile* file =
          lg.AddCustomCommandToOutput(std::move(cc), true)) {
      gt->AddSource(file->ResolveFullPath());
    } else {
      cmSystemTools::Error("Error adding rule for " + this->StampFile);
    }
    // Organize in the "predefined targets" folder:
    if (this->UseFolderProperty()) {
      tgt->SetProperty("FOLDER", this->GetPredefinedTargetsFolder());
    }
  }

  return true;
}

void cmGlobalIarGenerator::AddExtraIDETargets()
{

  /* Add Custom Target to check if CMake needs to be rerun.
   *
   * The build tool, currently, does not support rereading the project files
   * if they get updated.  So do not make the other targets dependent on this
   * check.
   */
  this->AddCheckTarget();
}


//----------------------------------------------------------------------------
void cmGlobalIarGenerator::Generate()
{
  const cmLocalGenerator* const lgs0 =
      this->GetLocalGenerators()[0].get();
  const cmMakefile* globalMakefile = lgs0->GetMakefile();
  
  std::string iarPath = globalMakefile->GetSafeDefinition("IAR_INSTALL_DIR");
  if (!iarPath.empty()) {
    GLOBALCFG.iarPath = iarPath;
  }

  GLOBALCFG.wbVersion = globalMakefile->GetSafeDefinition("IAR_WORKBENCH_VERSION");
  std::string version =
    globalMakefile->GetSafeDefinition("IAR_WORKBENCH_VERSION_MAJOR");
  if (!version.empty()) {
    GLOBALCFG.wbVersionMajor = std::stoi(version);
  }
  version = globalMakefile->GetSafeDefinition("IAR_WORKBENCH_VERSION_MINOR");
  if (!version.empty()) {
    GLOBALCFG.wbVersionMinor = std::stoi(version);
  }
  version = globalMakefile->GetSafeDefinition("IAR_WORKBENCH_VERSION_PATCH");
  if (!version.empty()) {
    GLOBALCFG.wbVersionPatch = std::stoi(version);
  }

  GLOBALCFG.buildType = globalMakefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
  std::string flagsWithType = std::string("CMAKE_C_FLAGS_") + cmSystemTools::UpperCase(GLOBALCFG.buildType);

  GLOBALCFG.iarCCompilerFlags = globalMakefile->GetSafeDefinition("CMAKE_C_FLAGS");
  GLOBALCFG.iarCCompilerFlags += std::string(" ") + globalMakefile->GetSafeDefinition(flagsWithType);

  flagsWithType = std::string("CMAKE_ASM_IAR_FLAGS_" +
                              cmSystemTools::UpperCase(GLOBALCFG.buildType));
  GLOBALCFG.iarAsmFlags =
    globalMakefile->GetSafeDefinition("CMAKE_ASM_IAR_FLAGS");
  GLOBALCFG.iarAsmFlags +=
    std::string(" ") + globalMakefile->GetSafeDefinition(flagsWithType);

  flagsWithType = std::string("CMAKE_CXX_FLAGS_"+cmSystemTools::UpperCase(GLOBALCFG.buildType));
  GLOBALCFG.iarCxxCompilerFlags = globalMakefile->GetSafeDefinition("CMAKE_CXX_FLAGS");
  GLOBALCFG.iarCxxCompilerFlags += std::string(" ") + globalMakefile->GetSafeDefinition(flagsWithType);

  flagsWithType = std::string("CMAKE_EXE_LINKER_FLAGS_") + cmSystemTools::UpperCase(GLOBALCFG.buildType);
  GLOBALCFG.iarLinkerFlags = globalMakefile->GetSafeDefinition("CMAKE_EXE_LINKER_FLAGS");
  GLOBALCFG.iarLinkerFlags += std::string(" ") + globalMakefile->GetSafeDefinition(flagsWithType);

  GLOBALCFG.compilerDlibConfig =
      globalMakefile->GetSafeDefinition("IAR_COMPILER_DLIB_CONFIG");

  for (int i = 0; i < 4; i++)
  {
      if (std::string(RUNTIME_LIBRARY_CONFIG[i]) == GLOBALCFG.compilerDlibConfig)
      {
          GLOBALCFG.compilerDlibConfigId = i;
      }
  }

  GLOBALCFG.bufferedTermOut = globalMakefile->GetSafeDefinition("IAR_GENERAL_BUFFERED_TERMINAL_OUTPUT");
  GLOBALCFG.scanfFmt = globalMakefile->GetSafeDefinition("IAR_GENERAL_SCANF_FORMATTER");
  GLOBALCFG.printfFmt = globalMakefile->GetSafeDefinition("IAR_GENERAL_PRINTF_FORMATTER");
  GLOBALCFG.semihostingEnabled = globalMakefile->GetSafeDefinition("IAR_SEMIHOSTING_ENABLE");
  GLOBALCFG.jobs = globalMakefile->GetSafeDefinition("IAR_BUILD_JOBS");

  for (int i = 0; i < SCANF_FORMATTING_CNT; i++)
  {
      if (std::string(SCANF_PRINTF_FORMATTING[i]) == GLOBALCFG.scanfFmt)
      {
          GLOBALCFG.scanfFmtId = i;
      }
  }

  for (int i = 0; i < PRINTF_FORMATTING_CNT; i++)
  {
      if (std::string(SCANF_PRINTF_FORMATTING[i]) == GLOBALCFG.printfFmt)
      {
          GLOBALCFG.printfFmtId = i;
      }
  }
  GLOBALCFG.compilerPathExe =
    globalMakefile->GetSafeDefinition("IAR_COMPILER_PATH_EXE");
  if (GLOBALCFG.compilerPathExe.empty()) {
    GLOBALCFG.compilerPathExe =
      globalMakefile->GetSafeDefinition("CMAKE_C_COMPILER");
  }
  GLOBALCFG.cpuName = globalMakefile->GetSafeDefinition("IAR_CPU_NAME");
  GLOBALCFG.systemName =
      globalMakefile->GetSafeDefinition("CMAKE_SYSTEM_NAME");
  GLOBALCFG.dbgExtraOptions =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_CSPY_EXTRAOPTIONS");
  GLOBALCFG.dbgCspyFlashLoaderv3 =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_CSPY_FLASHLOADER_V3");
  GLOBALCFG.dbgCspyMacfile =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_CSPY_MACFILE");
  GLOBALCFG.dbgCspyMemfile =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_CSPY_MEMFILE");
  GLOBALCFG.dbgIjetProbeconfig =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_IJET_PROBECONFIG");
  GLOBALCFG.dbgProbeSelection =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_PROBE");
  GLOBALCFG.dbgLogFile =
      globalMakefile->GetSafeDefinition("IAR_DEBUGGER_LOGFILE");
  GLOBALCFG.linkerEntryRoutine =
      globalMakefile->GetSafeDefinition("IAR_LINKER_ENTRY_ROUTINE");
  GLOBALCFG.linkerIcfFile =
    globalMakefile->GetSafeDefinition("IAR_LINKER_ICF_FILE");
  GLOBALCFG.tgtArch =
    globalMakefile->GetSafeDefinition("IAR_TARGET_ARCHITECTURE");
  GLOBALCFG.iarPath = globalMakefile->GetSafeDefinition("IAR_INSTALL_DIR");
  if (GLOBALCFG.iarPath.empty()) {
    cmSystemTools::Message(
      "IAR_INSTALL_DIR not set, using obsolete IAR_EW_ROOT");
    GLOBALCFG.iarPath = globalMakefile->GetSafeDefinition("IAR_EW_ROOT");
  }
  GLOBALCFG.iarArmPath = globalMakefile->GetSafeDefinition("IAR_TOOLKIT_DIR");
  if (GLOBALCFG.iarArmPath.empty()) {
    cmSystemTools::Message(
      "IAR_TOOLKIT_DIR not set, using obsolete IAR_ARM_PATH");
    GLOBALCFG.iarArmPath =
      globalMakefile->GetSafeDefinition("IAR_ARM_PATH");
  }

  if (GLOBALCFG.iarPath.empty()) {
    cmSystemTools::Message(
      "IAR_INSTALL_DIR not set, using obsolete ${IAR_ARM_PATH}/..");
    GLOBALCFG.iarPath = GLOBALCFG.iarArmPath + "/..";
  }

  GLOBALCFG.rtos = globalMakefile->GetSafeDefinition("IAR_TARGET_RTOS");

  // Pre-include support is not included in regular cmake (--include header.h)
  // todo after / if it is supported, remove this code for the sake of
  // standard variables.
  std::string preInclude =
      globalMakefile->GetSafeDefinition("IAR_COMPILER_PREINCLUDE");
  GLOBALCFG.compilerPreInclude = preInclude;

  // todo We are supporting only arm (different linker/compiler options must
  // be used for different IAR version).
  GLOBALCFG.wbVersion =
      globalMakefile->GetSafeDefinition("IAR_WORKBENCH_VERSION");

  GLOBALCFG.tgtArch = "ARM";

  // todo Add list of IAR variables, which the user is able to set.
  GLOBALCFG.chipSelection =
      globalMakefile->GetSafeDefinition("IAR_CHIP_SELECTION");
  if (GLOBALCFG.chipSelection.empty())
    {
    GLOBALCFG.chipSelection = "None";
    }

  // IAR needs a workspace name. This would be the root CMake project.
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
      it = this->GetProjectMap().begin();
      it!= this->GetProjectMap().end();
      ++it)
    {
    const cmMakefile* makeFile = it->second[0]->GetMakefile();

    // create a project file
    if (strcmp(makeFile->GetCurrentBinaryDirectory().c_str(),
        makeFile->GetHomeOutputDirectory().c_str()) == 0)
      {
      this->workspace.workspaceDir = makeFile->GetCurrentBinaryDirectory();
      this->workspace.name = lgs0->GetProjectName();
      }
    }

  this->cmGlobalGenerator::Generate();

  // Finally, create IAR workspace file containing a list of all IAR projects.
  this->workspace.CreateWorkspaceFile();
}

//----------------------------------------------------------------------------
std::string cmGlobalIarGenerator::ToToolkitPath(std::string absolutePath)
{
  if(!GLOBALCFG.iarArmPath.empty())
    {
    if (absolutePath.find(GLOBALCFG.iarArmPath) != std::string::npos)
      {
      std::string outStr = "$TOOLKIT_DIR$/";
      outStr += absolutePath.substr(GLOBALCFG.iarArmPath.length()+1);
      std::replace( outStr.begin(), outStr.end(), '/', '\\');
      return outStr;
      }
    }

  return absolutePath;
}

//----------------------------------------------------------------------------
std::string cmGlobalIarGenerator::ToWorkbenchPath(std::string absolutePath)
{
  if(!GLOBALCFG.iarArmPath.empty())
    {
    std::string ewPath = GLOBALCFG.iarArmPath.substr(0,
        GLOBALCFG.iarArmPath.length()-(sizeof("/arm")-1));
    if (absolutePath.find(ewPath) != std::string::npos)
      {
      std::string outStr = "$EW_DIR$/";
      outStr += absolutePath.substr(ewPath.length()+1);
      std::replace( outStr.begin(), outStr.end(), '/', '\\');
      return outStr;
      }
    }

  return absolutePath;
}

namespace IarArg
{
  const unsigned int KEY_LEN = 256;
  const unsigned int VAL_LEN = 256;

  enum ArgState
    {
      ARG_STATE_EXP_ARG,
      ARG_STATE_EXP_ARG_NAME,
      ARG_STATE_EXP_ARG_VALUE
    };

  const char* ParseValue(const char* pChar, char* pVal)
  {
    const char* pBegin = pChar;
    // Value.
    bool breakMe = false;
    bool parseError = false;
    char escape = 0;
    while (!breakMe)
      {
      switch(pChar[0])
      {
      case '"':
        if (pChar == pBegin && escape == '\0')
          {
          escape = '"';
          pBegin++;
          }
        else
          {
          parseError = breakMe = !(escape == '"' && (pChar[1] == ' ' || pChar[1] == '\0'));
          }

        break;

      case '\'':
        if (escape == '\0')
          {
          escape = '\'';
          }
        else
          {
          parseError = breakMe = !(escape == '\'' && (pChar[1] == ' ' || pChar[1] == '\0'));
          }

        break;

      case ' ':
        if (escape == '\0')
          {
          // Break if escape not in effect.
          breakMe = true;
          }
        break;
      case '\0':
        breakMe = true;
        break;
      }

      if (breakMe)
        {
        break;
        }

      // Move the char pointer.
      pChar++;
      }

    size_t valBytes = (pChar - pBegin - (escape != '\0'));
    if (!parseError && (valBytes + 1 < 256)) // 1 per \0.
      {
      memcpy((void*)pVal, (void*)pBegin, valBytes);
      pVal[valBytes] = '\0';
      }

    return (!parseError) ? pChar : NULL;
  }

  const char* ParseKey(const char* pChar, char* pKey)
  {
    const char* pBegin = pChar;
    // Key.
    bool breakMe = false;
    bool parseError = false;
    char escape = 0;
    while (!breakMe)
      {
      switch(pChar[0])
      {
      case '"':
        if (pChar == pBegin && escape == '\0')
          {
          escape = '"';
          pBegin++;
          }
        else
          {
          parseError = breakMe = !(escape == '"' && (pChar[1] == ' ' || pChar[1] == '\0'));
          }

        break;

      case '\'':
        if (escape == '\0')
          {
          escape = '\'';
          }
        else
          {
          parseError = breakMe = !(escape == '\'' && (pChar[1] == ' ' || pChar[1] == '\0'));
          }

        break;

      case '\\':
        parseError = breakMe = true;
        break;

      case ' ':
        if (escape == '\0')
          {
          // Break if escape not in effect.
          breakMe = true;
          }
        break;
      case '\0':
        breakMe = true;
        break;
      default:
        // Check the character:
        parseError = breakMe =  !((pChar[0] >= 'A' && pChar[0] <= 'Z') ||
            (pChar[0] >= 'a' && pChar[0] <= 'z') ||
            (pChar[0] >= '0' && pChar[0] <= '9') ||
            pChar[0] == '+' || pChar[0] == '-');
        break;
      }

      if (breakMe)
        {
        break;
        }

      // Move the char pointer.
      pChar++;
      }

    size_t valBytes = (pChar - pBegin - (escape != '\0'));
    if (!parseError && (valBytes + 1 < 256)) // 1 per \0.
      {
        memcpy((void*)pKey, (void*)pBegin, valBytes);
        pKey[valBytes] = '\0';
      }

    return (!parseError) ? pChar : NULL;
  }

  const char* ParseNext(const char* cmdChar, char* pKey, char* pValue)
  {
    bool breakMe = false;
    bool parseError = false;
    bool isGnuStyle = false; // gnu style: --attr{= }val | posix style: -attr val
    ArgState state = ARG_STATE_EXP_ARG;

    while (!breakMe)
      {
      switch(cmdChar[0])
      {
      case '\0':
        parseError = breakMe = true;
        break;
      case '-':
        if (state == ARG_STATE_EXP_ARG)
          {
          // Look ahead.
          if (cmdChar[1] == '-')
            {
            isGnuStyle = true;
            cmdChar++;
            }
          state = ARG_STATE_EXP_ARG_NAME;
          }
        else if (state == ARG_STATE_EXP_ARG_VALUE)
          {
          // If expecting value, but param happens, we have found a flag
          // without value. End now.
          breakMe = true;
          *pValue = '\0';
          }
        else
          {
          // Disallow ---.
          parseError = breakMe = true;
          }
        break;
      case ' ':
        if (state == ARG_STATE_EXP_ARG_NAME)
          {
          // This is an error.
          }
        break;
      case '=':
        break;
      default:
        // Any char.
        if (state == ARG_STATE_EXP_ARG_NAME)
          {
          // Parse name:
          cmdChar = ParseKey(cmdChar, pKey);
          if (cmdChar != NULL)
            {
            state = ARG_STATE_EXP_ARG_VALUE;
            }
          else
            {
            parseError = breakMe = true;
            }

          }
        else if (state == ARG_STATE_EXP_ARG_VALUE)
          {
          breakMe = true;
          cmdChar = ParseValue(cmdChar, pValue);
          parseError = (cmdChar == NULL);
          }
        break;
      }

      if (breakMe)
        {
        break;
        }

      cmdChar++;
      }

    return (!parseError) ? cmdChar : NULL;
  }
};

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::ParseCmdLineOpts(
  std::string cmdLine, const char* multiOpts[], size_t multiOptsLen,
                                            std::vector<std::string>& opts)
{
#if 0
  const char* pChar = cmdLine.c_str();
  while(pChar != NULL || *pChar != '\0')
    {
    char key[IarArg::KEY_LEN];
    char value[IarArg::VAL_LEN];
    // We need to accept whole string.
    pChar = IarArg::ParseNext(pChar, key, value);
    if (pChar != NULL)
      {
      // Find the key in a map.
      }
    }
#endif

  bool isMulti = false;
  std::string multicmd = "";
  std::vector<std::string> cmds = cmSystemTools::SplitString(cmdLine, ' ');
  for (std::vector<std::string>::const_iterator it = cmds.begin();
       it != cmds.end(); ++it) {

    if (isMulti) {
      multicmd += " " + *it;
      opts.push_back(multicmd);
      isMulti = false;
      continue;
    }

    for (int i = 0; i < multiOptsLen; i++) {
      if ((*it).find(multiOpts[i]) == 0) {
        // Multi option.
        isMulti = true;
        multicmd = *it;
        break;
      }
    }

    if (isMulti) {
      continue;
    }

    if ((*it) == std::string(""))
    {
      continue;
    }

    opts.push_back(*it);
  }
}

void cmGlobalIarGenerator::GetCmdLines(std::vector<cmCustomCommand> const& rTmpCmdVec,
                                      std::string& rBuildCmd,
                                      int& rStart)
{
    for(std::vector<cmCustomCommand>::const_iterator it = rTmpCmdVec.begin();
            it != rTmpCmdVec.end(); ++it)
    {
        rStart += 1;
        rBuildCmd += "REM Begin Command "+int2str(rStart)+"\n";
        rBuildCmd += std::string("REM Description: ")+(*it).GetComment()+"\n";
        rBuildCmd += "\n";
        rBuildCmd += "REM Change working directory:\n";
        std::string cwd = std::string((*it).GetWorkingDirectory());
        //std::replace( cwd.begin(), cwd.end(), '/', '\\');
        rBuildCmd += "cd " + cwd + "\n";
        rBuildCmd += "REM Executing command lines:\n";

        const cmCustomCommandLines& cmdLines = (*it).GetCommandLines();

        for(cmCustomCommandLines::const_iterator it2 = cmdLines.begin();
                it2 != cmdLines.end(); ++it2)
        {
            std::string line = "";
            bool firstTok = true;
            for(cmCustomCommandLine::const_iterator it3 = (*it2).begin();
                    it3 != (*it2).end(); ++it3)
            {
                //if (firstTok)
                //{
                //    // Most likely a path.
                //    std::string token = (*it3);
                //    std::replace( token.begin(), token.end(), '/', '\\');
                //    line += token + " ";
                //    firstTok = false;
                //    continue;
                //}

                line += (*it3) + " ";
            }

            line += "\n";
            rBuildCmd += line;
        }

        rBuildCmd += "REM End Command "+int2str(rStart)+"\n\n";
    }
}

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::ConvertTargetToProject(const cmTarget& tgt,
    cmGeneratorTarget* genTgt)
{
    std::string buildType = "empty";
    if (!GLOBALCFG.buildType.empty())
    {
      buildType = GLOBALCFG.buildType;
    }

  // For IAR, each code related target is considered a separate IAR project.
  cmGlobalIarGenerator::Project* project = new cmGlobalIarGenerator::Project();
  project->name = genTgt->GetName();

  // Is this a lib or a linkable type?
  cmStateEnums::TargetType type = genTgt->GetType();
  project->isLib =
      (type == cmStateEnums::STATIC_LIBRARY ||
          type == cmStateEnums::SHARED_LIBRARY ||
          type == cmStateEnums::MODULE_LIBRARY ||
          type == cmStateEnums::OBJECT_LIBRARY ||
          type == cmStateEnums::INTERFACE_LIBRARY ||
          type == cmStateEnums::UNKNOWN_LIBRARY );

  cmMakefile* makeFile = tgt.GetMakefile();

  project->projectDir = makeFile->GetCurrentSourceDirectory();
  project->binaryDir = makeFile->GetCurrentBinaryDirectory();

  // INCLUDE DIRECTORIES: Gather all includes.
  const std::vector <BT<std::string>> includeDirsVector =
      genTgt->GetIncludeDirectories("", "C");
  for (std::vector<BT<std::string>>::const_iterator it =
      includeDirsVector.begin();
      it != includeDirsVector.end(); ++it)
    {
    project->includes.push_back((*it).Value);
    }

  // SOURCE FILES: Gather all sources.
  std::vector<cmSourceFile*> sourceFilesVector;
  genTgt->GetSourceFiles(sourceFilesVector, "");

  for(std::vector<cmSourceFile*>::const_iterator it =
      sourceFilesVector.begin();
      it != sourceFilesVector.end();
      ++it)
    {
    project->sources.push_back((*it)->GetFullPath());
    }

  // Compose build configuration

  const std::vector<std::unique_ptr<cmTarget>>& owned =
      makeFile->GetOwnedImportedTargets();

  cmGlobalIarGenerator::BuildConfig buildCfg;

  buildCfg.name = buildType;
  buildCfg.isDebug = (buildType == "Debug");
  buildCfg.exeDir = buildCfg.name;
  if (buildType == "empty") {
    buildCfg.exeDir = "";
  }

  buildCfg.objectDir = buildCfg.exeDir;
  buildCfg.listDir = buildCfg.exeDir;
  buildCfg.browseInfoDir = buildCfg.exeDir;

  if (!buildCfg.exeDir.empty()) {
    buildCfg.objectDir += "/";
    buildCfg.listDir += "/";
    buildCfg.browseInfoDir += "/";
  }
  buildCfg.objectDir += "Object";
  buildCfg.listDir += "List";
  buildCfg.browseInfoDir += "BrowseInfo";
  buildCfg.toolchain = GLOBALCFG.tgtArch;
  buildCfg.outputFile = genTgt->GetName();

  buildCfg.preBuildCmd = "";
  buildCfg.postBuildCmd = "";

  std::string prebuild = cmStrCat(project->binaryDir, "/", project->name, "_",
                                  buildCfg.name, "_prebuild.bat");
  std::string postbuild = cmStrCat(project->binaryDir, "/", project->name, "_",
                                   buildCfg.name, "_postbuild.bat");

  buildCfg.icfPath = GLOBALCFG.linkerIcfFile;

  // Prebuild & postbuild.
  std::string buildCmd = "";
  buildCmd.reserve(2048);

  std::string cmdHdr = "";
  cmdHdr.reserve(1024);

  cmdHdr += "REM ==========================================================\n";
  cmdHdr += "REM This file has been generated from CMake cmGlobalIarGenerator.\n";
  cmdHdr += "REM DO NOT EDIT.\n";
  cmdHdr += "REM ==========================================================\n\n";

  int cmdIx = 0;
  // Pre-link and pre-build are prebuild, IAR does not have anything like pre-link...
  GetCmdLines(genTgt->GetPreBuildCommands(),
              buildCmd,
              cmdIx);

  GetCmdLines(genTgt->GetPreLinkCommands(),
              buildCmd,
              cmdIx);

  if (cmdIx > 0)
  {
      buildCfg.preBuildCmd = prebuild;
      FILE* pBuild = fopen(prebuild.c_str(), "w");
      if (pBuild != NULL)
      {
          fwrite(cmdHdr.c_str(), cmdHdr.length(), 1, pBuild);
          fwrite(buildCmd.c_str(), buildCmd.length(), 1, pBuild);
          fclose(pBuild);
      }
      else {
          cmSystemTools::Error(std::string("Cannot open ") + prebuild + " for writing!");
      }
  }

  // Post-build is postbuild...
  cmdIx = 0;
  buildCmd = "";
  GetCmdLines(genTgt->GetPostBuildCommands(),
              buildCmd,
              cmdIx);
  if (cmdIx > 0)
  {
      buildCfg.postBuildCmd = postbuild;
      FILE* pBuild = fopen(postbuild.c_str(), "w");
      if (pBuild != NULL)
      {
          fwrite(cmdHdr.c_str(), cmdHdr.length(), 1, pBuild);
          fwrite(buildCmd.c_str(), buildCmd.length(), 1, pBuild);
          fclose(pBuild);
      }
  }


  // Compile definitions:
  std::vector<std::string> compileDefs;
  genTgt->GetCompileDefinitions(compileDefs, buildCfg.name, "C");
  for(std::vector<std::string>::const_iterator it = compileDefs.begin();
      it != compileDefs.end(); ++it)
    {
    buildCfg.compileDefs.push_back(*it);
    }

  // Compiler options:
  std::vector<std::string> compilerOpts;
  genTgt->GetCompileOptions(compilerOpts, buildCfg.name, "C");
  for (std::vector<std::string>::const_iterator it = compilerOpts.begin();
       it != compilerOpts.end(); ++it) {
    buildCfg.compilerOpts.push_back(*it);
  }

  // Linker options:

   std::string linkLibs;
   std::string frameworkPath;
   std::string linkPath;
   std::string flags;
   std::string linkFlags;
   cmLocalGenerator* lg = genTgt->GetLocalGenerator();
   cmLinkLineComputer linkLineComputer(lg,
                                       lg->GetStateSnapshot().GetDirectory());
   lg->GetTargetFlags(&linkLineComputer, buildType, linkLibs, flags,
                      linkFlags, frameworkPath, linkPath,
                      (cmGeneratorTarget*)genTgt);

   std::string optimization = "";
   cmGlobalIarGenerator::ParseCmdLineOpts(
     linkFlags, cmGlobalIarGenerator::MULTIOPTS_LINKER,
     sizeof(cmGlobalIarGenerator::MULTIOPTS_LINKER) / sizeof(const char*),
     buildCfg.linkerOpts);

   cmComputeLinkInformation* pcli = genTgt->GetLinkInformation(buildCfg.name);

    if (pcli) {

     for (std::vector<std::string>::const_iterator it =
            pcli->GetDepends().begin();
          it != pcli->GetDepends().end(); ++it) {

         
        if (std::find(buildCfg.libraries.begin(), buildCfg.libraries.end(),  *it) == buildCfg.libraries.end()) {
         buildCfg.libraries.push_back(*it);
       }
     }
   }

  // Add configurations to the list.
  project->buildCfg = buildCfg;

  project->CreateProjectFile();

  // Register the project into a workspace.
  this->workspace.RegisterProject(project->name, project);
}

void cmGlobalIarGenerator::Project::CreateProjectFile()
{
  if (GLOBALCFG.wbVersionMajor <= 8) {
    
    if (GLOBALCFG.wbVersionMajor == 8 && (GLOBALCFG.wbVersionMinor < 4))
    {
        // Supported.
    }
    else {
      cmSystemTools::Message("Warning: IAR Workbench version " +
                             std::to_string(GLOBALCFG.wbVersionMajor) +
                             std::to_string(GLOBALCFG.wbVersionMinor) +
                             " is not explicitly supported.");
    }
    CreateProjectFile8();
  } else //(GLOBALCFG.wbVersionMajor >= 9)
  {
    if (GLOBALCFG.wbVersionMajor == 9 && GLOBALCFG.wbVersionMinor == 0)
    {
      CreateProjectFile8();
    }
    else if (GLOBALCFG.wbVersionMajor == 9 && GLOBALCFG.wbVersionMinor == 3)
    {
      CreateProjectFile9();
    }
    else {
      cmSystemTools::Message("Warning: IAR Workbench version " +
                             std::to_string(GLOBALCFG.wbVersionMajor) +
                             std::to_string(GLOBALCFG.wbVersionMinor) +
                             " is not explicitly supported.");
      CreateProjectFile9();
    }
  }
}

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::RegisterProject(const std::string& projectName)
{
  (void) projectName;
}

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::Project::CreateDebuggerFile()
{
  if (GLOBALCFG.wbVersionMajor <= 8) {
    CreateDebuggerFile8();
  } else //(GLOBALCFG.wbVersionMajor >= 9)
  {
    CreateDebuggerFile9();
  }

}

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::Workspace::CreateWorkspaceFile()
{
  const static std::string errorCheck =
      "if %ERRORLEVEL% NEQ 0 exit(1)\n";
  std::string wsFileName = this->workspaceDir + "/" + this->name + ".eww";
  this->workspacePath = wsFileName;
  std::string batFileName = this->workspaceDir + "/BUILD_" + this->name + ".bat";

  std::string settingsDirName = this->workspaceDir + "/settings";
  fs::create_directories(settingsDirName);
  std::string wsdtFilePath = this->workspaceDir + "/settings/" + this->name + ".wsdt";

  std::string iarBuildCmd = cmGlobalIarGenerator::GLOBALCFG.iarArmPath;
  std::size_t lastSlash = iarBuildCmd.find_last_of("/\\");
  if (lastSlash != std::string::npos)
  {
    iarBuildCmd = iarBuildCmd.substr(0, lastSlash) +
      "/common/bin/" + cmGlobalIarGenerator::DEFAULT_BUILD_PROGRAM;
  }
  else
  {
    iarBuildCmd = cmGlobalIarGenerator::DEFAULT_BUILD_PROGRAM;
  }

  std::replace( iarBuildCmd.begin(), iarBuildCmd.end(), '/', '\\');

  /*printf("Build cmd: %s.\n", iarBuildCmd.c_str());*/

  FILE* pFile = fopen(wsFileName.c_str(), "w");
  FILE* pBatFile = fopen(batFileName.c_str(), "w");
  FILE* pWsdtFile = fopen(wsdtFilePath.c_str(), "w");

  std::string output;
  output.reserve(1 << 20); // 1K.
  if (GLOBALCFG.wbVersionMajor >= 9) {
    output += XML_DECL_V9;
  } else {
    output += XML_DECL;
  }

  std::string wsdtOutput;
  wsdtOutput.reserve(1 << 20); // 1K.
  if (GLOBALCFG.wbVersionMajor >= 9) {
    wsdtOutput += XML_DECL_V9;
  } else {
    wsdtOutput += XML_DECL;
  }

  std::string batchOutput = "";
  batchOutput.reserve(1 << 20); // 1K.
  batchOutput += "REM ===================================================\n";
  batchOutput += "REM IAR BUILD (generated from CMake extraIarGenerator).\n";
  batchOutput += "REM ===================================================\n\n";
  batchOutput += "SET RETURN_VALUE=0\n\n";

  XmlNode root = XmlNode("workspace");

  XmlNode rootWsdt = XmlNode("workspace");
  XmlNode* wsdt = new XmlNode("CurrentConfigs");

  std::string buildType = "empty";
  if (!cmGlobalIarGenerator::GLOBALCFG.buildType.empty())
  {
    buildType = cmGlobalIarGenerator::GLOBALCFG.buildType;
  }

  XmlNode* batch = new XmlNode("batchDefinition");
  batch->NewChild("name", buildType + "_BuildAll");

  std::vector<Project*> vProjects;

  // Go through all registered projects.
  for (std::map<std::string, Project*>::const_iterator it =
          this->projects.begin();
          it != this->projects.end();
          ++it)
  {
      // Libraries first.

      if (it->second->isLib)
      {
          // Register project file to various structures.
          XmlNode* projEntry = root.NewChild("project");

          std::string projPath = it->second->binaryDir;
          projPath += std::string("/") + it->second->name + ".ewp";

          projEntry->NewChild("path", projPath);

          XmlNode* member = batch->NewChild("member");
          member->NewChild("project", it->first);
          member->NewChild("configuration", buildType);

          wsdt->NewChild("Project", it->first + "/" + buildType);

          // Add batch command.
          std::string projPathWin = projPath;
          std::replace( projPathWin.begin(), projPathWin.end(), '/', '\\');
          batchOutput += "\"" + iarBuildCmd + "\" \""
                  + projPathWin + "\" -build " + buildType + " -log all";
          if (!cmGlobalIarGenerator::GLOBALCFG.jobs.empty())
          {
              batchOutput += " -parallel " + cmGlobalIarGenerator::GLOBALCFG.jobs;
          }

          batchOutput += "\n";
          batchOutput += errorCheck;
      }
      else
      {
          vProjects.push_back(it->second);
      }
  }

  for (std::vector<Project*>::const_iterator it = vProjects.begin();
          it != vProjects.end();
       ++it)
    {
      // Executables next.

      // Register project file to various structures.
      XmlNode* projEntry = root.NewChild("project");

      std::string projPath = (*it)->binaryDir;
      projPath += std::string("/") + (*it)->name + ".ewp";

      projEntry->NewChild("path", projPath);

      XmlNode* member = batch->NewChild("member");
      member->NewChild("project", (*it)->name);
      member->NewChild("configuration", buildType);

      wsdt->NewChild("Project", (*it)->name + "/" + buildType);

      // Add batch command.
      std::string projPathWin = projPath;
      std::replace( projPathWin.begin(), projPathWin.end(), '/', '\\');
      batchOutput += "\"" + iarBuildCmd + "\" \""
              + projPathWin + "\" -build " + buildType + " -log all";
      if (!cmGlobalIarGenerator::GLOBALCFG.jobs.empty())
      {
          batchOutput += " -parallel " + cmGlobalIarGenerator::GLOBALCFG.jobs;
      }

      batchOutput += "\n";
      batchOutput += errorCheck;
    }

  batchOutput += "\nexit %RETURN_VALUE%\n\n";

  batchOutput += "\n\nREM ===================================================\n";
  batchOutput += "REM END IAR BUILD.\n";
  batchOutput += "REM ===================================================\n\n";

  XmlNode* batchBuild = root.NewChild("batchBuild");
  batchBuild->AddChild(batch);

  XmlNode* wsdtCfg = rootWsdt.NewChild("ConfigDictionary");
  wsdtCfg->AddChild(wsdt);
  wsdtCfg->NewChild("CurrentProj", this->name);
  wsdtCfg->NewChild("OverviewSelected", "1");

  root.ToString(0, output);
  rootWsdt.ToString(0, wsdtOutput);

  fwrite(output.c_str(), output.length(), 1, pFile);
  fwrite(batchOutput.c_str(), batchOutput.length(), 1, pBatFile);
  fwrite(wsdtOutput.c_str(), wsdtOutput.length(), 1, pWsdtFile);

  fclose(pFile);
  fclose(pBatFile);
  fclose(pWsdtFile);

  //this->CreateDebuggerFile();
}

//----------------------------------------------------------------------------
void cmGlobalIarGenerator::Workspace::RegisterProject(std::string wsName,
    Project* project)
{
  this->projects.insert(std::make_pair(wsName, project));
}


#ifdef _WIN32
#include <future>

#include <windows.h>

#include <objbase.h>
#include <shellapi.h>

static bool OpenWorkspace(std::string workspace, std::string iarIde)
{
    
    HRESULT comInitialized =
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(comInitialized)) {
        return false;
    }

    HINSTANCE hi = ShellExecuteA(NULL, "open", iarIde.c_str(), workspace.c_str(),
                                 NULL,
                                 SW_SHOWNORMAL);

    CoUninitialize();

    return reinterpret_cast<intptr_t>(hi) > 32;
}


bool cmGlobalIarGenerator::Open(const std::string& bindir,
    const std::string& projectName,
    bool dryRun)
{
  std::string projFile = bindir + "/" + projectName + ".eww";
  std::string iarIde = GLOBALCFG.iarPath + "/common/bin/IarIdePm.exe";
    // TODO: COMMENT
    // cmSystemTools::Message(std::string("Trying to OPEN: ") + projFile);
    // END TODO

    if (dryRun) {
      bool ok = true;
      ok = ok && cmSystemTools::FileExists(projFile, true);
      ok = ok && cmSystemTools::FileExists(iarIde, true);
      return ok;
    }

    return std::async(std::launch::async, OpenWorkspace, projFile, iarIde).get();
}
#endif // _WIN32
