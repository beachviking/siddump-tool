#pragma once
#include <stdbool.h>


class SidOutput {
  public:
      SidOutput() = default;
      virtual ~SidOutput() = default;

      // pure virtual function
      virtual bool preProcessing() = 0;
      virtual bool processFrame() = 0;
      virtual bool postProcessing() = 0;
 
};

class ScreenOutput : SidOutput {
  public:
      // pure virtual function
      bool preProcessing() {return true;}
      bool processFrame() {return true;}
      bool postProcessing() {return true;}
};