#pragma once

#include <cpu/cpu.hpp>
#include <functional>
#include <unordered_map>
#include <util/log.hpp>
#include <util/types.hpp>

namespace bios {
struct Function {
  const char* name;
  std::vector<const char*> args;
  std::function<bool(cpu::Cpu&)> callback;

  Function(const char* _name, std::vector<const char*> _args) : name(_name), args(_args) {}
  Function(const char* _name, std::vector<const char*> _args, std::function<bool(cpu::Cpu&)> _callback)
      : name(_name),
        args(_args),
        callback(_callback) {}
};

inline bool dont_log(cpu::Cpu& cpu) {
  return false;
}

inline bool dbg_output_char(cpu::Cpu& cpu) {
  cpu.m_tty_out_log += static_cast<char>(cpu.gpr(4));
  return false;
}

inline bool dbg_output_string(cpu::Cpu& cpu) {
  // TODO
  LOG_TODO();
  return true;
}

inline bool halt_system(cpu::Cpu& cpu) {
  // TODO
  LOG_TODO();
  return true;
}

const std::unordered_map<uint8_t, Function> A0 = {
  { 0x00, { "FileOpen", { "filename", "accessmode" } } },
  { 0x01, { "FileSeek", { "fd", "offset", "seektype" } } },
  { 0x02, { "FileRead", { "fd", "dst", "length" } } },
  { 0x03, { "FileWrite", { "fd", "src", "length" } } },
  { 0x04, { "FileClose", { "fd" } } },
  { 0x05, { "FileIoctl", { "fd", "cmd", "arg" } } },
  { 0x06, { "exit", { "exitcode" } } },
  { 0x07, { "FileGetDeviceFlag", { "fd" } } },
  { 0x08, { "FileGetc", { "fd" } } },
  { 0x09, { "FilePutc", { "char", "fd" } } },
  { 0x0A, { "todigit", { "char" } } },
  { 0x0B, { "atof", { "src" } } },
  { 0x0C, { "strtoul", { "src", "src_end", "base" } } },
  { 0x0D, { "strtol", { "src", "src_end", "base" } } },
  { 0x0E, { "abs", { "val" } } },
  { 0x0F, { "labs", { "val" } } },
  { 0x10, { "atoi", { "src" } } },
  { 0x11, { "atol", { "src" } } },
  { 0x12, { "atob", { "src", "num_dst" } } },
  { 0x13, { "SaveState", { "buf" } } },
  { 0x14, { "RestoreState", { "buf,param" } } },
  { 0x15, { "strcat", { "dst", "src" } } },
  { 0x16, { "strncat", { "dst", "src", "maxlen" } } },
  { 0x17, { "strcmp", { "str1", "str2" } } },
  { 0x18, { "strncmp", { "str1", "str2", "maxlen" } } },
  { 0x19, { "strcpy", { "dst", "src" } } },
  { 0x1A, { "strncpy", { "dst", "src", "maxlen" } } },
  { 0x1B, { "strlen", { "src" } } },
  { 0x1C, { "index", { "src", "char" } } },
  { 0x1D, { "rindex", { "src", "char" } } },
  { 0x1E, { "strchr", { "src", "char" } } },
  { 0x1F, { "strrchr", { "src", "char" } } },
  { 0x20, { "strpbrk", { "src", "list" } } },
  { 0x21, { "strspn", { "src", "list" } } },
  { 0x22, { "strcspn", { "src", "list" } } },
  { 0x23, { "strtok", { "src", "list" } } },
  { 0x24, { "strstr", { "str", "substr" } } },
  { 0x25, { "toupper", { "char" } } },
  { 0x26, { "tolower", { "char" } } },
  { 0x27, { "bcopy", { "src", "dst", "len" } } },
  { 0x28, { "bzero", { "dst", "len" } } },
  { 0x29, { "bcmp", { "ptr1", "ptr2", "len" } } },
  { 0x2A, { "memcpy", { "dst", "src", "len" } } },
  { 0x2B, { "memset", { "dst", "fillbyte,len" } } },
  { 0x2C, { "memmove", { "dst", "src", "len" } } },
  { 0x2D, { "memcmp", { "src1", "src2", "len" } } },
  { 0x2E, { "memchr", { "src", "scanbyte", "len" } } },
  { 0x2F, { "rand", {}, dont_log } },
  { 0x30, { "srand", { "seed" } } },
  { 0x31, { "qsort", { "base", "nel", "width", "callback" } } },
  { 0x32, { "strtod", { "src", "src_end" } } },
  { 0x33, { "malloc", { "size" } } },
  { 0x34, { "free", { "buf" } } },
  { 0x35, { "lsearch", { "key", "base", "nel", "width", "callback" } } },
  { 0x36, { "bsearch", { "key", "base", "nel", "width", "callback" } } },
  { 0x37, { "calloc", { "sizx", "sizy" } } },
  { 0x38, { "realloc", { "old_buf", "new_siz" } } },
  { 0x39, { "InitHeap", { "addr", "size" } } },
  { 0x3A, { "SystemErrorExit", { "exitcode" } } },
  { 0x3B, { "std_in_getchar", {} } },
  { 0x3C, { "std_out_putchar", { "char" }, dbg_output_char } },
  { 0x3D, { "std_in_gets", { "dst" } } },
  { 0x3E, { "std_out_puts", { "src" }, dbg_output_string } },
  { 0x3F, { "printf", { "txt" }, dont_log } },
  { 0x40, { "SystemErrorUnresolvedException", {}, halt_system } },
  { 0x41, { "LoadExeHeader", { "filename", "headerbuf" } } },
  { 0x42, { "LoadExeFile", { "filename", "headerbuf" } } },
  { 0x43, { "DoExecute", { "headerbuf", "param1", "param2" } } },
  { 0x44, { "FlushCache", {} } },
  { 0x45, { "init_a0_b0_c0_vectors", {} } },
  { 0x46, { "GPU_dw", { "Xdst", "Ydst", "Xsiz", "Ysiz", "src" } } },
  { 0x47, { "gpu_send_dma", { "Xdst", "Ydst", "Xsiz", "Ysiz", "src" } } },
  { 0x48, { "SendGP1Command", { "gp1cmd" } } },
  { 0x49, { "GPU_cw", { "gp0cmd" } } },
  { 0x4A, { "GPU_cwp", { "src", "num" } } },
  { 0x4B, { "send_gpu_linked_list", { "src" } } },
  { 0x4C, { "gpu_abort_dma", {} } },
  { 0x4D, { "GetGPUStatus", {} } },
  { 0x4E, { "gpu_sync", {} } },
  { 0x51, { "LoadAndExecute", { "filename", "stackbase", "stackoffset" } } },
  { 0x54, { "CdInit", {} } },
  { 0x55, { "_bu_init", {} } },
  { 0x56, { "CdRemove", {} } },
  { 0x5B, { "dev_tty_init", {} } },
  { 0x5C, { "dev_tty_open", { "fcb", "unused_path", "accessmode" } } },
  { 0x5D, { "dev_tty_in_out", { "fcb", "cmd" } } },
  { 0x5E, { "dev_tty_ioctl", { "fcb", "cmd", "arg" } } },
  { 0x5F, { "dev_cd_open", { "fcb", "unused_path", "accessmode" } } },
  { 0x60, { "dev_cd_read", { "fcb", "dst", "len" } } },
  { 0x61, { "dev_cd_close", { "fcb" } } },
  { 0x62, { "dev_cd_firstfile", { "fcb", "unused_path", "direntry" } } },
  { 0x63, { "dev_cd_nextfile", { "fcb", "direntry" } } },
  { 0x64, { "dev_cd_chdir", { "fcb", "path" } } },
  { 0x65, { "dev_card_open", { "fcb", "unused_path", "accessmode" } } },
  { 0x66, { "dev_card_read", { "fcb", "dst", "len" } } },
  { 0x67, { "dev_card_write", { "fcb", "src", "len" } } },
  { 0x68, { "dev_card_close", { "fcb" } } },
  { 0x69, { "dev_card_firstfile", { "fcb", "unused_path", "direntry" } } },
  { 0x6A, { "dev_card_nextfile", { "fcb", "direntry" } } },
  { 0x6B, { "dev_card_erase", { "fcb", "unused_path" } } },
  { 0x6C, { "dev_card_undelete", { "fcb", "unused_path" } } },
  { 0x6D, { "dev_card_format", { "fcb" } } },
  { 0x6E, { "dev_card_rename", { "fcb1", "path1", "fcb2", "path2" } } },
  { 0x70, { "_bu_init", {} } },
  { 0x71, { "CdInit", {} } },
  { 0x72, { "CdRemove", {} } },
  { 0x78, { "CdAsyncSeekL", { "src" } } },
  { 0x7C, { "CdAsyncGetStatus", { "dst" } } },
  { 0x7E, { "CdAsyncReadSector", { "count", "dst", "mode" } } },
  { 0x81, { "CdAsyncSetMode", { "mode" } } },
  { 0x90, { "CdromIoIrqFunc1", {} } },
  { 0x91, { "CdromDmaIrqFunc1", {} } },
  { 0x92, { "CdromIoIrqFunc2", {} } },
  { 0x93, { "CdromDmaIrqFunc2", {} } },
  { 0x94, { "CdromGetInt5errCode", { "dst1", "dst2" } } },
  { 0x95, { "CdInitSubFunc", {} } },
  { 0x96, { "AddCDROMDevice", {} } },
  { 0x97, { "AddMemCardDevice", {} } },
  { 0x98, { "AddDuartTtyDevice", {} } },
  { 0x99, { "AddDummyTtyDevice", {} } },
  { 0x9C, { "SetConf", { "num_EvCB", "num_TCB", "stacktop" } } },
  { 0x9D, { "GetConf", { "num_EvCB_dst", "num_TCB_dst", "stacktop_dst" } } },
  { 0x9E, { "SetCdromIrqAutoAbort", { "type", "flag" } } },
  { 0x9F, { "SetMemSize", { "megabytes" } } },
  { 0xA1, { "BootFailed", {}, halt_system } },
};

const std::unordered_map<uint8_t, Function> B0 = {
  { 0x00, { "alloc_kernel_memory", { "size" } } },
  { 0x01, { "free_kernel_memory", { "buf" } } },
  { 0x02, { "init_timer", { "t", "reload", "flags" } } },
  { 0x03, { "get_timer", { "t" } } },
  { 0x04, { "enable_timer_irq", { "t" } } },
  { 0x05, { "disable_timer_irq", { "t" } } },
  { 0x06, { "restart_timer", { "t" } } },
  { 0x07, { "DeliverEvent", { "class", "spec" } } },
  { 0x08, { "OpenEvent", { "class", "spec", "mode", "func" } } },
  { 0x09, { "CloseEvent", { "event" } } },
  { 0x0A, { "WaitEvent", { "event" } } },
  { 0x0B, { "TestEvent", { "event" }, dont_log } },
  { 0x0C, { "EnableEvent", { "event" } } },
  { 0x0D, { "DisableEvent", { "event" } } },
  { 0x0E, { "OpenThread", { "reg_PC", "reg_SP_FP", "reg_GP" } } },
  { 0x0F, { "CloseThread", { "handle" } } },
  { 0x10, { "ChangeThread", { "handle" } } },
  { 0x11, { "jump_to_00000000h", {} } },
  { 0x12, { "InitPad", { "buf1", "siz1", "buf2", "siz2" } } },
  { 0x13, { "StartPad", {} } },
  { 0x14, { "StopPad", {} } },
  { 0x15, { "OutdatedPadInitAndStart", { "type", "button_dest", "unused", "unused" } } },
  { 0x16, { "OutdatedPadGetButtons", {} } },
  { 0x17, { "ReturnFromException", {}, dont_log } },
  { 0x18, { "SetDefaultExitFromException", {} } },
  { 0x19, { "SetCustomExitFromException", { "addr" } } },
  { 0x1A, { "SystemError", {}, halt_system } },
  { 0x1B, { "SystemError", {}, halt_system } },
  { 0x1C, { "SystemError", {}, halt_system } },
  { 0x1D, { "SystemError", {}, halt_system } },
  { 0x1E, { "SystemError", {}, halt_system } },
  { 0x1F, { "SystemError", {}, halt_system } },
  { 0x20, { "UnDeliverEvent", { "class", "spec" } } },
  { 0x21, { "SystemError", {}, halt_system } },
  { 0x22, { "SystemError", {}, halt_system } },
  { 0x23, { "SystemError", {}, halt_system } },
  { 0x2A, { "SystemError", {}, halt_system } },
  { 0x2B, { "SystemError", {}, halt_system } },
  { 0x32, { "FileOpen", { "filename", "accessmode" } } },
  { 0x33, { "FileSeek", { "fd", "offset", "seektype" } } },
  { 0x34, { "FileRead", { "fd", "dst", "length" } } },
  { 0x35, { "FileWrite", { "fd", "src", "length" } } },
  { 0x36, { "FileClose", { "fd" } } },
  { 0x37, { "FileIoctl", { "fd", "cmd", "arg" } } },
  { 0x38, { "exit", { "exitcode" } } },
  { 0x39, { "FileGetDeviceFlag", { "fd" } } },
  { 0x3A, { "FileGetc", { "fd" } } },
  { 0x3B, { "FilePutc", { "char, fd" } } },
  { 0x3C, { "std_in_getchar", {} } },
  { 0x3D, { "std_out_putchar", { "char" }, dbg_output_char } },
  { 0x3E, { "std_in_gets", { "dst" } } },
  { 0x3F, { "std_out_puts", { "src" }, dbg_output_string } },
  { 0x40, { "chdir", { "name" } } },
  { 0x41, { "FormatDevice", { "devicename" } } },
  { 0x42, { "firstfile", { "filename", "direntry" } } },
  { 0x43, { "nextfile", { "direntry" } } },
  { 0x44, { "FileRename", { "old_filename", "new_filename" } } },
  { 0x45, { "FileDelete", { "filename" } } },
  { 0x46, { "FileUndelete", { "filename" } } },
  { 0x47, { "AddDevice", { "device_inf" } } },
  { 0x48, { "RemoveDevice", { "device_name_lowercase" } } },
  { 0x49, { "PrintInstalledDevices", {} } },
  { 0x4A, { "InitCard", { "pad_enable" } } },
  { 0x4B, { "StartCard", {} } },
  { 0x4C, { "StopCard", {} } },
  { 0x4D, { "_card_info_subfunc", { "port" } } },
  { 0x4E, { "write_card_sector", { "port", "sector", "src" } } },
  { 0x4F, { "read_card_sector", { "port", "sector", "dst" } } },
  { 0x50, { "allow_new_card", {} } },
  { 0x51, { "Krom2RawAdd", { "shiftjis_code" } } },
  { 0x52, { "SystemError", {}, halt_system } },
  { 0x53, { "Krom2Offset", { "shiftjis_code" } } },
  { 0x54, { "GetLastError", {} } },
  { 0x55, { "GetLastFileError", { "fd" } } },
  { 0x56, { "GetC0Table", {} } },
  { 0x57, { "GetB0Table", {} } },
  { 0x58, { "get_bu_callback_port", {} } },
  { 0x59, { "testdevice", { "devicename" } } },
  { 0x5A, { "SystemError", {}, halt_system } },
  { 0x5B, { "ChangeClearPad", { "int" } } },
  { 0x5C, { "get_card_status", { "slot" } } },
  { 0x5D, { "wait_card_status", { "slot" } } },
};

const std::unordered_map<uint8_t, Function> C0 = {
  { 0x00, { "EnqueueTimerAndVblankIrqs", { "priority" } } },  // ;used with prio=1
  { 0x01, { "EnqueueSyscallHandler", { "priority" } } },      // ;used with prio=0
  { 0x02, { "SysEnqIntRP", { "priority", "struc" } } },       // ;bugged, use with care
  { 0x03, { "SysDeqIntRP", { "priority", "struc" } } },       // ;bugged, use with care
  { 0x04, { "get_free_EvCB_slot", {} } },
  { 0x05, { "get_free_TCB_slot", {} } },
  { 0x06, { "ExceptionHandler", {} } },
  { 0x07, { "InstallExceptionHandlers", {} } },  // ;destroys/uses k0/k1
  { 0x08, { "SysInitMemory", { "addr", "size" } } },
  { 0x09, { "SysInitKernelVariables", {} } },
  { 0x0A, { "ChangeClearRCnt", { "t", "flag" } } },
  { 0x0B, { "SystemError", {} } },
  { 0x0C, { "InitDefInt", { "priority" } } },  // ;used with prio=3
  { 0x0D, { "SetIrqAutoAck", { "irq", "flag" } } },
  { 0x12, { "InstallDevices", { "ttyflag" } } },
  { 0x13, { "FlushStdInOutPut", {} } },
  { 0x15, { "tty_cdevinput", { "circ", "char" } } },
  { 0x16, { "tty_cdevscan", {} } },
  { 0x17, { "tty_circgetc", { "circ" } } },  // ;uses r5 as garbage txt for ioabort
  { 0x18, { "tty_circputc", { "char", "circ" } } },
  { 0x19, { "ioabort", { "txt1", "txt2" } } },
  { 0x1A, { "set_card_find_mode", { "mode" } } },  // ;0=normal, 1=find deleted files
  { 0x1B, { "KernelRedirect", { "ttyflag" } } },   // ;PS2: ttyflag=1 causes SystemError
  { 0x1C, { "AdjustA0Table", {} } },
  { 0x1D, { "get_card_find_mode", {} } },
};

}  // namespace bios
