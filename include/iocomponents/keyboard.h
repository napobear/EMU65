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
   * Called from the GUI thread; the CPU thread reads the same registers.
   */
  void onKeyPressed(char ch);
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

    DISALLOW_COPY_AND_ASSIGN(Keyboard);

    /**
     * {Checks whether the character specified is recognised by the AIM65.}
     * @param ch {Character to check.}
     * @return {True if it is recognised, false if it's not.}
     */
    bool IsValidChar(char ch) const;
};

#endif /* KEYBOARD_H */
