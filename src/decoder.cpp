#include "cpu.h"

void cpu_t::id_execute()
{
    last_adr = *get_reg(PC);
    reg8 instr = read_mem();
    last_instr = instr;

    if(last_adr > 0xFF00)
    {
        std::cout << "I'm likely overflowing!" << std::endl;
        panic();
    }

    if(!booted && last_adr > 0xFF)
    {
        booted = true;
        membus->disable_bootrom();
    }

    reg16_2x8 data16; data16.r16 = 0x00;
    reg8 data8 = 0x00;

    switch(instr)
    {
        case 0x06:  data8 = read_mem(); ld(B,  data8);   break;
        case 0x0E:  data8 = read_mem(); ld(C,  data8);   break;
        case 0x16:  data8 = read_mem(); ld(D,  data8);   break;
        case 0x1E:  data8 = read_mem(); ld(E,  data8);   break;
        case 0x26:  data8 = read_mem(); ld(H,  data8);   break;
        case 0x2E:  data8 = read_mem(); ld(L,  data8);   break;
        case 0x36:  data8 = read_mem(); ld(_HL_, data8); break;

        case 0x40:  ld(B, B);    break;
        case 0x41:  ld(B, C);    break;
        case 0x42:  ld(B, D);    break;
        case 0x43:  ld(B, E);    break;
        case 0x44:  ld(B, H);    break;
        case 0x45:  ld(B, L);    break;
        case 0x46:  ld(B, _HL_); break;

        case 0x48:  ld(C, B);    break;
        case 0x49:  ld(C, C);    break;
        case 0x4A:  ld(C, D);    break;
        case 0x4B:  ld(C, E);    break;
        case 0x4C:  ld(C, H);    break;
        case 0x4D:  ld(C, L);    break;
        case 0x4E:  ld(C, _HL_); break;

        case 0x50:  ld(D, B);    break;
        case 0x51:  ld(D, C);    break;
        case 0x52:  ld(D, D);    break;
        case 0x53:  ld(D, E);    break;
        case 0x54:  ld(D, H);    break;
        case 0x55:  ld(D, L);    break;
        case 0x56:  ld(D, _HL_); break;

        case 0x58:  ld(E, B);    break;
        case 0x59:  ld(E, C);    break;
        case 0x5A:  ld(E, D);    break;
        case 0x5B:  ld(E, E);    break;
        case 0x5C:  ld(E, H);    break;
        case 0x5D:  ld(E, L);    break;
        case 0x5E:  ld(E, _HL_); break;

        case 0x60:  ld(H, B);    break;
        case 0x61:  ld(H, C);    break;
        case 0x62:  ld(H, D);    break;
        case 0x63:  ld(H, E);    break;
        case 0x64:  ld(H, H);    break;
        case 0x65:  ld(H, L);    break;
        case 0x66:  ld(H, _HL_); break;

        case 0x68:  ld(L, B);    break;
        case 0x69:  ld(L, C);    break;
        case 0x6A:  ld(L, D);    break;
        case 0x6B:  ld(L, E);    break;
        case 0x6C:  ld(L, H);    break;
        case 0x6D:  ld(L, L);    break;
        case 0x6E:  ld(L, _HL_); break;

        case 0x70:  ld(_HL_, B);   break;
        case 0x71:  ld(_HL_, C);   break;
        case 0x72:  ld(_HL_, D);   break;
        case 0x73:  ld(_HL_, E);   break;
        case 0x74:  ld(_HL_, H);   break;
        case 0x75:  ld(_HL_, L);   break;

        case 0x7F:  ld(A, A);    break;
        case 0x78:  ld(A, B);    break;
        case 0x79:  ld(A, C);    break;
        case 0x7A:  ld(A, D);    break;
        case 0x7B:  ld(A, E);    break;
        case 0x7C:  ld(A, H);    break;
        case 0x7D:  ld(A, L);    break;

        case 0x0A:  ldabc();    break;
        case 0x1A:  ldade();    break;
        case 0x7E:  ldahl();    break;
        case 0xFA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhan_word(data16.r16); break;
        case 0x3E:  data8 = read_mem(); ld(A, data8);           break;

        case 0x47:  ld(B, A);   break;
        case 0x4F:  ld(C, A);   break;
        case 0x57:  ld(D, A);   break;
        case 0x5F:  ld(E, A);   break;
        case 0x67:  ld(H, A);   break;
        case 0x6F:  ld(L, A);   break;
        case 0x02:  ldbca();    break;
        case 0x12:  lddea();    break;
        case 0x77:  ldhla();    break;
        case 0xEA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhna_word(data16.r16);break;

        case 0xF2:  ld(A, _C_); break;
        case 0xE2:  ld(_C_, A); break;

        case 0x3A:  lddahl();   break;
        case 0x32:  lddhla();   break;
        case 0x2A:  ldiahl();   break;
        case 0x22:  ldihla();   break;

        case 0xE0:  data8 = read_mem(); ldhna_byte(data8);     break;
        case 0xF0:  data8 = read_mem(); ldhan_byte(data8);     break;

        case 0x01:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(BC, data16);         break;
        case 0x11:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(DE, data16);         break;
        case 0x21:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(HL, data16);         break;
        case 0x31:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ld(SP, data16);         break;
        case 0xF9:  ldsphl();                                                                   break;
        case 0xF8:  data8 = read_mem(); ldhlspn_byte(data8);                                    break;
        case 0x08:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); ldhnnsp(data16.r16);    break;

        case 0xF5:  push(AF);          break;
        case 0xC5:  push(BC);          break;
        case 0xD5:  push(DE);          break;
        case 0xE5:  push(HL);          break;

        case 0xF1:  pop(AF);           break;
        case 0xC1:  pop(BC);           break;
        case 0xD1:  pop(DE);           break;
        case 0xE1:  pop(HL);           break;

        case 0x09:  addhl(BC);           break;
        case 0x19:  addhl(DE);           break;
        case 0x29:  addhl(HL);           break;
        case 0x39:  addhl(SP);           break;

        case 0xE8:  data8 = read_mem(); addsp(data8); break;

        case 0x03:  inc(BC);           break;
        case 0x13:  inc(DE);           break;
        case 0x23:  inc(HL);           break;
        case 0x33:  inc(SP);           break;

        case 0x0B:  dec(BC);           break;
        case 0x1B:  dec(DE);           break;
        case 0x2B:  dec(HL);           break;
        case 0x3B:  dec(SP);           break;

        case 0x87:  add(A);   break;
        case 0x80:  add(B);   break;
        case 0x81:  add(C);   break;
        case 0x82:  add(D);   break;
        case 0x83:  add(E);   break;
        case 0x84:  add(H);   break;
        case 0x85:  add(L);   break;
        case 0x86:  add(_HL_);break;
        case 0xC6:  data8 = read_mem(); add(data8);          break;

        case 0x8F:  adc(A);   break;
        case 0x88:  adc(B);   break;
        case 0x89:  adc(C);   break;
        case 0x8A:  adc(D);   break;
        case 0x8B:  adc(E);   break;
        case 0x8C:  adc(H);   break;
        case 0x8D:  adc(L);   break;
        case 0x8E:  adc(_HL_);break;
        case 0xCE:  data8 = read_mem(); adc(data8);          break;

        case 0x97:  sub(A);   break;
        case 0x90:  sub(B);   break;
        case 0x91:  sub(C);   break;
        case 0x92:  sub(D);   break;
        case 0x93:  sub(E);   break;
        case 0x94:  sub(H);   break;
        case 0x95:  sub(L);   break;
        case 0x96:  sub(_HL_);break;
        case 0xD6:  data8 = read_mem(); sub(data8);          break; /* Opcode unconfirmed */

        case 0x9F:  sbc(A);   break;
        case 0x98:  sbc(B);   break;
        case 0x99:  sbc(C);   break;
        case 0x9A:  sbc(D);   break;
        case 0x9B:  sbc(E);   break;
        case 0x9C:  sbc(H);   break;
        case 0x9D:  sbc(L);   break;
        case 0x9E:  sbc(_HL_);break;
        case 0xDE:  data8 = read_mem(); sbc(data8);          break; /* Opcode unconfirmed */

        case 0xA7:  _and(A);   break;
        case 0xA0:  _and(B);   break;
        case 0xA1:  _and(C);   break;
        case 0xA2:  _and(D);   break;
        case 0xA3:  _and(E);   break;
        case 0xA4:  _and(H);   break;
        case 0xA5:  _and(L);   break;
        case 0xA6:  _and(_HL_);break;
        case 0xE6:  data8 = read_mem(); _and(data8);          break;

        case 0xB7:  _or(A);   break;
        case 0xB0:  _or(B);   break;
        case 0xB1:  _or(C);   break;
        case 0xB2:  _or(D);   break;
        case 0xB3:  _or(E);   break;
        case 0xB4:  _or(H);   break;
        case 0xB5:  _or(L);   break;
        case 0xB6:  _or(_HL_);break;
        case 0xF6:  data8 = read_mem(); _or(data8);          break;

        case 0xAF:  _xor(A);   break;
        case 0xA8:  _xor(B);   break;
        case 0xA9:  _xor(C);   break;
        case 0xAA:  _xor(D);   break;
        case 0xAB:  _xor(E);   break;
        case 0xAC:  _xor(H);   break;
        case 0xAD:  _xor(L);   break;
        case 0xAE:  _xor(_HL_);break;
        case 0xEE:  data8 = read_mem(); _xor(data8);          break;

        case 0xBF:  cp(A);              break;
        case 0xB8:  cp(B);              break;
        case 0xB9:  cp(C);              break;
        case 0xBA:  cp(D);              break;
        case 0xBB:  cp(E);              break;
        case 0xBC:  cp(H);              break;
        case 0xBD:  cp(L);              break;
        case 0xBE:  cp(_HL_);           break;
        case 0xFE:  data8 = read_mem(); cp(data8);                   break;

        case 0x3C:  inc(A);            break;
        case 0x04:  inc(B);            break;
        case 0x0C:  inc(C);            break;
        case 0x14:  inc(D);            break;
        case 0x1C:  inc(E);            break;
        case 0x24:  inc(H);            break;
        case 0x2C:  inc(L);            break;
        case 0x34:  inc(_HL_);         break;

        case 0x3D:  dec(A);            break;
        case 0x05:  dec(B);            break;
        case 0x0D:  dec(C);            break;
        case 0x15:  dec(D);            break;
        case 0x1D:  dec(E);            break;
        case 0x25:  dec(H);            break;
        case 0x2D:  dec(L);            break;
        case 0x35:  dec(_HL_);         break;

        case 0x27:  daa();          break;
        case 0x2F:  cpl();  break;
        case 0x37:  scf();  break;
        case 0x3F:  ccf();  break;
        case 0x00:  nop();  break;
        case 0x76:  halt(); break;
        case 0x10:  stop(); break; // TODO should be 0x10 0x00!
        case 0xF3:  di();   break;
        case 0xFB:  ei();   break;

        case 0x07:  rlca(); break;
        case 0x17:  rla();  break;
        case 0x0F:  rrca(); break;
        case 0x1F:  rra();  break;

        case 0xCB: /* Extended ALU Operations */
            instr = read_mem();
            switch(instr)
            {
                case 0x07:  rlc(A); break;
                case 0x00:  rlc(B); break;
                case 0x01:  rlc(C); break;
                case 0x02:  rlc(D); break;
                case 0x03:  rlc(E); break;
                case 0x04:  rlc(H); break;
                case 0x05:  rlc(L); break;
                case 0x06:  rlc(_HL_); break;

                case 0x17:  rl(A); break;
                case 0x10:  rl(B); break;
                case 0x11:  rl(C); break;
                case 0x12:  rl(D); break;
                case 0x13:  rl(E); break;
                case 0x14:  rl(H); break;
                case 0x15:  rl(L); break;
                case 0x16:  rl(_HL_); break;

                case 0x0F:  rrc(A); break;
                case 0x08:  rrc(B); break;
                case 0x09:  rrc(C); break;
                case 0x0A:  rrc(D); break;
                case 0x0B:  rrc(E); break;
                case 0x0C:  rrc(H); break;
                case 0x0D:  rrc(L); break;
                case 0x0E:  rrc(_HL_); break;

                case 0x1F:  rr(A); break;
                case 0x18:  rr(B); break;
                case 0x19:  rr(C); break;
                case 0x1A:  rr(D); break;
                case 0x1B:  rr(E); break;
                case 0x1C:  rr(H); break;
                case 0x1D:  rr(L); break;
                case 0x1E:  rr(_HL_); break;

                case 0x27:  sla(A); break;
                case 0x20:  sla(B); break;
                case 0x21:  sla(C); break;
                case 0x22:  sla(D); break;
                case 0x23:  sla(E); break;
                case 0x24:  sla(H); break;
                case 0x25:  sla(L); break;
                case 0x26:  sla(_HL_); break;

                case 0x2F:  sra(A); break;
                case 0x28:  sra(B); break;
                case 0x29:  sra(C); break;
                case 0x2A:  sra(D); break;
                case 0x2B:  sra(E); break;
                case 0x2C:  sra(H); break;
                case 0x2D:  sra(L); break;
                case 0x2E:  sra(_HL_); break;

                case 0x3F:  srl(A); break;
                case 0x38:  srl(B); break;
                case 0x39:  srl(C); break;
                case 0x3A:  srl(D); break;
                case 0x3B:  srl(E); break;
                case 0x3C:  srl(H); break;
                case 0x3D:  srl(L); break;
                case 0x3E:  srl(_HL_); break;

                default:
                    /* check for BIT op */
                    /* -x-- -xxx are relevant for the operation */
                    switch(instr & 0x47)
                    {
                        /* --xx x--- are relevant as argument */
                        case 0x47:  bit(((instr & 0x38) >> 3), A);   break;
                        case 0x40:  bit(((instr & 0x38) >> 3), B);   break;
                        case 0x41:  bit(((instr & 0x38) >> 3), C);   break;
                        case 0x42:  bit(((instr & 0x38) >> 3), D);   break;
                        case 0x43:  bit(((instr & 0x38) >> 3), E);   break;
                        case 0x44:  bit(((instr & 0x38) >> 3), H);   break;
                        case 0x45:  bit(((instr & 0x38) >> 3), L);   break;
                        case 0x46:  bit(((instr & 0x38) >> 3), _HL_);break;

                        case 0xC7:  set(((instr & 0x38) >> 3), A);   break;
                        case 0xC0:  set(((instr & 0x38) >> 3), B);   break;
                        case 0xC1:  set(((instr & 0x38) >> 3), C);   break;
                        case 0xC2:  set(((instr & 0x38) >> 3), D);   break;
                        case 0xC3:  set(((instr & 0x38) >> 3), E);   break;
                        case 0xC4:  set(((instr & 0x38) >> 3), H);   break;
                        case 0xC5:  set(((instr & 0x38) >> 3), L);   break;
                        case 0xC6:  set(((instr & 0x38) >> 3), _HL_);break;

                        case 0x87:  res(((instr & 0x38) >> 3), A);   break;
                        case 0x80:  res(((instr & 0x38) >> 3), B);   break;
                        case 0x81:  res(((instr & 0x38) >> 3), C);   break;
                        case 0x82:  res(((instr & 0x38) >> 3), D);   break;
                        case 0x83:  res(((instr & 0x38) >> 3), E);   break;
                        case 0x84:  res(((instr & 0x38) >> 3), H);   break;
                        case 0x85:  res(((instr & 0x38) >> 3), L);   break;
                        case 0x86:  res(((instr & 0x38) >> 3), _HL_);break;

                        default:
                        std::cout << "Unknown CB instruction 0x" << std::hex << (int)last_instr
                            << " at adr 0x" << std::hex << (int)last_adr << std::endl;
                        panic();
                            break;
                    }
                    break;
            }
            break;


        case 0xC3:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(data16.r16);     break;
        case 0xC2:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(NZ, data16.r16); break;
        case 0xCA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(Z, data16.r16);  break;
        case 0xD2:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(NC, data16.r16); break;
        case 0xDA:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); jp(CC, data16.r16); break;
        case 0xE9:  jphl(); break;

        case 0x18:  data8 = read_mem(); jr(data8);      break;
        case 0x20:  data8 = read_mem(); jr(NZ, data8);  break;
        case 0x28:  data8 = read_mem(); jr(Z, data8);   break;
        case 0x30:  data8 = read_mem(); jr(NC, data8);  break;
        case 0x38:  data8 = read_mem(); jr(CC, data8);  break;

        case 0xCD:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(data16.r16);       break;
        case 0xC4:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(NZ, data16.r16);   break;
        case 0xCC:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(Z, data16.r16);    break;
        case 0xD4:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(NC, data16.r16);   break;
        case 0xDC:  data16.r8.l = read_mem(); data16.r8.h = read_mem(); call(CC, data16.r16);   break;

        case 0xC7: rst(0x00);     break;
        case 0xCF: rst(0x08);     break;
        case 0xD7: rst(0x10);     break;
        case 0xDF: rst(0x18);     break;
        case 0xE7: rst(0x20);     break;
        case 0xEF: rst(0x28);     break;
        case 0xF7: rst(0x30);     break;
        case 0xFF: rst(0x38);     break;

        case 0xC9:  ret();      break;
        case 0xC0:  ret(NZ);    break;
        case 0xC8:  ret(Z);     break;
        case 0xD0:  ret(NC);    break;
        case 0xD8:  ret(CC);    break;

        case 0xD9:  reti(); break;

        default:
            std::cout << "Unknown instruction 0x" << std::hex << (int)last_instr
                << " at adr 0x" << std::hex << (int)last_adr << std::endl;
            panic();
                break;
    }


#if DEBUG_OUTPUT > 0
    if(!booted)
        std::cout << "[BOOT]";

    std::cout << "PC: " << std::hex << (int)last_adr << " INSTR: "
        << (int)last_instr << " ";

    cpu_debug_print(last_adr, last_instr, data8, data16, std::cout);
#endif


#ifdef BREAKPOINT
    if (last_adr == BREAKPOINT)
    {
        std::cout << "Breakpoint reached" << std::endl;
        panic();
        return;
    }
#endif
}
