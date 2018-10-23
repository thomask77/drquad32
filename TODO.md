# 2018-10-23

Serial port (and therefore serial Bluetooth and XBee) connections have some reliability problems.
-> This may be caused by a Bug in QSerialPort!!!

Use QuadControl/Attic/SerialThread2.cpp as a possible fix!


# 2016-07-22

## QuadControl

  * Qt Charts 5.7 statt qcustomplot
  * Qt 3D und/oder three.js statt eigenem OpenGL-View
  * Gamepad-Input

  * https://github.com/Riateche/toolwindowmanager


# 2016-01-04


## Use CRC32 Hardware in Bootloader

The bootloader checks for 4-byte aligned size.
Why does this work at all? Is this guaranteed by the linker script?

Maybe add a padding option to add_version_info.py


### Here's a trick for arbitrary (i.e. not 4 byte aligned) lengths:

https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FCRC%20computation&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=7852

Posted: 11/2/2013 9:20 PM
scaldov.miroslav


Fully hardware method:

<code>
uint32_t reverse_32(uint32_t data)
{
        asm("rbit r0,r0");
        return data;
};

uint32_t crc32_ether(char *buf, int len, int clear)
{
        uint32_t *p = (uint32_t*) buf;
        uint32_t crc, crc_reg;
        if(clear) CRC_ResetDR();
        while(len >= 4) {
                crc_reg = CRC_CalcCRC(reverse_32(*p++));
                len -= 4;
        }
        crc = reverse_32(crc_reg);
        if(len) {
                CRC_CalcCRC(crc_reg);
                switch(len) {
                        case 1:
                        crc_reg = CRC_CalcCRC(reverse_32((*p & 0xFF) ^ crc) >> 24);
                        crc = ( crc >> 8 ) ^ reverse_32(crc_reg);
                        break;
                        case 2:
                        crc_reg = CRC_CalcCRC(reverse_32((*p & 0xFFFF) ^ crc) >> 16);
                        crc = ( crc >> 16 ) ^ reverse_32(crc_reg);
                        break;
                        case 3:
                        crc_reg = CRC_CalcCRC(reverse_32((*p & 0xFFFFFF) ^ crc) >> 8);
                        crc = ( crc >> 24 ) ^ reverse_32(crc_reg);
                        break;
                }
        }
        return ~crc;
}
</code>



## Merge tk_quadcontrol branch

## Merge add_version_info

## Replace  #pragma once markers  by normal #ifdef guards.
