/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2004 Alexander Neundorf (neundorf@kde.org)
  Copyright 2007 Miguel A. Figueroa-Villanueva
  Copyright 2017 Jakub Korbel

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef cmGlobalIarGenerator_h
#define cmGlobalIarGenerator_h

#include <vector>

#include "cmGlobalGenerator.h"
#include "cmGlobalGeneratorFactory.h"
#include "cmDocumentationEntry.h"

class cmMakefile;
class cmGeneratedFileStream;


std::string getLvl(unsigned int level);


//------------------------------------------------------------------------------
///
/// @brief Converts integer to decimal string notation.
///
/// @param[in] val Value to convert.
///
/// @return Converted string.
///
//------------------------------------------------------------------------------
static inline std::string int2str(int val)
{
  std::stringstream tmp;
  tmp << val;
  return tmp.str();
}

//------------------------------------------------------------------------------
///
/// @brief Generic XML node.
///
/// This node can have multiple children and attributes, or a plain text
/// value.
///
//------------------------------------------------------------------------------
class XmlNode
{
  /// @brief Node name (represented later as tag name).
  std::string nodeName;

  /// @brief Plain text value.
  std::string plainValue;

  /// @brief Attribute pairs (attr="val").
  std::vector<std::pair<std::string, std::string>> attrs;

  /// @brief Children vector.
  std::vector<XmlNode*> children;

public:
  XmlNode(std::string name, std::string value)
    : nodeName(name)
    , plainValue(value)
  {
    // Just copy values.
  }

  XmlNode(std::string name)
    : nodeName(name)
    , plainValue("")
  {
    // Just copy values.
  }

  /// This function creates a new child node (dynamic memory allocation).
  XmlNode* NewChild(std::string name, std::string value)
  {
    XmlNode* child = new XmlNode(name, value);
    children.push_back(child);
    return child;
  }

  /// This function creates a new child node (dynamic memory allocation).
  XmlNode* NewChild(std::string name, int value)
  {
    XmlNode* child = new XmlNode(name, std::to_string(value));
    children.push_back(child);
    return child;
  }

  /// This function creates a new child node (dynamic memory allocation).
  XmlNode* NewChild(std::string name)
  {
    XmlNode* child = new XmlNode(name);
    children.push_back(child);
    return child;
  }

  /// This function adds a child node (no memory allocation).
  ///
  /// @warning delete will be issued for this node!
  XmlNode* AddChild(XmlNode* child)
  {
    children.push_back(child);
    return this;
  }

  void AddAttr(std::string name, std::string value)
  {
    attrs.push_back(std::make_pair(name, value));
  }

  std::string& AppendOpenTag(std::string& tag, unsigned int level) const
  {
    tag += getLvl(level) + "<" + nodeName;

    for (std::vector<std::pair<std::string, std::string>>::const_iterator it =
           attrs.begin();
         it != attrs.end(); ++it) {
      tag += std::string(" ") + it->first + "=\"" + it->second + "\"";
    }
    tag += ">";
    return tag;
  }

  std::string& AppendCloseTag(std::string& tag, unsigned int level) const
  {
    (void)level; // May come handy, when open and close are on a separate
    // line.

    tag += std::string("</") + nodeName + ">\n";
    return tag;
  }

  void ToString(unsigned int level, std::string& outStr) const
  {
    this->AppendOpenTag(outStr, level);

    bool moreThanOnce = false;

    for (std::vector<XmlNode*>::const_iterator it = this->children.begin();
         it != this->children.end(); ++it) {
      if (!moreThanOnce) {
        moreThanOnce = true;
        outStr += "\n";
      }

      (*it)->ToString(level + 1, outStr);
    }

    if (!moreThanOnce) {
      if (this->plainValue.empty()) {
        outStr[outStr.length() - 1] = '/';
        outStr += ">\n";
        // Do not append close tag.
      } else {
        outStr += this->plainValue;
        this->AppendCloseTag(outStr, level);
      }
    } else {
      outStr += getLvl(level);
      this->AppendCloseTag(outStr, level);
    }
  }

  virtual ~XmlNode()
  {
    for (std::vector<XmlNode*>::iterator it = this->children.begin();
         it != this->children.end(); ++it) {
      delete *it;
    }
  }
};

static const char* RUNTIME_LIBRARY_CONFIG[] = {
  "None",   // 0
  "Normal", // 1
  "Full",   // 2
  "Custom"  // 3
};

static const char* SCANF_PRINTF_FORMATTING[] = {
  "Auto",                     // 0
  "Full",                     // 1
  "Full without multibytes",  // 2
  "Large",                    // 3
  "Large without multibytes", // 4
  "Small",                    // 5
  "Small without multibytes", // 6
  "Tiny"                      // 7
};

static const int SCANF_FORMATTING_CNT = 7;
static const int PRINTF_FORMATTING_CNT = 8;

class FileTreeNode
{
public:
  std::string ftNodeName;

  std::vector<FileTreeNode*> children;

  FileTreeNode(std::string name)
    : ftNodeName(name)
  {
  }

  void TransformToIarTree(XmlNode* root)
  {
    if (!this->children.empty()) {
      XmlNode* group = root->NewChild("group");
      group->NewChild("name", this->ftNodeName);

      for (std::vector<FileTreeNode*>::const_iterator it =
             this->children.begin();
           it != this->children.end(); ++it) {
        (*it)->TransformToIarTree(group);
      }

    } else {
      XmlNode* file = root->NewChild("file");
      file->NewChild("name", this->ftNodeName);
    }
  }

  FileTreeNode* NewNode(std::string name)
  {
    FileTreeNode* node = new FileTreeNode(name);
    children.push_back(node);
    return node;
  }

  static void AddToTree(FileTreeNode* root, std::string path,
                        std::string const& fullpath)
  {
    size_t slashPos = path.find_first_of("/\\");
    std::string currentChunk;
    std::string restOfPath;
    if (slashPos != std::string::npos) {
      currentChunk = path.substr(0, slashPos);
      restOfPath = path.substr(slashPos + 1);
    } else {
      currentChunk = path;
      restOfPath = "";
    }

    if (!currentChunk.empty()) {
      bool found = false;
      for (std::vector<FileTreeNode*>::const_iterator it =
             root->children.begin();
           it != root->children.end(); ++it) {
        if ((*it)->ftNodeName == currentChunk) {
          // Found, move in tree.
          AddToTree(*it, restOfPath, fullpath);
          found = true;
          break;
        }
      }

      if (!found) {
        if (restOfPath.empty()) {
          // Not found, create new and finish.
          root->NewNode(fullpath);
          return;
        } else {
          // Not found, create new and move inside.
          FileTreeNode* newNode = root->NewNode(currentChunk);
          AddToTree(newNode, restOfPath, fullpath);
        }
      }
    }
  }

  virtual ~FileTreeNode()
  {
    for (std::vector<FileTreeNode*>::iterator it = children.begin();
         it != children.end(); ++it) {
      delete *it;
    }
  }
};

class IarOption : public XmlNode
{
private:
  int optVersion;
  std::string optName;

public:
  IarOption(std::string name, int version)
    : XmlNode("option")
    , optVersion(version)
    , optName(name)
  {
    if (!optName.empty()) {
      this->NewChild("name", optName);
    }

    if (optVersion >= 0) {
      this->NewChild("version", int2str(optVersion));
    }
  }

  IarOption(std::string name)
    : XmlNode("option")
    , optVersion(-1)
    , optName(name)
  {
    if (!optName.empty()) {
      this->NewChild("name", optName);
    }
  }

  void NewState(std::string state) { this->NewChild("state", state); }

  void NewStates(std::vector<std::string> states)
  {
    for (std::vector<std::string>::const_iterator it = states.begin();
         it != states.end(); ++it) {
      this->NewChild("state", *it);
    }
  }
};

class IarDebuggerPlugin : public XmlNode
{
public:
  IarDebuggerPlugin(std::string file, bool load)
    : XmlNode("plugin")
  {
    if (!file.empty()) {
      this->NewChild("file", file);
      this->NewChild("loadFlag", load ? "1" : "0");
    }
  }
};

class IarData : public XmlNode
{
private:
  int dataVersion;
  bool dataWantNonLocal;
  bool dataDebug;

public:
  IarData(int version, bool wantNonLocal, bool debug)
    : XmlNode("data")
    , dataVersion(version)
    , dataWantNonLocal(wantNonLocal)
    , dataDebug(debug)
  {
    this->NewChild("version", int2str(dataVersion));
    this->NewChild("wantNonLocal", dataWantNonLocal ? "1" : "0");
    this->NewChild("debug", dataDebug ? "1" : "0");
  }

  IarOption* NewOption(std::string name, int version)
  {
    IarOption* option = new IarOption(name, version);
    this->AddChild(option);
    return option;
  }

  IarOption* NewOption(std::string name)
  {
    IarOption* option = new IarOption(name);
    this->AddChild(option);
    return option;
  }
};

class IarSettings : public XmlNode
{
private:
  std::string settingsName;
  int archiveVersion;

public:
  IarSettings(std::string name, int version)
    : XmlNode("settings")
    , settingsName(name)
    , archiveVersion(version)
  {
    if (!settingsName.empty()) {
      this->NewChild("name", settingsName);
    }

    this->NewChild("archiveVersion", int2str(archiveVersion));
  }

  IarData* NewData(int version, bool wantNonLocal, bool debug)
  {
    IarData* data = new IarData(version, wantNonLocal, debug);
    this->AddChild(data);
    return data;
  }
};

class IarFsNode : public XmlNode
{
private:
  std::string fsPath;
  bool fsIsDir;

  std::string GetLastDir(std::string path)
  {
    size_t position = path.find_last_of("/\\");
    if (position != std::string::npos) {
      return path.substr(position + 1);
    }
    return std::string("");
  }

public:
  IarFsNode(std::string path, bool isDir)
    : XmlNode(isDir ? "group" : "file")
    , fsPath(path)
    , fsIsDir(isDir)
  {
    this->NewChild("name", isDir ? GetLastDir(fsPath) : fsPath);
  }

  IarData* NewData(int version, bool wantNonLocal, bool debug)
  {
    IarData* data = new IarData(version, wantNonLocal, debug);
    this->AddChild(data);
    return data;
  }
};

/** \class cmGlobalIarGenerator
 * \brief Write Eclipse project files for Makefile based projects
 */
class cmGlobalIarGenerator : public cmGlobalGenerator
{
public:
  static const char* XML_DECL;
  static char const* XML_DECL_V9;

  cmGlobalIarGenerator(cmake* cm);
  ~cmGlobalIarGenerator() override;

  static std::unique_ptr<cmGlobalGeneratorFactory> NewFactory()
  {
    return std::unique_ptr<cmGlobalGeneratorFactory>(
      new cmGlobalGeneratorSimpleFactory<cmGlobalIarGenerator>());
  }

  //! create the correct local generator
  std::unique_ptr<cmLocalGenerator> CreateLocalGenerator(
    cmMakefile* mf) override;

  /// @return the name of this generator.
  static std::string GetActualName() { return "IAR Workbench for ARM"; }

  //! Get the name for this generator
  std::string GetName() const override { return GetActualName(); }

  /// Overloaded methods. @see cmGlobalGenerator::GetDocumentation()
  static cmDocumentationEntry GetDocumentation();

  /**
   * Utilized by the generator factory to determine if this generator
   * supports toolsets.
   */
  static bool SupportsToolset() { return false; }

  /**
   * Utilized by the generator factory to determine if this generator
   * supports platforms.
   */
  static bool SupportsPlatform() { return true; }

  // Toolset / Platform Support
  bool SetGeneratorPlatform(std::string const& p, cmMakefile* mf) override;

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  void EnableLanguage(std::vector<std::string> const& languages, cmMakefile*,
                      bool optional) override;
  /*
   * Determine what program to use for building the project.
   */
  bool FindMakeProgram(cmMakefile* mf) override;

  void ComputeTargetObjectDirectory(cmGeneratorTarget* gt) const override;

  void Generate() override;
  void AddExtraIDETargets() override;
  

  bool Open(const std::string& bindir, const std::string& projectName,
            bool dryRun) override;

  bool IsMultiConfig() const override { return false; };

  const char* GetCMakeCFGIntDir() const override
  {
    return cmGlobalIarGenerator::GLOBALCFG.buildType.empty()
      ? "empty"
      : cmGlobalIarGenerator::GLOBALCFG.buildType.c_str();
  }

  /** Append the subdirectory for the given configuration.  */
  void AppendDirectoryForConfig(const std::string& prefix,
                                const std::string& config,
                                const std::string& suffix,
                                std::string& dir) override;

  static std::string ToToolkitPath(std::string absolutePath);

  static std::string ToWorkbenchPath(std::string absolutePath);

  void ConvertTargetToProject(const cmTarget& target,
                              cmGeneratorTarget* genTgt);

protected:
  std::vector<cmGlobalGenerator::GeneratedMakeCommand>
    GenerateBuildCommand(
    std::string const& makeProgram, std::string const& projectName,
    std::string const& projectDir, std::vector<std::string> const& targetNames,
    std::string const& config, int jobs, bool verbose,
    cmBuildOptions buildOptions, std::vector<std::string> const& makeOptions,
    BuildTryCompile /*isInTryCompile*/) override;

private:
  std::string StampFile;
  static const char* DEFAULT_BUILD_PROGRAM;
  static const char* CHECK_BUILD_SYSTEM_TARGET;
  bool AddCheckTarget();
  std::string FindIarBuildCommand();

  void RegisterProject(const std::string& projectName);

  void GetCmdLines(std::vector<cmCustomCommand> const& rTmpCmdVec,
                   std::string& rBuildCmd,
                   int& rStart);

  struct CompilerOpts
  {
    enum CxxStandard
    {
      /// --c++
      CXX_STANDARD_CXX,

      /// --ec++
      CXX_STANDARD_EMBEDDED_CXX,

      /// --eec++
      CXX_STANDARD_EXTENDED_EMBEDDED_CXX
    };

    enum Optimization
    {
      OPTIMIZATION_NONE,
      OPTIMIZATION_LOW,
      OPTIMIZATION_MEDIUM,
      OPTIMIZATION_HIGH_SIZE,
      OPTIMIZATION_HIGH_SPEED,
      OPTIMIZATION_HIGH_BALANCED
    };

    struct
    {
      /// Are debug symbols present? (--debug)
      unsigned int debug:1;

      /// Is IAR Extended standard in use? (-e)
      unsigned int iarExtStd:1;

      /// If to use thumb mode or arm mode (--cpu_mode={arm|a|thumb|t} or --arm)
      unsigned int useThumb:1;

      /// If char is signed (default unsigned).
      unsigned int signedChar:1;

      /// Whether to use big endian instead of little endian. (--endian={big|little})
      unsigned int useBigEndian:1;

      /// If C compiler uses C++ style inlines (--use_c++_inline).
      unsigned int cxxStyleInlines:1;

      /// If C++ supports exceptions (--no_exceptions).
      unsigned int hasExceptions:1;

      /// If C++ compiler supports RTTI (--no_rtti).
      unsigned int hasRtti:1;

      /// If C++ compiler supports Static destruction (--no_static_destruction).
      unsigned int hasStaticDestruction:1;

      /// If guard calls are enabled.
      unsigned int enableGuardCalls:1;

      // todo Add more options.

    } flags;

    /// What FPU to use. (--fpu={VFPv2|VFPv3|VFPv3_d16|VFPv4|VFPv4_sp|VFP9-S|none})
    std::string fpu;

    /// What CPU to use. (--fpu=<CPU_NAME>)
    std::string cpu;

    /// DLIB config header file. (--dlib_config <FILENAME.h>)
    std::string dlibCfgFile;

    /// Pre-include header file. (--preinclude <FILENAME.h>)
    std::string preinclude;

    /// Used C++ standard (--c++ or --ec++ or --eec++)
    CxxStandard cxxStd;

    /// What level of optimization is used.
    Optimization optimization;

    // todo Add more options.

  };

  struct LinkerOpts
  {
    struct
    {
      /// If to inline small routines (--inline).
      unsigned int inlineSmall:1;

      /// If linker supports exceptions (--no_exceptions).
      unsigned int hasExceptions:1;

      /// If to merge equivalent read-only sections (--merge_duplicate_sections).
      unsigned int mergeDuplicateSections:1;

      /// If to use virtual function elimination (--vfe).
      unsigned int vfe:1;

      /// If semihosting is enabled (--semihosting).
      unsigned int semihosting:1;

      // todo Add more options.
    } flags;

    /// Map file (--map <FILENAME.map>).
    std::string mapFile;

    /// Configuration file (--config <FILENAME.icf>).
    std::string configFile;

    /// Entry routine (--entry <entry_routine>).
    std::string entryRoutine;
  };

  struct BuildConfig
  {
    /// build configuration name, this could be anything.
    std::string name;

    /// build configuration name in uppercase, this could be anything.
    std::string nameUppercase;

    /// toolchain name - ARM for ARM.
    std::string toolchain;

    /// directory where to store all object files (.o).
    std::string browseInfoDir;

    /// directory where to store all object files (.o).
    std::string objectDir;

    /// directory where to store targets (executables and libs).
    std::string exeDir;

    /// directory for IAR lists.
    std::string listDir;

    /// true if the build contains debug information, false otherwise.
    bool isDebug;

    /// .icf path
    std::string icfPath;

    /// output file name.
    std::string outputFile;

    /// pre-build command line.
    std::string preBuildCmd;

    /// post-build command line.
    std::string postBuildCmd;

    /// Compile definitions <state></state>
    std::vector<std::string> compileDefs;

    /// Compiler options <state></state>
    std::vector<std::string> compilerOpts;

    /// Linker keep symbols <state></state>
    std::vector<std::string> linkerKeepSymbols;

    /// Linker options <state></state>
    std::vector<std::string> linkerOpts;

    /// ASM options <state></state>
    std::vector<std::string> asmOpts;

    /// Libraries <state></state>
    std::vector<std::string> libraries;
  };

  struct Project
  {
    /// Project name
    std::string name;

    /// Build config collection.
    BuildConfig buildCfg;

    /// output binary type: true if lib, false if exe.
    bool isLib;

    /// project directory. Everything should be relative to this path.
    std::string projectDir;

    /// binary directory.
    std::string binaryDir;

    /// Includes <state></state>
    std::vector<std::string> includes;

    /// Includes <state></state>
    std::vector<std::string> sources;

    void CreateProjectFile();
    void CreateProjectFile8(); // for v8
    void CreateProjectFile9(); // for v9

    void CreateDebuggerFile();
    void CreateDebuggerFile8();
    void CreateDebuggerFile9();
  };

  static void cmGlobalIarGenerator::ParseCmdLineOpts(std::string cmdLine,
                                              const char* multiOpts[],
                                              size_t multiOptsLen,
    std::vector<std::string>& opts);

  struct Workspace
  {
    /// Workspace name.
    std::string name;

    /// Workspace directory.
    std::string workspaceDir;

    /// Full workspace path
    std::string workspacePath;

    /// Project names collection.
    std::map<std::string, Project*> projects;

    void RegisterProject(std::string name, Project* project);

    void CreateWorkspaceFile();

    //void CreateDebuggerFile();
  };

  struct GlobalCmakeCfg
  {
    std::string buildType;
    std::string iarCCompilerFlags;
    std::string iarAsmFlags;
    std::string iarCxxCompilerFlags ;
    std::string iarLinkerFlags;
    std::string iarArmPath;
    std::string iarPath;
    std::string compilerDlibConfig  ;
    int compilerDlibConfigId;
    std::string compilerPathExe     ;
    std::string cpuName             ;
    std::string systemName          ;
    std::string dbgExtraOptions     ;
    std::string dbgCspyFlashLoaderv3;
    std::string dbgCspyMacfile      ;
    std::string dbgCspyMemfile      ;
    std::string dbgProbeSelection   ;
    std::string dbgIjetProbeconfig  ;
    std::string dbgLogFile          ;
    std::string linkerEntryRoutine  ;
    std::string linkerIcfFile       ;
    std::string tgtArch             ;
    std::string wbVersion;
    unsigned int wbVersionMajor;
    unsigned int wbVersionMinor;
    unsigned int wbVersionPatch;
    std::string chipSelection       ;
    std::string rtos                ;
    std::string compilerPreInclude  ;
    std::string scanfFmt  ;
    std::string printfFmt;
    int scanfFmtId  ;
    int printfFmtId  ;
    std::string bufferedTermOut;
    std::string semihostingEnabled;
    std::string jobs  ;
  };

  static const char* PROJ_FILE_EXT;
  static const char* WS_FILE_EXT;
  static const char* DEFAULT_MAKE_PROGRAM;
  static const char* MULTIOPTS_COMPILER[13];
  static const char* MULTIOPTS_LINKER[24];

  Workspace workspace;
  static GlobalCmakeCfg GLOBALCFG;

};

#endif
