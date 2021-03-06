#include "cpu.h"
#include <thread>

using std::cout;
using std::setw;
using std::hex;
using std::vector;
using std::thread;
using std::string;
using std::ifstream;
using std::exception;
using std::runtime_error;


// CPU cycle chart
static const uint8_t cycles[256] {
//////// 0 1 2 3 4 5 6 7 8 9 A B C D E F
/*0x00*/ 7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
/*0x10*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x20*/ 6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
/*0x30*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x40*/ 6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
/*0x50*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x60*/ 6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
/*0x70*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0x90*/ 2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
/*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0xB0*/ 2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
/*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xD0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0xE0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xF0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7
};


uint8_t CPU::read(uint16_t addr) const {

  if(addr < 0x2000) {

    return memory[addr & 0x7ff];

  } if(addr < 0x4000) {
    switch (addr & 7) {
      case 2: return ppu->regr_status();
      case 4: return ppu->regr_OAM_data();
      case 7: return ppu->regr_data();
      default: /* bad read */ return 0;
    }
  } else if(addr < 0x4020) {
    switch(addr & 0x1f) {
      case 0x15: return apu->read();
      case 0x16: return controller[0]->read();
      case 0x17: return controller[1]->read();
      default: return 0;
    }
  } else {

    return rom->read_prg(addr);

  }
}

void CPU::write(uint8_t value, uint16_t addr) {

  if(addr < 0x2000) { // Internal RAM

    // $0800 to $1fff: mirrors of $0000 - $0800
    memory[addr & 0x7ff] = value;

  } else if(addr < 0x4000) { // PPU registers

    switch (addr & 7) {
      case 0: ppu->regw_control(value); break;
      case 1: ppu->regw_mask(value); break;
      case 3: ppu->regw_OAM_address(value); break;
      case 4: ppu->regw_OAM_data(value); break;
      case 5: ppu->regw_scroll(value); break;
      case 6: ppu->regw_address(value); break;
      case 7: ppu->regw_data(value); break;
    }

  } else if(addr < 0x4020) { // APU and I/O registers

    switch (addr & 0x1f) {
      case 0x14: { // DMA transfer
        for(int i = 0; i < 256; ++i) {
          ppu->regw_OAM_data(read((value & 7) * 0x100 + i));
        }
      } break;
      case 0x16: 
        controller[value & 1]->strobe();
        break;
      default:
        apu->write(value, addr & 0x1f);
        break;
    }

  } else {

    rom->write_prg(value, addr);

  }

}

void CPU::push(uint8_t x) {
  memory[0x100 + SP--] = x;
}

void CPU::push2(uint16_t x) {
  memory[0x100 + SP--] = (uint8_t)(x >> 8);
  memory[0x100 + SP--] = (uint8_t)(x&0xff);
}

uint8_t CPU::pull() {
  return memory[++SP + 0x100];
}

uint16_t CPU::pull2() {
  uint16_t r = memory[++SP + 0x100];
  return r | (memory[++SP + 0x100] << 8);    
}

void CPU::addcyc() {
  ++result_cycle;
}

uint8_t CPU::next() {
  return read(PC++);
}

uint16_t CPU::next2() {
  uint16_t v = (uint16_t)read(PC++);
  return v | ((uint16_t)read(PC++) << 8);
}

CPU::CPU(IBus *bus, IAPU *apu, IPPU *ppu, IROM *rom, IController* controller0, IController* controller1)
  : bus(bus)
  , apu(apu)
  , ppu(ppu)
  , rom(rom)
  , controller {
    controller0,
    controller1,
  }
{
  memory[0x008] = 0xf7;
  memory[0x009] = 0xef;
  memory[0x00a] = 0xdf;
  memory[0x00f] = 0xbf;
  memory[0x1fc] = 0x69;
}

bool do_NMI { false };

void CPU::pull_NMI() {
  do_NMI = true;
}

void CPU::run() {

  PC = read(0xfffc) | (read(0xfffd) << 8);
  logi("PC: %x", (uint16_t)PC);

  for (;;) {

    last_PC = PC;
    last_op = next();
    
  #ifdef DEBUG_CPU
    print_status();
  #endif
    
    (this->*ops[last_op])();
    
    for (int i = 0; i < cycles[last_op] + result_cycle; ++i) {
      bus->on_cpu_tick();
    }

    result_cycle = 0;

    if (_done) {
      break;
    }

    if (IRQ && ((P & I_FLAG) == 0)) {
      print_status();
      push2(PC);
      stack_push<&CPU::ProcStatus>();
      P |= I_FLAG;
      PC = read(0xfffe) | (read(0xffff) << 8);
    } 

    else if (do_NMI) {
      do_NMI = false;
      push2(PC);
      stack_push<&CPU::ProcStatus>();
      PC = read(0xfffa) | (read(0xfffb) << 8);

    }

  }

  std::cout << "*** cpu done ***" << std::endl;

}

template<>
uint8_t CPU::read<&CPU::IMM>() {
  return (uint8_t)IMM();
}

void CPU::print_status() {
  cout 
  << hex << std::uppercase << std::setfill('0')
  << setw(4) << last_PC << "  "
  << setw(2) << (int)last_op << "   "
  << std::setfill(' ') << setw(16) 
  << std::left << opasm[last_op]
  << std::setfill('0')
  << " A:" << setw(2) << (int)A
  << " X:" << setw(2) << (int)X
  << " Y:" << setw(2) << (int)Y
  << " P:" << setw(2) << (int)P
  << " SP:" << setw(2) << (int)SP
  << std::setfill(' ')
  << " CYC:" << setw(3) << std::dec << (int)cyc
  //<< " SL:" << setw(3) << (int)bus::debug_ppu_get_scanline()
  << hex << std::setfill('0')
  << " ST0:" << setw(2) << (int)memory[0x101 + SP]
  << " ST1:" << setw(2) << (int)memory[0x102 + SP]
  << " ST2:" << setw(2) << (int)memory[0x103 + SP]
  << '\n';
}

void CPU::pull_IRQ() {
  IRQ = true;
}

void CPU::release_IRQ() {
  IRQ = false;
}

void CPU::stop() {
  _done = true;
}

void CPU::set_observer16(IObserver<uint16_t>* observer) {
#if 0
  std::cout << "Set observer\n";
  PC.add_observer(observer);
#endif
}

void CPU::set_observer(IObserver<uint8_t>* observer) {
#if 0
  std::cout << "Set observer8\n";
  for (auto& i : memory) {
    i.add_observer(observer);
    i = 0xff;
  }
#endif
}

void CPU::dump_memory() const {
  int w = 16;
  std::printf("CPU   ");
  for (int i = 0; i < w; ++i) {
    std::printf("%02x ", i);
  }
  for (int i = 0; i < 0x800; ++i) {
    if (i % w == 0) {
      std::printf("\n%03x   ", i);
    }
    std::printf("%02x ", (uint8_t)memory[i]);
  }
  std::printf("\n");
}


struct CPU_state : public IRestorable::State {
  std::array<uint8_t, 0x800> memory;
  uint8_t P, A, X, Y, SP;
  uint16_t PC;
};

IRestorable::State const* CPU::get_state() const {
  CPU_state *state = new CPU_state();
  for (size_t i = 0; i < memory.size(); ++i) {
    state->memory[i] = static_cast<uint8_t>(memory[i]);
  }

  state->P = P;
  state->A = A;
  state->X = X;
  state->Y = Y;
  state->SP = SP;
  state->PC = PC;

  return state;

}

void CPU::restore_state(IRestorable::State const* s) {
  CPU_state const* state = static_cast<CPU_state const*>(s);
  for (size_t i = 0; i < memory.size(); ++i) {
    memory[i] = state->memory[i];
  }

  P = state->P;
  A = state->A;
  X = state->X;
  Y = state->Y;
  SP = state->SP;
  PC = state->PC;

}



