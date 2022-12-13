#if !defined(DK_VM_H)
#define DK_VM_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DK_VM_MAX_STACK_SIZE (1 << 16) /* 64KB stack size */

  static uint16_t dk_vm_mem[DK_VM_MAX_STACK_SIZE];

  int dk_vm_running = 0;

  enum
  {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
  };

  uint16_t dk_vm_reg[R_COUNT];

  enum
  {
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RES,
    OP_LEA,
    OP_TRAP
  };

  enum
  {
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2  /* N */
  };

  enum
  {
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
  };

  enum
  {
    TRAP_GETC = 0x20,
    TRAP_OUT = 0x21,
    TRAP_PUTS = 0x22,
    TRAP_IN = 0x23,
    TRAP_PUTSP = 0x24,
    TRAP_HALT = 0x25,
    TRAP_RTI = 0x26,
    TRAP_XGETC = 0x27,
    TRAP_XOUT = 0x28,
    TRAP_XPUTS = 0x29
  };

  uint16_t dk_vm_mem_read(uint16_t address);

  void dk_vm_mem_write(uint16_t address, uint16_t value);

  void dk_vm_update_flags(uint16_t r);

  void dk_update_flags(uint16_t r);

  void dk_vm_cycle();

  uint16_t dk_vm_mem_read(uint16_t address);

  void dk_handle_interrupt(int signal);

  int dk_read_image(const char* image_path);

  int dk_check_key();

  uint16_t dk_sign_extend(uint16_t x, int bit_count);

  uint16_t dk_swap16(uint16_t x);

  void dk_add(uint16_t instr);

  void dk_and(uint16_t instr);

  void dk_not(uint16_t instr);

  void dk_br(uint16_t instr);

  void dk_jmp(uint16_t instr);

  void dk_jsr(uint16_t instr);

  void dk_ld(uint16_t instr);

  void dk_ldi(uint16_t instr);

  void dk_ldr(uint16_t instr);

  void dk_lea(uint16_t instr);

  void dk_st(uint16_t instr);

  void dk_sti(uint16_t instr);

  void dk_str(uint16_t instr);

  void dk_trap(uint16_t instr);

  void dk_trap_getc();

  void dk_trap_out();

  void dk_trap_puts();

  void dk_trap_in();

  void dk_trap_putsp();

  void dk_trap_halt();

  void dk_trap_rti();

  void dk_trap_xgetc();

  void dk_trap_xout();

  void dk_trap_xputs();

#if defined(DK_VM_IMPLEMENTATION)

  uint16_t dk_vm_mem_read(uint16_t address)
  {
    if (address == MR_KBSR) {
      if (dk_check_key()) {
        dk_vm_mem[MR_KBSR] = (1 << 15);
        dk_vm_mem[MR_KBDR] = getchar();
      } else {
        dk_vm_mem[MR_KBSR] = 0;
      }
    }
    return dk_vm_mem[address];
  }

  void dk_vm_mem_write(uint16_t address, uint16_t value)
  {
    dk_vm_mem[address] = value;
  }

  void dk_vm_update_flags(uint16_t r)
  {
    if (dk_vm_reg[r] == 0) {
      dk_vm_reg[R_COND] = FL_ZRO;
    } else if (dk_vm_reg[r] >> 15) {
      /* a 1 in the left-most bit indicates negative */
      dk_vm_reg[R_COND] = FL_NEG;
    } else {
      dk_vm_reg[R_COND] = FL_POS;
    }
  }

  void dk_vm_cycle()
  {
    uint16_t instr = dk_vm_mem_read(dk_vm_reg[R_PC]++);
    uint16_t op = instr >> 12;

    switch (op) {
      case OP_ADD:
        dk_add(instr);
        break;
      case OP_AND:
        dk_and(instr);
        break;
      case OP_NOT:
        dk_not(instr);
        break;
      case OP_BR:
        dk_br(instr);
        break;
      case OP_JMP:
        dk_jmp(instr);
        break;
      case OP_JSR:
        dk_jsr(instr);
        break;
      case OP_LD:
        dk_ld(instr);
        break;
      case OP_LDI:
        dk_ldi(instr);
        break;
      case OP_LDR:
        dk_ldr(instr);
        break;
      case OP_LEA:
        dk_lea(instr);
        break;
      case OP_ST:
        dk_st(instr);
        break;
      case OP_STI:
        dk_sti(instr);
        break;
      case OP_STR:
        dk_str(instr);
        break;
      case OP_TRAP:
        dk_trap(instr);
        break;
      case OP_RES:
      case OP_RTI:
      default:
        abort();
        break;
    }
  }

  void dk_handle_interrupt(int signal)
  {
    if (signal == SIGINT) {
      dk_vm_running = 0;
    }
  }

  int dk_read_image(const char* image_path)
  {
    FILE* file = fopen(image_path, "rb");
    if (!file) {
      return 0;
    }

    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = dk_swap16(origin);

    uint16_t max_read = UINT16_MAX - origin;
    uint16_t* p = dk_vm_mem + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    while (read-- > 0) {
      *p = dk_swap16(*p);
      ++p;
    }

    fclose(file);
    return 1;
  }

  int dk_check_key()
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(1, &readfds, NULL, NULL, &timeout) != 0;
  }

  uint16_t dk_sign_extend(uint16_t x, int bit_count)
  {
    if ((x >> (bit_count - 1)) & 1) {
      x |= (0xFFFF << bit_count);
    }
    return x;
  }

  uint16_t dk_swap16(uint16_t x) { return (x << 8) | (x >> 8); }

  void dk_and(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag) {
      uint16_t imm5 = dk_sign_extend(instr & 0x1F, 5);
      dk_vm_reg[r0] = dk_vm_reg[r1] & imm5;
    } else {
      uint16_t r2 = instr & 0x7;
      dk_vm_reg[r0] = dk_vm_reg[r1] & dk_vm_reg[r2];
    }

    dk_update_flags(r0);
  }

  void dk_add(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag) {
      uint16_t imm5 = dk_sign_extend(instr & 0x1F, 5);
      dk_vm_reg[r0] = dk_vm_reg[r1] + imm5;
    } else {
      uint16_t r2 = instr & 0x7;
      dk_vm_reg[r0] = dk_vm_reg[r1] + dk_vm_reg[r2];
    }

    dk_update_flags(r0);
  }

  void dk_not(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;

    dk_vm_reg[r0] = ~dk_vm_reg[r1];

    dk_update_flags(r0);
  }

  void dk_br(uint16_t instr)
  {
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;

    if (cond_flag & dk_vm_reg[R_COND]) {
      dk_vm_reg[R_PC] += pc_offset;
    }
  }

  void dk_jmp(uint16_t instr)
  {
    uint16_t r1 = (instr >> 6) & 0x7;
    dk_vm_reg[R_PC] = dk_vm_reg[r1];
  }

  void dk_jsr(uint16_t instr)
  {
    uint16_t long_flag = (instr >> 11) & 1;

    dk_vm_reg[R_R7] = dk_vm_reg[R_PC];

    if (long_flag) {
      uint16_t long_pc_offset = dk_sign_extend(instr & 0x7FF, 11);
      dk_vm_reg[R_PC] += long_pc_offset;
    } else {
      uint16_t r1 = (instr >> 6) & 0x7;
      dk_vm_reg[R_PC] = dk_vm_reg[r1];
    }
  }

  void dk_ld(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);

    dk_vm_reg[r0] = dk_vm_mem_read(dk_vm_reg[R_PC] + pc_offset);

    dk_update_flags(r0);
  }

  void dk_ldi(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);

    dk_vm_reg[r0] = dk_vm_mem_read(dk_vm_mem_read(dk_vm_reg[R_PC] + pc_offset));

    dk_update_flags(r0);
  }

  void dk_ldr(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = dk_sign_extend(instr & 0x3F, 6);

    dk_vm_reg[r0] = dk_vm_mem_read(dk_vm_reg[r1] + offset);

    dk_update_flags(r0);
  }

  void dk_lea(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);

    dk_vm_reg[r0] = dk_vm_reg[R_PC] + pc_offset;

    dk_update_flags(r0);
  }

  void dk_st(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);

    dk_vm_mem_write(dk_vm_reg[R_PC] + pc_offset, dk_vm_reg[r0]);
  }

  void dk_sti(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = dk_sign_extend(instr & 0x1FF, 9);

    dk_vm_mem_write(dk_vm_mem_read(dk_vm_reg[R_PC] + pc_offset), dk_vm_reg[r0]);
  }

  void dk_str(uint16_t instr)
  {
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = dk_sign_extend(instr & 0x3F, 6);

    dk_vm_mem_write(dk_vm_reg[r1] + offset, dk_vm_reg[r0]);
  }

  void dk_trap(uint16_t instr)
  {
    switch (instr & 0xFF) {
      case TRAP_GETC:
        dk_vm_reg[R_R0] = (uint16_t)getchar();
        break;
      case TRAP_OUT:
        putc((char)dk_vm_reg[R_R0], stdout);
        fflush(stdout);
        break;
      case TRAP_PUTS: {
        uint16_t* c = dk_vm_mem + dk_vm_reg[R_R0];
        while (*c) {
          putc((char)*c, stdout);
          ++c;
        }
        fflush(stdout);
      } break;
      case TRAP_IN: {
        printf("Enter a character: ");
        char c = getchar();
        putc(c, stdout);
        dk_vm_reg[R_R0] = (uint16_t)c;
      } break;
      case TRAP_PUTSP: {
        uint16_t* c = dk_vm_mem + dk_vm_reg[R_R0];
        while (*c) {
          char char1 = (*c) & 0xFF;
          putc(char1, stdout);
          char char2 = (*c) >> 8;
          if (char2) {
            putc(char2, stdout);
          }
          ++c;
        }
        fflush(stdout);
      } break;
      case TRAP_HALT:
        puts("HALT");
        fflush(stdout);
        dk_vm_running = 0;
        break;
    }
  }

  void dk_update_flags(uint16_t r)
  {
    if (dk_vm_reg[r] == 0) {
      dk_vm_reg[R_COND] = FL_ZRO;
    } else if (dk_vm_reg[r] >> 15) {
      dk_vm_reg[R_COND] = FL_NEG;
    } else {
      dk_vm_reg[R_COND] = FL_POS;
    }
  }

#endif

#if defined(__cplusplus)
}
#endif

#endif // DK_VM_H
