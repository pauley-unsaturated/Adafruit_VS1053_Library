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

#include <Adafruit_VS1053_FilePlayer.h>
#include <SD.h>

static boolean feedBufferLock = false;

static void FilePlayer_Feeder(void* ctx) {
  ((Adafruit_VS1053_FilePlayer*)ctx)->feedBuffer();
}

boolean Adafruit_VS1053_FilePlayer::useInterrupt(uint8_t type) {
  Adafruit_VS1053::Callback cb = {this, FilePlayer_Feeder};
  _vs.useInterrupt(type, cb);
}

Adafruit_VS1053_FilePlayer::Adafruit_VS1053_FilePlayer(
               Adafruit_VS1053& vs, int8_t cardCS)
  : _vs(vs), _cardCS(cardCS), playingMusic(false) {
}

boolean Adafruit_VS1053_FilePlayer::begin(void) {
  // Set the card to be disabled while we get the VS1053 up
  pinMode(_cardCS, OUTPUT);
  digitalWrite(_cardCS, HIGH);  

  uint8_t v  = _vs.begin();

  //dumpRegs();
  //Serial.print("Version = "); Serial.println(v);
  return (v == 4);
}


boolean Adafruit_VS1053_FilePlayer::playFullFile(const char *trackname) {
  if (! startPlayingFile(trackname)) return false;

  while (playingMusic) {
    // twiddle thumbs
    feedBuffer();
    delay(5);           // give IRQs a chance
  }
  // music file finished!
  return true;
}

void Adafruit_VS1053_FilePlayer::stopPlaying(void) {
  // cancel all playback
  _vs.sciWrite(VS1053_REG_MODE,
               VS1053_MODE_SM_LINE1
               | VS1053_MODE_SM_SDINEW
               | VS1053_MODE_SM_CANCEL);
  
  // wrap it up!
  playingMusic = false;
  currentTrack.close();
}

void Adafruit_VS1053_FilePlayer::pausePlaying(boolean pause) {
  if (pause) 
    playingMusic = false;
  else {
    playingMusic = true;
    feedBuffer();
  }
}

boolean Adafruit_VS1053_FilePlayer::paused(void) {
  return (!playingMusic && currentTrack);
}

boolean Adafruit_VS1053_FilePlayer::stopped(void) {
  return (!playingMusic && !currentTrack);
}


boolean Adafruit_VS1053_FilePlayer::startPlayingFile(const char *trackname) {
  // reset playback
  _vs.sciWrite(VS1053_REG_MODE, VS1053_MODE_SM_LINE1 | VS1053_MODE_SM_SDINEW);
  // resync
  _vs.sciWrite(VS1053_REG_WRAMADDR, 0x1e29);
  _vs.sciWrite(VS1053_REG_WRAM, 0);

  currentTrack = SD.open(trackname);
  if (!currentTrack) {
    return false;
  }

  // don't let the IRQ get triggered by accident here
  noInterrupts();

  // As explained in datasheet, set twice 0 in REG_DECODETIME to set time back to 0
  _vs.sciWrite(VS1053_REG_DECODETIME, 0x00);
  _vs.sciWrite(VS1053_REG_DECODETIME, 0x00);

  playingMusic = true;

  // wait till its ready for data
  while (! _vs.readyForData() ) {
#if defined(ESP8266)
	yield();
#endif
  }

  // fill it up!
  while (playingMusic && _vs.readyForData()) {
    feedBuffer();
  }
  
  // ok going forward, we can use the IRQ
  interrupts();

  return true;
}

void Adafruit_VS1053_FilePlayer::feedBuffer(void) {
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

void Adafruit_VS1053_FilePlayer::feedBuffer_noLock(void) {
  if ((! playingMusic) // paused or stopped
      || (! currentTrack) 
      || (! _vs.readyForData())) {
    return; // paused or stopped
  }

  // Feed the hungry buffer! :)
  while (_vs.readyForData()) {
    // Read some audio data from the SD card file
    int bytesread = currentTrack.read(_vs.out_buffer, VS1053_DATABUFFERLEN);
    
    if (bytesread == 0) {
      // must be at the end of the file, wrap it up!
      playingMusic = false;
      currentTrack.close();
      break;
    }

    _vs.playData(_vs.out_buffer, bytesread);
  }
}
