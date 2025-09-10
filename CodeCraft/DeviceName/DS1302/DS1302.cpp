#include "DS1302.h"

DS1302::DS1302(PinName SCLK, PinName IO, PinName CE) : _SCLK(SCLK), _IO(IO), _CE(CE)
{
    _CE = 0;
    _SCLK = 0;
    _IO.input();
    writeProtect = true;
}

void DS1302::set_time(time_t t)
{
    struct tm *_t = localtime(&t);
    writeReg(Seconds, (_t->tm_sec % 10) + ((_t->tm_sec / 10) << 4));
    writeReg(Minutes, (_t->tm_min % 10) + ((_t->tm_min / 10) << 4));
    writeReg(Hours, (_t->tm_hour % 10) + ((_t->tm_hour / 10) << 4));
    writeReg(Dates, (_t->tm_mday % 10) + ((_t->tm_mday / 10) << 4));
    writeReg(Months, ((_t->tm_mon + 1) % 10) + (((_t->tm_mon + 1) / 10) << 4));
    writeReg(Days, _t->tm_wday + 1);
    writeReg(Years, ((_t->tm_year - 100) % 10) + (((_t->tm_year - 100) / 10) << 4));
}

time_t DS1302::time(time_t  *t)
{
    char regs[7];
    _CE = 1;
    wait_us(4);
    writeByte(ClockBurst | 1);
    for (int i = 0; i<7; i++)
        regs[i] = readByte();
    _CE = 0;

    struct tm _t;
    _t.tm_sec = (regs[0] & 0xF) + (regs[0] >> 4) * 10;
    _t.tm_min = (regs[1] & 0xF) + (regs[1] >> 4) * 10;
    _t.tm_hour = (regs[2] & 0xF) + (regs[2] >> 4) * 10;
    _t.tm_mday = (regs[3] & 0xF) + (regs[3] >> 4) * 10;
    _t.tm_mon = (regs[4] & 0xF) + (regs[4] >> 4) * 10 - 1;
    _t.tm_year = (regs[6] & 0xF) + (regs[6] >> 4) * 10 + 100;

    // convert to timestamp and display (1256729737)
    return mktime(&_t);
}

void DS1302::storeByte(char address, char data)
{
    if (address > 30)
        return;
    char command = RAMBase + (address << 1);
    writeReg(command, data);
}

char DS1302::recallByte(char address)
{
    if (address > 30)
        return 0;
    char command = RAMBase + (address << 1) + 1;
    return readReg(command);
}

char DS1302::readReg(char reg)
{
    char retval;

    _CE = 1;
    wait_us(4);
    writeByte(reg);
    retval = readByte();
    wait_us(4);
    _CE = 0;
    return retval;
}

void DS1302::writeReg(char reg, char val)
{
    if (writeProtect) {
        writeProtect = false;
        writeReg(WriteProtect, 0);
    }
    _CE = 1;
    wait_us(4);
    writeByte(reg);
    writeByte(val);
    wait_us(4);
    _CE = 0;
}


/*********************PRIVATE***********************/
void DS1302::writeByte(char data)
{
    _IO.output();
    for (int i = 0; i<8; i++) {
        _IO = data & 0x01;
        wait_us(1);
        _SCLK = 1;
        wait_us(1);
        _SCLK = 0;
        data >>= 1;
    }
    _IO.input();
}

char DS1302::readByte(void)
{
    char retval = 0;

    _IO.input();
    for (int i = 0; i<8; i++) {
        retval |= _IO << i;
        wait_us(1);
        _SCLK = 1;
        wait_us(1);
        _SCLK = 0;
    }
    return retval;
}