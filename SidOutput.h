#pragma once
#include <stdbool.h>
#include "SidState.h"

struct SidOutputOptions
{
  int seconds = 60;
  int instr = 0;
  int frames = 0;
  int spacing = 0;
  int pattspacing = 0;
  int firstframe = 0;
  int counter = 0;
  int basefreq = 0;
  int basenote = 0xb0;
  int lowres = 0;
  int rows = 0;
  int oldnotefactor = 1;
  int timeseconds = 0;
  int profiling = 0;
  char songfilename[64] = {0}; 
};

class SidOutput {
  public:

    virtual ~SidOutput() = default;

    // pure virtual functions
    virtual void preProcessing() = 0;
    virtual void processCurrentFrame(SidState current, int frames) = 0;
    virtual void postProcessing() = 0;

    void setOptions(SidOutputOptions *options) {opts = options;}

  protected:
    SidOutputOptions *opts;
 
};

class BinaryFileOutputRegisterDumps : public SidOutput {
  public:

    // pure virtual functions
    virtual void preProcessing() 
    {
      char filename[64] = {0};
      strcpy(filename, opts->songfilename);
      strcat(filename, ".dmp");
      outbinary = fopen(filename, "wb");
      if (!outbinary)
      {
          printf("Error: couldn't write binary file");
          fclose(outbinary);
          return;
      }      
    };

    virtual void processCurrentFrame(SidState current, int frames) {
      for(int i=0; i < 25; i++)
        fputc(current.sidreg[i], outbinary);  
    };

    virtual void postProcessing() {
      fclose(outbinary);  
    };

  private:
    FILE *outbinary = NULL;
};

class BinaryFileOutputRegisterAndDtDumps : public SidOutput {
  public:

    // pure virtual functions
    virtual void preProcessing() 
    {
      char filename[64] = {0};
      strcpy(filename, opts->songfilename);
      strcat(filename, ".dmp");
      outbinary = fopen(filename, "wb");
      if (!outbinary)
      {
          printf("Error: couldn't write binary file");
          fclose(outbinary);
          return;
      }      
    };

    virtual void processCurrentFrame(SidState current, int frames) {
      // for(int i=0; i < 25; i++)
      for(int i=0; i < 27; i++)
        fputc(current.sidreg[i], outbinary);  
    };

    virtual void postProcessing() {
      fclose(outbinary);  
    };

  private:
    FILE *outbinary = NULL;
};

class IncludeFileOutputRegisterDumps : public SidOutput {
  public:

    // pure virtual functions
    virtual void preProcessing() 
    {
      char filename[64] = {0};
      strcpy(filename, opts->songfilename);
      strcat(filename, ".h");
      txtfile = fopen(filename, "w");
      if (!txtfile)
      {
          printf("Error: couldn't write binary file");
          fclose(txtfile);
          return;
      }
      fprintf(txtfile, "unsigned char %s[] = {\n", "sound_data");
    };
    
    virtual void processCurrentFrame(SidState current, int frames) {
      
      for(int i=0; i < 25; i++)
      {
        fprintf(txtfile, "  ");
        fprintf(txtfile, "0x%02x,", current.sidreg[i]);
      }

      fprintf(txtfile, "\n");          
    };

    virtual void postProcessing() {
      fprintf(txtfile, "};\n");
      fclose(txtfile);  
    };

  private:
    FILE *txtfile = NULL;
};

class ScreenOutputRegistersOnly : public SidOutput {
  public:
      ScreenOutputRegistersOnly() { prev_state.reset(); }
      // pure virtual function
      virtual void preProcessing()
      {
        printf("| Frame | 00 01 02 03 04 05 06 | 07 08 09 10 11 12 13 | 14 15 16 17 18 19 20 | 21 22 23 24 | dt_us |");
        printf("\n");
        printf("+-------+----------------------+----------------------+----------------------+-------------+-------+");
        printf("\n");
      }

      virtual void processCurrentFrame(SidState current, int frames)
      {
        static int counter = 0;
        static int rows = 0;

        char output[512];
        int time = frames - opts->firstframe;
        output[0] = 0;      

        if (!opts->timeseconds)
          sprintf(&output[strlen(output)], "| %5d | ", time);
        else
          sprintf(&output[strlen(output)], "|%01d:%02d.%02d| ", time/3000, (time/50)%60, time%50);

        // Loop for all registers
        for (int c = 0; c < 25; c++)
        {
          if ((current.sidreg[c] != prev_state.sidreg[c]) || (time == 0))
            sprintf(&output[strlen(output)], "%02X ", current.sidreg[c]);
          else
            sprintf(&output[strlen(output)], ".. ");

          if(c == 6 || c == 13 || c == 20)
            sprintf(&output[strlen(output)], "| ");

          prev_state.sidreg[c] = current.sidreg[c];
        } 

        //sprintf(&output[strlen(output)], "| %04X ", current.cia_val);
        sprintf(&output[strlen(output)], "|  %04X ", (current.sidreg[25] << 8) | (current.sidreg[26]));
        sprintf(&output[strlen(output)], "|\n");
        printf("%s", output);
      }

    virtual void postProcessing(){}

    private:
      SidState prev_state;
};

class ScreenOutputWithNotes : public SidOutput {
  public:
    void setOptions(SidOutputOptions *options) {
      SidOutput::setOptions(options);

      // Recalibrate frequencytable
      if (opts->basefreq)
      {
        opts->basenote &= 0x7f;
        if ((opts->basenote < 0) || (opts->basenote > 96))
        {
          printf("Warning: Calibration note out of range. Aborting recalibration.\n");
        }
        else
        {
          for (int c = 0; c < 96; c++)
          {
            double note = c - opts->basenote;
            double freq = (double)opts->basefreq * pow(2.0, note/12.0);
            int f = freq;
            if (freq > 0xffff) freq = 0xffff;
            freqtbllo[c] = f & 0xff;
            freqtblhi[c] = f >> 8;
          }
        }
      }
      // Check other parameters for correctness
      if ((opts->lowres) && (!opts->spacing)) opts->lowres = 0;
    }

      // pure virtual function
      virtual void preProcessing()
      {
        printf("Middle C frequency is $%04X\n\n", freqtbllo[48] | (freqtblhi[48] << 8));

        printf("| Frame | Freq Note/Abs WF ADSR Pul | Freq Note/Abs WF ADSR Pul | Freq Note/Abs WF ADSR Pul | FCut RC Typ V |");
        if (opts->profiling)

        { // CPU cycles, Raster lines, Raster lines with badlines on every 8th line, first line included
          printf(" Cycl RL RB |");
        }
        printf("\n");
        printf("+-------+---------------------------+---------------------------+---------------------------+---------------+");
        if (opts->profiling)
        {
          printf("------------+");
        }
        printf("\n");

        prev_state.reset();
        prev_state2.reset();
      }

      virtual void processCurrentFrame(SidState current, int frames)
      {
        static int counter = 0;
        static int rows = 0;

        char output[512];
        int time = frames - opts->firstframe;
        output[0] = 0;      

        if (!opts->timeseconds)
          sprintf(&output[strlen(output)], "| %5d | ", time);
        else
          sprintf(&output[strlen(output)], "|%01d:%02d.%02d| ", time/3000, (time/50)%60, time%50);

        // Loop for each channel
        for (int c = 0; c < 3; c++)
        {
          int newnote = 0;

          // Keyoff-keyon sequence detection
          if (current.voice[c].wave >= 0x10)
          {
            if ((current.voice[c].wave & 1) && ((!(prev_state2.voice[c].wave & 1)) || (prev_state.voice[c].wave < 0x10)))
              prev_state.voice[c].note = -1;
          }

          // Frequency
          if ((frames == opts->firstframe) || (prev_state.voice[c].note == -1) || (current.voice[c].freq != prev_state.voice[c].freq))
          {
            int d;
            int dist = 0x7fffffff;
            int delta = ((int)current.voice[c].freq) - ((int)prev_state2.voice[c].freq);

            sprintf(&output[strlen(output)], "%04X ", current.voice[c].freq);

            if (current.voice[c].wave >= 0x10)
            {
              // Get new note number
              for (d = 0; d < 96; d++)
              {
                int cmpfreq = freqtbllo[d] | (freqtblhi[d] << 8);
                int freq = current.voice[c].freq;

                if (abs(freq - cmpfreq) < dist)
                {
                  dist = abs(freq - cmpfreq);
                  // Favor the old note
                  if (d == prev_state.voice[c].note) dist /= opts->oldnotefactor;
                    current.voice[c].note = d;
                }
              }

              // Print new note
              if (current.voice[c].note != prev_state.voice[c].note)
              {
                if (prev_state.voice[c].note == -1)
                  {
                    if (opts->lowres) newnote = 1;
                    sprintf(&output[strlen(output)], " %s %02X  ", notename[current.voice[c].note], current.voice[c].note | 0x80);
                }
                  else
                  sprintf(&output[strlen(output)], "(%s %02X) ", notename[current.voice[c].note], current.voice[c].note | 0x80);
              }
              else
              {
                // If same note, print frequency change (slide/vibrato)
                if (delta)
                {
                  if (delta > 0)
                      sprintf(&output[strlen(output)], "(+ %04X) ", delta);
                    else
                      sprintf(&output[strlen(output)], "(- %04X) ", -delta);
                }
                else sprintf(&output[strlen(output)], " ... ..  ");
              }
            }
            else sprintf(&output[strlen(output)], " ... ..  ");
          }
          else sprintf(&output[strlen(output)], "....  ... ..  ");

          // Waveform
          if ((frames == opts->firstframe) || (newnote) || (current.voice[c].wave != prev_state.voice[c].wave))
            sprintf(&output[strlen(output)], "%02X ", current.voice[c].wave);
          else sprintf(&output[strlen(output)], ".. ");

          // ADSR
          if ((frames == opts->firstframe) || (newnote) || (current.voice[c].adsr != prev_state.voice[c].adsr)) sprintf(&output[strlen(output)], "%04X ", current.voice[c].adsr);
          else sprintf(&output[strlen(output)], ".... ");

          // Pulse
          if ((frames == opts->firstframe) || (newnote) || (current.voice[c].pulse != prev_state.voice[c].pulse)) sprintf(&output[strlen(output)], "%03X ", current.voice[c].pulse);
          else sprintf(&output[strlen(output)], "... ");

          sprintf(&output[strlen(output)], "| ");
        }

        // Filter cutoff
        if ((frames == opts->firstframe) || (current.filt.cutoff != prev_state.filt.cutoff)) sprintf(&output[strlen(output)], "%04X ", current.filt.cutoff);
        else sprintf(&output[strlen(output)], ".... ");

        // Filter control
        if ((frames == opts->firstframe) || (current.filt.ctrl != prev_state.filt.ctrl))
          sprintf(&output[strlen(output)], "%02X ", current.filt.ctrl);
        else sprintf(&output[strlen(output)], ".. ");

        // Filter passband
        if ((frames == opts->firstframe) || ((current.filt.type & 0x70) != (prev_state.filt.type & 0x70)))
          sprintf(&output[strlen(output)], "%s ", filtername[(current.filt.type >> 4) & 0x7]);
        else sprintf(&output[strlen(output)], "... ");

        // Mastervolume
        if ((frames == opts->firstframe) || ((current.filt.type & 0xf) != (prev_state.filt.type & 0xf))) sprintf(&output[strlen(output)], "%01X ", current.filt.type & 0xf);
        else sprintf(&output[strlen(output)], ". ");
        
        // Rasterlines / cycle count
        if (opts->profiling)
        {
          int cycles = cpucycles;
          int rasterlines = (cycles + 62) / 63;
          int badlines = ((cycles + 503) / 504);
          int rasterlinesbad = (badlines * 40 + cycles + 62) / 63;
          sprintf(&output[strlen(output)], "| %4d %02X %02X ", cycles, rasterlines, rasterlinesbad);
        }
        
        // End of frame display, print info so far and copy SID registers to old registers
        sprintf(&output[strlen(output)], "|\n");
        if ((!opts->lowres) || (!((frames - opts->firstframe) % opts->spacing)))
        {
          printf("%s", output);
          for (int c = 0; c < 3; c++)
          {
            prev_state.voice[c] = current.voice[c];
          }
          prev_state.filt = current.filt;
        }
        for (int c = 0; c < 3; c++) prev_state2.voice[c] = current.voice[c];

        // Print note/pattern separators
        if (opts->spacing)
        {
          opts->counter++;
          if (counter >= opts->spacing)
          {
            counter = 0;
            if (opts->pattspacing)
            {
              rows++;
              if (rows >= opts->pattspacing)
              {
                rows = 0;
                printf("+=======+===========================+===========================+===========================+===============+\n");
              }
              else
                if (!opts->lowres) printf("+-------+---------------------------+---------------------------+---------------------------+---------------+\n");
            }
            else
              if (!opts->lowres) printf("+-------+---------------------------+---------------------------+---------------------------+---------------+\n");
          }
        }
      }

    virtual void postProcessing(){}

    private:
      SidState prev_state;
      SidState prev_state2;

      static const char *notename[];
      static const char *filtername[];

      static unsigned int freqtbllo[];
      static unsigned int freqtblhi[];
};

const char *ScreenOutputWithNotes::notename[] =
{"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
  "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
  "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
  "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
  "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
  "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
  "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
  "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7"};

const char *ScreenOutputWithNotes::filtername[] =
{"Off", "Low", "Bnd", "L+B", "Hi ", "L+H", "B+H", "LBH"};

unsigned int ScreenOutputWithNotes::freqtbllo[] = {
  0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
  0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
  0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
  0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
  0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
  0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
  0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
  0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff};

unsigned int ScreenOutputWithNotes::freqtblhi[] = {
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
  0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
  0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
  0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
  0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
  0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
  0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff};

