IAR Workbench for ARM
-----------------

Generates IAR Workbench for ARM project and workspace files (experimental, work-in-progress).

Customizations are available through the following cache variables:

* ``IAR_INSTALL_DIR``
    * Where the IAR is installed, e.g. "C:/Program Files (x86)/IAR Systems/Embedded Workbench 9.0"
* ``IAR_TOOLKIT_DIR``
    * Where the IAR toolkit is installed, e.g. "C:/Program Files (x86)/IAR Systems/Embedded Workbench 9.0/arm"
* ``IAR_GENERAL_BUFFERED_TERMINAL_OUTPUT``
    * ON/OFF
* ``IAR_GENERAL_SCANF_FORMATTER``
    * "Auto"
    * "Full"
    * "Full without multibytes"
    * "Large"
    * "Large without multibytes"
    * "Small"
    * "Small without multibytes"
    * "Tiny"
* ``IAR_GENERAL_PRINTF_FORMATTER``
    * "Auto"
    * "Full"
    * "Full without multibytes"
    * "Large"
    * "Large without multibytes"
    * "Small"
    * "Small without multibytes"
    * "Tiny"
* ``IAR_SEMIHOSTING_ENABLE``
    * ON/OFF
* ``IAR_CPU_NAME``
    * e.g. Cortex-R4
* ``CMAKE_SYSTEM_NAME``
    * Read CMake Manual
* ``IAR_DEBUGGER_CSPY_EXTRAOPTIONS``
    * Commandline options for CSPY targets.
* ``IAR_DEBUGGER_CSPY_FLASHLOADER_V3``
    * Flash loader
* ``IAR_DEBUGGER_CSPY_MACFILE``
    * Mac file
* ``IAR_DEBUGGER_CSPY_MEMFILE``
    * Mem file
* ``IAR_DEBUGGER_IJET_PROBECONFIG``
    * Probe configuration
* ``IAR_DEBUGGER_PROBE``
    * Probe selection
* ``IAR_DEBUGGER_LOGFILE``
    * Debugger log file
* ``IAR_LINKER_ENTRY_ROUTINE``
    * linker entry routine
* ``IAR_LINKER_ICF_FILE``
    * Linker file
* ``IAR_TARGET_ARCHITECTURE``
    * Target architecture
* ``IAR_CHIP_THUMBSUPPORT``
    * Thumb support
* ``IAR_CHIP_FPU``
    * FPU selection
* ``IAR_TARGET_RTOS``
    * Target RTOS plugin
* ``IAR_COMPILER_PREINCLUDE``
    * Pre-include header file
* ``IAR_WORKBENCH_VERSION``
    * IAR workbench version - it is also calculated per IAR_INSTALL_DIR
* ``IAR_BUILD_JOBS``
    * How many jobs/processes to use in parallel for IAR Batch Build
* ``IAR_CORE_NAME``
    * Name of the Core



.. note::
  This generator is deemed experimental as of CMake |release|
  and is still a work in progress.  Future versions of CMake
  may make breaking changes as the generator matures.
