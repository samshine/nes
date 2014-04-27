#ifndef BUS_H
#define BUS_H

#include <string>
#include <vector>
#include <array>

using Framebuffer = std::array<uint32_t, 256 * 240>;

struct State {
};

/**
 * @brief Interface of writeable component
 **/
class IWriteableComponent {
  public:
    ~IWriteableComponent() {}

  public:
    /**
     * @brief write a value to a memory location
     * @param what the 8-bit value to write
     * @param where the address where to write the value
     **/
    virtual void write(uint8_t what, uint16_t where) =0;

};


/**
 * @brief Interface of a picture processing unit
 **/
class IPPU {
  public:
    virtual ~IPPU(){}

  public:
    /**
     * @brief advance the component's internal clock
     **/
    virtual void tick() =0;

  public: // Register write
    /**
     * @brief write to the ppu control register
     * @param value the value to write
     *
     * This register controls various aspects of the PPU operation:
     * - base nametable address (can be $2000, $2400, $2800 or $2C00)
     * - vram address increment per CPU read/write of PPUDATA (1 or 32)
     * - sprite pattern table address for 8x8 sprites ($0000 or $1000)
     * - background pattern table address ($0000 or $1000)
     * - sprite size (8x8 or 8x16)
     * - PPU master/slave select
     * - whether to generate an NMI at the start of the vertical blanking interval
     * 
     * See http://wiki.nesdev.com/w/index.php/PPU_registers#Controller_.28.242000.29_.3E_write for details.
     **/
    virtual void regw_control(uint8_t value) =0;

    /**
     * @brief write to the ppu mask register
     * @param value the value to write
     *
     * This register can enable or disable certain rendering options:
     * - grayscale
     * - show background in leftmost 8 pixels of screen
     * - show sprites in leftmost 8 pixels of screen
     * - show background
     * - show sprites
     * - intensify reds
     * - intensify greens
     * - intensify blues
     **/
    virtual void regw_mask(uint8_t value) =0;

    /**
     * @brief set the object attribute memory address
     * @param value new OAM address
     **/
    virtual void regw_OAM_address(uint8_t value) =0;

    /**
     * @brief write OAM data, incrementing the OAM address
     * @value the value to write
     *
     * OAM address is incremented after the write.
     **/
    virtual void regw_OAM_data(uint8_t value) =0;

    /**
     * @brief set the scroll position, i.e. where in the nametable to start rendering
     * @param value the address within the nametable that should be drawn at the top left corner: x and y on alternate writes.
     **/
    virtual void regw_scroll(uint8_t value) =0;

    /**
     * @brief set the vram address
     * @param value 1/2 of the 2-byte VRAM address. Upper byte on first write, lower byte on second.
     *
     * An internal latch that determines whether to write to the upper or lower byte is toggled upon write.
     * The latch is reset by a read to $2002 (status register).
     **/
    virtual void regw_address(uint8_t value) =0;

    /**
     * @brief write to the memory at the current vram address
     * @param the value to write
     * 
     * After read/write, the vram address is incremented by either 1 or 32 (as set in the ppu control register).
     **/
    virtual void regw_data(uint8_t value) =0;

  public: // Register read
    /**
     * @brief read the status register
     * @return the state of the ppu
     *
     * This register holds various state information:
     * - sprite overflow
     * - sprite 0 hit
     * - whether vertical blank has started
     *
     * Reading has certain side effects:
     * - clears the vblank started flag
     * - resets the address latch of the scroll and address registers
     *
     * @note reading this register at exact start of vblank returns 0, but still clears the latch.
     **/
    virtual uint8_t regr_status() =0;

    /**
     * @brief read the object attribute memory data
     * @return the current value pointed to by the oam address
     * @note reads during vblank do not increment the address.
     **/
    virtual uint8_t regr_OAM_data() =0;

    /**
     * @brief read the value pointed to by the vram address
     * @return the value pointed to by the vram address
     * @note reading this register increments the vram address and may update an internal buffer
     * 
     * Reading non-palette memory area (i.e. below $3f00) returns the contents of an internal buffer,
     * which is then filled with the data at the vram address prior to increment.
     * 
     * When reading palette data, the buffer is filled with the name table data 
     * that would be accessible if the name table mirrors extended up to $3fff.
     **/
    virtual uint8_t regr_data() =0;

};


/**
 * @brief Interface of an onboard audio processing unit
 **/
class IAPU {
  public:
    virtual ~IAPU(){}

  public:
    /**
     * @brief read the current state of the apu
     * @note has side effects
     **/
    virtual uint8_t read() const =0;

    /**
     * @brief write a value to a register
     * @param value the value to write
     * @param index the register to write to
     **/
    virtual void write(uint8_t value, uint8_t index) =0;

    /**
     * @brief advance the component's internal clock
     **/
    virtual void tick() =0;

};


/**
 * Interface for game controller (i.e., GamePad)
 **/
class IController {
  public:
    virtual ~IController(){}

  public:
    using ButtonState = uint8_t;
    static const ButtonState BUTTON_OFF { 0 }, BUTTON_ON { 0xff };

    enum Button {
      CONTROL_RIGHT = 0,
      CONTROL_LEFT  = 1,
      CONTROL_DOWN  = 2,
      CONTROL_UP    = 3,
      CONTROL_START = 4,
      CONTROL_SELECT = 5,
      CONTROL_B = 6,
      CONTROL_A = 7,
    };

  public:
    /**
     * @brief read the current state of the controller
     **/
    virtual ButtonState read() =0;

    /**
     * @brief set the state for a button
     * @param button the button to change
     * @param state the new state (on/off)
     **/
    virtual void set_button_state(Button button, ButtonState state) =0;

    /**
     * @brief strobe the controller
     **/
    virtual void strobe() =0;

};


/**
 * Interface for a video output device
 **/
class IVideoDevice {
  public:
    virtual ~IVideoDevice(){}

  public:
    /**
     * @brief set the video buffer to the given raster
     * @param buffer the pixel data to set
     **/
    virtual void set_buffer(Framebuffer const& buffer) =0;

};


/**
 * Interface for an audio output device
 **/
class IAudioDevice {
  public:
    virtual ~IAudioDevice(){}
};


/**
 * Interface for an input device
 **/
class IInputDevice {
  public:
    virtual ~IInputDevice(){}

  public:
    /**
     * @brief handle the input at the current frame
     **/
    virtual void tick() =0;

};


/**
 * @brief interface of the gamepak/cartridge/ROM
 **/
class IROM : public IWriteableComponent {
  public:
    virtual ~IROM(){}

  public:
    /**
     * @brief get a reference to the value at a memory location
     * @param addr the address where the reference should be found
     * @return a reference to the value at the memory location
     **/
    virtual uint8_t& getmemref(uint16_t addr) =0;

    /**
     * @brief retrieve a reference to the value at a nametable memory location
     * @param table the index of the nametable
     * @param addr the address within the nametable
     * @return a reference to the value at the nametable memory location
     **/
    virtual uint8_t& getntref(uint8_t table, uint16_t addr) =0;
    virtual uint8_t const& getntref(uint8_t table, uint16_t addr) const =0;
    virtual void write_nt(uint8_t value, uint8_t table, uint16_t addr) =0;

    /**
     * @brief retrieve a reference to a location in the vbank
     * @param addr the address to look up
     * @return a reference to the vbank location
     **/
    virtual uint8_t& getvbankref(uint16_t addr) =0;
    virtual uint8_t const& getvbankref(uint16_t addr) const =0;

};



/**
 * @brief interface for a basic system, handling interrupts
 **/
class IBus {
  public:
    virtual ~IBus(){}

  public:
    virtual void pull_NMI() =0;
    virtual void pull_IRQ() =0;
    virtual void reset_IRQ() =0;

};


/**
 * @brief Interface for the CPU
 **/
class ICPU {
  public:
    virtual ~ICPU(){}

  public:
    virtual void run() =0;

  public:
    virtual void pull_NMI() =0;
    virtual void pull_IRQ() =0;
    virtual void reset_IRQ() =0;

};


/**
 * @brief basic system
 **/
class NES : public IBus {
  protected:
    IVideoDevice *video;
    IAudioDevice *audio;
    IController *controller[2];
    IInputDevice *input;
    IROM *rom { nullptr };
    IPPU *ppu;
    IAPU *apu;
    ICPU *cpu;

  public:
    NES(std::string const&);
    virtual ~NES();

  public:
    void pull_NMI();
    void pull_IRQ();
    void reset_IRQ();

};

#endif
