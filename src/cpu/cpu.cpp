#include <cpu/cpu.hpp>

#include <bios/functions.hpp>
#include <bus/bus.hpp>
#include <cpu/instruction.hpp>
#include <cpu/interrupt.hpp>
#include <cpu/opcode.hpp>
#include <memory/ram.hpp>
#include <util/log.hpp>

#include <climits>
#include <cstdio>
#include <iostream>
#include <string>

#define TRACE_NONE 0
#define TRACE_DISASM 1
#define TRACE_REGS 2
#define TRACE_PC_ONLY 3

#define TRACE_MODE TRACE_NONE

#define LOAD_EXE_HOOK 0

namespace cpu {

Cpu::Cpu(bus::Bus& bus) : m_bus(bus) {}

void Cpu::step(u32 cycles_to_execute) {
  for (u32 cycle = 0; cycle < cycles_to_execute; cycle += APPROX_CYCLES_PER_INSTRUCTION) {
#if LOAD_EXE_HOOK
    // mid-boot hook to load an executable
    if (m_pc == 0x80030000) {
      memory::PSEXELoadInfo psxexe_load_info;
      if (m_bus.m_ram.load_executable(psxexe_load_info)) {
        set_pc(psxexe_load_info.pc);
        m_gpr[28] = psxexe_load_info.r28;
        m_gpr[29] = psxexe_load_info.r29_r30;
        m_gpr[30] = psxexe_load_info.r29_r30;
      }
    }
#endif

#ifdef LOG_BIOS_CALLS
    bool was_branch_cycle = m_branch_taken_saved;
#endif

    // Store state for potential exceptions (and reset current state)
    store_exception_state();

    // Check for interrupts and trigger an exception if any
    m_bus.m_interrupts.check_and_trigger();

    // Fetch current instruction
    u32 cur_instr;
    if (!load32(m_pc, cur_instr)) {
      LOG_CRITICAL("PC unaligned: {:08X}", m_pc);
      continue;
    }

    // Decode current instruction
    const Instruction instr(cur_instr);

    if (instr.opcode() == Opcode::INVALID) {
      LOG_CRITICAL("Invalid instruction {:02X}", cur_instr);
      trigger_exception(ExceptionCause::ReservedInstruction);
      return;
    }

#if TRACE_MODE == TRACE_REGS  // Log all registers
    char debug_str[512];
    // This is ugly but much faster than a loop
    // clang-format off
    std::snprintf(debug_str, 512, "[%08X]: at:%X v0:%X v1:%X a0:%X a1:%X a2:%X a3:%X t0:%X t2:%X t3:%X t4:%X t5:%X t6:%X t7:%X s0:%X s1:%X s2:%X s3:%X s4:%X s5:%X s6:%X s7:%X t8:%X t9:%X k0:%X k1:%X gp:%X sp:%X fp:%X ra:%X hi:%X lo:%X",
      m_pc, m_gpr[1], m_gpr[2], m_gpr[3], m_gpr[4], m_gpr[5], m_gpr[6], m_gpr[7], m_gpr[8], m_gpr[10], m_gpr[11], m_gpr[12], m_gpr[13], m_gpr[14], m_gpr[15], m_gpr[16], m_gpr[17], m_gpr[18], m_gpr[19], m_gpr[20], m_gpr[21], m_gpr[22], m_gpr[23], m_gpr[24], m_gpr[25], m_gpr[26], m_gpr[27], m_gpr[28], m_gpr[29], m_gpr[30], m_gpr[31], m_hi, m_lo);
    // clang-format on
    LOG_CPU_NOFMT(debug_str);
#elif TRACE_MODE == TRACE_DISASM   // Log instruction disassembly
    LOG_CPU("[{:08X}]: {:08X} {}", m_pc, cur_instr, instr.disassemble());
    // In PC_ONLY mode we print the PC-4 for branch delay slot instructions (to match no$psx's output)
#elif TRACE_MODE == TRACE_PC_ONLY  // Log PC register only
    LOG_CPU("{:08X}", m_pc);
#endif

    // Advance PC
    set_pc(m_pc_next);

    // Ensures(m_gpr[0] == 0);
    // Execute instruction
    execute_instruction(instr);
    //  Ensures(m_gpr[0] == 0);

    do_pending_load();

#ifdef LOG_BIOS_CALLS
    if (was_branch_cycle) {
      const auto masked_pc = m_pc_current & 0x1FFFFF;

      // If we jumped to a BIOS function
      if (masked_pc == 0xA0 || masked_pc == 0xB0 || masked_pc == 0xC0)
        on_bios_call(masked_pc);
    }
#endif
  }
}

void Cpu::execute_instruction(const Instruction& i) {
  switch (i.opcode()) {
      // Arithmetic
    case Opcode::ADD: op_add(i); break;
    case Opcode::ADDU: set_rd(i, rs(i) + rt(i)); break;
    case Opcode::SUB: op_sub(i); break;
    case Opcode::SUBU: set_rd(i, rs(i) - rt(i)); break;
    case Opcode::ADDI: op_addi(i); break;
    case Opcode::ADDIU: set_rt(i, rs(i) + i.imm16_se()); break;
    case Opcode::DIV: op_sdiv(i); break;
    case Opcode::DIVU: op_udiv(i); break;
    case Opcode::MULT: op_mult(i); break;
    case Opcode::MULTU: op_multu(i); break;
    // Comparison
    case Opcode::SLT: set_rd(i, (s32)rs(i) < (s32)rt(i) ? 1 : 0); break;
    case Opcode::SLTU: set_rd(i, rs(i) < rt(i) ? 1 : 0); break;
    case Opcode::SLTI: set_rt(i, (s32)rs(i) < (s32)i.imm16_se() ? 1 : 0); break;
    case Opcode::SLTIU: set_rt(i, rs(i) < (u32)i.imm16_se() ? 1 : 0); break;
    // Shifting
    case Opcode::LUI: set_rt(i, i.imm16() << 16); break;
    case Opcode::SLLV: set_rd(i, rt(i) << (rs(i) & 0x1F)); break;
    case Opcode::SRLV: set_rd(i, rt(i) >> (rs(i) & 0x1F)); break;
    case Opcode::SRAV: set_rd(i, (s32)rt(i) >> (rs(i) & 0x1F)); break;
    case Opcode::SLL: set_rd(i, rt(i) << i.imm5()); break;
    case Opcode::SRL: set_rd(i, rt(i) >> i.imm5()); break;
    case Opcode::SRA: set_rd(i, (s32)rt(i) >> i.imm5()); break;
    // Logical
    case Opcode::ANDI: set_rt(i, rs(i) & i.imm16()); break;
    case Opcode::ORI: set_rt(i, rs(i) | i.imm16()); break;
    case Opcode::XORI: set_rt(i, rs(i) ^ i.imm16()); break;
    case Opcode::AND: set_rd(i, rs(i) & rt(i)); break;
    case Opcode::OR: set_rd(i, rs(i) | rt(i)); break;
    case Opcode::XOR: set_rd(i, rs(i) ^ rt(i)); break;
    case Opcode::NOR: set_rd(i, ~(rs(i) | rt(i))); break;
    // Memory operations (loads)
    case Opcode::LBU: op_lbu(i); break;
    case Opcode::LB: op_lb(i); break;
    case Opcode::LHU: op_lhu(i); break;
    case Opcode::LH: op_lh(i); break;
    case Opcode::LW: op_lw(i); break;
    case Opcode::LWL: op_lwl(i); break;
    case Opcode::LWR: op_lwr(i); break;
    // Memory operations (stores)
    case Opcode::SB: op_sb(i); break;
    case Opcode::SH: op_sh(i); break;
    case Opcode::SW: op_sw(i); break;
    case Opcode::SWL: op_swl(i); break;
    case Opcode::SWR: op_swr(i); break;
    // Jumps/Branches
    case Opcode::J: op_j(i); break;
    case Opcode::JR: op_jr(i); break;
    case Opcode::JAL: op_jal(i); break;
    case Opcode::JALR: op_jalr(i); break;
    case Opcode::BEQ:
      m_in_branch_delay_slot = true;
      if (rs(i) == rt(i))
        op_branch(i);
      break;
    case Opcode::BNE:
      m_in_branch_delay_slot = true;
      if (rs(i) != rt(i))
        op_branch(i);
      break;
    case Opcode::BGTZ:
      m_in_branch_delay_slot = true;
      if ((s32)rs(i) > 0)
        op_branch(i);
      break;
    case Opcode::BLEZ:
      m_in_branch_delay_slot = true;
      if ((s32)rs(i) <= 0)
        op_branch(i);
      break;
    case Opcode::BCONDZ: {
      m_in_branch_delay_slot = true;

      // Format of rt is "X0000Y", where:
      // - X is set if we need to link,
      // - Y is set to the inverse of the branch target's sign bit
      const u8 opcode_2 = i.rt();
      const bool should_link = ((opcode_2 & 0x1E) == 0x10);
      const bool should_branch = ((s32)(rs(i) ^ (opcode_2 << 31)) < 0);

      if (should_link)
        gpr(31) = m_pc_next;

      if (should_branch)
        op_branch(i);
      break;
    }
    // Syscall/Breakpoint
    case Opcode::SYSCALL: op_syscall(i); break;
    case Opcode::BREAK: trigger_exception(ExceptionCause::Breakpoint); break;
    // Co-processor 0
    case Opcode::MTC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_BPC: m_cop0_bpc = rt(i); goto unhandled_mtc;
        case Cop0Register::COP0_BDA: m_cop0_bda = rt(i); goto unhandled_mtc;
        case Cop0Register::COP0_DCIC: m_cop0_dcic = rt(i); break;
        case Cop0Register::COP0_BDAM: m_cop0_bdam = rt(i); goto unhandled_mtc;
        case Cop0Register::COP0_BPCM: m_cop0_bpcm = rt(i); goto unhandled_mtc;
        case Cop0Register::COP0_SR: m_cop0_status.word = rt(i); break;
        case Cop0Register::COP0_CAUSE: m_cop0_cause.word = rt(i); break;
        case Cop0Register::COP0_EPC:
          m_cop0_epc = rt(i);
          break;

        unhandled_mtc:
        default: LOG_WARN("Unhandled Cop1 register {} write", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFC0: {
      const auto cop_dst_reg = static_cast<Cop0Register>(i.rd());
      switch (cop_dst_reg) {
        case Cop0Register::COP0_BPC: issue_delayed_load(i.rt(), m_cop0_bpc); goto unhandled_mfc;
        case Cop0Register::COP0_BDA: issue_delayed_load(i.rt(), m_cop0_bda); goto unhandled_mfc;
        case Cop0Register::COP0_JUMPDEST: issue_delayed_load(i.rt(), m_cop0_jumpdest); break;
        case Cop0Register::COP0_DCIC: issue_delayed_load(i.rt(), m_cop0_dcic); break;
        case Cop0Register::COP0_BAD_VADDR: issue_delayed_load(i.rt(), m_cop0_bad_vaddr); break;
        case Cop0Register::COP0_BDAM: issue_delayed_load(i.rt(), m_cop0_bdam); goto unhandled_mfc;
        case Cop0Register::COP0_BPCM: issue_delayed_load(i.rt(), m_cop0_bpcm); goto unhandled_mfc;
        case Cop0Register::COP0_SR: issue_delayed_load(i.rt(), m_cop0_status.word); break;
        case Cop0Register::COP0_CAUSE: issue_delayed_load(i.rt(), m_cop0_cause.word); break;
        case Cop0Register::COP0_EPC: issue_delayed_load(i.rt(), m_cop0_epc); break;
        case Cop0Register::COP0_PRID:
          issue_delayed_load(i.rt(), 0x2);  // CXD8606CQ CPU
          goto unhandled_mfc;

        unhandled_mfc:
        default: LOG_WARN("Unhandled Cop0 register {} read", static_cast<u32>(cop_dst_reg));
      }
      break;
    }
    case Opcode::MFLO: set_rd(i, m_lo); break;
    case Opcode::MFHI: set_rd(i, m_hi); break;
    case Opcode::MTLO: m_lo = rs(i); break;
    case Opcode::MTHI: m_hi = rs(i); break;
    case Opcode::RFE: op_rfe(i); break;

    default: LOG_ERROR("Unimplemented instruction executed"); assert(0);
  }
}

void Cpu::on_bios_call(u32 masked_pc) {
  std::unordered_map<uint8_t, bios::Function>::const_iterator function;
  const u8 func_number = gpr(9);
  const auto type = masked_pc >> 4;
  bool is_known_func = false;
  bool log_known_func = true;

  if (masked_pc == 0xA0) {
    if ((function = bios::A0.find(func_number)) != bios::A0.end()) {
      is_known_func = true;
      if (function->second.callback != nullptr)
        log_known_func = function->second.callback(*this);
    }
  } else if (masked_pc == 0xB0) {
    if ((function = bios::B0.find(func_number)) != bios::B0.end()) {
      is_known_func = true;
      if (function->second.callback != nullptr)
        log_known_func = function->second.callback(*this);
    }
  } else {
    if ((function = bios::C0.find(func_number)) != bios::C0.end()) {
      is_known_func = true;
      if (function->second.callback != nullptr)
        log_known_func = function->second.callback(*this);
    }
  }

  if (!is_known_func) {
    m_bios_calls_log += fmt::format("[{:08X}] {:01X}({:02X})\n", gpr(31), type, func_number);
    return;
  }

  if (log_known_func) {
    const bios::Function func = function->second;

    const auto arg_count = func.args.size();
    // Arguments after the 4th are passed on the stack, unimplemented
    Ensures(arg_count <= 4);

    std::string log_text =
        fmt::format("[{:08X}] {:01X}({:02X}): {}(", gpr(31), type, func_number, func.name);

    for (auto i = 0; i < arg_count; ++i) {
      log_text += fmt::format("{}=0x{:X}{}", func.args[i], gpr(4 + i), i == (arg_count - 1) ? "" : ", ");
    }
    log_text += ")\n";

    m_bios_calls_log += log_text;
  }
}

void Cpu::store_exception_state() {
  // Store state that we'll need if an exception happens
  m_pc_current = m_pc;
  m_in_branch_delay_slot_saved = m_in_branch_delay_slot;
  m_branch_taken_saved = m_branch_taken;

  // Reset state so that we can use/set it appropriately on the current cycle
  m_in_branch_delay_slot = false;
  m_branch_taken = false;
}

void Cpu::trigger_exception(ExceptionCause cause) {
  // Interrupt vectors for general interrupts and exceptions
  constexpr auto EXCEPTION_VECTOR_GENERAL_RAM = 0x80000080u;
  constexpr auto EXCEPTION_VECTOR_GENERAL_ROM = 0xBFC00180u;
  // Interrupt vector for breakpoints
  constexpr auto EXCEPTION_VECTOR_BREAKPOINT = 0x80000040u;

  u32 handler_addr;
  if (cause == ExceptionCause::Breakpoint)
    handler_addr = EXCEPTION_VECTOR_BREAKPOINT;
  else
    handler_addr = m_cop0_status.boot_exception_vectors ? EXCEPTION_VECTOR_GENERAL_ROM
                                                        : EXCEPTION_VECTOR_GENERAL_RAM;

  // Shift bits [5:0] of the Status Register two bits to the left (reverse of RFE instruction)
  // This has the effect of disabling interrupts and enabling kernel mode.
  // The last two bits in [5:4] are discarded, it's up to the kernel to handle more exception recursion
  // levels.
  m_cop0_status.word = (m_cop0_status.word & ~0b111111u) | ((m_cop0_status.word << 2) & 0b111111);

  // Clear CAUSE except for the pending interrupt bit
  m_cop0_cause.word &= ~0xFFFF00FF;

  m_cop0_cause.word |= (static_cast<u8>(cause) << 2);

  // Update EPC with the return address
  if (cause == ExceptionCause::Interrupt)
    m_cop0_epc = m_pc;  // on interrupt this is the next PC
  else
    m_cop0_epc = m_pc_current;  // on everything else, the instruction that caused the exception

  if (m_in_branch_delay_slot_saved) {
    m_cop0_epc -= 4;  // need to set this to the branch instruction if we're in a branch delay slot
    m_cop0_cause.branch_delay = true;  // also set this CAUSE bit which indicates this edge case

    if (m_branch_taken_saved)
      m_cop0_cause.branch_delay_taken = true;  // Another edge case, if the branch was actually taken

    m_cop0_jumpdest = m_pc;  // Update JUMPDEST ()
  }

  if (cause == ExceptionCause::Breakpoint)
    m_cop0_dcic |= 1;

  // Exceptions don't have a branch delay, jump directly to the handler
  set_pc(handler_addr);
}

void Cpu::trigger_load_exception(const address addr) {
  m_cop0_bad_vaddr = addr;
  trigger_exception(ExceptionCause::LoadAddressError);
}

void Cpu::trigger_store_exception(const address addr) {
  m_cop0_bad_vaddr = addr;
  trigger_exception(ExceptionCause::StoreAddressError);
}

void Cpu::op_add(const Instruction& i) {
  u32 res;
  if (checked_add(rs(i), rt(i), res))
    set_rd(i, res);
}

void Cpu::op_sub(const Instruction& i) {
  u32 res;
  if (checked_sub(rs(i), rt(i), res))
    set_rd(i, res);
}

void Cpu::op_addi(const Instruction& i) {
  u32 res;
  if (checked_add(rs(i), i.imm16_se(), res))
    set_rt(i, res);
}

void Cpu::op_sb(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  return store8(addr, (u8)rt(i));
}

void Cpu::op_sh(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  store16(addr, (u16)rt(i));
}

void Cpu::op_sw(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  store32(addr, rt(i));
}

void Cpu::op_swl(const Instruction& i) {
  const address addr = rs(i) + i.imm16_se();

  // Load aligned word
  const auto aligned_addr = addr & ~0b11u;
  u32 cur_mem;
  if (!load32(aligned_addr, cur_mem))
    return;

  const Register t = rt(i);

  // Depending on the alignment, fetch different number of bytes
  u32 val{};
  switch (addr & 0b11u) {
    case 0: val = cur_mem & 0xFFFFFF00 | t >> 24; break;
    case 1: val = cur_mem & 0xFFFF0000 | t >> 16; break;
    case 2: val = cur_mem & 0xFF000000 | t >> 8; break;
    case 3: val = cur_mem & 0x00000000 | t >> 0; break;
  }
  store32(aligned_addr, val);
}

void Cpu::op_swr(const Instruction& i) {
  const address addr = rs(i) + i.imm16_se();

  // Load aligned word
  const auto aligned_addr = addr & ~0b11u;
  u32 cur_mem;
  if (!load32(aligned_addr, cur_mem))
    return;

  const Register t = rt(i);

  // Depending on the alignment, fetch different number of bytes
  u32 val{};
  switch (addr & 0b11u) {
    case 0: val = cur_mem & 0x00000000 | t << 0; break;
    case 1: val = cur_mem & 0x000000FF | t << 8; break;
    case 2: val = cur_mem & 0x0000FFFF | t << 16; break;
    case 3: val = cur_mem & 0x00FFFFFF | t << 24; break;
  }
  store32(aligned_addr, val);
}

void Cpu::op_lbu(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  u8 val;
  load8(addr, val);
  issue_delayed_load(i.rt(), val);
}

void Cpu::op_lb(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  u8 val;
  load8(addr, val);
  issue_delayed_load(i.rt(), (s8)val);
}

void Cpu::op_lhu(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  u16 val;
  if (!load16(addr, val))
    return;
  issue_delayed_load(i.rt(), val);
}

void Cpu::op_lh(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  u16 val;
  if (!load16(addr, val))
    return;
  issue_delayed_load(i.rt(), (s16)val);
}

void Cpu::op_lw(const Instruction& i) {
  const address addr = i.imm16_se() + rs(i);
  u32 val;
  if (!load32(addr, val))
    return;
  issue_delayed_load(i.rt(), val);
}

void Cpu::op_lwl(const Instruction& i) {
  const address addr = rs(i) + i.imm16_se();

  Register cur_v;
  if (m_slot_current.reg == i.rt())
    cur_v = m_slot_current.val;  // No load delay on consecutive LWLs with the same RT
  else
    cur_v = rt(i);

  // Load aligned word
  const auto aligned_addr = addr & ~0b11u;
  u32 aligned_word;
  if (!load32(aligned_addr, aligned_word))
    return;

  // Depending on the alignment, fetch different number of bytes
  u32 val{};
  switch (addr & 0b11u) {
    case 0: val = cur_v & 0x00FFFFFF | aligned_word << 24; break;
    case 1: val = cur_v & 0x0000FFFF | aligned_word << 16; break;
    case 2: val = cur_v & 0x000000FF | aligned_word << 8; break;
    case 3: val = cur_v & 0x00000000 | aligned_word << 0; break;
  }
  issue_delayed_load(i.rt(), val);
}

void Cpu::op_lwr(const Instruction& i) {
  const address addr = rs(i) + i.imm16_se();

  Register cur_v;
  if (m_slot_current.reg == i.rt())
    cur_v = m_slot_current.val;  // No load delay on consecutive LWRs with the same RT
  else
    cur_v = rt(i);

  // Load aligned word
  const auto aligned_addr = addr & ~0b11u;
  u32 aligned_word;
  if (!load32(aligned_addr, aligned_word))
    return;

  // Depending on the alignment, fetch different number of bytes
  u32 val{};
  switch (addr & 0b11u) {
    case 0: val = cur_v & 0x00000000 | aligned_word >> 0; break;
    case 1: val = cur_v & 0xFF000000 | aligned_word >> 8; break;
    case 2: val = cur_v & 0xFFFF0000 | aligned_word >> 16; break;
    case 3: val = cur_v & 0xFFFFFF00 | aligned_word >> 24; break;
  }
  issue_delayed_load(i.rt(), val);
}

void Cpu::op_j(const Instruction& i) {
  m_in_branch_delay_slot = true;
  const address addr = m_pc_next & 0xF0000000 | (i.imm26() << 2);

  set_pc_next(addr);
}

void Cpu::op_jr(const Instruction& i) {
  m_in_branch_delay_slot = true;
  const address addr = rs(i);

  if (addr % 4 != 0) {
    trigger_load_exception(addr);
    return;
  }
  set_pc_next(addr);

#if LOG_TTY_OUTPUT_WITH_HOOK
  // Hook std_out_putchar function in the B0 kernel table. Assumes there's an ADDIU/"LI" instruction
  // right after the JR, containing the kernel procedure vector.
  if (m_pc_next == 0xB0) {
    u32 instr_word;
    if (!load32(m_pc, instr_word))
      return;
    const Instruction next_instr(instr_word);
    Ensures(next_instr.opcode() == Opcode::ADDIU);

    if (next_instr.imm16_se() == 0x3D) {
      char tty_out_char = gpr(4);
      m_tty_out += tty_out_char;
      //          std::cout << tty_out_char;
    }
  }
#endif
}

void Cpu::op_jal(const Instruction& i) {
  m_in_branch_delay_slot = true;

  gpr(31) = m_pc_next;
  const address addr = m_pc_next & 0xF0000000 | (i.imm26() << 2);

  set_pc_next(addr);
}

void Cpu::op_jalr(const Instruction& i) {
  m_in_branch_delay_slot = true;

  const address addr = rs(i);
  set_rd(i, m_pc_next);

  if (addr % 4 != 0) {
    trigger_load_exception(addr);
    return;
  }
  set_pc_next(addr);
}

void Cpu::op_mult(const Instruction& i) {
  const auto a = (s64)(s32)rs(i);
  const auto b = (s64)(s32)rt(i);
  const auto result = (u64)(a * b);

  m_hi = result >> 32;
  m_lo = (u32)result;
}

void Cpu::op_multu(const Instruction& i) {
  const auto result = (u64)rs(i) * (u64)rt(i);
  m_hi = result >> 32;
  m_lo = (u32)result;
}

void Cpu::op_branch(const Instruction& i) {
  const address addr = m_pc + (i.imm16_se() << 2);

  set_pc_next(addr);
}

void Cpu::op_udiv(const Instruction& i) {
  const u32 numerator = rs(i);
  const u32 denominator = rt(i);

  if (denominator == 0) {
    m_lo = 0xFFFFFFFF;
    m_hi = numerator;
  } else {
    m_lo = numerator / denominator;
    m_hi = numerator % denominator;
  }
}

void Cpu::op_sdiv(const Instruction& i) {
  const s32 numerator = rs(i);
  const s32 denominator = rt(i);

  if (denominator == 0) {
    if (numerator >= 0)
      m_lo = 0xFFFFFFFF;
    else
      m_lo = 1;
    m_hi = numerator;
  } else if (denominator == 0xFFFFFFFF && numerator == 0x80000000) {
    m_lo = 0x80000000;
    m_hi = 0;
  } else {
    m_lo = numerator / denominator;
    m_hi = numerator % denominator;
  }
}

void Cpu::op_syscall(const Instruction& i) {
  const u32 syscall_function = gpr(4);

  switch (syscall_function) {
    case 0: LOG_DEBUG("SYSCALL(0x00) NoFunction()"); break;
    case 1: LOG_DEBUG("SYSCALL(0x01) EnterCriticalSection()"); break;
    case 2: LOG_DEBUG("SYSCALL(0x02) ExitCriticalSection()"); break;
    case 3: LOG_DEBUG("SYSCALL(0x03) ChangeThreadSubFunction({:08X})", gpr(5)); break;
    default: LOG_DEBUG("SYSCALL(0x{:02X} DeliverEvent(F0000010h, 4000h)", syscall_function);
  }

  trigger_exception(ExceptionCause::Syscall);
}

void Cpu::op_rfe(const Instruction& i) {
  // Restore the mode before the exception by shifting the Interrupt Enable / User Mode stack back to its
  // original position.
  m_cop0_status.word = (m_cop0_status.word & ~0b1111u) | ((m_cop0_status.word >> 2) & 0xF);
}

bool Cpu::checked_add(u32 op1, u32 op2, u32& out) {
  u32 result = op1 + op2;
  if (~(op1 ^ op2) & (op1 ^ result) & 0x80000000) {
    trigger_exception(ExceptionCause::Overflow);
    return false;
  }
  out = result;
  return true;
}

bool Cpu::checked_sub(u32 op1, u32 op2, u32& out) {
  u32 result = op1 - op2;
  if ((op1 ^ op2) & (op1 ^ result) & 0x80000000) {
    trigger_exception(ExceptionCause::Overflow);
    return false;
  }
  out = result;
  return true;
}

bool Cpu::load32(u32 addr, u32& out_val) {
  if (addr % 4 != 0) {
    trigger_load_exception(addr);
    return false;
  }
  out_val = m_bus.read32(addr);
  return true;
}

bool Cpu::load16(u32 addr, u16& out_val) {
  if (addr % 2 != 0) {
    trigger_load_exception(addr);
    return false;
  }
  out_val = m_bus.read16(addr);
  return true;
}

void Cpu::load8(u32 addr, u8& out_val) {
  out_val = m_bus.read8(addr);
}

void Cpu::store32(u32 addr, u32 val) {
  if (addr % 4 != 0) {
    trigger_store_exception(addr);
    return;
  }
  if (m_cop0_status.isolate_cache) {
    LOG_TRACE("Ignoring write 0x{:08X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write32(addr, val);
}

void Cpu::store16(u32 addr, u16 val) {
  if (addr % 2 != 0) {
    trigger_store_exception(addr);
    return;
  }
  if (m_cop0_status.isolate_cache) {
    LOG_TRACE("Ignoring write 0x{:04X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write16(addr, val);
}

void Cpu::store8(u32 addr, u8 val) {
  if (m_cop0_status.isolate_cache) {
    LOG_TRACE("Ignoring write 0x{:02X} to 0x{:08X} due to cache isolation", val, addr);
    return;
  }
  m_bus.write8(addr, val);
}

void Cpu::issue_delayed_load(RegisterIndex reg, u32 val) {
  if (reg == 0)
    return;
  invalidate_reg(reg);

  m_slot_next.reg = reg;
  m_slot_next.val = val;
  m_slot_next.val_prev = gpr(reg);
}

void Cpu::do_pending_load() {
  if (m_slot_current.is_valid()) {
    const auto cur_reg = m_slot_current.reg;

    if (gpr(cur_reg) == m_slot_current.val_prev)
      set_gpr(cur_reg, m_slot_current.val);
  }

  m_slot_current = m_slot_next;
  m_slot_next.invalidate();
}

void Cpu::invalidate_reg(RegisterIndex r) {
  if (m_slot_current.reg == r)
    // Consecutive writes to the same register results in them getting lost (except the last)
    m_slot_current.invalidate();
}

}  // namespace cpu
