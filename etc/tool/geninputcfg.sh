#!/bin/sh

if [ "$#" -ne 1 ] ; then
  echo "Usage: $0 OUTPUT"
  exit 1
fi

DSTPATH="$1"
rm -f "$DSTPATH"

cat - >>"$DSTPATH" <<EOF
# GP1 Input Configuration
# Generated at $(date -Iseconds).

#----------------------------------------------------------------------
# My devices. TODO Don't force all this text on every user. Or do? I'm not sure.

device 3250:1002 Atari Game Controller
  driver evdev
#  player 1
  0x0100008b AUX1 # hamburger
  0x0100009e AUX2 # back arrow
  0x010000ac AUX3 # heart
  0x01000130 SOUTH
  0x01000131 EAST
  0x01000133 WEST
  0x01000134 NORTH
  0x01000136 L1
  0x01000137 R1
  0x0100013d FULLSCREEN # lp
  0x0100013e QUIT # rp
#  0x03000000 HORZ # lx
#  0x03000001 VERT # ly
#  0x03000002 NONE # rx
#  0x03000005 NONE # ry
  0x03000009 1..1023 R2
  0x0300000a 1..1023 L2
  0x03000010 HORZ # dpad
  0x03000011 VERT # dpad
  
device 2dc8:3010 8BitDo 8BitDo Pro 2
  driver evdev
  0x01000130 EAST
  0x01000131 SOUTH
  0x01000133 NORTH
  0x01000134 WEST
  0x01000136 L1
  0x01000137 R1
#  0x01000138 L2 # kicks in about halfway; use analogue instead
#  0x01000139 R2 # ''
  0x0100013a AUX2 # select
  0x0100013b AUX1 # start
  0x0100013c AUX3 # heart
#  0x0100013d NONE # lp
#  0x0100013e NONE # rp
#  0x03000000 HORZ # lx
#  0x03000001 VERT # ly
#  0x03000002 HORZ # lx
#  0x03000005 VERT # ly
  0x03000009 1..255 R2
  0x0300000a 1..255 L2
  0x03000010 HORZ # dx
  0x03000011 VERT # dy

#----------------------------------------------------------------------
# Generic keyboard, assuming HID usages are available.

device *Keyboard*

  0x00070029 QUIT # Escape (maybe should be MENU?)
  0x0007003a RESET # F1
  0x0007003b LOAD_STATE # F2
  0x0007003c SAVE_STATE # F3
  # Reserve F4 for some future "state" thing.
  0x0007003e SUSPEND # F5
  0x0007003f RESUME # F6
  0x00070040 STEP_FRAME # F7
  # Reserve F8 for some future "debug" thing.
  # F9,F10 available.
  0x00070044 FULLSCREEN # F11
  0x00070045 MENU # F12
  0x00070046 SCREENCAP # PrintScreen
  0x00070048 SUSPEND # Pause
  # Insert,Delete,Home,End,PageUp,PageDown available.
  
  0x0007001a UP    # w
  0x00070004 LEFT  # a
  0x00070016 DOWN  # s
  0x00070007 RIGHT # d
  
  0x00070050 LEFT  # arrows...
  0x0007004f RIGHT
  0x00070052 UP
  0x00070051 DOWN
  
  0x00070060 UP # keypad as dpad...
  0x0007005c LEFT
  0x0007005d DOWN
  0x0007005e RIGHT
  0x0007005a DOWN
  
  0x00070037 SOUTH # dot. pairs well with WASD...
  0x0007000f WEST  # l
  0x00070033 EAST  # semicolon
  0x00070013 NORTH # p
  
  0x0007001d SOUTH # z. pairs well with arrows...
  0x0007001b WEST  # x
  0x00070006 EAST  # c
  0x00070019 NORTH # v
  
  0x00070062 SOUTH # kp 0
  0x00070058 WEST  # kp enter
  0x00070057 EAST  # kp plus
  0x00070063 NORTH # kp dot
  0x0007005f L1    # kp 7
  0x00070061 R1    # kp 9
  0x00070059 L2    # kp 1
  0x0007005b R2    # kp 3
  0x00070054 AUX1  # kp slash
  0x00070055 AUX2  # kp star
  0x00070056 AUX3  # kp dash
  
  0x00070028 AUX1 # enter
  0x0007002c AUX1 # space
  0x00070030 AUX2 # right bracket
  0x0007002f AUX3 # left bracket
  
  0x0007002b L1 # tab
  0x00070031 R1 # backslash
  0x00070035 L2 # tilde
  0x0007002a R2 # backspace

EOF
