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

#include "cmGlobalIarGenerator.h"
#include "cmGlobalGenerator.h"

#include "cmsys/Glob.hxx"

#include <filesystem>
namespace fs = std::filesystem;

  //----------------------------------------------------------------------------
void cmGlobalIarGenerator::Project::CreateProjectFile9()
{
  std::string fileName = this->binaryDir;
  fileName += std::string("/") + this->name + ".ewp";

  FILE* pFile = fopen(fileName.c_str(), "w");

  XmlNode root("project", "");
  root.NewChild("fileVersion", "4");

  XmlNode* config = root.NewChild("configuration");
  if (this->buildCfg.name.empty()) {
    config->NewChild("name", this->buildCfg.isDebug ? "Debug" : "Release");
  }
  else {
    config->NewChild("name", this->buildCfg.name);
  }

  XmlNode* toolchain = config->NewChild("toolchain");
  toolchain->NewChild("name", this->buildCfg.toolchain);

  config->NewChild("debug", this->buildCfg.isDebug ? "1" : "0");


  // GENERAL SETTINGS:
  IarSettings* generalSettings = new IarSettings("General", 3);
  config->AddChild(generalSettings);

  IarData* generalData =
    generalSettings->NewData(36, true, this->buildCfg.isDebug);
  generalData->NewOption("BrowseInfoPath")->NewState(this->buildCfg.browseInfoDir);
  generalData->NewOption("ExePath")->NewState(this->buildCfg.exeDir);
  generalData->NewOption("ObjPath")->NewState(this->buildCfg.objectDir + "/" + this->name);
  generalData->NewOption("ListPath")->NewState(this->buildCfg.listDir + "/" + this->name);
  generalData->NewOption("GEndianMode")->NewState("0");


  std::string pPrintfIdStr = int2str(GLOBALCFG.printfFmtId);
  std::string pScanfIdStr = int2str(GLOBALCFG.scanfFmtId);

  generalData->NewOption("Input description")
              ->NewState("No specifier n, no float nor "
                  "long long, no scan set,"
                  " no assignment suppressing, without multibyte support.");
  generalData->NewOption("Output description")
                ->NewState("No specifier a, A, without multibyte support.");
  generalData->NewOption("GOutputBinary")
                ->NewState(this->isLib ? "1" : "0");
  generalData->NewOption("OGCoreOrChip")->NewState("1");


  std::string pDlibIdStr = int2str(GLOBALCFG.compilerDlibConfigId);

  generalData->NewOption("GRuntimeLibSelect", 0)->NewState(std::string(pDlibIdStr));
  generalData->NewOption("GRuntimeLibSelectSlave", 0)->NewState(std::string(pDlibIdStr));
  generalData->NewOption("RTDescription")
              ->NewState("Use the normal configuration of the C/C++ runtime"
                  " library. No locale interface, C locale, no file descriptor"
                  " support, no multibytes in printf and scanf, and no hex floats"
                  " in strtod.");
  generalData->NewOption("OGProductVersion")
                ->NewState(cmGlobalIarGenerator::GLOBALCFG.wbVersion);
  generalData->NewOption("OGLastSavedByProductVersion")
                ->NewState(cmGlobalIarGenerator::GLOBALCFG.wbVersion);
  std::string chipSelection = cmGlobalIarGenerator::GLOBALCFG.chipSelection;
  chipSelection += "\t" + cmGlobalIarGenerator::GLOBALCFG.chipSelection;

  generalData->NewOption("OGChipSelectEditMenu")->NewState(chipSelection);


  const char* pGenLowLevelIfaceStr = GLOBALCFG.semihostingEnabled == "ON" ? "1" : "0";

  generalData->NewOption("GenLowLevelInterface")
                ->NewState(pGenLowLevelIfaceStr);
  generalData->NewOption("GEndianModeBE")->NewState("1");

  const char* pBufferedStr = GLOBALCFG.bufferedTermOut == "ON" ? "1" : "0";

  generalData->NewOption("OGBufferedTerminalOutput")->NewState(pBufferedStr);
  generalData->NewOption("GenStdoutInterface")->NewState("0");
  generalData->NewOption("RTConfigPath2")
              ->NewState(std::string("$TOOLKIT_DIR$\\inc\\c\\DLib_Config_") + cmGlobalIarGenerator::GLOBALCFG.compilerDlibConfig + ".h");
  generalData->NewOption("GBECoreSlave", 33)->NewState("43");
  generalData->NewOption("OGUseCmsis")->NewState("0");
  generalData->NewOption("OGUseCmsisDspLib")->NewState("0");
  generalData->NewOption("GRuntimeLibThreads")->NewState("0");
  generalData->NewOption("CoreVariant", 33)->NewState("42");

  generalData->NewOption("GFPUDeviceSlave")->NewState(chipSelection);
  generalData->NewOption("FPU2", 0)->NewState("3");
  generalData->NewOption("NrRegs", 0)->NewState("1");
  generalData->NewOption("NEON")->NewState("0");
  generalData->NewOption("GFPUCoreSlave2", 33)->NewState("42");
  generalData->NewOption("OGCMSISPackSelectDevice");
  generalData->NewOption("OgLibHeap")->NewState("0");
  generalData->NewOption("OGLibAdditionalLocale")->NewState("0");
  generalData->NewOption("OGPrintfVariant", 0)->NewState("0");
  generalData->NewOption("OGPrintfMultibyteSupport")->NewState("0");
  generalData->NewOption("OGScanfVariant", 0)->NewState("0");
  generalData->NewOption("OGScanfMultibyteSupport")->NewState("0");
  generalData->NewOption("GenLocaleTags")->NewState("");
  generalData->NewOption("GenLocaleDisplayOnly")->NewState("");
  generalData->NewOption("DSPExtension")->NewState("0");
  generalData->NewOption("TrustZone")->NewState("0");
  //v9
  generalData->NewOption("TrustZoneModes", 0)->NewState("0");
  generalData->NewOption("OGAarch64Abi")->NewState("0");
  generalData->NewOption("OG_32_64Device")->NewState("0");
  generalData->NewOption("BuildFilesPath")->NewState(this->buildCfg.name); // Debug?
  generalData->NewOption("PointerAuthentication")->NewState("0");
  generalData->NewOption("FPU64")->NewState("1");
  generalData->NewOption("OG_32_64DeviceCoreSlave", 33)->NewState("42");

  // ARM Compiler (ICCARM):
  IarSettings* iccArmSettings = new IarSettings("ICCARM", 2);
  config->AddChild(iccArmSettings);
  std::string optimization = "";
  cmGlobalIarGenerator::ParseCmdLineOpts(
    GLOBALCFG.iarCCompilerFlags, MULTIOPTS_COMPILER,
    sizeof(MULTIOPTS_COMPILER) / sizeof(const char*),
    this->buildCfg.compilerOpts);

  for (std::vector<std::string>::const_iterator it =
         this->buildCfg.compilerOpts.begin();
       it != this->buildCfg.compilerOpts.end();) {

    if (it->find("-O") == 0) {
      optimization = *it;
      it = this->buildCfg.compilerOpts.erase(it);
    } else {
      ++it;
    }
  }

  std::string subexpElimination = "0";
  std::string loopUnrolling = "0";
  std::string functionInlining = "0";
  std::string codeMotion = "0";
  std::string typeBasedAlias = "0";
  std::string staticClustering = "0";
  std::string instructionScheduling = "0";
  std::string vectorization = "0";

  std::string optLvl = "0";
  std::string optStrategy = "0";
  if (!this->buildCfg.isDebug) {
    optLvl = "3";
    optStrategy = "1";

    subexpElimination = "1";
    loopUnrolling = "1";
    functionInlining = "1";
    codeMotion = "1";
    typeBasedAlias = "1";
    staticClustering = "1";
    instructionScheduling = "1";
  }

  if (optimization == "-On") {
    optLvl = "0";
    optStrategy = "0";

  } else if (optimization == "-Ol") {
    optLvl = "1";
    optStrategy = "0";

  } else if (optimization == "-Om") {
    optLvl = "2";
    optStrategy = "0";

    subexpElimination = "1";
    codeMotion = "1";
    staticClustering = "1";
  } else if (optimization == "-Oh") {
    optLvl = "3";
    optStrategy = "0";

    subexpElimination = "1";
    loopUnrolling = "1";
    functionInlining = "1";
    codeMotion = "1";
    typeBasedAlias = "1";
    staticClustering = "1";
    instructionScheduling = "1";
  } else if (optimization == "-Ohs") {
    optLvl = "3";
    optStrategy = "2";

    subexpElimination = "1";
    loopUnrolling = "1";
    functionInlining = "1";
    codeMotion = "1";
    typeBasedAlias = "1";
    staticClustering = "1";
    instructionScheduling = "1";
  } else if (optimization == "-Ohz") {
    optLvl = "3";
    optStrategy = "1";

    subexpElimination = "1";
    loopUnrolling = "1";
    functionInlining = "1";
    codeMotion = "1";
    typeBasedAlias = "1";
    staticClustering = "1";
    instructionScheduling = "1";
  } else {
    ; // Keep defaults.
  }

  std::string ccAllowList = subexpElimination + loopUnrolling + functionInlining + codeMotion + typeBasedAlias + staticClustering +
    instructionScheduling + vectorization;

  IarData* iccArmData = iccArmSettings->NewData(38, true, this->buildCfg.isDebug);

  iccArmData->NewOption("CCOptimizationNoSizeConstraints")->NewState("0");
  iccArmData->NewOption("CCDefines")->NewStates(this->buildCfg.compileDefs);
  iccArmData->NewOption("CCPreprocFile")->NewState("0");
  iccArmData->NewOption("CCPreprocComments")->NewState("0");
  iccArmData->NewOption("CCPreprocLine")->NewState("0");
  iccArmData->NewOption("CCListCFile")->NewState(this->buildCfg.isDebug ? "1" : "0");
  iccArmData->NewOption("CCListCMnemonics")->NewState("0");
  iccArmData->NewOption("CCListCMessages")->NewState("0");
  iccArmData->NewOption("CCListAssFile")->NewState("0");
  iccArmData->NewOption("CCListAssSource")->NewState("0");
  iccArmData->NewOption("CCEnableRemarks")->NewState("0");
  iccArmData->NewOption("CCDiagSuppress")->NewState("");
  iccArmData->NewOption("CCDiagRemark")->NewState("");
  iccArmData->NewOption("CCDiagWarning")->NewState("");
  iccArmData->NewOption("CCDiagError")->NewState("");
  iccArmData->NewOption("CCObjPrefix")->NewState("1");
  iccArmData->NewOption("CCAllowList", 1)->NewState(ccAllowList);
  iccArmData->NewOption("CCDebugInfo")->NewState(this->buildCfg.isDebug ? "1" : "0");
  iccArmData->NewOption("IEndianMode")->NewState("1");
  iccArmData->NewOption("IProcessor")->NewState("1");
  iccArmData->NewOption("IExtraOptionsCheck")->NewState("1");

  iccArmData->NewOption("IExtraOptions")->NewStates(this->buildCfg.compilerOpts);
  iccArmData->NewOption("CCLangConformance")->NewState("0");
  iccArmData->NewOption("CCSignedPlainChar")->NewState("1");
  iccArmData->NewOption("CCRequirePrototypes")->NewState("0");
  iccArmData->NewOption("CCDiagWarnAreErr")->NewState("0");
  iccArmData->NewOption("CCCompilerRuntimeInfo")->NewState("0");
  iccArmData->NewOption("IFpuProcessor")->NewState("1");
  iccArmData->NewOption("OutputFile")->NewState(this->binaryDir + "/" + "$FILE_BNAME$.o");
  iccArmData->NewOption("CCLibConfigHeader")->NewState("1");
  iccArmData->NewOption("PreInclude")
                ->NewState(cmGlobalIarGenerator::GLOBALCFG.compilerPreInclude);
  iccArmData->NewOption("CCIncludePath2")->NewStates(this->includes);
  iccArmData->NewOption("CCStdIncCheck")->NewState("0");
  iccArmData->NewOption("CCCodeSection")->NewState(".text");
  iccArmData->NewOption("IProcessorMode2")->NewState("1");


  iccArmData->NewOption("CCOptLevel")->NewState(optLvl);
  iccArmData->NewOption("CCOptStrategy", 0)->NewState(optStrategy);
  iccArmData->NewOption("CCOptLevelSlave")->NewState(optLvl);

  iccArmData->NewOption("CompilerMisraRules98", 0)
                ->NewState("100011111011010110111001110011111110111001101100010111"
                    "011110110110011111111111110011001111100111011100111111"
                    "1111111111111111111");
  iccArmData->NewOption("CompilerMisraRules04", 0)
                ->NewState("111101110010111111111000110111111111111111111111111110"
                    "010111101111010101111111111111111111111111101111111011"
                    "111001111011111011111111111111111");
  iccArmData->NewOption("CCPosIndRopi")->NewState("0");
  iccArmData->NewOption("CCPosIndRwpi")->NewState("0");
  iccArmData->NewOption("CCPosIndNoDynInit")->NewState("0");
  iccArmData->NewOption("IccLang")->NewState("2");
  iccArmData->NewOption("IccCDialect")->NewState("1");
  iccArmData->NewOption("IccAllowVLA")->NewState("0");
  iccArmData->NewOption("IccStaticDestr")->NewState("0");
  iccArmData->NewOption("IccCppInlineSemantics")->NewState("1");
  iccArmData->NewOption("IccCmsis")->NewState("1");
  iccArmData->NewOption("IccFloatSemantics")->NewState("0");
  iccArmData->NewOption("CCNoLiteralPool")->NewState("0");
  iccArmData->NewOption("CCOptStrategySlave")->NewState("0");
  iccArmData->NewOption("CCGuardCalls")->NewState("1");
  iccArmData->NewOption("CCEncSource")->NewState("0");
  iccArmData->NewOption("CCEncOutput")->NewState("0");
  iccArmData->NewOption("CCEncOutputBom")->NewState("1");
  iccArmData->NewOption("CCEncInput")->NewState("0");
  iccArmData->NewOption("IccExceptions2")->NewState("0");
  iccArmData->NewOption("IccRTTI2")->NewState("0");

  // AARM:
  IarSettings* aArmSettings = new IarSettings("AARM", 2);
  config->AddChild(aArmSettings);
  IarData* aArmData = aArmSettings->NewData(10, true, this->buildCfg.isDebug);

  aArmData->NewOption("AObjPrefix")->NewState("1");
  aArmData->NewOption("AEndian")->NewState("1");
  aArmData->NewOption("ACaseSensitivity")->NewState("1");
  aArmData->NewOption("MacroChars", 0)->NewState("0");
  aArmData->NewOption("AWarnEnable")->NewState("0");
  aArmData->NewOption("AWarnWhat")->NewState("0");
  aArmData->NewOption("AWarnOne")->NewState("");
  aArmData->NewOption("AWarnRange1")->NewState("");
  aArmData->NewOption("AWarnRange2")->NewState("");
  aArmData->NewOption("ADebug")->NewState(this->buildCfg.isDebug ? "1" : "0");
  aArmData->NewOption("AltRegisterNames")->NewState("0");
  aArmData->NewOption("ADefines")->NewState("");
  aArmData->NewOption("AList")->NewState("0");
  aArmData->NewOption("AListHeader")->NewState("1");
  aArmData->NewOption("AListing")->NewState("1");
  aArmData->NewOption("Includes")->NewState("0");
  aArmData->NewOption("MacDefs")->NewState("0");
  aArmData->NewOption("MacExps")->NewState("1");
  aArmData->NewOption("MacExec")->NewState("0");
  aArmData->NewOption("OnlyAssed")->NewState("0");
  aArmData->NewOption("MultiLine")->NewState("0");
  aArmData->NewOption("PageLengthCheck")->NewState("0");
  aArmData->NewOption("PageLength")->NewState("80");
  aArmData->NewOption("TabSpacing")->NewState("8");
  aArmData->NewOption("AXRef")->NewState("0");
  aArmData->NewOption("AXRefDefines")->NewState("0");
  aArmData->NewOption("AXRefInternal")->NewState("0");
  aArmData->NewOption("AXRefDual")->NewState("0");
  aArmData->NewOption("AProcessor")->NewState("1");
  aArmData->NewOption("AFpuProcessor")->NewState("1");
  aArmData->NewOption("AOutputFile")
    ->NewState(this->binaryDir + "/" + "$FILE_BNAME$.o");
  aArmData->NewOption("ALimitErrorsCheck")->NewState("0");
  aArmData->NewOption("ALimitErrorsEdit")->NewState("100");
  aArmData->NewOption("AIgnoreStdInclude")->NewState("0");
  aArmData->NewOption("AUserIncludes")->NewState("");
  aArmData->NewOption("AExtraOptionsCheckV2")->NewState("0");
  aArmData->NewOption("AExtraOptionsV2")->NewState("");
  aArmData->NewOption("AsmNoLiteralPool")->NewState("0");


  // OBJCOPY:
  IarSettings* objCopySettings = new IarSettings("OBJCOPY", 0);
  config->AddChild(objCopySettings);
  IarData* objCopyData = objCopySettings->NewData(1, true, this->buildCfg.isDebug);

  objCopyData->NewOption("OOCOutputFormat", 3)->NewState("3");
  objCopyData->NewOption("OCOutputOverride")->NewState("0");

  std::string outFile = this->buildCfg.outputFile;
  outFile += ".bin";
  objCopyData->NewOption("OOCOutputFile")->NewState(outFile);
  objCopyData->NewOption("OOCCommandLineProducer")->NewState("1");
  objCopyData->NewOption("OOCObjCopyEnable")->NewState("0");


  // CUSTOM:
  IarSettings* customSettings = new IarSettings("CUSTOM", 3);
  XmlNode* customData = customSettings->NewChild("data");
  customData->NewChild("extensions");
  customData->NewChild("cmdline");
  customData->NewChild("hasPrio", "0");
  config->AddChild(customSettings);


  // BICOMP:
  IarSettings* bicompSettings = new IarSettings("BICOMP", 0);
  bicompSettings->NewChild("data");
  config->AddChild(bicompSettings);


  // BUILDACTION:
  IarSettings* bactionSettings = new IarSettings("BUILDACTION", 1);
  XmlNode* bactionData = bactionSettings->NewChild("data");
  bactionData->NewChild("prebuild", this->buildCfg.preBuildCmd);
  bactionData->NewChild("postbuild", this->buildCfg.postBuildCmd);
  config->AddChild(bactionSettings);


  // IAR Linker (ILINK):
  IarSettings* ilinkSettings = new IarSettings("ILINK", 0);
  config->AddChild(ilinkSettings);
  IarData* ilinkData = ilinkSettings->NewData(20, true, this->buildCfg.isDebug);

  outFile = this->buildCfg.outputFile + ".elf";
  ilinkData->NewOption("IlinkOutputFile")->NewState(outFile);
  ilinkData->NewOption("IlinkLibIOConfig")->NewState("1");
  ilinkData->NewOption("XLinkMisraHandler")->NewState("0");
  ilinkData->NewOption("IlinkInputFileSlave")->NewState("0");
  ilinkData->NewOption("IlinkDebugInfoEnable")->NewState("1");
  ilinkData->NewOption("IlinkKeepSymbols")->NewState("");
  ilinkData->NewOption("IlinkRawBinaryFile")->NewState("");
  ilinkData->NewOption("IlinkRawBinarySymbol")->NewState("");
  ilinkData->NewOption("IlinkRawBinarySegment")->NewState("");
  ilinkData->NewOption("IlinkRawBinaryAlign")->NewState("");
  ilinkData->NewOption("IlinkDefines")->NewState("");
  ilinkData->NewOption("IlinkConfigDefines")->NewState("");
  ilinkData->NewOption("IlinkMapFile")->NewState("1");
  ilinkData->NewOption("IlinkLogFile")->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkLogInitialization")
                ->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkLogModule")->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkLogSection")->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkLogVeneer")->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkIcfOverride")->NewState("1");
  ilinkData->NewOption("IlinkIcfFile")->NewState(this->buildCfg.icfPath);
  ilinkData->NewOption("IlinkIcfFileSlave")->NewState("");
  ilinkData->NewOption("IlinkEnableRemarks")->NewState("0");
  ilinkData->NewOption("IlinkSuppressDiags")->NewState("");
  ilinkData->NewOption("IlinkTreatAsRem")->NewState("");
  ilinkData->NewOption("IlinkTreatAsWarn")->NewState("");
  ilinkData->NewOption("IlinkTreatAsErr")->NewState("");
  ilinkData->NewOption("IlinkWarningsAreErrors")->NewState("0");
  ilinkData->NewOption("IlinkUseExtraOptions")->NewState("1");
  ilinkData->NewOption("IlinkExtraOptions")->NewStates(this->buildCfg.linkerOpts);
  ilinkData->NewOption("IlinkLowLevelInterfaceSlave")->NewState("1");
  ilinkData->NewOption("IlinkAutoLibEnable")->NewState("1");
  ilinkData->NewOption("IlinkAdditionalLibs")->NewStates(this->buildCfg.libraries);
  ilinkData->NewOption("IlinkOverrideProgramEntryLabel")->NewState("0");
  ilinkData->NewOption("IlinkProgramEntryLabelSelect")->NewState("0");
  ilinkData->NewOption("IlinkProgramEntryLabel")
                ->NewState(cmGlobalIarGenerator::GLOBALCFG.linkerEntryRoutine);
  ilinkData->NewOption("DoFill")->NewState("0");
  ilinkData->NewOption("FillerByte")->NewState("0xFF");
  ilinkData->NewOption("FillerStart")->NewState("0x0");
  ilinkData->NewOption("FillerEnd")->NewState("0x0");
  ilinkData->NewOption("CrcSize", 0)->NewState("1");
  ilinkData->NewOption("CrcAlign")->NewState("1");
  ilinkData->NewOption("CrcPoly")->NewState("0x11021");
  ilinkData->NewOption("CrcCompl",0)->NewState("0");
  ilinkData->NewOption("CrcBitOrder",0)->NewState("0");
  ilinkData->NewOption("CrcInitialValue")->NewState("0x0");
  ilinkData->NewOption("DoCrc")->NewState("0");
  ilinkData->NewOption("IlinkBE8Slave")->NewState("1");
  ilinkData->NewOption("IlinkBufferedTerminalOutput")->NewState("1");
  ilinkData->NewOption("IlinkStdoutInterfaceSlave")->NewState("1");
  ilinkData->NewOption("CrcFullSize")->NewState("0");
  ilinkData->NewOption("IlinkIElfToolPostProcess")->NewState("0");
  ilinkData->NewOption("IlinkLogAutoLibSelect")
                ->NewState(this->buildCfg.isDebug ? "1" : "0");
  ilinkData->NewOption("IlinkLogRedirSymbols")->NewState("0");
  ilinkData->NewOption("IlinkLogUnusedFragments")->NewState("0");
  ilinkData->NewOption("IlinkCrcReverseByteOrder")->NewState("0");
  ilinkData->NewOption("IlinkCrcUseAsInput")->NewState("1");
  ilinkData->NewOption("IlinkOptInline")->NewState("1");
  ilinkData->NewOption("IlinkOptExceptionsAllow")->NewState("0");
  ilinkData->NewOption("IlinkOptExceptionsForce")->NewState("0");
  ilinkData->NewOption("IlinkCmsis")->NewState("1");
  ilinkData->NewOption("IlinkOptMergeDuplSections")->NewState("1");
  ilinkData->NewOption("IlinkOptUseVfe")->NewState("1");
  ilinkData->NewOption("IlinkOptForceVfe")->NewState("0");
  ilinkData->NewOption("IlinkStackAnalysisEnable")->NewState("0");
  ilinkData->NewOption("IlinkStackControlFile")->NewState("");
  ilinkData->NewOption("IlinkStackCallGraphFile")->NewState("");
  ilinkData->NewOption("CrcAlgorithm", 1)->NewState("1");
  ilinkData->NewOption("CrcUnitSize", 0)->NewState("0");
  ilinkData->NewOption("IlinkThreadsSlave")->NewState("1");
  ilinkData->NewOption("IlinkLogCallGraph")->NewState("0");
  ilinkData->NewOption("IlinkIcfFile_AltDefault")->NewState("");
  ilinkData->NewOption("IlinkEncInput")->NewState("0");
  ilinkData->NewOption("IlinkEncOutput")->NewState("0");
  ilinkData->NewOption("IlinkEncOutputBom")->NewState("1");
  ilinkData->NewOption("IlinkHeapSelect")->NewState("1");
  ilinkData->NewOption("IlinkLocaleSelect")->NewState("1");

  // IARCHIVE:
  IarSettings* iArchiveSettings = new IarSettings("IARCHIVE", 0);
  config->AddChild(iArchiveSettings);
  IarData* iArchiveData = iArchiveSettings->NewData(0, true, this->buildCfg.isDebug);

  // 00
  iArchiveData->NewOption("IarchiveInputs")->NewState("");
  iArchiveData->NewOption("IarchiveOverride")->NewState("0");
  iArchiveData->NewOption("IarchiveOutput")->NewState("###Unitialized###");


  // BILINK:
  IarSettings* bilinkSettings = new IarSettings("BILINK", 0);
  bilinkSettings->NewChild("data");
  config->AddChild(bilinkSettings);


  // This file is outside our source directory.
  XmlNode* groupExternal = new XmlNode("group");
  groupExternal->NewChild("name", "external");


  FileTreeNode ftRoot = FileTreeNode(this->name);

  // ADD FILES:
  for (std::vector<std::string>::const_iterator it = this->sources.begin();
      it != this->sources.end();
      ++it)
    {
    if (it->substr(0, this->projectDir.length()) == this->projectDir)
      {
      // This file is inside our directory.
      FileTreeNode::AddToTree(&ftRoot,
          it->substr(this->projectDir.length()+1),
          *it);
      }
    else
      {
      // This file is outside our source directory.
      XmlNode* extFile = groupExternal->NewChild("file");
      extFile->NewChild("name", *it);
      }

    }

  ftRoot.TransformToIarTree(&root);
  root.AddChild(groupExternal);


  std::string output;
  output.reserve(1 << 20); // 16K.
  if (GLOBALCFG.wbVersionMajor >= 9) {
    output += XML_DECL_V9;
  } else
  {
    output += XML_DECL;
  }
  root.ToString(0, output);

  fwrite(output.c_str(), output.length(), 1, pFile);

  fclose(pFile);

  if (!this->isLib)
  {
      this->CreateDebuggerFile();
  }
}


//----------------------------------------------------------------------------
void cmGlobalIarGenerator::Project::CreateDebuggerFile9()
{
  std::string debuggerFileName = this->binaryDir;
  debuggerFileName += std::string("/") + this->name + ".ewd";

  XmlNode root = XmlNode("project");
  root.NewChild("fileVersion", "3");

  for (unsigned int i = 0; i < 2; i++)
    {
    bool isDebug = (i == 0);

    XmlNode* config = root.NewChild("configuration");
    config->NewChild("name", isDebug ? "Debug" : "Release");

    XmlNode* toolchain = config->NewChild("toolchain");
    toolchain->NewChild("name", "ARM");

    config->NewChild("debug", isDebug ? "1" : "0");


    // GENERAL SETTINGS:
    IarSettings* cspySettings = new IarSettings("C-SPY", 2);
    config->AddChild(cspySettings);

    IarData* cspyData = cspySettings->NewData(25, true, isDebug);

    cspyData->NewOption("CInput")->NewState("1");
    cspyData->NewOption("CEndian")->NewState("1");
    cspyData->NewOption("CProcessor")->NewState("1");
    cspyData->NewOption("OCVariant")->NewState("0");
    cspyData->NewOption("MacOverride")->NewState("1");
    cspyData->NewOption("MacFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgCspyMacfile);
    cspyData->NewOption("MemOverride")->NewState("1");
    cspyData->NewOption("MemFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgCspyMemfile);
    cspyData->NewOption("RunToEnable")->NewState(isDebug ? "1" : "0");
    cspyData->NewOption("RunToName")->NewState("main");

    cspyData->NewOption("CExtraOptionsCheck")->NewState(isDebug ? "1" : "0");
    cspyData->NewOption("CExtraOptions")
            ->NewState(isDebug ? "--jet_use_hw_breakpoint_for_semihosting" : "");
    cspyData->NewOption("CFpuProcessor")->NewState("1");
    cspyData->NewOption("OCDDFArgumentProducer")->NewState("");
    cspyData->NewOption("OCDownloadSuppressDownload")->NewState("0");
    cspyData->NewOption("OCDownloadVerifyAll")->NewState("1");
    cspyData->NewOption("OCProductVersion")->NewState(GLOBALCFG.wbVersion);

    if (GLOBALCFG.dbgProbeSelection == "J-Link")
    {
        cspyData->NewOption("OCDynDriverList")->NewState("JLINK_ID");
    }
    else if (GLOBALCFG.dbgProbeSelection == "I-Jet")
    {
        cspyData->NewOption("OCDynDriverList")->NewState("IJET_ID");
    }
    else if (GLOBALCFG.dbgProbeSelection == "Simulator")
    {
        cspyData->NewOption("OCDynDriverList")->NewState("ARMSIM_ID");
    }	
    else
    {
        // I-Jet is the default probe.
        cspyData->NewOption("OCDynDriverList")->NewState("IJET_ID");
    }

    cspyData->NewOption("OCLastSavedByProductVersion")
            ->NewState(GLOBALCFG.wbVersion);
    cspyData->NewOption("OCDownloadAttachToProgram")->NewState("0");

    cspyData->NewOption("UseFlashLoader")->NewState("0");
    cspyData->NewOption("CLowLevel")->NewState("1");
    cspyData->NewOption("OCBE8Slave")->NewState("1");
    cspyData->NewOption("MacFile2")->NewState("");
    cspyData->NewOption("CDevice")->NewState("1");
    cspyData->NewOption("FlashLoadersV3")
            ->NewState(isDebug ? "" :
                cmGlobalIarGenerator::GLOBALCFG.dbgCspyFlashLoaderv3);
    cspyData->NewOption("OCImagesSuppressCheck1")->NewState("0");
    cspyData->NewOption("OCImagesPath1")->NewState("");
    cspyData->NewOption("OCImagesSuppressCheck2")->NewState("0");
    cspyData->NewOption("OCImagesPath2")->NewState("");

    cspyData->NewOption("OCImagesSuppressCheck3")->NewState("0");
    cspyData->NewOption("OCImagesPath3")->NewState("");
    cspyData->NewOption("OverrideDefFlashBoard")
            ->NewState(isDebug ? "0" : "1");
    cspyData->NewOption("OCImagesOffset1")->NewState("");
    cspyData->NewOption("OCImagesOffset2")->NewState("");
    cspyData->NewOption("OCImagesOffset3")->NewState("");
    cspyData->NewOption("OCImagesUse1")->NewState("0");
    cspyData->NewOption("OCImagesUse2")->NewState("0");
    cspyData->NewOption("OCImagesUse3")->NewState("0");
    cspyData->NewOption("OCDeviceConfigMacroFile")->NewState("1");

    cspyData->NewOption("OCDebuggerExtraOption")->NewState("1");
    cspyData->NewOption("OCAllMTBOptions")->NewState("1");

    // ARMSIM_ID
    IarSettings* armsimId = new IarSettings("ARMSIM_ID", 2);
    config->AddChild(armsimId);

    IarData* armsimData = armsimId->NewData(1, true, isDebug);

    armsimData->NewOption("OCSimDriverInfo")->NewState("1");
    armsimData->NewOption("OCSimEnablePSP")->NewState("0");
    armsimData->NewOption("OCSimPspOverrideConfig")->NewState("0");
    armsimData->NewOption("OCSimPspConfigFile")->NewState("");

    // ANGEL_ID

    IarSettings* angelId = new IarSettings("ANGEL_ID", 2);
    config->AddChild(angelId);

    IarData* angelData = angelId->NewData(0, true, isDebug);

    angelData->NewOption("CCAngelHeartbeat")->NewState("1");
    angelData->NewOption("CAngelCommunication")->NewState("1");
    angelData->NewOption("CAngelCommBaud",0)->NewState("3");
    angelData->NewOption("CAngelCommPort",0)->NewState("0");
    angelData->NewOption("ANGELTCPIP")->NewState("aaa.bbb.ccc.ddd");
    angelData->NewOption("DoAngelLogfile")->NewState("0");
    angelData->NewOption("AngelLogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    angelData->NewOption("OCDriverInfo")->NewState("1");

    // CMSISDAP_ID

    IarSettings* cmsisdapId = new IarSettings("CMSISDAP_ID", 2);
    config->AddChild(cmsisdapId);

    IarData* cmsisdapData = cmsisdapId->NewData(0, true, isDebug);

    cmsisdapData->NewOption("OCDriverInfo")->NewState("1");
    cmsisdapData->NewOption("CMSISDAPAttachSlave")->NewState("1");
    cmsisdapData->NewOption("OCIarProbeScriptFile")->NewState("1");
    cmsisdapData->NewOption("CMSISDAPResetList",1)->NewState("10");
    cmsisdapData->NewOption("CMSISDAPHWResetDuration")->NewState("300");
    cmsisdapData->NewOption("CMSISDAPHWResetDelay")->NewState("200");
    cmsisdapData->NewOption("CMSISDAPDoLogfile")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPLogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    cmsisdapData->NewOption("CMSISDAPInterfaceRadio")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPInterfaceCmdLine")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPMultiTargetEnable")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPMultiTarget")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPJtagSpeedList",0)->NewState("0");
    cmsisdapData->NewOption("CMSISDAPBreakpointRadio")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPRestoreBreakpointsCheck")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPUpdateBreakpointsEdit")
            ->NewState("_call_main");
    cmsisdapData->NewOption("RDICatchReset")->NewState("0");
    cmsisdapData->NewOption("RDICatchUndef")->NewState("0");
    cmsisdapData->NewOption("RDICatchSWI")->NewState("0");
    cmsisdapData->NewOption("RDICatchData")->NewState("0");
    cmsisdapData->NewOption("RDICatchPrefetch")->NewState("0");
    cmsisdapData->NewOption("RDICatchIRQ")->NewState("0");
    cmsisdapData->NewOption("RDICatchFIQ")->NewState("0");
    cmsisdapData->NewOption("CatchCORERESET")->NewState("0");
    cmsisdapData->NewOption("CatchMMERR")->NewState("0");
    cmsisdapData->NewOption("CatchNOCPERR")->NewState("0");
    cmsisdapData->NewOption("CatchCHKERR")->NewState("0");
    cmsisdapData->NewOption("CatchSTATERR")->NewState("0");
    cmsisdapData->NewOption("CatchBUSERR")->NewState("0");
    cmsisdapData->NewOption("CatchINTERR")->NewState("0");
    cmsisdapData->NewOption("CatchHARDERR")->NewState("0");
    cmsisdapData->NewOption("CatchDummy")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPMultiCPUEnable")->NewState("0");
    cmsisdapData->NewOption("CMSISDAPMultiCPUNumber")->NewState("0");

    IarSettings* gdbId = new IarSettings("GDBSERVER_ID", 2);
    config->AddChild(gdbId);

    IarData* gdbData = gdbId->NewData(0, true, isDebug);

    gdbData->NewOption("OCDriverInfo")->NewState("1");
    gdbData->NewOption("TCPIP")->NewState("aaa.bbb.ccc.ddd");
    gdbData->NewOption("DoLogfile")->NewState("0");
    gdbData->NewOption("LogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    gdbData->NewOption("CCJTagBreakpointRadio")->NewState("0");
    gdbData->NewOption("CCJTagDoUpdateBreakpoints")->NewState("0");
    gdbData->NewOption("CCJTagUpdateBreakpoints")->NewState("main");

    IarSettings* iarromId = new IarSettings("IARROM_ID", 2);
    config->AddChild(iarromId);

    IarData* iarromData = iarromId->NewData(1, true, isDebug);

    iarromData->NewOption("CRomLogFileCheck")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    iarromData->NewOption("CRomCommPort",0)->NewState("0");
    iarromData->NewOption("CRomCommBaud",0)->NewState("7");
    iarromData->NewOption("OCDriverInfo")->NewState("1");


    IarSettings* ijetId = new IarSettings("IJET_ID", 2);
    config->AddChild(ijetId);

    IarData* ijetData = ijetId->NewData(2, true, isDebug);

    ijetData->NewOption("OCDriverInfo")->NewState("1");
    ijetData->NewOption("IjetAttachSlave")->NewState("1");
    ijetData->NewOption("OCIarProbeScriptFile")->NewState("1");
    ijetData->NewOption("IjetResetList",1)->NewState("2");
    ijetData->NewOption("IjetHWResetDuration")->NewState("300");
    ijetData->NewOption("IjetHWResetDelay")->NewState("200");
    ijetData->NewOption("IjetPowerFromProbe")->NewState(isDebug ? "0" : "1");
    ijetData->NewOption("IjetPowerRadio")->NewState(isDebug ? "1" : "0");
    ijetData->NewOption("IjetDoLogfile")->NewState("0");
    ijetData->NewOption("IjetLogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    ijetData->NewOption("IjetInterfaceRadio")->NewState("0");
    ijetData->NewOption("IjetInterfaceCmdLine")->NewState("0");
    ijetData->NewOption("IjetMultiTargetEnable")->NewState("0");
    ijetData->NewOption("IjetMultiTarget")->NewState("0");
    ijetData->NewOption("IjetScanChainNonARMDevices")->NewState("0");
    ijetData->NewOption("IjetIRLength")->NewState("0");
    ijetData->NewOption("IjetJtagSpeedList",0)->NewState(isDebug ? "5" : "0");
    ijetData->NewOption("IjetProtocolRadio")->NewState("0");
    ijetData->NewOption("IjetSwoPin")->NewState("0");
    ijetData->NewOption("IjetCpuClockEdit")->NewState("72.0");
    ijetData->NewOption("IjetSwoPrescalerList",1)->NewState("0");
    ijetData->NewOption("IjetBreakpointRadio")->NewState("0");
    ijetData->NewOption("IjetRestoreBreakpointsCheck")->NewState("0");
    ijetData->NewOption("IjetUpdateBreakpointsEdit")->NewState("_call_main");
    ijetData->NewOption("RDICatchReset")->NewState("0");
    ijetData->NewOption("RDICatchUndef")->NewState("0");
    ijetData->NewOption("RDICatchSWI")->NewState("0");
    ijetData->NewOption("RDICatchData")->NewState("0");
    ijetData->NewOption("RDICatchPrefetch")->NewState("0");
    ijetData->NewOption("RDICatchIRQ")->NewState("0");
    ijetData->NewOption("RDICatchFIQ")->NewState("0");
    ijetData->NewOption("CatchCORERESET")->NewState("0");
    ijetData->NewOption("CatchMMERR")->NewState("0");
    ijetData->NewOption("CatchNOCPERR")->NewState("0");
    ijetData->NewOption("CatchCHKERR")->NewState("0");
    ijetData->NewOption("CatchSTATERR")->NewState("0");
    ijetData->NewOption("CatchBUSERR")->NewState("0");
    ijetData->NewOption("CatchINTERR")->NewState("0");
    ijetData->NewOption("CatchHARDERR")->NewState("0");
    ijetData->NewOption("CatchDummy")->NewState("0");
    ijetData->NewOption("OCProbeCfgOverride")->NewState(isDebug ? "1" : "0");
    ijetData->NewOption("OCProbeConfig")
            ->NewState(isDebug ?
                cmGlobalIarGenerator::GLOBALCFG.dbgIjetProbeconfig : "");
    ijetData->NewOption("IjetProbeConfigRadio")
            ->NewState(isDebug ? "1" : "0");
    ijetData->NewOption("IjetMultiCPUEnable")->NewState("0");
    ijetData->NewOption("IjetMultiCPUNumber")->NewState("0");
    ijetData->NewOption("IjetSelectedCPUBehaviour")
            ->NewState(isDebug ? "R4" : "0");
    ijetData->NewOption("ICpuName")->NewState(isDebug ? "R4" : "");

    IarSettings* jlinkId = new IarSettings("JLINK_ID", 2);
    config->AddChild(jlinkId);

    IarData* jlinkData = jlinkId->NewData(15, true, isDebug);

    jlinkData->NewOption("JLinkSpeed")->NewState("10000");
    jlinkData->NewOption("CCJLinkDoLogfile")->NewState("0");
    jlinkData->NewOption("CCJLinkLogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    jlinkData->NewOption("CCJLinkHWResetDelay")->NewState("0");
    jlinkData->NewOption("OCDriverInfo")->NewState("1");
    jlinkData->NewOption("JLinkInitialSpeed")->NewState("32");
    jlinkData->NewOption("CCDoJlinkMultiTarget")->NewState("0");
    jlinkData->NewOption("CCScanChainNonARMDevices")->NewState("0");
    jlinkData->NewOption("CCJLinkMultiTarget")->NewState("0");
    jlinkData->NewOption("CCJLinkIRLength")->NewState("0");
    jlinkData->NewOption("CCJLinkCommRadio")->NewState("0");
    jlinkData->NewOption("CCJLinkTCPIP")->NewState("aaa.bbb.ccc.ddd");
    jlinkData->NewOption("CCJLinkSpeedRadioV2")->NewState("1");
    jlinkData->NewOption("CCUSBDevice",1)->NewState("1");
    jlinkData->NewOption("CCRDICatchReset")->NewState("0");
    jlinkData->NewOption("CCRDICatchUndef")->NewState("0");
    jlinkData->NewOption("CCRDICatchSWI")->NewState("0");
    jlinkData->NewOption("CCRDICatchData")->NewState("0");
    jlinkData->NewOption("CCRDICatchPrefetch")->NewState("0");
    jlinkData->NewOption("CCRDICatchIRQ")->NewState("0");
    jlinkData->NewOption("CCRDICatchFIQ")->NewState("0");
    jlinkData->NewOption("CCJLinkBreakpointRadio")->NewState("0");
    jlinkData->NewOption("CCJLinkDoUpdateBreakpoints")->NewState("0");
    jlinkData->NewOption("CCJLinkUpdateBreakpoints")->NewState("main");
    jlinkData->NewOption("CCJLinkInterfaceRadio")->NewState("0");
    jlinkData->NewOption("OCJLinkAttachSlave")->NewState("1");
    jlinkData->NewOption("CCJLinkResetList",6)
            ->NewState(isDebug ? "1" : "5");
    jlinkData->NewOption("CCJLinkInterfaceCmdLine")->NewState("0");
    jlinkData->NewOption("CCCatchCORERESET")->NewState("0");
    jlinkData->NewOption("CCCatchMMERR")->NewState("0");
    jlinkData->NewOption("CCCatchNOCPERR")->NewState("0");
    jlinkData->NewOption("CCCatchCHRERR")->NewState("0");
    jlinkData->NewOption("CCCatchSTATERR")->NewState("0");
    jlinkData->NewOption("CCCatchBUSERR")->NewState("0");
    jlinkData->NewOption("CCCatchINTERR")->NewState("0");
    jlinkData->NewOption("CCCatchHARDERR")->NewState("0");
    jlinkData->NewOption("CCCatchDummy")->NewState("0");
    jlinkData->NewOption("OCJLinkScriptFile")->NewState("1");
    jlinkData->NewOption("CCJLinkUsbSerialNo")->NewState("");
    jlinkData->NewOption("CCTcpIpAlt",0)->NewState("0");
    jlinkData->NewOption("CCJLinkTcpIpSerialNo")->NewState("");
    jlinkData->NewOption("CCCpuClockEdit")->NewState("72.0");
    jlinkData->NewOption("CCSwoClockAuto")->NewState("0");
    jlinkData->NewOption("CCSwoClockEdit")->NewState("2000");
    jlinkData->NewOption("OCJLinkTraceSource")->NewState("0");
    jlinkData->NewOption("OCJLinkTraceSourceDummy")->NewState("0");
    jlinkData->NewOption("OCJLinkDeviceName")->NewState("1");

    IarSettings* lmiftdiId = new IarSettings("LMIFTDI_ID", 2);
    config->AddChild(lmiftdiId);

    IarData* lmiftdiData = lmiftdiId->NewData(2, true, isDebug);

    lmiftdiData->NewOption("OCDriverInfo")->NewState("1");
    lmiftdiData->NewOption("LmiftdiSpeed")->NewState("500");
    lmiftdiData->NewOption("CCLmiftdiDoLogfile")->NewState("0");
    lmiftdiData->NewOption("CCLmiftdiLogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    lmiftdiData->NewOption("CCLmiFtdiInterfaceRadio")->NewState("0");
    lmiftdiData->NewOption("CCLmiFtdiInterfaceCmdLine")->NewState("0");

    IarSettings* macraigorId = new IarSettings("MACRAIGOR_ID", 2);
    config->AddChild(macraigorId);

    IarData* macraigorData = macraigorId->NewData(3, true, isDebug);

    macraigorData->NewOption("jtag",0)->NewState("0");
    macraigorData->NewOption("EmuSpeed")->NewState("1");
    macraigorData->NewOption("TCPIP")->NewState("aaa.bbb.ccc.ddd");
    macraigorData->NewOption("DoLogfile")->NewState("0");
    macraigorData->NewOption("LogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    macraigorData->NewOption("DoEmuMultiTarget")->NewState("0");
    macraigorData->NewOption("EmuMultiTarget")->NewState("0@ARM7TDMI");
    macraigorData->NewOption("EmuHWReset")->NewState("0");
    macraigorData->NewOption("CEmuCommBaud",0)->NewState("4");
    macraigorData->NewOption("CEmuCommPort",0)->NewState("0");
    macraigorData->NewOption("jtago",0)->NewState("0");
    macraigorData->NewOption("OCDriverInfo")->NewState("1");
    macraigorData->NewOption("UnusedAddr")->NewState("0x00800000");
    macraigorData->NewOption("CCMacraigorHWResetDelay")->NewState("");
    macraigorData->NewOption("CCJTagBreakpointRadio")->NewState("0");
    macraigorData->NewOption("CCJTagDoUpdateBreakpoints")->NewState("0");
    macraigorData->NewOption("CCJTagUpdateBreakpoints")->NewState("main");
    macraigorData->NewOption("CCMacraigorInterfaceRadio")->NewState("0");
    macraigorData->NewOption("CCMacraigorInterfaceCmdLine")->NewState("0");

    IarSettings* pemicroId = new IarSettings("PEMICRO_ID", 2);
    config->AddChild(pemicroId);

    IarData* pemicroData = pemicroId->NewData(1, true, isDebug);

    pemicroData->NewOption("OCDriverInfo")->NewState("1");
    pemicroData->NewOption("OCPEMicroAttachSlave")->NewState("1");
    pemicroData->NewOption("CCPEMicroInterfaceList",0)->NewState("0");
    pemicroData->NewOption("CCPEMicroResetDelay")->NewState("");
    pemicroData->NewOption("CCPEMicroJtagSpeed")->NewState("#UNINITIALIZED#");
    pemicroData->NewOption("CCJPEMicroShowSettings")->NewState("0");
    pemicroData->NewOption("DoLogfile")->NewState("0");
    pemicroData->NewOption("LogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    pemicroData->NewOption("CCPEMicroUSBDevice",0)->NewState("0");
    pemicroData->NewOption("CCPEMicroSerialPort",0)->NewState("0");
    pemicroData->NewOption("CCJPEMicroTCPIPAutoScanNetwork")->NewState("1");
    pemicroData->NewOption("CCPEMicroTCPIP")->NewState("10.0.0.1");
    pemicroData->NewOption("CCPEMicroCommCmdLineProducer")->NewState("0");
    pemicroData->NewOption("CCSTLinkInterfaceRadio")->NewState("0");
    pemicroData->NewOption("CCSTLinkInterfaceCmdLine")->NewState("0");


    IarSettings* rdiId = new IarSettings("RDI_ID", 2);
    config->AddChild(rdiId);

    IarData* rdiData = rdiId->NewData(2, true, isDebug);

    rdiData->NewOption("CRDIDriverDll")->NewState("###Uninitialized###");
    rdiData->NewOption("CRDILogFileCheck")->NewState("0");
    rdiData->NewOption("CRDILogFileEdit")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    rdiData->NewOption("CCRDIHWReset")->NewState("0");
    rdiData->NewOption("CCRDICatchReset")->NewState("0");
    rdiData->NewOption("CCRDICatchUndef")->NewState("0");
    rdiData->NewOption("CCRDICatchSWI")->NewState("0");
    rdiData->NewOption("CCRDICatchData")->NewState("0");
    rdiData->NewOption("CCRDICatchPrefetch")->NewState("0");
    rdiData->NewOption("CCRDICatchIRQ")->NewState("0");
    rdiData->NewOption("CCRDICatchFIQ")->NewState("0");
    rdiData->NewOption("OCDriverInfo")->NewState("1");


    IarSettings* stlinkId = new IarSettings("STLINK_ID", 2);
    config->AddChild(stlinkId);

    IarData* stlinkData = stlinkId->NewData(2, true, isDebug);


    stlinkData->NewOption("OCDriverInfo")->NewState("1");
    stlinkData->NewOption("CCSTLinkInterfaceRadio")->NewState("0");
    stlinkData->NewOption("CCSTLinkInterfaceCmdLine")->NewState("0");
    stlinkData->NewOption("CCSTLinkResetList",1)->NewState("0");
    stlinkData->NewOption("CCCpuClockEdit")->NewState("72.0");
    stlinkData->NewOption("CCSwoClockAuto")->NewState("0");
    stlinkData->NewOption("CCSwoClockEdit")->NewState("2000");


    IarSettings* thirdPartyId = new IarSettings("THIRDPARTY_ID", 2);
    config->AddChild(thirdPartyId);

    IarData* thirdPartyData = thirdPartyId->NewData(0, true, isDebug);

    thirdPartyData->NewOption("CThirdPartyDriverDll")
            ->NewState("###Uninitialized###");
    thirdPartyData->NewOption("CThirdPartyLogFileCheck")->NewState("0");
    thirdPartyData->NewOption("CThirdPartyLogFileEditB")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);
    thirdPartyData->NewOption("OCDriverInfo")->NewState("1");

    IarSettings* xds100Id = new IarSettings("XDS100_ID", 2);
    config->AddChild(xds100Id);

    IarData* xds100Data = xds100Id->NewData(2, true, isDebug);

    xds100Data->NewOption("OCDriverInfo")->NewState("1");
    xds100Data->NewOption("OCXDS100AttachSlave")->NewState("1");
    xds100Data->NewOption("TIPackageOverride")->NewState("0");
    xds100Data->NewOption("TIPackage")->NewState("");
    xds100Data->NewOption("CCXds100InterfaceList",0)->NewState("0");
    xds100Data->NewOption("BoardFile")->NewState("");
    xds100Data->NewOption("DoLogfile")->NewState("0");
    xds100Data->NewOption("LogFile")
            ->NewState(cmGlobalIarGenerator::GLOBALCFG.dbgLogFile);


    XmlNode* debuggerPlugins = new XmlNode("debuggerPlugins");

    cmsys::Glob g;
    g.SetRecurse(true);
    g.RecurseThroughSymlinksOff();
    std::string expr = cmGlobalIarGenerator::GLOBALCFG.iarArmPath;
    expr += std::string("/plugins/") + "*.ewplugin";

    g.FindFiles(expr);
    std::vector<std::string>& files = g.GetFiles();

    // Allow language checking.
    for(std::vector<std::string>::const_iterator it = files.begin();
        it != files.end(); ++it)
      {

      if (it->find("JPN") == std::string::npos)
        {
        debuggerPlugins->AddChild(
            new IarDebuggerPlugin(cmGlobalIarGenerator::ToToolkitPath(*it),
                ((it->find(GLOBALCFG.rtos) != std::string::npos))));
        }
      }

    expr = cmGlobalIarGenerator::GLOBALCFG.iarArmPath
        .substr(0, cmGlobalIarGenerator::GLOBALCFG.iarArmPath.length() -
            (sizeof("/arm")-1));
    expr += std::string("/common/plugins/") + "*.ewplugin";

    g.FindFiles(expr);
    files = g.GetFiles();

    for(std::vector<std::string>::const_iterator it = files.begin();
        it != files.end(); ++it)
      {
      if (it->find("JPN") == std::string::npos)
        {
        bool load = (it->find("SymList") != std::string::npos) ||
            (it->find("CodeCoverage") != std::string::npos);

        debuggerPlugins->AddChild(
            new IarDebuggerPlugin(cmGlobalIarGenerator::ToWorkbenchPath(*it),
                load));
        }
      }

    config->AddChild(debuggerPlugins);
    }

  FILE* pFile = fopen(debuggerFileName.c_str(), "w");

  std::string output;
  output.reserve(1 << 20); // 1K.
  if (GLOBALCFG.wbVersionMajor >= 9) {
    output += XML_DECL_V9;
  } else {
    output += XML_DECL;
  }

  root.ToString(0, output);
  fwrite(output.c_str(), output.length(), 1, pFile);


  fclose(pFile);

}
