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
#ifndef ADAFRUIT_VS1053_STREAMPLAYER_H
#define ADAFRUIT_VS1053_STREAMPLAYER_H

#include <Adafruit_VS1053.h>

typedef int (*Adafruit_VS1053_AudioStremaCallbackFunc)(void* context,
                                                       uint8_t* audio_buf,
                                                       uint16_t audio_buf_size);
struct Adafruit_VS1053_AudioStreamCallback {
  Adafruit_VS1053_AudioStreamCallback(void* c,
                                      Adafruit_VS1053_AudioStremaCallbackFunc f)
    : cb(f), ctx(c) {}
  
  Adafruit_VS1053_AudioStremaCallbackFunc cb;
  void* ctx;
};

class Adafruit_VS1053_PCMStreamPlayer {
 public:
  Adafruit_VS1053_PCMStreamPlayer(Adafruit_VS1053& vs,
                                  Adafruit_VS1053_AudioStreamCallback cb);

  boolean begin(void);
  boolean useInterrupt(uint8_t type);
  boolean start();
  boolean stop();

  boolean stopped();

  void feedBuffer(void);

  int pcmHeader(uint8_t* headerBuf, int length);
  
 private:
  void feedBuffer_noLock(void);
  
  Adafruit_VS1053& _vs;
  Adafruit_VS1053_AudioStreamCallback _streamCallback;
  uint32_t _sampleRate;
  uint16_t _numChannels;
  uint16_t _bitsPerSample;
  boolean _playing;
};

#endif
