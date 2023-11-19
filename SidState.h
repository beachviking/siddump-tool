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

struct TimingInfo
{
  unsigned int current_time;  // in us
  unsigned int end_time;  // in us
  unsigned int current_frame;
};

struct SidState
{
  Voice voice[3];
  Filter filt;
  TimingInfo time;
  bool isPlaying;
  unsigned int sidreg[27];  // 0-24 sid regs, 25 = dt HI, 26 = dt LO
  // unsigned int sidreg[25];

  SidState () {isPlaying = false;}

  void reset();
  void update(unsigned char *mem);
  void tick();
  void dumpCurrentState();

  int sid_baseaddr = 0xd400;
  // u_int32_t cia_val = 0x0000;
};

void SidState::reset()
{
  memset(&voice, 0, sizeof(voice));
  memset(&filt, 0, sizeof(filt));
  memset(&sidreg, 0, sizeof(sidreg));
  memset(&time, 0, sizeof(time));
  isPlaying = true;
}

// update sid variables from memory
void SidState::update(unsigned char *mem)
{
    // update properties
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

    // update registers
    for(int i = 0; i < 25; i++)
      sidreg[i] = mem[sid_baseaddr + i];

    // cia_val = (mem[0xdc04] << 8) | (mem[0xdc05]);
    if (mem[0xdc05] == 0 && mem[0xdc04] == 0) {
      // Most likely vbi driven, ie. 20000us
      sidreg[25] = 0x4e; // dt HI
      sidreg[26] = 0x20; // dt LO
    } else {
      // CIA timer is used to control updates...
      sidreg[25] = mem[0xdc05]; // dt HI
      sidreg[26] = mem[0xdc04]; // dt LO
    }
}

// increment frame and time variables based on simulation timings
void SidState::tick()
{
    ++time.current_frame;
    time.current_time += (sidreg[25] << 8) | (sidreg[26]);

    if (time.current_time >= time.end_time)
      isPlaying = false;
}

void SidState::dumpCurrentState()
{
  char output[512];
  output[0] = 0;

  sprintf(&output[strlen(output)],"");
}
