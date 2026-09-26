#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <mutex>
#include "iocomponentirq.h"

class Keyboard : public IOComponentIRQ
{
 public:
  Keyboard();
  Keyboard(const byte *registers, std::size_t registersSize, const word minAddress, const word maxAddress);
  Keyboard(const word minAddress, const word maxAddress);
  /**
   * Presses the key that produces the given ASCII character on the AIM-65
   * keyboard, together with SHIFT when the character needs it. The key stays
   * down until ReleaseKey(). Called from the GUI thread; the CPU thread reads
   * the matrix through DRB2.
   * @param ch Upper case ASCII character, or one of the control codes the
   *           keyboard has keys for (CR, DEL, ESC, backspace).
   * @return True if the keyboard has a key for the character.
   */
  bool PressKey(char ch);
  void ReleaseKey();
  /**
   * DRB2 returns the state of the key matrix rows for the columns selected
   * by the last value written to DRA2 (active low, 0xFF when nothing is
   * pressed), not the last value written to it.
   */
  byte GetRegisterValue(word address) override;
  void SetRegister(word address, byte value) override;
  void UpdateDebugStatus(word address);
 private:
    // Keyboard address bounds: 0xA480-0xA497.
    const word MIN_KEYBOARD_ADDR = 0xA480;
    const word MAX_KEYBOARD_ADDR = 0xA497;

    const word DRA2_ADDR = 0xA480;
    const word DRB2_ADDR = 0xA482;
    const word DNPA7_ADDR = 0xA484;
    const word DPPA7_ADDR = 0xA485;
    // Note RINT and ENPA7 have the same address.
    const word RINT_ADDR = 0xA486;
    const word ENPA7_ADDR = 0xA486;
    const word EPPA7_ADDR = 0xA487;

    // Recursive: SetRegister() -> UpdateDebugStatus() dumps registers of
    // this same component while the lock is held.
    std::recursive_mutex m_mutex;

    /**
     * A key of the 8x8 matrix: the Monitor drives one column low at a time
     * through DRA2 and reads the rows (0 = pressed) from DRB2.
     */
    struct MatrixPosition
    {
        int row;
        int column;
    };

    // Modifier keys sit in row 0, columns 4-6 (column 4 is CTRL, 5 and 6 are
    // the two SHIFT keys); the Monitor ROM applies them to the base character.
    static const int MODIFIER_ROW = 0;
    static const int SHIFT_COLUMN = 6;

    bool m_keyDown = false;
    bool m_shiftDown = false;
    MatrixPosition m_key = {0, 0};

    /**
     * Finds the matrix position (and whether SHIFT is needed) of a character.
     */
    static bool FindKey(char ch, MatrixPosition &position, bool &shift);
    /**
     * Rows read back for the columns selected by the given DRA2 value.
     */
    byte ReadRows(byte columnSelect) const;

    DISALLOW_COPY_AND_ASSIGN(Keyboard);
};

#endif /* KEYBOARD_H */
