#include "../../include/iocomponents/keyboard.h"

/** Keyboard Adrress Mappings **********************************************/
/**                                                                       **/
/** NOTE: There is a gap between 0xA488-0xA493 incl.                      **/
/** KEY:                                                                  **/
/**  KI : Keyboard Input.                                                 **/
/**  KO : Keyboard Output.                                                **/
/**                                                                       **/
/** 0xA480 - DRA2: Data Register A (KI1-KI8). Pressed: 0, Unpressed: 1.   **/
/** 0xA481 - DDRA2: Data Direction Register A.                            **/
/** 0xA482 - DRB2: Data Register B (KO1-KO8). Pressed: 0, Unpressed: 1.   **/
/** 0xA483 - DDRB2: Data Direction Register B.                            **/
/** 0xA484 - DNPA7: Write Disable PA7 Interrupt Negative Edge Detect.     **/
/** 0xA485 - DPPA7: Write Disable PA7 Interrupt Positive Edge Detect.     **/
/** 0xA486 - RINT: Read Bit 7 = Timer Flag, Bit 6 = PA7 Flag, Clear Int.  **/
/** 0xA486 - ENPA7: Write Enable PA7 Interrupt, Negative Edge Detect.     **/
/** 0xA487 - EPPA7: Write Enable PA7 Interrupt, Positive Edge Detect.     **/
/** 0xA494 - DIV1: Div. by 0001 (Disable), Add 8 to Enable.               **/
/** 0xA495 - DIV8: Div. by 0008 (Disable), Add 8 to Enable.               **/
/** 0xA496 - DIV64: Div. by 0064 (Disable), Add 8 to Enable.              **/
/** 0xA497 - DIV1024: Div. by 1024 (Disable), Add 8 to Enable.            **/
/***************************************************************************/

Keyboard::Keyboard() : IOComponentIRQ()
{
    this->m_registers.insert(std::make_pair(DRA2_ADDR, 0));
    this->m_registers.insert(std::make_pair(DRB2_ADDR, 0));
    this->m_registers.insert(std::make_pair(DNPA7_ADDR, 0));
    this->m_registers.insert(std::make_pair(DPPA7_ADDR, 0));
    this->m_registers.insert(std::make_pair(RINT_ADDR, 0));
    this->m_registers.insert(std::make_pair(ENPA7_ADDR, 0));
    this->m_registers.insert(std::make_pair(EPPA7_ADDR, 0));
    this->SetAddressRange();
}

Keyboard::Keyboard(const byte *registers, std::size_t registersSize, const word minAddress, const word maxAddress)
    : IOComponentIRQ(registers, registersSize, minAddress, maxAddress)
{
}

Keyboard::Keyboard(const word minAddress, const word maxAddress)
    : IOComponentIRQ(minAddress, maxAddress)
{
}

// Character produced by each matrix position, index = row * 8 + column.
// This is the Monitor ROM's own table at 0xF421 (AIMMON11); the ROM looks a
// key up with the row read from DRB2 and the column that was being driven.
// 0 marks positions without a printable key (the row 0 modifiers, unused).
// 0x60 and 0x5C are function keys the ROM handles itself.
static const byte KEY_TABLE[64] = {
    0x20, 0x08, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0x5C, 0x00, 0x00, 0x00, 0x7F, 0x00,
    '.',  'L',  'P',  '-',  ':',  '0',  ';',  '/',
    'M',  'J',  'I',  'O',  '9',  '8',  'K',  ',',
    'B',  'G',  'Y',  'U',  '7',  '6',  'H',  'N',
    'C',  'D',  'R',  'T',  '5',  '4',  'F',  'V',
    'Z',  'A',  'W',  'E',  '3',  '2',  'S',  'X',
    0x00, 0x00, 0x1B, 'Q',  '1',  '^',  ']',  '['
};

// What the ROM turns a base character into while SHIFT is held (the logic at
// 0xECA4-0xECBF): letters and symbols in 0x40-0x5F are left alone, digits and
// ':' ';' lose bit 4 ('1' -> '!'), and ',' '-' '.' '/' gain it ('-' -> '=').
static byte ShiftedChar(byte base)
{
    if ((base & 0x40) != 0 || (base & 0x0F) == 0)
    {
        return base;
    }
    return (base & 0x0F) < 0x0C ? (base & 0xEF) : (base | 0x10);
}

bool Keyboard::FindKey(char ch, bool ctrl, KeyPress &keyPress)
{
    for (int i = 0; i < 64; ++i)
    {
        const byte base = KEY_TABLE[i];
        if (base == 0 || base == 0x60 || base == 0x5C)
        {
            continue;
        }
        const bool printable = base >= 0x20 && base < 0x60;
        if (base == static_cast<byte>(ch))
        {
            keyPress = {i / 8, i % 8, false, ctrl};
            return true;
        }
        if (printable && ShiftedChar(base) == static_cast<byte>(ch))
        {
            keyPress = {i / 8, i % 8, true, ctrl};
            return true;
        }
    }
    return false;
}

void Keyboard::PressModifiers(const KeyPress &keyPress)
{
    std::lock_guard<std::recursive_mutex> lock(this->m_mutex);
    this->m_shiftDown = keyPress.shift;
    this->m_ctrlDown = keyPress.ctrl;
}

void Keyboard::PressKey(const KeyPress &keyPress)
{
    std::lock_guard<std::recursive_mutex> lock(this->m_mutex);
    this->m_key = keyPress;
    this->m_shiftDown = keyPress.shift;
    this->m_ctrlDown = keyPress.ctrl;
    this->m_keyDown = true;
}

void Keyboard::ReleaseKey()
{
    std::lock_guard<std::recursive_mutex> lock(this->m_mutex);
    this->m_keyDown = false;
    this->m_shiftDown = false;
    this->m_ctrlDown = false;
}

byte Keyboard::ReadRows(byte columnSelect) const
{
    byte rows = 0xFF;
    if (this->m_keyDown && (columnSelect & (1 << this->m_key.column)) == 0)
    {
        rows &= ~(1 << this->m_key.row);
    }
    if (this->m_shiftDown && (columnSelect & (1 << SHIFT_COLUMN)) == 0)
    {
        rows &= ~(1 << MODIFIER_ROW);
    }
    if (this->m_ctrlDown && (columnSelect & (1 << CTRL_COLUMN)) == 0)
    {
        rows &= ~(1 << MODIFIER_ROW);
    }
    return rows;
}

void Keyboard::UpdateDebugStatus(word address)
{
    AimInspector::GetInstance()->UpdateKeyboardStatus(this->DumpMemory(address));
}

byte Keyboard::GetRegisterValue(word address)
{
    std::lock_guard<std::recursive_mutex> lock(this->m_mutex);
    if (address == DRB2_ADDR)
    {
        // Keep the register in step so the debugger shows what was read.
        this->m_registers[DRB2_ADDR] = this->ReadRows(this->m_registers[DRA2_ADDR]);
    }
    return IOComponentIRQ::GetRegisterValue(address);
}

void Keyboard::SetRegister(word address, byte value)
{
    std::lock_guard<std::recursive_mutex> lock(this->m_mutex);
    IOComponentIRQ::SetRegister(address, value);
}
