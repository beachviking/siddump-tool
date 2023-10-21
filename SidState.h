#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Voice
{
  unsigned short freq;
  unsigned short pulse;
  unsigned short adsr;
  unsigned char wave;
  int note;
};

struct Filter
{
  unsigned short cutoff;
  unsigned char ctrl;
  unsigned char type;
};

struct SidState
{
  Voice voice[3];
  Filter filt;

  void reset();
  void update(unsigned char *mem);
  void dumpCurrentState();

  int sid_baseaddr = 0xd400;
};

void SidState::reset()
{
  memset(&voice, 0, sizeof voice);
  memset(&filt, 0, sizeof filt);
}

void SidState::update(unsigned char *mem)
{
    for (int v = 0; v < 3; v++)
    {
      voice[v].freq = mem[sid_baseaddr + 7*v] | (mem[sid_baseaddr + 1 + 7*v] << 8);
      voice[v].pulse = (mem[sid_baseaddr + 2 + 7*v] | (mem[sid_baseaddr + 3 + 7*v] << 8)) & 0xfff;
      voice[v].wave = mem[sid_baseaddr + 4 + 7*v];
      voice[v].adsr = mem[sid_baseaddr + 6 + 7*v] | (mem[sid_baseaddr + 5 + 7*v] << 8);
    }
    filt.cutoff = (mem[sid_baseaddr + 0x15] & 0x7) | (mem[sid_baseaddr + 0x16] << 3);
    filt.ctrl = mem[sid_baseaddr + 0x17];
    filt.type = mem[sid_baseaddr + 0x18];  
}

void SidState::dumpCurrentState()
{
  char output[512];
  output[0] = 0;

  sprintf(&output[strlen(output)],"");
}
