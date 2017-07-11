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

#include <Adafruit_VS1053_StreamPlayer.h>

volatile boolean feedBufferLock = false;

/***************************************************************/
/*          VS1053 LPCM sample streaming interface             */
/***************************************************************/

#define FLIP_ENDIAN_32(x) (    \
  (((x) & (0xFF<<24)) >> 24) | \
  (((x) & (0xFF<<16)) >> 8)  | \
  (((x) & (0xFF<< 8)) << 8)  | \
  (((x) & (0xFF<< 0)) << 24))

#define FLIP_ENDIAN_16(x) (    \
  (((x) & (0xFF<<8)) >> 8)   | \
  (((x) & (0xFF<<0)) << 8))

struct __attribute__ ((packed)) LPCM_WAV_HEADER {
  const char ChunkID[4] = {'R','I','F','F'};
  const uint32_t ChunkSize = FLIP_ENDIAN_32(0xFFFFFFFF);
  const char Format[4] = {'W','A','V','E'};
  const char SubChunk1ID[4] = {'f','m','t',' '};
  const uint32_t SubChunk1Size = 0x00000010;
  const uint16_t AudioFormat = 0x01; // PCM
  const uint16_t NumChannels = 0x01; // Mono (for now)
  const uint32_t SampleRate  = 44100;
  const uint32_t ByteRate    = 88200; // 16 bits
  const uint16_t BlockAlign  = 2;
  const uint16_t BitsPerSample = 16;
  const char SubChunk2ID[4] = {'d','a','t','a'};
  const uint32_t SubChunkSize = 0xFFFFFFFF;
};

Adafruit_VS1053_PCMStreamPlayer::Adafruit_VS1053_PCMStreamPlayer(
  Adafruit_VS1053& vs,
  Adafruit_VS1053_AudioStreamCallback cb)
  : _vs(vs), _streamCallback(cb), _playing(false) {
  // empty
}

int Adafruit_VS1053_PCMStreamPlayer::pcmHeader(uint8_t *headerBuf, int length) {
  LPCM_WAV_HEADER header;

  if(length < sizeof(header)) {
    return 0;
  }
  memcpy(headerBuf, &header, sizeof(header));

  return sizeof(header);
}

boolean Adafruit_VS1053_PCMStreamPlayer::begin(void) {
  uint8_t v  = _vs.begin();

  //dumpRegs();
  //Serial.print("Version = "); Serial.println(v);
  return (v == 4);
}

boolean Adafruit_VS1053_PCMStreamPlayer::start(void) {
  LPCM_WAV_HEADER header;
  
  // don't let the IRQ get triggered by accident here
  noInterrupts();

  // As explained in datasheet, set twice 0 in REG_DECODETIME to set time back to 0
  _vs.sciWrite(VS1053_REG_DECODETIME, 0x00);
  _vs.sciWrite(VS1053_REG_DECODETIME, 0x00);

  _playing = true;
  
  // wait till its ready for data
  while (! _vs.readyForData() ) {
#if defined(ESP8266)
    yield();
#endif
  }

  _vs.playData((uint8_t*)&header, sizeof(header));
  
  // fill it up!
  while (_playing && _vs.readyForData()) {
    feedBuffer();
  }
  
  // ok going forward, we can use the IRQ
  interrupts();
  
  return true;
}

boolean Adafruit_VS1053_PCMStreamPlayer::stop(void) {
  _playing = false;
  return true;
}

boolean Adafruit_VS1053_PCMStreamPlayer::stopped(void) {
  return !_playing;
}

void Adafruit_VS1053_PCMStreamPlayer::feedBuffer(void) {
  noInterrupts();
  // dont run twice in case interrupts collided
  // This isn't a perfect lock as it may lose one feedBuffer request if
  // an interrupt occurs before feedBufferLock is reset to false. This
  // may cause a glitch in the audio but at least it will not corrupt
  // state.
  if (feedBufferLock) {
    interrupts();
    return;
  }
  feedBufferLock = true;
  interrupts();

  feedBuffer_noLock();

  feedBufferLock = false;
}

void Adafruit_VS1053_PCMStreamPlayer::feedBuffer_noLock(void) {
  if ((! _playing) // paused or stopped
      || (! _vs.readyForData())) {
    return; // paused or stopped
  }

  // Feed the hungry buffer! :)
  while (_vs.readyForData()) {
    // Read some audio data from the stream callback
    int bytesread = _streamCallback.cb(_streamCallback.ctx,
                                       _vs.out_buffer, VS1053_DATABUFFERLEN);
    
    if (bytesread == 0) {
      // must be at the end of the stream, wrap it up!
      _playing = false;
      break;
    }

    _vs.playData(_vs.out_buffer, bytesread);
  }
}

static void PCMStreamPlayer_Feeder(void* context) {
  ((Adafruit_VS1053_PCMStreamPlayer*)context)->feedBuffer();
}

boolean Adafruit_VS1053_PCMStreamPlayer::useInterrupt(uint8_t type)
{
  Adafruit_VS1053::Callback cb = {this, PCMStreamPlayer_Feeder};
  return _vs.useInterrupt(type, cb);
}
