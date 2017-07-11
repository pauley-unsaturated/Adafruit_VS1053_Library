/*************************************************** 
  This is a library for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#ifndef ADAFRUIT_VS1053_FILEPLAYER_H
#define ADAFRUIT_VS1053_FILEPLAYER_H

#include <Adafruit_VS1053.h>

class Adafruit_VS1053_FilePlayer {
 public:
  Adafruit_VS1053_FilePlayer (Adafruit_VS1053& vs, int8_t cardCS);

  boolean begin(void);
  boolean useInterrupt(uint8_t type);
  File currentTrack;
  volatile boolean playingMusic;
  void feedBuffer(void);
  boolean startPlayingFile(const char *trackname);
  boolean playFullFile(const char *trackname);
  void stopPlaying(void);
  boolean paused(void);
  boolean stopped(void);
  void pausePlaying(boolean pause);

 private:
  void feedBuffer_noLock(void);

  Adafruit_VS1053& _vs;
  uint8_t _cardCS;
};

#endif // ADAFRUIT_VS1053 FILEPLAYER_H
