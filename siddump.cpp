#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include "cpu.h"
#include "SidOutput.h"
#include "SidState.h"

#define MAX_INSTR 0x100000

int main(int argc, char **argv);
unsigned char readbyte(FILE *f);
unsigned short readword(FILE *f);

extern unsigned short pc;

SidOutput *output;
SidState sid;
SidOutputOptions options;
SidOutputFactory factory;

int main(int argc, char **argv)
{
  int subtune = 0;
  int instr = 0;
  int firstframe = 0;
  int usage = 0;
  int mode = 0;

  unsigned loadend;
  unsigned loadpos;
  unsigned loadsize;
  unsigned loadaddress;
  unsigned initaddress;
  unsigned playaddress;
  unsigned dataoffset;

  FILE *in;
  char *sidname = 0;
  int c;

  // Scan arguments
  for (c = 1; c < argc; c++)
  {
    if (argv[c][0] == '-')
    {
      switch(toupper(argv[c][1]))
      {
        case '?':
        usage = 1;
        break;

        case 'A':
        sscanf(&argv[c][2], "%u", &subtune);
        break;

        case 'C':
        sscanf(&argv[c][2], "%x", &options.basefreq);
        break;

        case 'D':
        sscanf(&argv[c][2], "%x", &options.basenote);
        break;

        case 'F':
        sscanf(&argv[c][2], "%u", &options.firstframe);
        break;

        case 'L':
        options.lowres = 1;
        break;

        case 'M':
        sscanf(&argv[c][2], "%u", &mode);
        break;

        case 'N':
        sscanf(&argv[c][2], "%u", &options.spacing);
        break;

        case 'O':
        sscanf(&argv[c][2], "%u", &options.oldnotefactor);
        if (options.oldnotefactor < 1) options.oldnotefactor = 1;
        break;

        case 'P':
        sscanf(&argv[c][2], "%u", &options.pattspacing);
        break;

        case 'S':
        options.timeseconds = 1;
        break;

        case 'T':
        sscanf(&argv[c][2], "%u", &options.seconds);
        break;
        
        case 'Z':
        options.profiling = 1;
        break;
      }
    }
    else 
    {
      if (!sidname) sidname = argv[c];
    }
  }

  // Usage display
  if ((argc < 2) || (usage))
  {
    printf("Usage: SIDDUMP <sidfile> [options]\n"
           "Warning: CPU emulation may be buggy/inaccurate, illegals support very limited\n\n"
           "Options:\n"
           "-a<value> Accumulator value on init (subtune number) default = 0\n"
           "-c<value> Frequency recalibration. Give note frequency in hex\n"
           "-d<value> Select calibration note (abs.notation 80-DF). Default middle-C (B0)\n"
           "-f<value> First frame to display, default 0\n"
           "-l        Low-resolution mode (only display 1 row per note)\n"
           "-m        Output mode, default 0\n"
           "          0 = output to screen, with note information\n"
           "          1 = output to screen, sid registers only\n"
           "          2 = output to binary file, all sid registers per frame\n"
           "          3 = output to c friendly include file, all sid registers per frame\n"
           "          4 = output to binary file, all sid registers + timing HI/LO bytes per frame\n"
           "          5 = output to screen, only output changed sid registers inc. timing HI/LO bytes\n"
           "          6 = output to binary file, only output changed sid registers inc. timing HI/LO bytes\n"
           "-n<value> Note spacing, default 0 (none)\n"
           "-o<value> ""Oldnote-sticky"" factor. Default 1, increase for better vibrato display\n"
           "          (when increased, requires well calibrated frequencies)\n"
           "-p<value> Pattern spacing, default 0 (none)\n"
           "-s        Display time in minutes:seconds:frame format\n"
           "-t<value> Playback time in seconds, default 60\n"
           "-z        Include CPU cycles+rastertime (PAL)+rastertime, badline corrected\n");
    return 1;
  }
  // use the factory to create the requested output object type
  output = factory.create(mode);

  strcpy(options.songfilename, sidname);
  output->setOptions(&options);

  // Open SID file
  if (!sidname)
  {
    printf("Error: no SID file specified.\n");
    return 1;
  }

  in = fopen(sidname, "rb");
  if (!in)
  {
    printf("Error: couldn't open SID file.\n");
    return 1;
  }

  // Read interesting parts of the SID header
  fseek(in, 6, SEEK_SET);
  dataoffset = readword(in);
  loadaddress = readword(in);
  initaddress = readword(in);
  playaddress = readword(in);
  fseek(in, dataoffset, SEEK_SET);
  if (loadaddress == 0)
    loadaddress = readbyte(in) | (readbyte(in) << 8);

  // Load the C64 data
  loadpos = ftell(in);
  fseek(in, 0, SEEK_END);
  loadend = ftell(in);
  fseek(in, loadpos, SEEK_SET);
  loadsize = loadend - loadpos;
  if (loadsize + loadaddress >= 0x10000)
  {
    printf("Error: SID data continues past end of C64 memory.\n");
    fclose(in);
    return 1;
  }
  fread(&mem[loadaddress], loadsize, 1, in);
  fclose(in);

  // Print info & run initroutine
  printf("Load address: $%04X Init address: $%04X Play address: $%04X\n", loadaddress, initaddress, playaddress);
  printf("Calling initroutine with subtune %d\n", subtune);
  mem[0x01] = 0x37;
  initcpu(initaddress, subtune, 0, 0);
  instr = 0;
  while (runcpu())
  {
    // Allow SID model detection (including $d011 wait) to eventually terminate
    ++mem[0xd012];
    if (!mem[0xd012] || ((mem[0xd011] & 0x80) && mem[0xd012] >= 0x38))
    {
        mem[0xd011] ^= 0x80;
        mem[0xd012] = 0x00;
    }
    instr++;
    if (instr > MAX_INSTR)
    {
      printf("Warning: CPU executed a high number of instructions in init, breaking\n");
      break;
    }
  }

  if (playaddress == 0)
  {
    printf("Warning: SID has play address 0, reading from interrupt vector instead\n");
    if ((mem[0x01] & 0x07) == 0x5)
      playaddress = mem[0xfffe] | (mem[0xffff] << 8);
    else
      playaddress = mem[0x314] | (mem[0x315] << 8);
    printf("New play address is $%04X\n", playaddress);
  }

  sid.reset();
  sid.time.end_time = options.seconds * 1000000;  // in us
  printf("Calling playroutine for %d seconds, starting from frame %d\n", options.seconds, firstframe);
  // printf("Calling playroutine for %d frames, starting from frame %d\n", options.seconds*50, firstframe);

  output->preProcessing();
  
  // Data collection & display loop
  // while (frames < firstframe + options.seconds*50)
  while (sid.isPlaying)
  {
    // Run the playroutine
    instr = 0;
    initcpu(playaddress, 0, 0, 0);
    while (runcpu())
    {
      instr++;
      if (instr > MAX_INSTR)
      {
        printf("Error: CPU executed abnormally high amount of instructions in playroutine, exiting\n");
        return 1;
      }
      // Test for jump into Kernal interrupt handler exit
      if ((mem[0x01] & 0x07) != 0x5 && (pc == 0xea31 || pc == 0xea81))
        break;
    }

    // Get SID parameters from each channel and the filter
    sid.update(mem);

    // Frame display
    // if (frames >= firstframe)
    if (sid.time.current_frame >= firstframe)
      output->processCurrentFrame(sid);

    // Advance to next frame
    sid.tick();
    // frames++;
  }

  output->postProcessing();

  // cleanup
  delete output;
  return 0;
}

unsigned char readbyte(FILE *f)
{
  unsigned char res;

  fread(&res, 1, 1, f);
  return res;
}

unsigned short readword(FILE *f)
{
  unsigned char res[2];

  fread(&res, 2, 1, f);
  return (res[0] << 8) | res[1];
}
