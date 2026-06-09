; Shared boot-time data written by stage1 (real mode) and read by the kernel.
%ifndef BOOT_PARAMS_INC
%define BOOT_PARAMS_INC

%define BOOT_PARAMS_PHYS   0x00000500
%define BOOT_PARAM_MEM_MB  0
%define BOOT_PARAM_DRIVE   4

%endif
