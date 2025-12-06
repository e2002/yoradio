#include "../core/options.h"
#if I2S_DOUT==255
/*
 *  vs1053_ext.cpp
 *
 *  Created on: Jul 09.2017
 *  Updated on: Oct 22.2023
 *      Author: Wolle
 */
#ifndef VS_PATCH_ENABLE
#define VS_PATCH_ENABLE  true
#endif
#include "../core/config.h"
#include "audioVS1053Ex.h"

//---------------------------------------------------------------------------------------------------------------------
AudioBuffer::AudioBuffer(size_t maxBlockSize) {
    // if maxBlockSize isn't set use defaultspace (1600 bytes) is enough for aac and mp3 player
    if(maxBlockSize) m_resBuffSizeRAM  = maxBlockSize;
    if(maxBlockSize) m_maxBlockSize = maxBlockSize;
}

AudioBuffer::~AudioBuffer() {
    if(m_buffer)
        free(m_buffer);
    m_buffer = NULL;
}

size_t AudioBuffer::init() {
    if(m_buffer) free(m_buffer);
    m_buffer = NULL;
    if(psramInit()) {
        // PSRAM found, AudioBuffer will be allocated in PSRAM
        m_buffSize = m_buffSizePSRAM;
        if(m_buffer == NULL) {
            m_buffer = (uint8_t*) ps_calloc(m_buffSize, sizeof(uint8_t));
            m_buffSize = m_buffSizePSRAM - m_resBuffSizePSRAM;
            if(m_buffer == NULL) {
                // not enough space in PSRAM, use ESP32 Flash Memory instead
                m_buffer = (uint8_t*) calloc(m_buffSize, sizeof(uint8_t));
                m_buffSize = m_buffSizeRAM - m_resBuffSizeRAM;
            }
        }
    } else {  // no PSRAM available, use ESP32 Flash Memory"
        m_buffSize = m_buffSizeRAM * config.store.abuff;
        m_buffer = (uint8_t*) calloc(m_buffSize, sizeof(uint8_t));
        m_buffSize = m_buffSizeRAM * config.store.abuff - m_resBuffSizeRAM;
    }
    if(!m_buffer)
        return 0;
    resetBuffer();
    return m_buffSize;
}

void AudioBuffer::changeMaxBlockSize(uint16_t mbs){
    m_maxBlockSize = mbs;
    return;
}

uint16_t AudioBuffer::getMaxBlockSize(){
    return m_maxBlockSize;
}

size_t AudioBuffer::freeSpace() {
    if(m_readPtr >= m_writePtr) {
        m_freeSpace = (m_readPtr - m_writePtr);
    } else {
        m_freeSpace = (m_endPtr - m_writePtr) + (m_readPtr - m_buffer);
    }
    if(m_f_start)
        m_freeSpace = m_buffSize;
    return m_freeSpace - 1;
}

size_t AudioBuffer::writeSpace() {
    if(m_readPtr >= m_writePtr) {
        m_writeSpace = (m_readPtr - m_writePtr - 1); // readPtr must not be overtaken
    } else {
        if(getReadPos() == 0)
            m_writeSpace = (m_endPtr - m_writePtr - 1);
        else
            m_writeSpace = (m_endPtr - m_writePtr);
    }
    if(m_f_start)
        m_writeSpace = m_buffSize - 1;
    return m_writeSpace;
}

size_t AudioBuffer::bufferFilled() {
    if(m_writePtr >= m_readPtr) {
        m_dataLength = (m_writePtr - m_readPtr);
    } else {
        m_dataLength = (m_endPtr - m_readPtr) + (m_writePtr - m_buffer);
    }
    return m_dataLength;
}

void AudioBuffer::bytesWritten(size_t bw) {
    m_writePtr += bw;
    if(m_writePtr == m_endPtr) {
        m_writePtr = m_buffer;
    }
    if(bw && m_f_start)
        m_f_start = false;
}

void AudioBuffer::bytesWasRead(size_t br) {
    m_readPtr += br;
    if(m_readPtr >= m_endPtr) {
        size_t tmp = m_readPtr - m_endPtr;
        m_readPtr = m_buffer + tmp;
    }
}

uint8_t* AudioBuffer::getWritePtr() {
    return m_writePtr;
}

uint8_t* AudioBuffer::getReadPtr() {
    size_t len = m_endPtr - m_readPtr;
    if(len < m_maxBlockSize) { // be sure the last frame is completed
        memcpy(m_endPtr, m_buffer, m_maxBlockSize - len);  // cpy from m_buffer to m_endPtr with len
    }
return m_readPtr;
}

void AudioBuffer::resetBuffer() {
    m_writePtr = m_buffer;
    m_readPtr = m_buffer;
    m_endPtr = m_buffer + m_buffSize;
    m_f_start = true;
    // memset(m_buffer, 0, m_buffSize); //Clear Inputbuffer
}

uint32_t AudioBuffer::getWritePos() {
    return m_writePtr - m_buffer;
}

uint32_t AudioBuffer::getReadPos() {
    return m_readPtr - m_buffer;
}
//---------------------------------------------------------------------------------------------------------------------
// **** VS1053 Impl ****
//---------------------------------------------------------------------------------------------------------------------
Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, SPIClass *spi)
{
    dreq_pin = _dreq_pin;
    dcs_pin  = _dcs_pin;
    cs_pin   = _cs_pin;

    spi_VS1053 = spi;
    spi_VS1053->begin();

#ifdef AUDIO_LOG
    m_f_Log = true;
#endif

    #define __malloc_heap_psram(size) \
        heap_caps_malloc_prefer(size, 2, MALLOC_CAP_DEFAULT|MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT|MALLOC_CAP_INTERNAL)

    m_f_psramFound = psramInit();
    if(m_f_psramFound) m_chbufSize = 4096; else m_chbufSize = 512 + 64;
    if(m_f_psramFound) m_ibuffSize = 4096; else m_ibuffSize = 512 + 64;
    m_lastHost = (char*)    __malloc_heap_psram(512);
    m_chbuf    = (char*)    __malloc_heap_psram(m_chbufSize);
    m_ibuff    = (char*)    __malloc_heap_psram(m_ibuffSize);

    if(!m_chbuf || !m_lastHost || !m_ibuff) log_e("oom");

    #define AUDIO_INFO(...) {sprintf(m_ibuff,__VA_ARGS__); if(audio_info) audio_info(m_ibuff);}

    clientsecure.setInsecure();                 // update to ESP32 Arduino version 1.0.5-rc05 or higher
    curvol=50;
    m_LFcount=0;
}
Audio::~Audio(){
    // destructor
    if(m_chbuf)      {free(m_chbuf);       m_chbuf       = NULL;}
    if(m_lastHost)   {free(m_lastHost);    m_lastHost    = NULL;}
    if(m_ibuff)      {free(m_ibuff);       m_ibuff       = NULL;}
    if(m_lastM3U8host){free(m_lastM3U8host); m_lastM3U8host = NULL;}
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::initInBuff() {
    static bool f_already_done = false;
    if(!f_already_done) {
        size_t size = InBuff.init();
        if(size == m_buffSizeRAM - m_resBuffSizeRAM) {
            sprintf(m_chbuf, "PSRAM not found, inputBufferSize: %u bytes", size - 1);
            if(audio_info)  audio_info(m_chbuf);
            f_already_done = true;
        }
        if(size == m_buffSizePSRAM - m_resBuffSizePSRAM) {
            sprintf(m_chbuf, "PSRAM found, inputBufferSize: %u bytes", size - 1);
            if(audio_info) audio_info(m_chbuf);
            f_already_done = true;
        }
    }
    changeMaxBlockSize(4096);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::control_mode_off()
{
    CS_HIGH();                                     // End control mode
    spi_VS1053->endTransaction();                  // Allow other SPI users
}
void Audio::control_mode_on()
{
    spi_VS1053->beginTransaction(VS1053_SPI);       // Prevent other SPI users
    DCS_HIGH();                                     // Bring slave in control mode
    CS_LOW();
}
void Audio::data_mode_on()
{
    spi_VS1053->beginTransaction(VS1053_SPI);       // Prevent other SPI users
    CS_HIGH();                                      // Bring slave in data mode
    DCS_LOW();
}
void Audio::data_mode_off()
{
    //digitalWrite(dcs_pin, HIGH);              // End data mode
    DCS_HIGH();
    spi_VS1053->endTransaction();                       // Allow other SPI users
}
//---------------------------------------------------------------------------------------------------------------------
uint16_t Audio::read_register(uint8_t _reg)
{
    uint16_t result=0;
    control_mode_on();
    spi_VS1053->write(3);                                           // Read operation
    spi_VS1053->write(_reg);                                        // Register to write (0..0xF)
    // Note: transfer16 does not seem to work
    result=(spi_VS1053->transfer(0xFF) << 8) | (spi_VS1053->transfer(0xFF));  // Read 16 bits data
    await_data_request();                                   // Wait for DREQ to be HIGH again
    control_mode_off();
    return result;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::write_register(uint8_t _reg, uint16_t _value)
{
    control_mode_on();
    spi_VS1053->write(2);                                           // Write operation
    spi_VS1053->write(_reg);                                        // Register to write (0..0xF)
    spi_VS1053->write16(_value);                                    // Send 16 bits data
    await_data_request();
    control_mode_off();
}
//---------------------------------------------------------------------------------------------------------------------
size_t Audio::sendBytes(uint8_t* data, size_t len){
    size_t chunk_length = 0;                                // Length of chunk 32 byte or shorter
    size_t bytesDecoded = 0;

    data_mode_on();
    while(len){                                             // More to do?
        if(!digitalRead(dreq_pin)) break;
        chunk_length = len;
        if(len > vs1053_chunk_size){
            chunk_length = vs1053_chunk_size;
        }
        spi_VS1053->writeBytes(data, chunk_length);
        data         += chunk_length;
        len          -= chunk_length;
        bytesDecoded += chunk_length;
    }
    data_mode_off();
    return bytesDecoded;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::sdi_send_buffer(uint8_t* data, size_t len)
{
    size_t chunk_length;                                    // Length of chunk 32 byte or shorter

    data_mode_on();
    while(len){                                             // More to do?

        await_data_request();                               // Wait for space available
        chunk_length=len;
        if(len > vs1053_chunk_size){
            chunk_length=vs1053_chunk_size;
        }
        len-=chunk_length;
        spi_VS1053->writeBytes(data, chunk_length);
        data+=chunk_length;
    }
    data_mode_off();
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::sdi_send_fillers(size_t len){

    size_t chunk_length = 0;                                // Length of chunk 32 byte or shorter

    data_mode_on();
    while(len) {                                            // More to do?
        await_data_request();                               // Wait for space available
        if(len > 0){
            chunk_length = min((size_t)vs1053_chunk_size, len);
        }
        len -= chunk_length;
        while(chunk_length--){
            spi_VS1053->write(m_endFillByte);
        }
    }
    data_mode_off();
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::wram_write(uint16_t address, uint16_t data){

    write_register(SCI_WRAMADDR, address);
    write_register(SCI_WRAM, data);
}
//---------------------------------------------------------------------------------------------------------------------
uint16_t Audio::wram_read(uint16_t address){

    write_register(SCI_WRAMADDR, address);                  // Start reading from WRAM
    return read_register(SCI_WRAM);                         // Read back result
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::begin(){

    pinMode(dreq_pin, INPUT_PULLUP);                        // DREQ is an input
    pinMode(cs_pin, OUTPUT);                                // The SCI and SDI signals
    pinMode(dcs_pin, OUTPUT);
    DCS_HIGH();
    CS_HIGH();
    delay(170);

    // Init SPI in slow mode (0.2 MHz)
    VS1053_SPI = SPISettings(200000, MSBFIRST, SPI_MODE0);
    // printDetails("Right after reset/startup");
    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write(0xC017, 3);                                  // GPIO DDR=3
    wram_write(0xC019, 0);                                  // GPIO ODATA=0
    // printDetails("After test loop");
    softReset();                                            // Do a soft reset

    // Switch on the analog parts
    write_register(SCI_AUDATA, 44100 + 1);                  // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register(SCI_CLOCKF, 6 << 12);                    // Normal clock settings multiplyer 3.0=12.2 MHz
    VS1053_SPI = SPISettings(6700000, MSBFIRST, SPI_MODE0); // SPIDIV 12 -> 80/12=6.66 MHz
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_LINE1));

    await_data_request();
    // Set vu meter
    setVUmeter();
    // m_endFillByte = wram_read(0x1E06) & 0xFF;
    // printDetails("After last clocksetting \n");
    if(VS_PATCH_ENABLE) loadUserCode(); // load in VS1053B if you want to play flac
}
//---------------------------------------------------------------------------------------------------------------------
size_t Audio::bufferFilled(){
    return InBuff.bufferFilled();
}
//---------------------------------------------------------------------------------------------------------------------
size_t Audio::bufferFree(){
    return InBuff.freeSpace();
}
//---------------------------------------------------------------------------------------------------------------------

void Audio::setVolume(uint8_t vol){
    // Set volume.  Both left and right.
    // Input value is 0..21.  21 is the loudest.
    // Clicking reduced by using 0xf8 to 0x00 as limits.
    uint16_t value;                                         // Value to send to SCI_VOL
    uint8_t valueL, valueR;
    int16_t balance_map = map(m_balance, -16, 16, -100, 100);
    
    valueL = vol;
    valueR = vol;
    if (balance_map < 0) {
        valueR = (float)valueR-(float)valueR * abs((float)balance_map)/100;
    } else if (balance_map > 0) {
        valueL = (float)valueL-(float)valueL * abs((float)balance_map)/100;
    }
    curvol = vol;

    uint8_t lgvolL = VS1053VOL(valueL);
    uint8_t lgvolR = VS1053VOL(valueR);
    if(lgvolL==VS1053VOLM) lgvolL=0;
    if(lgvolR==VS1053VOLM) lgvolR=0;
    valueL=map(lgvolL, 0, 254, 0xF8, 0x00);
    valueR=map(lgvolR, 0, 254, 0xF8, 0x00);
    value=(valueL << 8) | valueR;
    write_register(SCI_VOL, value);
/*    uint16_t value;                                         // Value to send to SCI_VOL

    if(vol > 21) vol=21;

    if(vol != curvol){
        curvol = vol;                                       // #20
        vol=volumetable[vol];                               // Save for later use
        value=map(vol, 0, 100, 0xF8, 0x00);                 // 0..100% to one channel
        value=(value << 8) | value;
        write_register(SCI_VOL, value);                     // Volume left and right
    }*/
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setTone(int8_t *rtone){                       // Set bass/treble (4 nibbles)

    // Set tone characteristics.  See documentation for the 4 nibbles.
    uint16_t value=0;                                       // Value to send to SCI_BASS
    int i;                                                  // Loop control

    for(i=0; i < 4; i++)
            {
        value=(value << 4) | rtone[i];                      // Shift next nibble in
    }
    write_register(SCI_BASS, value);                        // Volume left and right
}
/*
Name            Bits      Description
ST AMPLITUDE    15:12      Treble Control in 1.5 dB steps (-8..7, 0 = off)
ST FREQLIMIT    11:8      Lower limit frequency in 1000 Hz steps (1..15)       // 1000..15000
SB AMPLITUDE    7:4        Bass Enhancement in 1 dB steps (0..15, 0 = off)
SB FREQLIMIT    3:0        Lower limit frequency in 10 Hz steps (2..15)        // 20..150
*/
/*
void Audio::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){ 
  if(gainLowPass<0) gainLowPass=0;
  if(gainLowPass>15) gainLowPass=15;
  if(gainBandPass<0) gainBandPass=0;
  if(gainBandPass>13) gainBandPass=13;
  int8_t rtone[] = {(int8_t)map(gainHighPass, -16, 16, -8, 7), (int8_t)(2+gainBandPass), gainLowPass, (int8_t)(15-gainBandPass)};
  setTone(rtone);
}
*/
void Audio::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){ 
  gainHighPass = constrain(gainHighPass, -16, 16);
  gainBandPass = constrain(gainBandPass, -16, 16);
  gainLowPass = constrain(gainLowPass, -16, 16);

  uint8_t trebleFreqLimit = map(-gainBandPass, -16, 16, 1, 15);
  uint8_t bassFreqLimit = map(gainBandPass, -16, 16, 2, 15);
  uint8_t st_amplitude = map(gainHighPass, -16, 16, -8, 7);
  uint8_t sb_amplitude = map(gainLowPass, -16, 16, 0, 15);
  uint16_t sci_bass = (st_amplitude << 12) | (trebleFreqLimit << 8) |
                      (sb_amplitude << 4) | bassFreqLimit;
  write_register(SCI_BASS, sci_bass);
}

void Audio::setBalance(int8_t bal){ 
  m_balance = bal;
  setVolume(curvol);
}
//---------------------------------------------------------------------------------------------------------------------
uint8_t Audio::getVolume(){                                 // Get the currenet volume setting.
    return curvol;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::stopSong(){
    uint16_t modereg;                                       // Read from mode register
    int i;                                                  // Loop control

    if(m_f_running) {
        // It is important to collect endFillByte while still in normal playback
        m_endFillByte = wram_read(0x1E06) & 0xFF;
        m_f_running = false;

        sdi_send_fillers(vs1053_chunk_size * 54);
        delay(10);
        write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_CANCEL));
        for(i=0; i < 200; i++) {
            sdi_send_fillers(vs1053_chunk_size);
            modereg = read_register(SCI_MODE);  // Read status
            if((modereg & _BV(SM_CANCEL)) == 0) {
                sdi_send_fillers(vs1053_chunk_size * 54);
                sprintf(m_chbuf, "Song stopped correctly after %d msec", i * 10);
                if(audio_info) audio_info(m_chbuf);
                return;
            }
            delay(10);
        }
        if(audio_info) audio_info("Song stopped incorrectly!");
        printDetails("after song stopped incorrectly");
    }

    if(audiofile){
      audiofile.close();
    }
    setDatamode(AUDIO_NONE);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::softReset()
{
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_RESET));
    delay(100);
    await_data_request();
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::printDetails(const char* str){

    if(strlen(str) && audio_info) audio_info(str);

    char decbuf[16][6];
    char hexbuf[16][5];
    char binbuf[16][17];
    uint8_t i;

    const char regName[16][12] = {
        "MODE       ", "STATUS     ", "BASS       ", "CLOCKF     ",
        "DECODE_TIME", "AUDATA     ", "WRAM       ", "WRAMADDR   ",
        "HDAT0      ", "HDAT1      ", "AIADDR     ", "VOL        ",
        "AICTRL0    ", "AICTRL1    ", "AICTRL2    ", "AICTRL3    ",
    };

    for(i=0; i <= SCI_AICTRL3; i++){
        sprintf(hexbuf[i], "%04X", read_register(i));
        sprintf(decbuf[i], "%05d", read_register(i));

        uint16_t tmp   = read_register(i);
        uint16_t shift = 0x8000;
        for(int8_t j = 0; j < 16; j++){
            binbuf[i][j] = (tmp & shift ? '1' : '0');
            shift >>= 1;
        }
        binbuf[i][16] = '\0';
    }

    if(audio_info) audio_info("REG            dec      bin               hex");
    if(audio_info) audio_info("-----------  -------  ----------------  -------");

    for(i=0; i <= SCI_AICTRL3; i++){
        sprintf(m_chbuf, "%s   %s   %s   %s", regName[i], decbuf[i], binbuf[i], hexbuf[i]);
        if(audio_info) audio_info(m_chbuf);
    }
}
//---------------------------------------------------------------------------------------------------------------------
uint8_t Audio::printVersion(){
    uint16_t reg = wram_read(0x1E02) & 0xFF;
    return reg;
}

uint32_t Audio::printChipID(){
    uint32_t chipID = 0;
    chipID =  wram_read(0x1E00) << 16;
    chipID += wram_read(0x1E01);
    return chipID;
}
//---------------------------------------------------------------------------------------------------------------------

uint32_t Audio::getBitRate(){
    return (wram_read(0x1e05) & 0xFF) * 1000;  // Kbit/s => bit/s
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::showstreamtitle(const char* ml) {
    // example for ml:
    // StreamTitle='Oliver Frank - Mega Hitmix';StreamUrl='www.radio-welle-woerthersee.at';
    // or adw_ad='true';durationMilliseconds='10135';adId='34254';insertionType='preroll';

    int16_t idx1, idx2, idx4, idx5, idx6, idx7, titleLen = 0, artistLen = 0;
    uint16_t i = 0, hash = 0;

    idx1 = indexOf(ml, "StreamTitle=", 0);         // Streamtitle found
    if(idx1 < 0) idx1 = indexOf(ml, "Title:", 0);  // Title found (e.g. https://stream-hls.bauermedia.pt/comercial.aac/playlist.m3u8)

    if(idx1 >= 0){
        if(indexOf(ml, "xml version=", 7) > 0) {

            /* e.g. xmlStreamTitle
            StreamTitle='<?xml version="1.0" encoding="utf-8"?><RadioInfo><Table><DB_ALBUM_ID>37364</DB_ALBUM_ID>
            <DB_ALBUM_IMAGE>00000037364.jpg</DB_ALBUM_IMAGE><DB_ALBUM_NAME>Boyfriend</DB_ALBUM_NAME>
            <DB_ALBUM_TYPE>Single</DB_ALBUM_TYPE><DB_DALET_ARTIST_NAME>DOVE CAMERON</DB_DALET_ARTIST_NAME>
            <DB_DALET_ITEM_CODE>CD4161</DB_DALET_ITEM_CODE><DB_DALET_TITLE_NAME>BOYFRIEND</DB_DALET_TITLE_NAME>
            <DB_FK_SITE_ID>2</DB_FK_SITE_ID><DB_IS_MUSIC>1</DB_IS_MUSIC><DB_LEAD_ARTIST_ID>26303</DB_LEAD_ARTIST_ID>
            <DB_LEAD_ARTIST_NAME>Dove Cameron</DB_LEAD_ARTIST_NAME><DB_RADIO_IMAGE>cidadefm.jpg</DB_RADIO_IMAGE>
            <DB_RADIO_NAME>Cidade</DB_RADIO_NAME><DB_SONG_ID>120126</DB_SONG_ID><DB_SONG_LYRIC>60981</DB_SONG_LYRIC>
            <DB_SONG_NAME>Boyfriend</DB_SONG_NAME></Table><AnimadorInfo><TITLE>Cidade</TITLE>
            <START_TIME_UTC>2022-11-15T22:00:00+00:00</START_TIME_UTC><END_TIME_UTC>2022-11-16T06:59:59+00:00
            </END_TIME_UTC><SHOW_NAME>Cidade</SHOW_NAME><SHOW_HOURS>22h às 07h</SHOW_HOURS><SHOW_PANEL>0</SHOW_PANEL>
            </AnimadorInfo></RadioInfo>';StreamUrl='';
            */

            idx4 = indexOf(ml, "<DB_DALET_TITLE_NAME>");
            idx5 = indexOf(ml, "</DB_DALET_TITLE_NAME>");

            idx6 = indexOf(ml, "<DB_LEAD_ARTIST_NAME>");
            idx7 = indexOf(ml, "</DB_LEAD_ARTIST_NAME>");

            if(idx4 == -1 || idx5 == -1) return;
            idx4 += 21; // <DB_DALET_TITLE_NAME>
            titleLen = idx5 - idx4;

            if(idx6 != -1 && idx7 != -1){
                idx6 += 21; // <DB_LEAD_ARTIST_NAME>
                artistLen = idx7 - idx6;
            }

            char *title = NULL;
            title = (char*)malloc(titleLen + artistLen + 4);
            memcpy(title, ml + idx4, titleLen); title[titleLen] = '\0';

            char *artist = NULL;
            if(artistLen){
                memcpy(title + titleLen, " - ", 3);
                memcpy(title + titleLen + 3, ml + idx6, artistLen); title[titleLen + 3 + artistLen] = '\0';
            }

            if(title){
                while(i < strlen(title)) {
                    hash += title[i] * i + 1;
                    i++;
                }              
                if(m_streamTitleHash != hash) {
                    m_streamTitleHash = hash;
                    if(audio_showstreamtitle) audio_showstreamtitle(title);
                }
                free(title);
                title = NULL;
            }
            if(artist) {
                free(artist);
                artist = NULL;
            }
            return;
        }

        idx2 = indexOf(ml, ";", idx1);
        char *sTit;
        if(idx2 >= 0){sTit = strndup(ml + idx1, idx2 + 1); sTit[idx2] = '\0';}
        else          sTit =  strdup(ml);

        while(i < strlen(sTit)){hash += sTit[i] * i+1; i++;}

        if(m_streamTitleHash != hash){
            m_streamTitleHash = hash;
            if(audio_info) audio_info(sTit);
            uint8_t pos = 12;                                                   // remove "StreamTitle="
            if(sTit[pos] == '\'') pos++;                                        // remove leading  \'
            if(sTit[strlen(sTit) - 1] == '\'') sTit[strlen(sTit) -1] = '\0';    // remove trailing \'
            if(sTit[pos]==0xEF && sTit[pos+1] == 0xBB && sTit[pos+2] == 0xBF) pos+=3; // remove ZERO WIDTH NO-BREAK SPACE
            if(audio_showstreamtitle) audio_showstreamtitle(sTit + pos);
        }
        free(sTit);
    }
    m_streamTitleHash = 0;
    idx1 = indexOf(ml, "StreamUrl=", 0);
    idx2 = indexOf(ml, ";", idx1);
    if(idx1 >= 0 && idx2 > idx1){                                               // StreamURL found
        uint16_t len = idx2 - idx1;
        char *sUrl;
        sUrl = strndup(ml + idx1, len + 1); sUrl[len] = '\0';

        while(i < strlen(sUrl)){hash += sUrl[i] * i+1; i++;}
        if(m_streamUrlHash != hash){
            m_streamUrlHash = hash;
            if(audio_info) audio_info(sUrl);
        }
        free(sUrl);
    }

    idx1 = indexOf(ml, "adw_ad=", 0);
    if(idx1 >= 0){                                                              // Advertisement found
        idx1 = indexOf(ml, "durationMilliseconds=", 0);
        idx2 = indexOf(ml, ";", idx1);
        if(idx1 >= 0 && idx2 > idx1){
            uint16_t len = idx2 - idx1;
            char *sAdv;
            sAdv = strndup(ml + idx1, len + 1); sAdv[len] = '\0';
            if(audio_info) audio_info(sAdv);
            uint8_t pos = 21;                                                   // remove "StreamTitle="
            if(sAdv[pos] == '\'') pos++;                                        // remove leading  \'
            if(sAdv[strlen(sAdv) - 1] == '\'') sAdv[strlen(sAdv) -1] = '\0';    // remove trailing \'
            if(audio_commercial) audio_commercial(sAdv + pos);
            free(sAdv);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Audio::loop(){

    if(!m_f_running) return;

    if(m_playlistFormat != FORMAT_M3U8){ // normal process
        switch(getDatamode()){
            case AUDIO_LOCALFILE:
                processLocalFile();
                break;
            case HTTP_RESPONSE_HEADER:
                parseHttpResponseHeader();
                break;
            case AUDIO_PLAYLISTINIT:
                readPlayListData();
                break;
            case AUDIO_PLAYLISTDATA:
                if(m_playlistFormat == FORMAT_M3U)  connecttohost(parsePlaylist_M3U());
                if(m_playlistFormat == FORMAT_PLS)  connecttohost(parsePlaylist_PLS());
                if(m_playlistFormat == FORMAT_ASX)  connecttohost(parsePlaylist_ASX());
                break;
            case AUDIO_DATA:
                if(m_streamType == ST_WEBSTREAM) processWebStream();
                if(m_streamType == ST_WEBFILE)   processWebFile();
                break;
        }
    }
    else { // m3u8 datastream only
        const char* host;

        switch(getDatamode()) {
        case HTTP_RESPONSE_HEADER:
            playAudioData(); // fill I2S DMA buffer
            if(!parseHttpResponseHeader()){
                if(m_f_timeout) connecttohost(m_lastHost);
            }
            break;
        case AUDIO_PLAYLISTINIT:
            playAudioData(); // fill I2S DMA buffer
            readPlayListData();
            break;
        case AUDIO_PLAYLISTDATA:
            playAudioData(); // fill I2S DMA buffer
            host = parsePlaylist_M3U8();
            playAudioData(); // fill I2S DMA buffer
            if(host) { // host contains the next playlist URL
                httpPrint(host);
            }
            else { // host == NULL means connect to m3u8 URL
                if(m_lastM3U8host) httpPrint(m_lastM3U8host);
                else               httpPrint(m_lastHost);        // if url has no first redirection
                setDatamode(HTTP_RESPONSE_HEADER);               // we have a new playlist now
            }
            break;
        case AUDIO_DATA:
            if(m_f_ts) { processWebStreamTS(); } // aac or aacp with ts packets
            else { processWebStreamHLS(); }      // aac or aacp normal stream

            if(m_f_continue) { // at this point m_f_continue is true, means processWebStream() needs more data
                setDatamode(AUDIO_PLAYLISTDATA);
                m_f_continue = false;
            }
            break;
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processLocalFile() {

    if(!(audiofile && m_f_running && getDatamode() == AUDIO_LOCALFILE)) return;

    int bytesDecoded = 0;
    uint32_t bytesCanBeWritten = 0;
    uint32_t bytesCanBeRead = 0;
    int32_t bytesAddedToBuffer = 0;
    static bool f_stream;

    if(m_f_firstCall) {  // runs only one time per connection, prepare for start
        m_f_firstCall = false;
        f_stream = false;
        return;
    }

    if(!f_stream && m_controlCounter == 100) {
        f_stream = true;
        if(audio_info) audio_info("stream ready");
        if(m_resumeFilePos){
            InBuff.resetBuffer();
            setFilePos(m_resumeFilePos);
            log_i("m_resumeFilePos %i", m_resumeFilePos);
        }
    }

    bytesCanBeWritten = InBuff.writeSpace();

    //----------------------------------------------------------------------------------------------------
    // some files contain further data after the audio block (e.g. pictures).
    // In that case, the end of the audio block is not the end of the file. An 'eof' has to be forced.
    if((m_controlCounter == 100) && (m_contentlength > 0)) { // fileheader was read
           if(bytesCanBeWritten + getFilePos() >= m_contentlength){
               if(m_contentlength > getFilePos()) bytesCanBeWritten = m_contentlength - getFilePos();
               else bytesCanBeWritten = 0;
           }
    }
    //----------------------------------------------------------------------------------------------------
    bytesAddedToBuffer = audiofile.read(InBuff.getWritePtr(), bytesCanBeWritten);
    if(bytesAddedToBuffer > 0) {
        InBuff.bytesWritten(bytesAddedToBuffer);
    }

    if(bytesAddedToBuffer == -1) bytesAddedToBuffer = 0; // read error? eof?
    bytesCanBeRead = InBuff.bufferFilled();
    if(bytesCanBeRead > InBuff.getMaxBlockSize()) bytesCanBeRead = InBuff.getMaxBlockSize();
    if(bytesCanBeRead == InBuff.getMaxBlockSize()) { // mp3 or aac frame complete?

        if(m_controlCounter != 100){
            if(m_codec == CODEC_WAV){
            //     int res = read_WAV_Header(InBuff.getReadPtr(), bytesCanBeRead);
                m_audioDataSize = getFileSize();
                if(audio_progress) audio_progress(0, m_audioDataSize);
                m_controlCounter = 100;
                eofHeader = true;
            }
            if(m_codec == CODEC_MP3){
                int res = read_ID3_Header(InBuff.getReadPtr(), bytesCanBeRead);
                if(res >= 0) bytesDecoded = res;
                else{ // error, skip header
                    m_controlCounter = 100;
                    eofHeader = true;
                }
            }
            if(m_codec == CODEC_M4A){
            //     int res = read_M4A_Header(InBuff.getReadPtr(), bytesCanBeRead);
            //     if(res >= 0) bytesDecoded = res;
            //     else{ // error, skip header
                    m_audioDataSize = getFileSize();
                    if(audio_progress) audio_progress(0, m_audioDataSize);
                    m_controlCounter = 100;
                    eofHeader = true;
            //     }
            }
            if(m_codec == CODEC_AAC){
                // stream only, no header
                m_audioDataSize = getFileSize();
                if(audio_progress) audio_progress(0, m_audioDataSize);
                m_controlCounter = 100;
                eofHeader = true;
            }

            if(m_codec == CODEC_FLAC){
            //     int res = read_FLAC_Header(InBuff.getReadPtr(), bytesCanBeRead);
            //     if(res >= 0) bytesDecoded = res;
            //     else{ // error, skip header
            //         stopSong();
                    m_audioDataSize = getFileSize();
                    if(audio_progress) audio_progress(0, m_audioDataSize);
                    m_controlCounter = 100;
                    eofHeader = true;
            //     }
            }

            if(m_codec == CODEC_OGG){
                m_controlCounter = 100;
                eofHeader = true;
            }
        }
        else {
            bytesDecoded = sendBytes(InBuff.getReadPtr(), bytesCanBeRead);
        }
        if(bytesDecoded > 0) {InBuff.bytesWasRead(bytesDecoded);}
        return;
    }

    if(!bytesAddedToBuffer) {  // eof
        bytesCanBeRead = InBuff.bufferFilled();
        if(bytesCanBeRead > 200){
            if(bytesCanBeRead > InBuff.getMaxBlockSize()) bytesCanBeRead = InBuff.getMaxBlockSize();
            bytesDecoded = sendBytes(InBuff.getReadPtr(), bytesCanBeRead); // play last chunk(s)
            if(bytesDecoded > 0){
                InBuff.bytesWasRead(bytesDecoded);
                return;
            }
        }
        InBuff.resetBuffer();

        // if(m_f_loop  && f_stream){  //eof
        //     sprintf(m_chbuf, "loop from: %u to: %u", getFilePos(), m_audioDataStart);  //TEST loop
        //     if(audio_info) audio_info(m_chbuf);
        //     setFilePos(m_audioDataStart);
        //     if(m_codec == CODEC_FLAC) FLACDecoderReset();
        //     /*
        //         The current time of the loop mode is not reset,
        //         which will cause the total audio duration to be exceeded.
        //         For example: current time   ====progress bar====>  total audio duration
        //                         3:43        ====================>        3:33
        //     */
        //     m_audioCurrentTime = 0;
        //     return;
        // } //TEST loop

        f_stream = false;
        setDatamode(AUDIO_NONE);
        char *afn =strdup(audiofile.name()); // store temporary the name
        stopSong();
        sprintf(m_chbuf, "End of file \"%s\"", afn);
        if(audio_info) audio_info(m_chbuf);
        if(audio_eof_mp3) audio_eof_mp3(afn);
        if(afn) free(afn);
    }
}
//----------------------------------------------------------------------------------------------------------------------
void Audio::processWebStream() {

    const uint16_t  maxFrameSize = InBuff.getMaxBlockSize();    // every mp3/aac frame is not bigger
    static bool     f_stream;                                   // first audio data received
    static uint32_t chunkSize;                                  // chunkcount read from stream

    // first call, set some values to default  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        f_stream = false;
        chunkSize = 0;
        m_metacount = m_metaint;
        readMetadata(0, true); // reset all static vars
    }

    if(getDatamode() != AUDIO_DATA) return;              // guard
    uint32_t availableBytes = _client->available();      // available from stream

    // chunked data tramsfer - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked && availableBytes){
        uint8_t readedBytes = 0;
        if(!chunkSize) chunkSize = chunkedDataTransfer(&readedBytes);
        availableBytes = min(availableBytes, chunkSize);
    }
    // we have metadata  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_metadata && availableBytes){
        if(m_metacount == 0) {chunkSize -= readMetadata(availableBytes); return;}
        availableBytes = min(availableBytes, m_metacount);
    }

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        if(streamDetection(availableBytes)) return;
    }

    // buffer fill routine - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(availableBytes) {
        availableBytes = min(availableBytes, (uint32_t)InBuff.writeSpace());
        int16_t bytesAddedToBuffer = _client->read(InBuff.getWritePtr(), availableBytes);

        if(bytesAddedToBuffer > 0) {
            if(m_f_metadata)            m_metacount  -= bytesAddedToBuffer;
            if(m_f_chunked)             chunkSize    -= bytesAddedToBuffer;
            InBuff.bytesWritten(bytesAddedToBuffer);
        }

        if(InBuff.bufferFilled() > maxFrameSize && !f_stream) {  // waiting for buffer filled
            f_stream = true;  // ready to play the audio data
            AUDIO_INFO("stream ready");
        }
        if(!f_stream) return;
        if(m_codec == CODEC_OGG){ // log_i("determine correct codec here");
            uint8_t codec = determineOggCodec(InBuff.getReadPtr(), maxFrameSize);
            if(codec == CODEC_FLAC)   {m_codec = CODEC_FLAC;   return;}
            if(codec == CODEC_OPUS)   {m_codec = CODEC_OPUS; AUDIO_INFO("can't play OPUS"); stopSong(); return;}
            if(codec == CODEC_VORBIS) {m_codec = CODEC_VORBIS; return;}
            stopSong();
            return;
        }
    }

    // play audio data - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        static uint8_t cnt = 0;
        cnt++;
        if(cnt == 3){playAudioData(); cnt = 0;}
    }
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processWebStreamTS() {

    const uint16_t  maxFrameSize = InBuff.getMaxBlockSize();    // every mp3/aac frame is not bigger
    uint32_t        availableBytes;                             // available bytes in stream
    static bool     f_stream;                                   // first audio data received
    static bool     f_firstPacket;
    static bool     f_chunkFinished;
    static uint32_t byteCounter;                                // count received data
    static uint8_t  ts_packet[188];                             // m3u8 transport stream is always 188 bytes long
    uint8_t         ts_packetStart = 0;
    uint8_t         ts_packetLength = 0;
    static uint8_t  ts_packetPtr = 0;
    const uint8_t   ts_packetsize = 188;
    static size_t   chunkSize = 0;

    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        f_stream = false;
        f_firstPacket = true;
        f_chunkFinished = false;
        byteCounter = 0;
        chunkSize = 0;
        m_t0 = millis();
        ts_packetPtr = 0;
        m_controlCounter = 0;
        m_f_firstCall = false;
        m_f_m3u8data = true;
    }

    if(getDatamode() != AUDIO_DATA) return;        // guard

    if(InBuff.freeSpace() < maxFrameSize && f_stream){playAudioData(); return;}

    availableBytes = _client->available();
    if(availableBytes){

        /* If the m3u8 stream uses 'chunked data transfer' no content length is supplied. Then the chunk size determines the audio data to be processed.
           However, the chunk size in some streams is limited to 32768 bytes, although the chunk can be larger. Then the chunk size is
           calculated again. The data used to calculate (here readedBytes) the chunk size is not part of it.
        */

        uint8_t readedBytes = 0;
        uint32_t minBytes = 0;
        if(m_f_chunked && chunkSize == byteCounter) chunkSize += chunkedDataTransfer(&readedBytes);
        if(chunkSize) minBytes = min3(availableBytes, ts_packetsize - ts_packetPtr, chunkSize - byteCounter);
        else          minBytes = min(availableBytes, (uint32_t)(ts_packetsize - ts_packetPtr));

        int res = _client->read(ts_packet + ts_packetPtr, minBytes);
        if(res > 0){
            ts_packetPtr += res;
            byteCounter += res;
            if((chunkSize == byteCounter) && (chunkSize % 32768 == 0)) {return;} // recalculate chunk size
            if(ts_packetPtr < ts_packetsize) return; // not enough data yet, the process must be repeated if the packet size (188 bytes) is not reached
            ts_packetPtr = 0;
            if(f_firstPacket){  // search for ID3 Header in the first packet
                f_firstPacket = false;
                uint8_t ID3_HeaderSize = process_m3u8_ID3_Header(ts_packet);
                if(ID3_HeaderSize > ts_packetsize){
                    log_e("ID3 Header is too big");
                    stopSong();
                    return;
                }
                if(ID3_HeaderSize){
                    memcpy(ts_packet, &ts_packet[ID3_HeaderSize], ts_packetsize - ID3_HeaderSize);
                    ts_packetPtr = ts_packetsize - ID3_HeaderSize;
                    return;
                }
            }

            ts_parsePacket(&ts_packet[0], &ts_packetStart, &ts_packetLength); // todo: check for errors

            if(ts_packetLength) {
                size_t ws = InBuff.writeSpace();
                if(ws >= ts_packetLength){
                    memcpy(InBuff.getWritePtr(), ts_packet + ts_packetStart, ts_packetLength);
                    InBuff.bytesWritten(ts_packetLength);
                }
                else{
                    memcpy(InBuff.getWritePtr(), ts_packet + ts_packetStart, ws);
                    InBuff.bytesWritten(ws);
                    memcpy(InBuff.getWritePtr(), &ts_packet[ws + ts_packetStart], ts_packetLength -ws);
                    InBuff.bytesWritten(ts_packetLength -ws);
                }
            }
            if(byteCounter == m_contentlength  || byteCounter == chunkSize){
                f_chunkFinished = true;
                byteCounter = 0;
                int av = _client->available();
                if(av == 7) for(int i = 0; i < av; i++) _client->read(); // waste last chunksize: 0x0D 0x0A 0x30 0x0D 0x0A 0x0D 0x0A (==0, end of chunked data transfer)
            }
            if(m_contentlength && byteCounter > m_contentlength) {log_e("byteCounter overflow, byteCounter: %d, contentlength: %d", byteCounter, m_contentlength); return;}
            if(chunkSize       && byteCounter > chunkSize)       {log_e("byteCounter overflow, byteCounter: %d, chunkSize: %d",     byteCounter, chunkSize); return;}
        }

    }
    if(f_chunkFinished) {
        if(m_f_psramFound) {
            if(InBuff.bufferFilled() < 50000) { f_chunkFinished = false; m_f_continue = true;}
        }
        else {
            f_chunkFinished = false;
            m_f_continue = true;
        }
    }

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        if(streamDetection(availableBytes)) return;
    }

    // buffer fill routine  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(true) { // statement has no effect
        if(InBuff.bufferFilled() > maxFrameSize && !f_stream) {  // waiting for buffer filled
            f_stream = true;  // ready to play the audio data
            uint16_t filltime = millis() - m_t0;
            if(m_f_Log) AUDIO_INFO("stream ready");
            if(m_f_Log) AUDIO_INFO("buffer filled in %d ms", filltime);
        }
        if(!f_stream) return;
    }

    // play audio data - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        static uint8_t cnt = 0;
        cnt++;
        if(cnt == 1){playAudioData(); cnt = 0;} // aac only
    }
    return;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processWebStreamHLS() {

    const uint16_t  maxFrameSize = InBuff.getMaxBlockSize();    // every mp3/aac frame is not bigger
    uint16_t  ID3BuffSize = 1024; if(m_f_psramFound) ID3BuffSize = 4096;
    uint32_t        availableBytes;                             // available bytes in stream
    static bool     f_stream;                                   // first audio data received
    static bool     firstBytes;
    static bool     f_chunkFinished;
    static uint32_t byteCounter;                                // count received data
    static size_t   chunkSize = 0;
    static uint16_t ID3WritePtr;
    static uint16_t ID3ReadPtr;
    static uint8_t* ID3Buff;

    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        f_stream = false;
        f_chunkFinished = false;
        byteCounter = 0;
        chunkSize = 0;
        ID3WritePtr = 0;
        ID3ReadPtr = 0;
        m_t0 = millis();
        m_f_firstCall = false;
        m_f_m3u8data = true;
        firstBytes = true;
        ID3Buff = (uint8_t*)malloc(ID3BuffSize);
        m_controlCounter = 0;
    }

    if(getDatamode() != AUDIO_DATA) return;        // guard

    availableBytes = _client->available();
    if(availableBytes){ // an ID3 header could come here
        uint8_t readedBytes = 0;

        if(m_f_chunked && !chunkSize) {chunkSize = chunkedDataTransfer(&readedBytes); byteCounter += readedBytes;}

        if(firstBytes){
            if(ID3WritePtr < ID3BuffSize){
                ID3WritePtr += _client->readBytes(&ID3Buff[ID3WritePtr], ID3BuffSize - ID3WritePtr);
                return;
            }
            if(m_controlCounter < 100){
                int res = read_ID3_Header(&ID3Buff[ID3ReadPtr], ID3BuffSize - ID3ReadPtr);
                if(res >= 0) ID3ReadPtr += res;
                if(ID3ReadPtr > ID3BuffSize) {AUDIO_INFO("ID3 buffer overflow, use PSRAM to play this stream!"); stopSong(); return;}
                return;
            }
            if(m_controlCounter != 100) return;

            size_t ws = InBuff.writeSpace();
            if(ws >= ID3BuffSize - ID3ReadPtr){
                memcpy(InBuff.getWritePtr(), &ID3Buff[ID3ReadPtr], ID3BuffSize - ID3ReadPtr);
                InBuff.bytesWritten(ID3BuffSize - ID3ReadPtr);
            }
            else{
                memcpy(InBuff.getWritePtr(), &ID3Buff[ID3ReadPtr], ws);
                InBuff.bytesWritten(ws);
                memcpy(InBuff.getWritePtr(), &ID3Buff[ws + ID3ReadPtr], ID3BuffSize - (ID3ReadPtr + ws));
                InBuff.bytesWritten(ID3BuffSize - (ID3ReadPtr + ws));
            }
            if(ID3Buff) free(ID3Buff);
            byteCounter += ID3BuffSize;
            ID3Buff = NULL;
            firstBytes = false;
        }

        size_t bytesWasWritten = 0;
        if(InBuff.writeSpace() >= availableBytes){
            bytesWasWritten = _client->read(InBuff.getWritePtr(), availableBytes);
        }
        else{
            bytesWasWritten = _client->read(InBuff.getWritePtr(), InBuff.writeSpace());
        }
        InBuff.bytesWritten(bytesWasWritten);

        byteCounter += bytesWasWritten;

        if(byteCounter == m_contentlength || byteCounter == chunkSize){
            f_chunkFinished = true;
            byteCounter = 0;
        }
    }

    if(f_chunkFinished) {
        if(m_f_psramFound) {
            if(InBuff.bufferFilled() < 50000) {f_chunkFinished = false; m_f_continue = true;}
        }
        else {
            f_chunkFinished = false;
            m_f_continue = true;
        }
    }

    // if the buffer is often almost empty issue a warning or try a new connection - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        if(streamDetection(availableBytes)) return;
    }

    if(InBuff.bufferFilled() > maxFrameSize && !f_stream) {  // waiting for buffer filled
        f_stream = true;  // ready to play the audio data
        uint16_t filltime = millis() - m_t0;
        if(m_f_Log) AUDIO_INFO("stream ready");
        if(m_f_Log) AUDIO_INFO("buffer filled in %d ms", filltime);
    }

    // play audio data - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        static uint8_t cnt = 0;
        cnt++;
        if(cnt == 1){playAudioData(); } // aac only
    }
    return;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processWebFile(){

    const uint32_t  maxFrameSize = InBuff.getMaxBlockSize();    // every mp3/aac frame is not bigger
    static bool     f_stream;                                   // first audio data received
    static bool     f_webFileDataComplete;                      // all file data received
    static uint32_t byteCounter;                                // count received data
    static uint32_t chunkSize;                                  // chunkcount read from stream
    static size_t   audioDataCount;                             // counts the decoded audiodata only

    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        m_t0 = millis();
        f_webFileDataComplete = false;
        f_stream = false;
        byteCounter = 0;
        chunkSize = 0;
        audioDataCount = 0;
    }

    if(!m_contentlength && !m_f_tts) {log_e("webfile without contentlength!"); stopSong(); return;} // guard

    uint32_t availableBytes = _client->available(); // available from stream

    // chunked data tramsfer - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked){
        uint8_t readedBytes = 0;
        if(!chunkSize) chunkSize = chunkedDataTransfer(&readedBytes);
        availableBytes = min(availableBytes, chunkSize);
        if(m_f_tts) m_contentlength = chunkSize;
    }

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(!f_webFileDataComplete && f_stream){
        if(streamDetection(availableBytes)) return;
    }

    availableBytes = min((uint32_t)InBuff.writeSpace(), availableBytes);
    availableBytes = min(m_contentlength - byteCounter, availableBytes);
    if(m_audioDataSize) availableBytes = min(m_audioDataSize - (byteCounter - m_audioDataStart), availableBytes);

    int16_t bytesAddedToBuffer = _client->read(InBuff.getWritePtr(), availableBytes);

    if(bytesAddedToBuffer > 0) {
        byteCounter  += bytesAddedToBuffer;  // Pull request #42
        if(m_f_chunked)             m_chunkcount   -= bytesAddedToBuffer;
        if(m_controlCounter == 100) audioDataCount += bytesAddedToBuffer;
        InBuff.bytesWritten(bytesAddedToBuffer);
    }

    if(InBuff.bufferFilled() > maxFrameSize && !f_stream) {  // waiting for buffer filled
        f_stream = true;  // ready to play the audio data
        uint16_t filltime = millis() - m_t0;
        if(m_f_Log) AUDIO_INFO("stream ready\nbuffer filled in %d ms", filltime);
    }

    if(!f_stream) return;

    if(m_codec == CODEC_OGG){ // log_i("determine correct codec here");
        uint8_t codec = determineOggCodec(InBuff.getReadPtr(), maxFrameSize);
        if(codec == CODEC_FLAC)   {m_codec = CODEC_FLAC;   return;}
        if(codec == CODEC_OPUS)   {m_codec = CODEC_OPUS; AUDIO_INFO("can't play OPUS"); stopSong(); return;}
        if(codec == CODEC_VORBIS) {m_codec = CODEC_VORBIS; return;}
        stopSong();
        return;
    }

    // // if we have a webfile, read the file header first - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_streamType == ST_WEBFILE && m_controlCounter != 100){
        int32_t bytesRead = 0;
        if(InBuff.bufferFilled() < maxFrameSize) return;
         if(m_codec == CODEC_WAV){
            m_controlCounter = 100;
            eofHeader = true;
        }
        if(m_codec == CODEC_MP3){
            int res = read_ID3_Header(InBuff.getReadPtr(), InBuff.bufferFilled());
            if(res >= 0) bytesRead = res;
            else{m_controlCounter = 100;eofHeader = true;} // error, skip header
        }
        if(m_codec == CODEC_M4A){
            m_controlCounter = 100;
            eofHeader = true;
        }
        if(m_codec == CODEC_FLAC){
            m_controlCounter = 100;
            eofHeader = true;
        }
        if(m_codec == CODEC_VORBIS){
            m_controlCounter = 100;
            eofHeader = true;
        }
        InBuff.bytesWasRead(bytesRead);
        return;
    }

    // end of webfile reached? - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_webFileDataComplete && InBuff.bufferFilled() < InBuff.getMaxBlockSize()){
        if(InBuff.bufferFilled()){
            if(!readID3V1Tag()){
                int bytesDecoded = sendBytes(InBuff.getReadPtr(), InBuff.bufferFilled());
                if(bytesDecoded > 2){InBuff.bytesWasRead(bytesDecoded); return;}
            }
        }
        stopSong(); // Correct close when play known length sound #74 and before callback #11
        if(m_f_tts){
            AUDIO_INFO("End of speech: \"%s\"", m_lastHost);
            if(audio_eof_speech) audio_eof_speech(m_lastHost);
        }
        else{
            AUDIO_INFO("End of webstream: \"%s\"", m_lastHost);
            if(audio_eof_stream) audio_eof_stream(m_lastHost);
        }
        return;
    }

    if(byteCounter == m_contentlength)                    {f_webFileDataComplete = true;}
    if(byteCounter - m_audioDataStart == m_audioDataSize) {f_webFileDataComplete = true;}

    // play audio data - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream){
        static uint8_t cnt = 0;
        uint8_t compression;
        if(m_codec == CODEC_WAV)  compression = 1;
        if(m_codec == CODEC_FLAC) compression = 2;
        else compression = 3;
        cnt++;
        if(cnt == compression){playAudioData(); cnt = 0;}
    }
    return;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::pauseResume() {
    bool retVal = false;
    if(getDatamode() == AUDIO_LOCALFILE || m_streamType == ST_WEBSTREAM || m_streamType == ST_WEBFILE) {
        m_f_running = !m_f_running;
        retVal = true;
    }
    return retVal;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::playAudioData(){

    if(InBuff.bufferFilled() < InBuff.getMaxBlockSize()) return; // guard

    int bytesDecoded = sendBytes(InBuff.getReadPtr(), InBuff.getMaxBlockSize());

    if(bytesDecoded < 0) {  // no syncword found or decode error, try next chunk
        log_i("err bytesDecoded %i", bytesDecoded);
        uint8_t next = 200;
        if(InBuff.bufferFilled() < next) next = InBuff.bufferFilled();
        InBuff.bytesWasRead(next); // try next chunk
    //    m_bytesNotDecoded += next;
    }
    else {
        if(bytesDecoded > 0) {InBuff.bytesWasRead(bytesDecoded); return;}
        if(bytesDecoded == 0) return; // syncword at pos0 found
    }

    return;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::readPlayListData() {

    if(getDatamode() != AUDIO_PLAYLISTINIT) return false;
    if(_client->available() == 0) return false;

    uint32_t chunksize = 0; uint8_t readedBytes = 0;
    if(m_f_chunked) chunksize = chunkedDataTransfer(&readedBytes);

    // reads the content of the playlist and stores it in the vector m_contentlength
    // m_contentlength is a table of pointers to the lines
    char pl[512] = {0}; // playlistLine
    uint32_t ctl  = 0;
    int lines = 0;
    // delete all memory in m_playlistContent
    if(!psramFound() && m_playlistFormat == FORMAT_M3U8){log_e("m3u8 playlists requires PSRAM enabled!");}
    vector_clear_and_shrink(m_playlistContent);
    while(true){  // outer while

        uint32_t ctime = millis();
        uint32_t timeout = 2000; // ms

        while(true) { // inner while
            uint16_t pos = 0;
            while(_client->available()){ // super inner while :-))
                pl[pos] = _client->read();
                ctl++;
                if(pl[pos] == '\n') {pl[pos] = '\0'; pos++; break;}
            //    if(pl[pos] == '&' ) {pl[pos] = '\0'; pos++; break;}
                if(pl[pos] == '\r') {pl[pos] = '\0'; pos++; continue;;}
                pos++;
                if(pos == 510){ pos--; continue;}
                if(pos == 509) {pl[pos] = '\0';}
                if(ctl == chunksize) {pl[pos] = '\0'; break;}
                if(ctl == m_contentlength) {pl[pos] = '\0'; break;}
            }
            if(ctl == chunksize) break;
            if(ctl == m_contentlength) break;
            if(pos) {pl[pos] = '\0'; break;}

            if(ctime + timeout < millis()) {
                log_e("timeout");
                for(int i = 0; i<m_playlistContent.size(); i++) log_e("pl%i = %s", i, m_playlistContent[i]);
                goto exit;}
        } // inner while

        if(startsWith(pl, "<!DOCTYPE")) {AUDIO_INFO("url is a webpage!"); goto exit;}
        if(startsWith(pl, "<html"))     {AUDIO_INFO("url is a webpage!"); goto exit;}
        if(strlen(pl) > 0) m_playlistContent.push_back(x_strdup(pl));
        if(!m_f_psramFound && m_playlistContent.size() == 101){
            AUDIO_INFO("the number of lines in playlist > 100, for bigger playlist use PSRAM!");
            break;
        }
        if(m_playlistContent.size() && m_playlistContent.size() % 1000 == 0) {
            AUDIO_INFO("current playlist line: %lu", (long unsigned)m_playlistContent.size());
        }
        // termination conditions
        // 1. The http response header returns a value for contentLength -> read chars until contentLength is reached
        // 2. no contentLength, but Transfer-Encoding:chunked -> compute chunksize and read until chunksize is reached
        // 3. no chunksize and no contentlengt, but Connection: close -> read all available chars
        if(ctl == m_contentlength){while(_client->available()) _client->read(); break;} // read '\n\n' if exists
        if(ctl == chunksize)      {while(_client->available()) _client->read(); break;}
        if(!_client->connected() && _client->available() == 0) break;

    } // outer while
    lines = m_playlistContent.size();
    for (int i = 0; i < lines ; i++) { // print all string in first vector of 'arr'
        if(m_f_Log) log_i("pl=%i \"%s\"", i, m_playlistContent[i]);
    }
    setDatamode(AUDIO_PLAYLISTDATA);
    return true;

    exit:
        vector_clear_and_shrink(m_playlistContent);
        m_f_running = false;
        setDatamode(AUDIO_NONE);
    return false;
}
//----------------------------------------------------------------------------------------------------------------------
const char* Audio::parsePlaylist_M3U(){
    uint8_t lines = m_playlistContent.size();
    int pos = 0;
    char* host = nullptr;

    for(int i= 0; i < lines; i++){
        if(indexOf(m_playlistContent[i], "#EXTINF:") >= 0) {            // Info?
            pos = indexOf(m_playlistContent[i], ",");                   // Comma in this line?
            if(pos > 0) {
                // Show artist and title if present in metadata
                AUDIO_INFO(m_playlistContent[i] + pos + 1);
            }
            continue;
        }
        if(startsWith(m_playlistContent[i], "#")) {                     // Commentline?
            continue;
        }

        pos = indexOf(m_playlistContent[i], "http://:@", 0);            // ":@"??  remove that!
        if(pos >= 0) {
            AUDIO_INFO("Entry in playlist found: %s", (m_playlistContent[i] + pos + 9));
            host = m_playlistContent[i] + pos + 9;
            break;
        }
        // AUDIO_INFO("Entry in playlist found: %s", pl);
        pos = indexOf(m_playlistContent[i], "http", 0);                 // Search for "http"
        if(pos >= 0) {                                                  // Does URL contain "http://"?
            host = m_playlistContent[i] + pos;                        // Yes, set new host
            break;
        }
    }
    // vector_clear_and_shrink(m_playlistContent);
    return host;
}
//----------------------------------------------------------------------------------------------------------------------
const char* Audio::parsePlaylist_PLS(){
    uint8_t lines = m_playlistContent.size();
    int pos = 0;
    char* host = nullptr;

    for(int i= 0; i < lines; i++){
        if(i == 0){
            if(strlen(m_playlistContent[0]) == 0) goto exit;            // empty line
            if(strcmp(m_playlistContent[0] , "[playlist]") != 0){       // first entry in valid pls
                setDatamode(HTTP_RESPONSE_HEADER);                      // pls is not valid
                AUDIO_INFO("pls is not valid, switch to HTTP_RESPONSE_HEADER");
                goto exit;
            }
            continue;
        }
        if(startsWith(m_playlistContent[i], "File1")) {
            if(host) continue;                                          // we have already a url
            pos = indexOf(m_playlistContent[i], "http", 0);             // File1=http://streamplus30.leonex.de:14840/;
            if(pos >= 0) {                                              // yes, URL contains "http"?
                host = m_playlistContent[i] + pos;                      // Now we have an URL for a stream in host.
            }
            continue;
        }
        if(startsWith(m_playlistContent[i], "Title1")) {                // Title1=Antenne Tirol
            const char* plsStationName = (m_playlistContent[i] + 7);
            if(audio_showstation) audio_showstation(plsStationName);
            AUDIO_INFO("StationName: \"%s\"", plsStationName);
            continue;
        }
        if(startsWith(m_playlistContent[i], "Length1")){
            continue;
        }
        if(indexOf(m_playlistContent[i], "Invalid username") >= 0){     // Unable to access account:
            goto exit;                                                  // Invalid username or password
        }
    }
    return host;

exit:
    m_f_running = false;
    stopSong();
    vector_clear_and_shrink(m_playlistContent);
    setDatamode(AUDIO_NONE);
    return nullptr;
}
//----------------------------------------------------------------------------------------------------------------------
const char* Audio::parsePlaylist_ASX(){                             // Advanced Stream Redirector
    uint8_t lines = m_playlistContent.size();
    bool f_entry = false;
    int pos = 0;
    char* host = nullptr;

    for(int i= 0; i < lines; i++){
        int p1 = indexOf(m_playlistContent[i], "<", 0);
        int p2 = indexOf(m_playlistContent[i], ">", 1);
        if(p1 >= 0 && p2 > p1){                                     // #196 set all between "< ...> to lowercase
            for(uint8_t j = p1; j < p2; j++){
                m_playlistContent[i][j] = toLowerCase(m_playlistContent[i][j]);
            }
        }
        if(indexOf(m_playlistContent[i], "<entry>") >= 0) f_entry = true; // found entry tag (returns -1 if not found)
        if(f_entry) {
            if(indexOf(m_playlistContent[i], "ref href") > 0) {     //  <ref href="http://87.98.217.63:24112/stream" />
                pos = indexOf(m_playlistContent[i], "http", 0);
                if(pos > 0) {
                    host = (m_playlistContent[i] + pos);            // http://87.98.217.63:24112/stream" />
                    int pos1 = indexOf(host, "\"", 0);              // http://87.98.217.63:24112/stream
                    if(pos1 > 0) host[pos1] = '\0';                 // Now we have an URL for a stream in host.
                }
            }
        }
        pos = indexOf(m_playlistContent[i], "<title>", 0);
        if(pos >= 0) {
            char* plsStationName = (m_playlistContent[i] + pos + 7);            // remove <Title>
            pos = indexOf(plsStationName, "</", 0);
            if(pos >= 0){
                *(plsStationName +pos) = 0;                                     // remove </Title>
            }
            if(audio_showstation) audio_showstation(plsStationName);
            AUDIO_INFO("StationName: \"%s\"", plsStationName);
        }

        if(indexOf(m_playlistContent[i], "http") == 0 && !f_entry) {            //url only in asx
            host = m_playlistContent[i];
        }
    }
    return host;
}
//----------------------------------------------------------------------------------------------------------------------
const char* Audio::parsePlaylist_M3U8() {


    // example: audio chunks
    // #EXTM3U
    // #EXT-X-TARGETDURATION:10
    // #EXT-X-MEDIA-SEQUENCE:163374040
    // #EXT-X-DISCONTINUITY
    // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374038.aac
    // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374039.aac

    static uint64_t xMedSeq = 0;
    static boolean f_mediaSeq_found = false;
    boolean f_EXTINF_found = false;
    char llasc[21]; // uint64_t max = 18,446,744,073,709,551,615  thats 20 chars + \0
    if(m_f_firstM3U8call){
        m_f_firstM3U8call = false;
        xMedSeq = 0;
        f_mediaSeq_found = false;
    }

    uint8_t lines = m_playlistContent.size();
    bool f_begin = false;
    const char* ret;
    if(lines){
        for(uint16_t i = 0; i < lines; i++) {
            if(strlen(m_playlistContent[i]) == 0) continue;                    // empty line
            if(startsWith(m_playlistContent[i], "#EXTM3U")) {f_begin = true;  continue;} // what we expected
            if(!f_begin) continue;

            if(startsWith(m_playlistContent[i], "#EXT-X-STREAM-INF:")){
                uint8_t codec = CODEC_NONE;
                ret = m3u8redirection(&codec);
                if(ret) {
                    m_m3u8Codec = codec; // can be AAC or MP3
                    return ret;
                }
            }
            if(m_codec == CODEC_NONE) {m_codec = CODEC_AAC; if(m_m3u8Codec == CODEC_MP3) m_codec = CODEC_MP3;}  // if we have no redirection

            // "#EXT-X-DISCONTINUITY-SEQUENCE: // not used, 0: seek for continuity numbers, is sometimes not set
            // "#EXT-X-MEDIA-SEQUENCE:"        // not used, is unreliable
            if (startsWith(m_playlistContent[i], "#EXT-X-VERSION:")) continue;
            if (startsWith(m_playlistContent[i], "#EXT-X-ALLOW-CACHE:")) continue;
            if (startsWith(m_playlistContent[i], "##")) continue;
            if (startsWith(m_playlistContent[i], "#EXT-X-INDEPENDENT-SEGMENTS")) continue;
            if (startsWith(m_playlistContent[i], "#EXT-X-PROGRAM-DATE-TIME:")) continue;

            if(!f_mediaSeq_found){
                xMedSeq = m3u8_findMediaSeqInURL();
                if(xMedSeq == UINT64_MAX) {
                    log_e("X MEDIA SEQUENCE NUMBER not found");
                    stopSong();
                    return NULL;
                }
                if(xMedSeq > 0) f_mediaSeq_found = true;
                if(xMedSeq == 0){ // mo mediaSeqNr but min 3 times #EXTINF found
                    ;
                }
            }

            if(startsWith(m_playlistContent[i],"#EXTINF")) {
                f_EXTINF_found = true;
                if(STfromEXTINF(m_playlistContent[i])) {showstreamtitle(m_chbuf);}
                i++;
                if(startsWith(m_playlistContent[i], "#")) i++;   // #MY-USER-CHUNK-DATA-1:ON-TEXT-DATA="20....
                if(i == lines) continue; // and exit for()

                char* tmp = nullptr;
                if(!startsWith(m_playlistContent[i], "http")){

                        //  playlist:   http://station.com/aaa/bbb/xxx.m3u8
                        //  chunklist:  http://station.com/aaa/bbb/ddd.aac
                        //  result:     http://station.com/aaa/bbb/ddd.aac

                    if(m_lastM3U8host != 0){
                        tmp = (char*) malloc(strlen(m_lastM3U8host) + strlen(m_playlistContent[i]) + 1);
                        strcpy(tmp, m_lastM3U8host);
                    }
                    else{
                        tmp = (char*) malloc(strlen(m_lastHost) + strlen(m_playlistContent[i]) + 1);
                        strcpy(tmp, m_lastHost);
                    }
                    if(m_playlistContent[i][0] != '/'){

                        //  playlist:   http://station.com/aaa/bbb/xxx.m3u8  // tmp
                        //  chunklist:  ddd.aac                              // m_playlistContent[i]
                        //  result:     http://station.com/aaa/bbb/ddd.aac   // m_playlistContent[i]

                        int idx = lastIndexOf(tmp, "/");
                        tmp[idx  + 1] = '\0';
                        strcat(tmp, m_playlistContent[i]);
                    }
                    else{

                        //  playlist:   http://station.com/aaa/bbb/xxx.m3u8
                        //  chunklist:  /aaa/bbb/ddd.aac
                        //  result:     http://station.com/aaa/bbb/ddd.aac

                        int idx = indexOf(tmp, "/", 8);
                        tmp[idx] = '\0';
                        strcat(tmp, m_playlistContent[i]);
                    }
                }
                else { tmp = strdup(m_playlistContent[i]); }

                if(f_mediaSeq_found){
                    lltoa(xMedSeq, llasc, 10);
                    if(indexOf(tmp, llasc) > 0){
                        m_playlistURL.insert(m_playlistURL.begin(), strdup(tmp));
                        xMedSeq++;
                    }
                    else{
                        lltoa(xMedSeq + 1, llasc, 10);
                        if(indexOf(tmp, llasc) > 0) {
                            m_playlistURL.insert(m_playlistURL.begin(), strdup(tmp));
                            log_w("mediaseq %llu skipped", xMedSeq);
                            xMedSeq+= 2;
                        }
                    }
                }
                else{ // without mediaSeqNr, with hash
                    uint32_t hash = simpleHash(tmp);
                    if(m_hashQueue.size() == 0){
                        m_hashQueue.insert(m_hashQueue.begin(), hash);
                        m_playlistURL.insert(m_playlistURL.begin(), strdup(tmp));
                    }
                    else{
                        bool known = false;
                        for(int i = 0; i< m_hashQueue.size(); i++){
                            if(hash == m_hashQueue[i]){
                                if(m_f_Log) log_i("file already known %s", tmp);
                                known = true;
                            }
                        }
                        if(!known){
                            m_hashQueue.insert(m_hashQueue.begin(), hash);
                            m_playlistURL.insert(m_playlistURL.begin(), strdup(tmp));
                        }
                    }
                    if(m_hashQueue.size() > 20)  m_hashQueue.pop_back();
				}

                if(tmp) {free(tmp); tmp = NULL;}

                continue;
            }
        }
        vector_clear_and_shrink(m_playlistContent); //clear after reading everything, m_playlistContent.size is now 0
    }

    if(m_playlistURL.size() > 0){
        if(m_playlistBuff) {free(m_playlistBuff); m_playlistBuff = NULL;}

        if(m_playlistURL[m_playlistURL.size() -1]) {
                m_playlistBuff = strdup(m_playlistURL[m_playlistURL.size() -1]);
                free( m_playlistURL[m_playlistURL.size() -1]);
                m_playlistURL[m_playlistURL.size() -1] = NULL;
                m_playlistURL.pop_back();
                m_playlistURL.shrink_to_fit();
        }
        if(m_f_Log) log_i("now playing %s", m_playlistBuff);
        if(endsWith(m_playlistBuff, "ts")) m_f_ts = true;
        if(indexOf(m_playlistBuff, ".ts?") > 0) m_f_ts = true;
        return m_playlistBuff;
    }
    else {
        if(f_EXTINF_found){
            if(f_mediaSeq_found){
                if(m_playlistContent.size() == 0) return NULL;
                uint64_t mediaSeq = m3u8_findMediaSeqInURL();
                if(xMedSeq == 0 || xMedSeq == UINT64_MAX) {log_e("xMediaSequence not found"); connecttohost(m_lastHost);}
                if(mediaSeq < xMedSeq){
                    uint64_t diff = xMedSeq - mediaSeq;
                    if(diff < 10) {;}
                    else {
                        if(m_playlistContent.size() > 0){
                            for(int j = 0; j < lines; j++){
                                if(m_f_Log) log_i("lines %i, %s",lines, m_playlistContent[j]);
                            }
                        }
                        else{;}

                        if(m_playlistURL.size() > 0){
                            for(int j = 0; j < m_playlistURL.size(); j++){
                                if(m_f_Log) log_i("m_playlistURL lines %i, %s",j, m_playlistURL[j]);
                            }
                        }
                        else{;}

                        if(m_playlistURL.size() == 0) {connecttohost(m_lastHost);}
                    }
                }
                else{
                    if(mediaSeq != UINT64_MAX){
                        log_e("err, %u packets lost from %u, to %u", mediaSeq - xMedSeq, xMedSeq, mediaSeq);
                    }
                    xMedSeq = mediaSeq;
                }
            } // f_medSeq_found
        }
    }
    playAudioData(); // avoid audio gap
    return NULL;
}
//---------------------------------------------------------------------------------------------------------------------
const char* Audio::m3u8redirection(uint8_t* codec){
    // example: redirection
    // #EXTM3U
    // #EXT-X-STREAM-INF:BANDWIDTH=117500,AVERAGE-BANDWIDTH=117000,CODECS="mp4a.40.2"
    // 112/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174
    // #EXT-X-STREAM-INF:BANDWIDTH=69500,AVERAGE-BANDWIDTH=69000,CODECS="mp4a.40.5"
    // 64/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174
    // #EXT-X-STREAM-INF:BANDWIDTH=37500,AVERAGE-BANDWIDTH=37000,CODECS="mp4a.40.29"
    // 32/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174

    const char codecString[9][11]={
        "mp4a.40.34", // mp3 stream
        "mp4a.40.01", // AAC Main
        "mp4a.40.2",  // MPEG-4 AAC LC
        "mp4a.40.02", // MPEG-4 AAC LC, leading 0 for Aud-OTI compatibility
        "mp4a.40.29", // MPEG-4 HE-AAC v2 (AAC LC + SBR + PS)
        "mp4a.40.42", // xHE-AAC
        "mp4a.40.5",  // MPEG-4 HE-AAC v1 (AAC LC + SBR)
        "mp4a.40.05", // MPEG-4 HE-AAC v1 (AAC LC + SBR), leading 0 for Aud-OTI compatibility
        "mp4a.67",    // MPEG-2 AAC LC
    };

    uint16_t choosenLine = 0;
    uint16_t plcSize = m_playlistContent.size();
    int8_t   cS = 100;

    for(uint16_t i = 0; i < plcSize; i++) { // looking for lowest codeString
        int16_t posCodec = indexOf(m_playlistContent[i], "CODECS=\"mp4a");

        if(posCodec > 0){
            bool found = false;
            for(uint8_t j = 0; j < sizeof(codecString); j++){
                if(indexOf(m_playlistContent[i], codecString[j]) > 0){
                    if(j < cS){cS = j; choosenLine = i;}
                    found = true;
                }
            }
            if(!found) log_w("codeString %s not in list", m_playlistContent[i] + posCodec);
        }
        if(cS == 0)            *codec = CODEC_MP3;
        if(cS > 0 && cS < 100) *codec = CODEC_AAC;
    }

    if(cS == 100) {                             // "mp4a.xx.xx" not found
        *codec = CODEC_AAC;                     // assume AAC
        for(uint16_t i = 0; i < plcSize; i++) { // we have no codeString, looking for "http"
            if(startsWith(m_playlistContent[i], "#EXT-X-STREAM-INF")) choosenLine = i;
        }
    }
    
    char* tmp = nullptr;
    
    // if((!endsWith(m_playlistContent[choosenLine], "m3u8") && indexOf(m_playlistContent[choosenLine], "m3u8?") == -1)) {
    //     // we have a new m3u8 playlist, skip to next line
    //     int pos = indexOf(m_playlistContent[choosenLine - 1], "CODECS=\"mp4a", 18);
    //     if(pos < 0) { // not found
    //         int pos1 = indexOf(m_playlistContent[choosenLine - 1], "CODECS=", 18);
    //         if(pos1 < 0) pos1 = 0;
    //         log_e("codec %s in m3u8 playlist not supported", m_playlistContent[choosenLine - 1] + pos1);
    //         goto exit;
    //     }
    // }
    choosenLine++; // next line is the redirection url

    if(!startsWith(m_playlistContent[choosenLine], "http")) {

        // http://livees.com/prog_index.m3u8 and prog_index48347.aac -->
        // http://livees.com/prog_index48347.aac http://livees.com/prog_index.m3u8 and chunklist022.m3u8 -->
        // http://livees.com/chunklist022.m3u8


        tmp = (char*)malloc(strlen(m_lastHost) + strlen(m_playlistContent[choosenLine]));
        strcpy(tmp, m_lastHost);
        int idx1 = lastIndexOf(tmp, "/");
        strcpy(tmp + idx1 + 1, m_playlistContent[choosenLine]);
    }
    else { tmp = strdup(m_playlistContent[choosenLine]); }

    if(startsWith(m_playlistContent[choosenLine], "../")){
        // ../../2093120-b/RISMI/stream01/streamPlaylist.m3u8
        if(tmp) { free(tmp); tmp = NULL;}
        tmp = (char*)malloc(strlen(m_lastHost) + strlen(m_playlistContent[choosenLine]));
        strcpy(tmp, m_lastHost);
        int idx1 = lastIndexOf(tmp, "/");
        tmp[idx1] = '\0';

        while(startsWith(m_playlistContent[choosenLine], "../")){
            memcpy(m_playlistContent[choosenLine], m_playlistContent[choosenLine] + 3, strlen(m_playlistContent[choosenLine] +3) + 1); // shift << 3
            idx1 = lastIndexOf(tmp, "/");
            tmp[idx1] = '\0';
        }
        strcat(tmp, "/");
        strcat(tmp, m_playlistContent[choosenLine]);
    }

    if(m_playlistContent[choosenLine]) {
        free(m_playlistContent[choosenLine]);
        m_playlistContent[choosenLine] = NULL;
    }
    m_playlistContent[choosenLine] = strdup(tmp);
    if(m_lastM3U8host){free(m_lastM3U8host); m_lastM3U8host = NULL;}
    m_lastM3U8host = strdup(tmp);
    if(tmp) {
        free(tmp);
        tmp = NULL;
    }
    AUDIO_INFO("redirect to %s", m_playlistContent[choosenLine]);
    _client->stop();
    return m_playlistContent[choosenLine];  // it's a redirection, a new m3u8 playlist
}
//---------------------------------------------------------------------------------------------------------------------
uint64_t Audio::m3u8_findMediaSeqInURL(){ // We have no clue what the media sequence is

    char* pEnd;
    uint64_t MediaSeq = 0;
    uint8_t  idx = 0;
    uint16_t linesWithURL[3] = {0};
    char llasc[21]; // uint64_t max = 18,446,744,073,709,551,615  thats 20 chars + \0

    for(uint16_t i = 0; i < m_playlistContent.size(); i++){
        if(startsWith(m_playlistContent[i], "#EXTINF:")) {
            linesWithURL[idx] = i + 1;
            idx++;
            if(idx == 3) break;
        }
    }
    if(idx < 2){
        log_e("not enough lines with \"#EXTINF:\" found");
        return UINT64_MAX;
    }

    // Look for differences from right:                                                    ∨
    // http://lampsifmlive.mdc.akamaized.net/strmLampsi/userLampsi/l_50551_3318804060_229668.aac
    // http://lampsifmlive.mdc.akamaized.net/strmLampsi/userLampsi/l_50551_3318810050_229669.aac
    // go back to first digit:                                                        ∧

    // log_i("m_playlistContent[linesWithURL[0]] %s", m_playlistContent[linesWithURL[0]]);
    // log_i("m_playlistContent[linesWithURL[1]] %s", m_playlistContent[linesWithURL[1]]);
    // log_i("m_playlistContent[linesWithURL[2]] %s", m_playlistContent[linesWithURL[2]]);

    int16_t len = strlen(m_playlistContent[linesWithURL[0]]) - 1;
    int16_t qm = indexOf(m_playlistContent[linesWithURL[0]], "?", 0);
    if(qm > 0) len = qm; // If we find a question mark, look to the left of it

    for(int16_t pos = len; pos >= 0  ; pos--){
        if(isdigit(m_playlistContent[linesWithURL[0]][pos])){
            while(isdigit(m_playlistContent[linesWithURL[0]][pos])) pos--;
            pos++;
            uint64_t a, b, c;
            a = strtoull(m_playlistContent[linesWithURL[0]] + pos, &pEnd, 10);
            b = a + 1;
            c = b + 1;
            lltoa(b, llasc, 10);
            int16_t idx_b = indexOf(m_playlistContent[linesWithURL[1]], llasc, pos - 1);
            lltoa(c, llasc, 10);
            int16_t idx_c = indexOf(m_playlistContent[linesWithURL[2]], llasc, pos - 1);
            if(idx_b > 0 && idx_c > 0 && idx_b - pos < 3 && idx_c - pos < 3){ // idx_b and idx_c must be positive and near pos
                MediaSeq = a;
                AUDIO_INFO("media sequence number: %llu", MediaSeq);
                break;
            }
        }
    }
    return MediaSeq;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::STfromEXTINF(char* str){
    // the result is copied in m_chbuf!!
    // extraxt StreamTitle from m3u #EXTINF line to icy-format
    // orig: #EXTINF:10,title="text="TitleName",artist="ArtistName"
    // conv: StreamTitle=TitleName - ArtistName
    // orig: #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // conv: StreamTitle=text=\"Spot Block End\" amgTrackId=\"9876543\" -

    int t1, t2, t3, n0 = 0, n1 = 0, n2 = 0;

    t1 = indexOf(str, "title", 0);
    if(t1 > 0){
        strcpy(m_chbuf, "StreamTitle="); n0 = 12;
        t2 = t1 + 7; // title="
        t3 = indexOf(str, "\"", t2);
        while(str[t3 - 1] == '\\'){
            t3 = indexOf(str, "\"", t3 + 1);
        }
        if(t2 < 0 || t2 > t3) return false;
        n1 = t3 - t2;
        strncpy(m_chbuf + n0, str + t2, n1);
        m_chbuf[n0 + n1] = '\0';
    }
    t1 = indexOf(str, "artist", 0);
    if(t1 > 0){
        strcpy(m_chbuf + n0 + n1, " - ");   n1 += 3;
        t2 = indexOf(str, "=\"", t1); t2 += 2;
        t3 = indexOf(str, "\"", t2);
        if(t2 < 0 || t2 > t3) return false;
        n2 = t3 - t2;
        strncpy(m_chbuf + n0 + n1, str + t2, n2);
        m_chbuf[n0 + n1 + n2] = '\0';
    }
    return true;
}
//---------------------------------------------------------------------------------------------------------------------
size_t Audio::process_m3u8_ID3_Header(uint8_t* packet){
    uint8_t         ID3version;
    size_t          id3Size;
    bool            m_f_unsync = false, m_f_exthdr = false;
    uint64_t        current_timestamp = 0;

    (void) m_f_unsync;        // [-Wunused-but-set-variable]
    (void) current_timestamp; // [-Wunused-but-set-variable]

    if(specialIndexOf(packet, "ID3", 4) != 0) { // ID3 not found
        if(m_f_Log) log_i("m3u8 file has no mp3 tag");
        return 0; // error, no ID3 signature found
    }
    ID3version = *(packet + 3);
    switch(ID3version){
            case 2:
                m_f_unsync = (*(packet + 5) & 0x80);
                m_f_exthdr = false;
                break;
            case 3:
            case 4:
                m_f_unsync = (*(packet + 5) & 0x80); // bit7
                m_f_exthdr = (*(packet + 5) & 0x40); // bit6 extended header
                break;
    };
    id3Size = bigEndian(&packet[6], 4, 7); //  ID3v2 size  4 * %0xxxxxxx (shift left seven times!!)
    id3Size += 10;
    if(m_f_Log) log_i("ID3 framesSize: %i", id3Size);
    if(m_f_Log) log_i("ID3 version: 2.%i", ID3version);

    if(m_f_exthdr) {
        log_e("ID3 extended header in m3u8 files not supported");
        return 0;
    }
    if(m_f_Log) log_i("ID3 normal frames");

    if(specialIndexOf(&packet[10], "PRIV", 5) != 0) { // tag PRIV not found
        log_e("tag PRIV in m3u8 Id3 Header not found");
        return 0;
    }
    // if tag PRIV exists assume content is "com.apple.streaming.transportStreamTimestamp"
    // a time stamp is expected in the header.

    current_timestamp = (double)bigEndian(&packet[69], 4) / 90000; // seconds

    return id3Size;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::latinToUTF8(char* buff, size_t bufflen, bool UTF8check) {
    // most stations send  strings in UTF-8 but a few sends in latin. To standardize this, all latin strings are
    // converted to UTF-8. If UTF-8 is already present, nothing is done and true is returned.
    // A conversion to UTF-8 extends the string. Therefore it is necessary to know the buffer size. If the converted
    // string does not fit into the buffer, false is returned

    bool     isUTF8 = true;  // assume UTF8
    uint16_t pos = 0;
    uint16_t in = 0;
    uint16_t out = 0;
    uint16_t len = strlen(buff);
    uint8_t  c;

    // We cannot detect if a given string (or byte sequence) is a UTF-8 encoded text as for example each and every series
    // of UTF-8 octets is also a valid (if nonsensical) series of Latin-1 (or some other encoding) octets.
    // However not every series of valid Latin-1 octets are valid UTF-8 series. So you can rule out strings that do not conform
    // to the UTF-8 encoding schema:

    if(UTF8check){
        while(pos < len) {  // check first, if we have a clear UTF-8 string
            c = buff[pos];
            if(c >= 0xC2 && c <= 0xDF) { // may be 2 bytes UTF8, e.g. 0xC2B5 is 'µ' (MICRO SIGN)
                if(pos + 1 == len){
                    isUTF8 = false;
                    break;
                }
                if(buff[pos + 1] < 0x80){
                    log_e("buff[pos + 1] 0x%02x", buff[pos + 1] );
                    isUTF8 = false;
                    break;
                }
                pos += 2;
                continue;
            }
            if(c >= 0xE0 && c <= 0xEF){ // may  be 3 bytes UTF8, e.g. 0xE0A484 is 'ऄ' (DEVANAGARI LETTER SHORT A)
                if(pos + 2 >= len){ //
                    isUTF8 = false;
                    break;
                }
                if(buff[pos + 1] < 0x80 || buff[pos + 2] < 0x80){
                    isUTF8 = false;
                    break;
                }
                pos += 3;
                continue;
            }
            if(c >= 0xF0){ // may  be 4 bytes UTF8, e.g. 0xF0919AA6 (TAKRI LETTER VA)
                if(pos + 3 >= len){ //
                    isUTF8 = false;
                    break;
                }
                if(buff[pos + 1] < 0x80 || buff[pos + 2] < 0x80 || buff[pos + 3] < 0x80){
                    isUTF8 = false;
                    break;
                }
                pos += 4;
                continue;
            }
            pos++;
        }
        if(isUTF8 == true) return true; // is UTF-8, do nothing
    }

    char* iso8859_1 = x_strdup(buff);
    if(!iso8859_1){log_e("oom"); return false;}

    while(iso8859_1[in] != '\0'){
        if(iso8859_1[in] < 0x80){
            buff[out] = iso8859_1[in];
            out++;
            in++;
            if(out > bufflen) goto exit;
        }
        else{
            buff[out] = (0xC0 | iso8859_1[in] >> 6);
            out++;
            if(out + 1 > bufflen) goto exit;
            buff[out] = (0x80 | (iso8859_1[in] & 0x3F));
            out++;
            in++;
        }
    }
    buff[out] = '\0';
    if(iso8859_1) {free(iso8859_1); iso8859_1 = NULL;}
    return true;

exit:
    if(iso8859_1) {free(iso8859_1); iso8859_1 = NULL;}
    return false;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::parseHttpResponseHeader() { // this is the response to a GET / request
    static uint32_t notavailablefor = 0;
    if(getDatamode() != HTTP_RESPONSE_HEADER) return false;
    if(_client->available() == 0) {
      if (notavailablefor == 0) notavailablefor = millis();
      if (millis() - notavailablefor > HEADER_TIMEOUT) {
        notavailablefor = 0;
        if(audio_showstation) audio_showstation("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
        AUDIO_ERROR("Host not available");
        m_lastHost[0] = '\0';
        setDatamode(AUDIO_NONE);
        stopSong();
      }
      return false;
    }
    notavailablefor = 0;
    char rhl[512] = {0}; // responseHeaderline
    bool ct_seen = false;
    uint32_t ctime = millis();
    uint32_t timeout = 2500; // ms

    while(true){  // outer while
        uint16_t pos = 0;
        if((millis() - ctime) > timeout) {
            log_e("timeout");
            m_f_timeout = true;
            goto exit;
        }
        while(_client->available()){
            uint8_t b = _client->read();
            if(b == '\n') {
                if(!pos){ // empty line received, is the last line of this responseHeader
                    if(ct_seen) goto lastToDo;
                    else
                        goto exit;
                }
                break;
            }
            if(b == '\r') rhl[pos] = 0;
            if(b < 0x20) continue;
            rhl[pos] = b;
            pos++;
            if(pos == 511) {
                pos = 510;
                continue;
            }
            if(pos == 510){
                rhl[pos] = '\0';
                if(m_f_Log) log_i("responseHeaderline overflow");
            }
        } // inner while

        if(!pos) {
            vTaskDelay(3);
            continue;
        }

        if(m_f_Log) {log_i("httpResponseHeader: %s", rhl);}

        int16_t posColon = indexOf(rhl, ":", 0); // lowercase all letters up to the colon
        if(posColon >= 0) {
            for(int i=0; i< posColon; i++) {
                rhl[i] = toLowerCase(rhl[i]);
            }
        }

        if(startsWith(rhl, "HTTP/")){ // HTTP status error code
            char statusCode[5];
            statusCode[0] = rhl[9];
            statusCode[1] = rhl[10];
            statusCode[2] = rhl[11];
            statusCode[3] = '\0';
            int sc = atoi(statusCode);
            if(sc > 310){ // e.g. HTTP/1.1 301 Moved Permanently
                if(audio_showstreamtitle) audio_showstreamtitle(rhl);
                goto exit;
            }
        }

        else if(startsWith(rhl, "content-type:")){ // content-type: text/html; charset=UTF-8
            int idx = indexOf(rhl + 13, ";");
            if(idx >0) rhl[13 + idx] = '\0';
            if(parseContentType(rhl + 13)) ct_seen = true;
            else goto exit;
        }

        else if(startsWith(rhl, "location:")) {
            int pos = indexOf(rhl, "http", 0);
            if(pos >= 0){
                const char* c_host = (rhl + pos);
                if(strcmp(c_host, m_lastHost) != 0) { // prevent a loop
                    int pos_slash = indexOf(c_host, "/", 9);
                    if(pos_slash > 9){
                        if(!strncmp(c_host, m_lastHost, pos_slash)) {
							AUDIO_INFO("redirect to new extension at existing host \"%s\"", c_host);
							if(m_playlistFormat == FORMAT_M3U8) {
								strcpy(m_lastHost, c_host);
								m_f_m3u8data = true;
							}
							httpPrint(c_host);
							while(_client->available()) _client->read();  // empty client buffer
							return true;
						}
                    }
                    AUDIO_INFO("redirect to new host \"%s\"", c_host);
                    connecttohost(c_host);
                    return true;
                }
            }
        }

        else if(startsWith(rhl, "content-encoding:")){
            if(indexOf(rhl, "gzip")){
                AUDIO_INFO("can't extract gzip");
                goto exit;
            }
        }

        else if(startsWith(rhl, "content-disposition:")) {
            int pos1, pos2; // pos3;
            // e.g we have this headerline:  content-disposition: attachment; filename=stream.asx
            // filename is: "stream.asx"
            pos1 = indexOf(rhl, "filename=", 0);
            if(pos1 > 0){
                pos1 += 9;
                if(rhl[pos1] == '\"') pos1++;  // remove '\"' around filename if present
                pos2 = strlen(rhl);
                if(rhl[pos2 - 1] == '\"') rhl[pos2 - 1] = '\0';
            }
            AUDIO_INFO("Filename is %s", rhl + pos1);
        }

        // if(startsWith(rhl, "set-cookie:")         ||
        //         startsWith(rhl, "pragma:")        ||
        //         startsWith(rhl, "expires:")       ||
        //         startsWith(rhl, "cache-control:") ||
        //         startsWith(rhl, "icy-pub:")       ||
        //         startsWith(rhl, "p3p:")           ||
        //         startsWith(rhl, "accept-ranges:") ){
        //     ; // do nothing
        // }

        else if(startsWith(rhl, "connection:")) {
            if(indexOf(rhl, "close", 0) >= 0) {; /* do nothing */}
        }

        else if(startsWith(rhl, "icy-genre:")) {
            ; // do nothing Ambient, Rock, etc
        }

        else if(startsWith(rhl, "icy-logo:")) {
            char* c_icylogo = (rhl + 9);  // Get logo URL
            trim(c_icylogo);
            if(strlen(c_icylogo) > 0) {
                if(m_f_Log) AUDIO_INFO("icy-logo: %s", c_icylogo);
                if(audio_icylogo) audio_icylogo(c_icylogo);
            }
        }

        else if(startsWith(rhl, "icy-br:")) {
            const char* c_bitRate = (rhl + 7);
            int32_t br = atoi(c_bitRate); // Found bitrate tag, read the bitrate in Kbit
            br = br * 1000;
            m_bitrate= br;
            sprintf(m_chbuf, "%ld", br);
            if(audio_bitrate) audio_bitrate(m_chbuf);
        }

        else if(startsWith(rhl, "icy-metaint:")) {
            const char* c_metaint = (rhl + 12);
            int32_t i_metaint = atoi(c_metaint);
            m_metaint = i_metaint;
            if(m_metaint) m_f_metadata = true; // stream has metadata
        }

        else if(startsWith(rhl, "icy-name:")) {
            char* c_icyname = (rhl + 9); // Get station name
            trim(c_icyname);
            if(strlen(c_icyname) > 0) {
                if(m_f_Log) AUDIO_INFO("icy-name: %s", c_icyname);
                if(audio_showstation) audio_showstation(c_icyname);
            }
        }

        else if(startsWith(rhl, "content-length:")) {
            const char* c_cl = (rhl + 15);
            int32_t i_cl = atoi(c_cl);
            m_contentlength = i_cl;
            m_streamType = ST_WEBFILE; // Stream comes from a fileserver
            if(m_f_Log) AUDIO_INFO("content-length: %lu", m_contentlength);
        }

        else if(startsWith(rhl, "icy-description:")) {
            const char* c_idesc = (rhl + 16);
            while(c_idesc[0] == ' ') c_idesc++;
            latinToUTF8(rhl, sizeof(rhl)); // if already UTF-0 do nothing, otherwise convert to UTF-8
            if(audio_icydescription) audio_icydescription(c_idesc);
        }

        else if((startsWith(rhl, "transfer-encoding:"))){
            if(endsWith(rhl, "chunked") || endsWith(rhl, "Chunked") ) { // Station provides chunked transfer
                m_f_chunked = true;
                if(m_f_Log) AUDIO_INFO("chunked data transfer");
                m_chunkcount = 0;                         // Expect chunkcount in DATA
            }
        }

        else if(startsWith(rhl, "icy-url:")) {
            char* icyurl = (rhl + 8);
            trim(icyurl);
            if(audio_icyurl) audio_icyurl(icyurl);
        }

        else if(startsWith(rhl, "www-authenticate:")) {
            AUDIO_INFO("authentification failed, wrong credentials?");
            goto exit;
        }
        else {;}
    } // outer while

    exit:  // termination condition
        if(audio_showstation) audio_showstation("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
        if(m_playlistFormat == FORMAT_M3U8) return false;
        // m_lastHost[0] = '\0';
        setDatamode(AUDIO_NONE);
        stopSong();
        return false;

    lastToDo:
        if(m_codec != CODEC_NONE){
            setDatamode(AUDIO_DATA); // Expecting data now
            // if(!initializeDecoder()) return false;
            if(m_f_Log) {log_i("Switch to DATA, metaint is %d", m_metaint);}
            if(m_playlistFormat != FORMAT_M3U8 && audio_lasthost) audio_lasthost(m_lastHost);
            m_controlCounter = 0;
            m_f_firstCall = true;
        }
        else if(m_playlistFormat != FORMAT_NONE){
            setDatamode(AUDIO_PLAYLISTINIT); // playlist expected
            if(m_f_Log) {log_i("now parse playlist");}
        }
        else{
            AUDIO_INFO("unknown content found at: %s", m_lastHost);
            AUDIO_ERROR("unknown content found at: %s", m_lastHost);
            goto exit;
        }
        return true;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::parseContentType(char* ct) {

    enum : int {CT_NONE, CT_MP3, CT_AAC, CT_M4A, CT_WAV, CT_OGG, CT_FLAC, CT_PLS, CT_M3U, CT_ASX,
                CT_M3U8, CT_TXT, CT_AACP};

    strlower(ct);
    trim(ct);
    m_codec = CODEC_NONE;
    int ct_val = CT_NONE;

    if(!strcmp(ct, "audio/mpeg"))            ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/mpeg3"))      ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/x-mpeg"))     ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/x-mpeg-3"))   ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/mp3"))        ct_val = CT_MP3;

    else if(!strcmp(ct, "audio/aac"))        ct_val = CT_AAC;
    else if(!strcmp(ct, "audio/x-aac"))      ct_val = CT_AAC;
    else if(!strcmp(ct, "audio/aacp"))       ct_val = CT_AAC;
    else if(!strcmp(ct, "video/mp2t")){      ct_val = CT_AAC; if(m_m3u8Codec == CODEC_MP3) ct_val = CT_MP3;} // see m3u8redirection()
    else if(!strcmp(ct, "audio/mp4"))        ct_val = CT_M4A;
    else if(!strcmp(ct, "audio/m4a"))        ct_val = CT_M4A;

    else if(!strcmp(ct, "audio/wav"))        ct_val = CT_WAV;
    else if(!strcmp(ct, "audio/x-wav"))      ct_val = CT_WAV;

    else if(!strcmp(ct, "audio/flac"))       ct_val = CT_FLAC;
    else if(!strcmp(ct, "audio/x-flac"))     ct_val = CT_FLAC;

    else if(!strcmp(ct, "audio/scpls"))      ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/x-scpls"))    ct_val = CT_PLS;
    else if(!strcmp(ct, "application/pls+xml")) ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/mpegurl"))   {ct_val = CT_M3U; if(m_expectedPlsFmt == FORMAT_M3U8) ct_val = CT_M3U8;}
    else if(!strcmp(ct, "audio/x-mpegurl"))  ct_val = CT_M3U;
    else if(!strcmp(ct, "audio/ms-asf"))     ct_val = CT_ASX;
    else if(!strcmp(ct, "video/x-ms-asf"))   ct_val = CT_ASX;
    else if(!strcmp(ct, "audio/x-ms-asx"))   ct_val = CT_ASX; // #413

    else if(!strcmp(ct, "application/ogg"))  ct_val = CT_OGG;
    else if(!strcmp(ct, "audio/ogg"))        ct_val = CT_OGG;
    else if(!strcmp(ct, "application/vnd.apple.mpegurl")) ct_val = CT_M3U8;
    else if(!strcmp(ct, "application/x-mpegurl")) ct_val =CT_M3U8;

    else if(!strcmp(ct, "application/octet-stream")) ct_val = CT_TXT; // ??? listen.radionomy.com/1oldies before redirection
    else if(!strcmp(ct, "text/html"))        ct_val = CT_TXT;
    else if(!strcmp(ct, "text/plain"))       ct_val = CT_TXT;

    else if(ct_val == CT_NONE){
        AUDIO_INFO("ContentType %s not supported", ct);
        AUDIO_ERROR("ContentType %s not supported", ct);
        return false; // nothing valid had been seen
    }
    else {;}

    switch(ct_val){
        case CT_MP3:
            m_codec = CODEC_MP3;
            if(m_f_Log) { log_i("ContentType %s, format is mp3", ct); } //ok is likely mp3
            if(audio_info) audio_info("format is mp3");
            break;
        case CT_AAC:
            m_codec = CODEC_AAC;
            if(m_f_Log) { log_i("ContentType %s, format is aac", ct); }
            if(audio_info) audio_info("format is aac");
            break;
        case CT_M4A:
            m_codec = CODEC_M4A;
            if(m_f_Log) { log_i("ContentType %s, format is aac", ct); }
            if(audio_info) audio_info("format is aac");
            break;
        case CT_FLAC:
            m_codec = CODEC_FLAC;
            if(m_f_Log) { log_i("ContentType %s, format is flac", ct); }
            if(audio_info) audio_info("format is flac");
            break;
        case CT_WAV:
            m_codec = CODEC_WAV;
            if(m_f_Log) { log_i("ContentType %s, format is wav", ct); }
            if(audio_info) audio_info("format is wav");
            break;
        case CT_OGG:
            m_codec = CODEC_OGG;
            if(m_f_Log) { log_i("ContentType %s found", ct); }
            if(audio_info) audio_info("format is ogg");
            break;

        case CT_PLS:
            m_playlistFormat = FORMAT_PLS;
            break;
        case CT_M3U:
            m_playlistFormat = FORMAT_M3U;
            break;
        case CT_ASX:
            m_playlistFormat = FORMAT_ASX;
            break;
        case CT_M3U8:
            m_playlistFormat = FORMAT_M3U8;
            break;
        case CT_TXT: // overwrite text/plain
            if(m_expectedCodec == CODEC_AAC){ m_codec = CODEC_AAC; if(m_f_Log) log_i("set ct from M3U8 to AAC");}
            if(m_expectedCodec == CODEC_MP3){ m_codec = CODEC_MP3; if(m_f_Log) log_i("set ct from M3U8 to MP3");}

            if(m_expectedPlsFmt == FORMAT_ASX){ m_playlistFormat = FORMAT_ASX;  if(m_f_Log) log_i("set playlist format to ASX");}
            if(m_expectedPlsFmt == FORMAT_M3U){ m_playlistFormat = FORMAT_M3U;  if(m_f_Log) log_i("set playlist format to M3U");}
            if(m_expectedPlsFmt == FORMAT_M3U8){m_playlistFormat = FORMAT_M3U8; if(m_f_Log) log_i("set playlist format to M3U8");}
            if(m_expectedPlsFmt == FORMAT_PLS){ m_playlistFormat = FORMAT_PLS;  if(m_f_Log) log_i("set playlist format to PLS");}
            break;
        default:
            AUDIO_INFO("%s, unsupported audio format", ct);
            return false;
            break;
    }
    return true;
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::stop_mp3client(){
    uint32_t pos = 0;
    if(getDatamode() == AUDIO_LOCALFILE){
        pos = getFilePos() - InBuff.bufferFilled();
        audiofile.close();
        setDatamode(AUDIO_NONE);
    }
    stopSong();
    int v=read_register(SCI_VOL);
    m_f_running = false;
    write_register(SCI_VOL, 0);                         // Mute while stopping
    #ifdef ESP_ARDUINO_3
    _client->clear();                                     // Flush stream client
    #else
    _client->flush();
    #endif
    _client->stop();                                      // Stop stream client
    write_register(SCI_VOL, v);
    return pos;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setDefaults(){
    // initializationsequence
    stopSong();
    initInBuff();                                           // initialize InputBuffer if not already done
    InBuff.resetBuffer();
    if(m_playlistBuff){free(m_playlistBuff); m_playlistBuff = NULL;} // free if stream is not m3u8
    vector_clear_and_shrink(m_playlistURL);
    vector_clear_and_shrink(m_playlistContent);
    m_hashQueue.clear(); m_hashQueue.shrink_to_fit(); // uint32_t vector
    if(config.getMode()!=PM_SDCARD){
      if(_client) _client->stop();
      _client = static_cast<WiFiClient*>(&client); /* default to *something* so that no NULL deref can happen */
    }
    ts_parsePacket(0, 0, 0); // reset ts routine
    if(m_lastM3U8host){free(m_lastM3U8host); m_lastM3U8host = NULL;}

    m_f_timeout = false;
    m_f_ctseen=false;                                       // Contents type not seen yet
    m_metaint=0;                                            // No metaint yet
    m_LFcount=0;                                            // For detection end of header
    m_bitrate=0;                                            // Bitrate still unknown
    m_f_firstCall = true;                                   // InitSequence for processWebstream and processLokalFile
    m_f_firstM3U8call = true;                               // InitSequence for parsePlaylist_M3U8
    m_controlCounter = 0;
    m_f_firstchunk=true;                                    // First chunk expected
    m_f_chunked=false;                                      // Assume not chunked
    m_f_ssl=false;
    m_f_metadata = false;
    m_f_tts = false;                                        // text to speech
    m_f_ts = false;
    m_f_m3u8data = false;                                   // set again in processM3U8entries() if necessary
    setDatamode(AUDIO_NONE);
    m_contentlength = 0;                                    // If Content-Length is known, count it
    m_streamTitleHash = 0;
    m_streamUrlHash = 0;
    m_streamType = ST_NONE;
    m_codec = CODEC_NONE;
    m_playlistFormat = FORMAT_NONE;
    m_localBitrateSend = false;
    m_audioFileDuration = 0;
}
//------------------------------------------------------------------------------
/**
 * \brief enable VSdsp VU Meter
 *
 * \param[in] enable when set will enable the VU meter
 *
 * Writes the SS_VU_ENABLE bit of the SCI_STATUS register to enable VU meter on
 * board to the VSdsp.
 *
 * See data patches data sheet VU meter for details.
 * \warning This feature is only available with patches that support VU meter.
 * \n The VU meter takes about 0.2MHz of processing power with 48 kHz samplerate.
 */
void Audio::setVUmeter() {
  if(!VS_PATCH_ENABLE) return;
  uint16_t MP3Status = read_register(SCI_STATUS);
  if(MP3Status==0) {
    Serial.println("VS1053 Error: Unable to write SCI_STATUS");
    _vuInitalized = false;
    return;
  }
  _vuInitalized = true;
  write_register(SCI_STATUS, MP3Status | _BV(9));
}

//------------------------------------------------------------------------------
/**
 * \brief get current measured VU Meter
 *
 * Returns the calculated peak sample values from both channels in 3 dB
 * increaments through. Where the high byte represent the left channel,
 * and the low bytes the right channel.
 *
 * Values from 0 to 31 are valid for both channels.
 *
 * \warning This feature is only available with patches that support VU meter.
 */
//const uint8_t everyn = 4;
void Audio::computeVUlevel() {
  /*if(!VS_PATCH_ENABLE) return;
  static uint8_t cc = 0;
  cc++;
  if(!_vuInitalized || !config.store.vumeter || cc!=everyn) return;
  if(cc==everyn) cc=0;*/
  int16_t reg = read_register(SCI_AICTRL3);
  vuLeft = map((uint8_t)(reg & 0x00FF), 85, 92, 0, 255);
  vuRight = map((uint8_t)(reg >> 8), 85, 92, 0, 255);
  if(vuLeft>config.vuThreshold)  config.vuThreshold=vuLeft;
  if(vuRight>config.vuThreshold) config.vuThreshold=vuRight;
}

uint16_t Audio::get_VUlevel(uint16_t dimension){
  if(!VS_PATCH_ENABLE) return 0;
  if(!_vuInitalized || !config.store.vumeter/* || config.vuThreshold==0*/) return 0;
  computeVUlevel();
  uint8_t L = map(vuLeft, config.vuThreshold, 0, 0, dimension);
  uint8_t R = map(vuRight, config.vuThreshold, 0, 0, dimension);
  return (L << 8) | R;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl){
    if(timeout_ms)     m_timeout_ms     = timeout_ms;
    if(timeout_ms_ssl) m_timeout_ms_ssl = timeout_ms_ssl;
}

void Audio::connectTask(void* pvParams) {
  ConnectParams* params = static_cast<ConnectParams*>(pvParams);
  Audio* self = params->instance;
  if(self->_client){
    self->_connectionResult = self->_client->connect(params->hostwoext, params->port/*, self->m_f_ssl ? self->m_timeout_ms_ssl : self->m_timeout_ms*/);
  }else{
    self->_connectionResult = false;
  }
  free((void*)params->hostwoext);
  delete params;
  self->_connectTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(String host){
    return connecttohost(host.c_str());
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(const char* host, const char* user, const char* pwd) {
    // user and pwd for authentification only, can be empty

     if(host == NULL) {
        AUDIO_INFO("cth Hostaddress is empty");
        if(audio_error) audio_error("Hostaddress is empty");
        stopSong();
        return false;
    }

    uint16_t lenHost = strlen(host);

    if(lenHost >= 512 + 64 - 10) {
        AUDIO_INFO("Hostaddress is too long");
        if(audio_error) audio_error("Hostaddress is too long");
        stopSong();
        return false;
    }

    int idx = indexOf(host, "http");
    char* l_host = (char*)malloc(lenHost + 10);
    if(idx < 0){strcpy(l_host, "http://"); strcat(l_host, host); } // amend "http;//" if not found
    else       {strcpy(l_host, (host + idx));}                     // trim left if necessary

    char* h_host = NULL; // pointer of l_host without http:// or https://
    if(startsWith(l_host, "https")) h_host = strdup(l_host + 8);
    else                            h_host = strdup(l_host + 7);

    // initializationsequence
    int16_t pos_slash;                                        // position of "/" in hostname
    int16_t pos_colon;                                        // position of ":" in hostname
    int16_t pos_ampersand;                                    // position of "&" in hostname
    uint16_t port = 80;                                       // port number

    // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
    pos_slash     = indexOf(h_host, "/", 0);
    pos_colon     = indexOf(h_host, ":", 0);
        if(isalpha(h_host[pos_colon + 1])) pos_colon = -1; // no portnumber follows
    pos_ampersand = indexOf(h_host, "&", 0);

    char *hostwoext = NULL;                                  // "skonto.ls.lv:8002" in "skonto.ls.lv:8002/mp3"
    char *extension = NULL;                                  // "/mp3" in "skonto.ls.lv:8002/mp3"

    if(pos_slash > 1) {
        hostwoext = (char*)malloc(pos_slash + 2);
        memcpy(hostwoext, h_host, pos_slash);
        hostwoext[pos_slash] = '\0';
        uint16_t extLen =  urlencode_expected_len(h_host + pos_slash);
        extension = (char *)malloc(extLen + 20);
        memcpy(extension, h_host + pos_slash, extLen);
        urlencode(extension, extLen, true);
    }
    else{  // url has no extension
        hostwoext = strdup(h_host);
        extension = strdup("/");
    }

    if((pos_colon >= 0) && ((pos_ampersand == -1) || (pos_ampersand > pos_colon))) {
        port = atoi(h_host + pos_colon + 1);// Get portnumber as integer
        hostwoext[pos_colon] = '\0';// Host without portnumber
    }

    AUDIO_INFO("Connect to new host: \"%s\"", l_host);
    setDefaults();  // no need to stop clients if connection is established (default is true)

    if(startsWith(l_host, "https")) m_f_ssl = true;
    else                            m_f_ssl = false;

    // optional basic authorization
	uint16_t auth = strlen(user) + strlen(pwd);
	char     authorization[base64_encode_expected_len(auth + 1) + 1];
	authorization[0] = '\0';
	if(auth > 0) {
        char toEncode[auth + 4];
        strcpy(toEncode, user);
        strcat(toEncode, ":");
        strcat(toEncode, pwd);
        b64encode((const char*)toEncode, strlen(toEncode), authorization);
    }

    //  AUDIO_INFO("Connect to \"%s\" on port %d, extension \"%s\"", hostwoext, port, extension);

    char rqh[strlen(h_host) + strlen(authorization) + 220]; // http request header
    rqh[0] = '\0';

    strcat(rqh, "GET ");
    strcat(rqh, extension);
    strcat(rqh, " HTTP/1.1\r\n");
    strcat(rqh, "Host: ");
    strcat(rqh, hostwoext);
    strcat(rqh, "\r\n");
    strcat(rqh, "Icy-MetaData:1\r\n");
    strcat(rqh, "Icy-MetaData:2\r\n");
    if (auth > 0) {
      strcat(rqh, "Authorization: Basic ");
      strcat(rqh, authorization);
      strcat(rqh, "\r\n");
    }
    strcat(rqh, "Accept-Encoding: identity;q=1,*;q=0\r\n");
    //    strcat(rqh, "User-Agent: Mozilla/5.0\r\n"); #363
    strcat(rqh, "Connection: keep-alive\r\n\r\n");

    //    if(ESP_ARDUINO_VERSION_MAJOR == 2 && ESP_ARDUINO_VERSION_MINOR == 0 && ESP_ARDUINO_VERSION_PATCH >= 3){
	//        m_timeout_ms_ssl = UINT16_MAX;  // bug in v2.0.3 if hostwoext is a IPaddr not a name
	//        m_timeout_ms = UINT16_MAX;  // [WiFiClient.cpp:253] connect(): select returned due to timeout 250 ms for fd 48
	//    } fix in V2.0.8
    bool res = true; // no need to reconnect if connection exists

    if(m_f_ssl){ _client = static_cast<WiFiClient*>(&clientsecure); if(port == 80) port = 443;}
    else       { _client = static_cast<WiFiClient*>(&client);}

    uint32_t t = millis();
    if(m_f_Log) AUDIO_INFO("connect to %s on port %d path %s", hostwoext, port, extension);
    //res = _client->connect(hostwoext, port, m_f_ssl ? m_timeout_ms_ssl : m_timeout_ms);
    if(!config.store.watchdog){
      res = _client->connect(hostwoext, port, m_f_ssl ? m_timeout_ms_ssl : m_timeout_ms);
    }else{
      ConnectParams* params = new ConnectParams{ strdup(hostwoext), port, this }; _connectionResult = false;
      xTaskCreatePinnedToCore(connectTask, "ConnectTask", WATCHDOG_TASK_SIZE, params, WATCHDOG_TASK_PRIORITY, &_connectTaskHandle, WATCHDOG_TASK_CORE_ID);
      for(;;){
        if(millis()-t>(m_f_ssl ? m_timeout_ms_ssl : m_timeout_ms) || _connectionResult) break;
        vTaskDelay(10);
      }
      res = _connectionResult;
      if (_connectTaskHandle!=nullptr) {
        vTaskDelete(_connectTaskHandle);
        _connectTaskHandle = nullptr;
        AUDIO_INFO("WATCH DOG HAS FINISHED A WORK, BYE!");
      }
    }
    
    if(res){
        uint32_t dt = millis() - t;
        strcpy(m_lastHost, l_host);
        AUDIO_INFO("%s has been established in %lu ms, free Heap: %lu bytes",
                    m_f_ssl?"SSL":"Connection", dt, ESP.getFreeHeap());
        m_f_running = true;
    }

    m_expectedCodec = CODEC_NONE;
    m_expectedPlsFmt = FORMAT_NONE;

    if(res){
        _client->print(rqh);
        if(endsWith(extension, ".mp3" ))   m_expectedCodec = CODEC_MP3;
        if(endsWith(extension, ".aac" ))   m_expectedCodec = CODEC_AAC;
        if(endsWith(extension, ".wav" ))   m_expectedCodec = CODEC_WAV;
        if(endsWith(extension, ".m4a" ))   m_expectedCodec = CODEC_M4A;
        if(endsWith(extension, ".ogg" ))   m_expectedCodec = CODEC_OGG;
        if(endsWith(extension, ".flac"))   m_expectedCodec = CODEC_FLAC;
        if(endsWith(extension, "-flac"))   m_expectedCodec = CODEC_FLAC;
        if(endsWith(extension, ".opus"))   m_expectedCodec = CODEC_OPUS;
        if(endsWith(extension, "/opus"))   m_expectedCodec = CODEC_OPUS;
        if(endsWith(extension, ".asx" ))    m_expectedPlsFmt = FORMAT_ASX;
        if(endsWith(extension, ".m3u" ))    m_expectedPlsFmt = FORMAT_M3U;
        if(endsWith(extension, ".pls" ))    m_expectedPlsFmt = FORMAT_PLS;
        if(indexOf( h_host, ".m3u8") >= 0){ m_expectedPlsFmt = FORMAT_M3U8; if(audio_lasthost) audio_lasthost(m_lastHost);}

        setDatamode(HTTP_RESPONSE_HEADER);   // Handle header
        m_streamType = ST_WEBSTREAM;
    }
    else{
        AUDIO_INFO("Request %s failed!", l_host);
        AUDIO_ERROR("Request %s failed!", l_host);
        if(audio_showstation) audio_showstation("");
        if(audio_showstreamtitle) audio_showstreamtitle("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
        m_lastHost[0] = '\0';
    }
    if(hostwoext) {free(hostwoext); hostwoext = NULL;}
    if(extension) {free(extension); extension = NULL;}
    if(l_host   ) {free(l_host);    l_host    = NULL;}
    if(h_host   ) {free(h_host);    h_host    = NULL;}
    return res;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::httpPrint(const char* host) {
    // user and pwd for authentification only, can be empty

    if(host == NULL) {
        AUDIO_INFO("Hostaddress is empty");
        stopSong();
        return false;
    }

    char* h_host = NULL; // pointer of l_host without http:// or https://

    if(startsWith(host, "https")) m_f_ssl = true;
    else                          m_f_ssl = false;
    if(m_f_ssl) h_host = strdup(host + 8);
    else        h_host = strdup(host + 7);

    int16_t pos_slash;                                        // position of "/" in hostname
    int16_t pos_colon;                                        // position of ":" in hostname
    int16_t pos_ampersand;                                    // position of "&" in hostname
    uint16_t port = 80;                                       // port number

    // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
    pos_slash     = indexOf(h_host, "/", 0);
    pos_colon     = indexOf(h_host, ":", 0);
        if(isalpha(h_host[pos_colon + 1])) pos_colon = -1; // no portnumber follows
    pos_ampersand = indexOf(h_host, "&", 0);

    char *hostwoext = NULL;                                  // "skonto.ls.lv:8002" in "skonto.ls.lv:8002/mp3"
    char *extension = NULL;                                  // "/mp3" in "skonto.ls.lv:8002/mp3"

    if(pos_slash > 1) {
        hostwoext = (char*)malloc(pos_slash + 1);
        memcpy(hostwoext, h_host, pos_slash);
        hostwoext[pos_slash] = '\0';
        uint16_t extLen =  urlencode_expected_len(h_host + pos_slash);
        extension = (char *)malloc(extLen + 20);
        memcpy(extension, h_host + pos_slash, extLen);
        urlencode(extension, extLen, true);
    }
    else{  // url has no extension
        hostwoext = strdup(h_host);
        extension = strdup("/");
    }

    if((pos_colon >= 0) && ((pos_ampersand == -1) || (pos_ampersand > pos_colon))) {
        port = atoi(h_host + pos_colon + 1);// Get portnumber as integer
        hostwoext[pos_colon] = '\0';// Host without portnumber
    }

    AUDIO_INFO("new request: \"%s\"", host);

    char rqh[strlen(h_host) + 200]; // http request header
    rqh[0] = '\0';

    strcat(rqh, "GET ");
    strcat(rqh, extension);
    strcat(rqh, " HTTP/1.1\r\n");
    strcat(rqh, "Host: ");
    strcat(rqh, hostwoext);
    strcat(rqh, "\r\n");
    strcat(rqh, "Accept-Encoding: identity;q=1,*;q=0\r\n");
    //    strcat(rqh, "User-Agent: Mozilla/5.0\r\n"); #363
    strcat(rqh, "Connection: keep-alive\r\n\r\n");

    if(m_f_ssl) {
		_client = static_cast<WiFiClient*>(&clientsecure);
		if(port == 80) port = 443;
	}
    else       { _client = static_cast<WiFiClient*>(&client);}

    if(!_client->connected()){
        AUDIO_INFO("The host has disconnected, reconnecting");
        if(!_client->connect(hostwoext, port)){
            log_e("connection lost");
            stopSong();
            return false;
        }
    }
    _client->print(rqh);

    if(     endsWith(extension, ".mp3"))       m_expectedCodec  = CODEC_MP3;
    else if(endsWith(extension, ".aac"))       m_expectedCodec  = CODEC_AAC;
    else if(endsWith(extension, ".wav"))       m_expectedCodec  = CODEC_WAV;
    else if(endsWith(extension, ".m4a"))       m_expectedCodec  = CODEC_M4A;
    else if(endsWith(extension, ".flac"))      m_expectedCodec  = CODEC_FLAC;
    else                                       m_expectedCodec  = CODEC_NONE;

    if(     endsWith(extension, ".asx"))       m_expectedPlsFmt = FORMAT_ASX;
    else if(endsWith(extension, ".m3u"))       m_expectedPlsFmt = FORMAT_M3U;
    else if(indexOf( extension, ".m3u8") >= 0) m_expectedPlsFmt = FORMAT_M3U8;
    else if(endsWith(extension, ".pls"))       m_expectedPlsFmt = FORMAT_PLS;
    else                                       m_expectedPlsFmt = FORMAT_NONE;

    setDatamode(HTTP_RESPONSE_HEADER);   // Handle header
    m_streamType = ST_WEBSTREAM;
    m_contentlength = 0;
    m_f_chunked = false;

    if(hostwoext) {free(hostwoext); hostwoext = NULL;}
    if(extension) {free(extension); extension = NULL;}
    if(h_host   ) {free(h_host);    h_host    = NULL;}
    return true;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::loadUserCode(void) {
  int i = 0;

  while (i<sizeof(flac_plugin)/sizeof(flac_plugin[0])) {
    unsigned short addr, n, val;
    addr = flac_plugin[i++];
    n = flac_plugin[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = flac_plugin[i++];
      while (n--) {
        write_register(addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
        val = flac_plugin[i++];
        write_register(addr, val);
      }
    }
  }
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::UTF8toASCII(char* str){

    const uint8_t ascii[60] = {
    //129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148  // UTF8(C3)
    //                Ä    Å    Æ    Ç         É                                       Ñ                  // CHAR
      000, 000, 000, 142, 143, 146, 128, 000, 144, 000, 000, 000, 000, 000, 000, 000, 165, 000, 000, 000, // ASCII
    //149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168
    //      Ö                             Ü              ß    à                   ä    å    æ         è
      000, 153, 000, 000, 000, 000, 000, 154, 000, 000, 225, 133, 000, 000, 000, 132, 134, 145, 000, 138,
    //169, 170, 171, 172. 173. 174. 175, 176, 177, 179, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188
    //      ê    ë    ì         î    ï         ñ    ò         ô         ö              ù         û    ü
      000, 136, 137, 141, 000, 140, 139, 000, 164, 149, 000, 147, 000, 148, 000, 000, 151, 000, 150, 129};

    uint16_t i = 0, j=0, s = 0;
    bool f_C3_seen = false;

    while(str[i] != 0) {                                     // convert UTF8 to ASCII
        if(str[i] == 195){                                   // C3
            i++;
            f_C3_seen = true;
            continue;
        }
        str[j] = str[i];
        if(str[j] > 128 && str[j] < 189 && f_C3_seen == true) {
            s = ascii[str[j] - 129];
            if(s != 0) str[j] = s;                         // found a related ASCII sign
            f_C3_seen = false;
        }
        i++; j++;
    }
    str[j] = 0;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttoSD(String sdfile, uint32_t resumeFilePos){
    return connecttoFS(SD, sdfile.c_str(), resumeFilePos);
}

bool Audio::connecttoSD(const char* sdfile, uint32_t resumeFilePos){
    return connecttoFS(SD, sdfile, resumeFilePos);
}

bool Audio::connecttoFS(fs::FS &fs, const char* path, uint32_t resumeFilePos) {

    if(strlen(path)>255) return false;
    m_resumeFilePos = resumeFilePos;

    char audioName[256];
    m_avr_bitrate = 0;
    setDefaults(); // free buffers an set defaults

    memcpy(audioName, path, strlen(path)+1);
    if(audioName[0] != '/'){
        for(int i = 255; i > 0; i--){
            audioName[i] = audioName[i-1];
        }
        audioName[0] = '/';
    }
    if(endsWith(audioName, "\n")) audioName[strlen(audioName) -1] = 0;

    sprintf(m_chbuf, "Reading file: \"%s\"", audioName);
    if(audio_info) {vTaskDelay(2); audio_info(m_chbuf);}
    if(audio_beginSDread) audio_beginSDread();
    audiofile.close();
    if(fs.exists(audioName)) {
        audiofile = fs.open(audioName);
    } else {
        UTF8toASCII(audioName);
        if(fs.exists(audioName)) {
            audiofile = fs.open(audioName);
        }
    }

    if(!audiofile) {
        if(audio_info) {vTaskDelay(2); audio_info("Failed to open file for reading");}
        return false;
    }

    setDatamode(AUDIO_LOCALFILE);
    m_file_size = audiofile.size();//TEST loop
    
    char* afn = strdup(audiofile.name());                   // audioFileName
    uint8_t dotPos = lastIndexOf(afn, ".");
    for(uint8_t i = dotPos + 1; i < strlen(afn); i++){
        afn[i] = toLowerCase(afn[i]);
    }

    if(endsWith(afn, ".mp3")){      // MP3 section
        m_codec = CODEC_MP3;
        m_f_running = true;
        if(audio_info) audio_info("format is mp3");
        return true;
    } // end MP3 section

    if(endsWith(afn, ".m4a")){      // M4A section, iTunes
        m_codec = CODEC_M4A;
        m_f_running = true;
        if(audio_info) audio_info("format is aac");
        return true;
    } // end M4A section

    if(endsWith(afn, ".aac")){      // AAC section, without FileHeader
        m_codec = CODEC_AAC;
        if(audio_info) audio_info("format is aac");
        m_f_running = true;
        return true;
    } // end AAC section

    if(endsWith(afn, ".wav")){      // WAVE section
        m_codec = CODEC_WAV;
        m_f_running = true;
        if(audio_info) audio_info("format is wav");
        return true;
    } // end WAVE section

    if(endsWith(afn, ".flac")) {     // FLAC section
        m_codec = CODEC_FLAC;
        m_f_running = true;
        if(audio_info) audio_info("format is flac");
        return true;
    } // end FLAC section

    if(endsWith(afn, ".ogg")) {     // FLAC section
        m_codec = CODEC_OGG;
        m_f_running = true;
        if(audio_info) audio_info("format is ogg");
        return true;
    } // end FLAC section

    sprintf(m_chbuf, "The %s format is not supported", afn + dotPos);
    if(audio_info) audio_info(m_chbuf);
    if(audio_error) audio_error(m_chbuf);
    audiofile.close();
    if(afn) free(afn);
    return false;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttospeech(const char* speech, const char* lang){

    setDefaults();
    char host[] = "translate.google.com.vn";
    char path[] = "/translate_tts";

    uint16_t speechLen = strlen(speech);
    uint16_t speechBuffLen = speechLen + 300;
    memcpy(m_lastHost, speech, 256);
    char* speechBuff = (char*)malloc(speechBuffLen);
    if(!speechBuff) {log_e("out of memory"); return false;}
    memcpy(speechBuff, speech, speechLen);
    speechBuff[speechLen] = '\0';
    urlencode(speechBuff, speechBuffLen);

    char resp[strlen(speechBuff) + 200] = "";
    strcat(resp, "GET ");
    strcat(resp, path);
    strcat(resp, "?ie=UTF-8&tl=");
    strcat(resp, lang);
    strcat(resp, "&client=tw-ob&q=");
    strcat(resp, speechBuff);
    strcat(resp, " HTTP/1.1\r\n");
    strcat(resp, "Host: ");
    strcat(resp, host);
    strcat(resp, "\r\n");
    strcat(resp, "User-Agent: Mozilla/5.0 \r\n");
    strcat(resp, "Accept-Encoding: identity\r\n");
    strcat(resp, "Accept: text/html\r\n");
    strcat(resp, "Connection: close\r\n\r\n");

    free(speechBuff);

    if(!clientsecure.connect(host, 443)) {
        log_e("Connection failed");
        return false;
    }
    clientsecure.print(resp);
    sprintf(m_chbuf, "SSL has been established, free Heap: %lu bytes", ESP.getFreeHeap());
    if(audio_info) audio_info(m_chbuf);

    m_f_running = true;
    m_f_ssl = true;
    m_f_tts = true;
    setDatamode(HTTP_RESPONSE_HEADER);

    return true;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::unicode2utf8(char* buff, uint32_t len){
    // converts unicode in UTF-8, buff contains the string to be converted up to len
    // range U+1 ... U+FFFF
    uint8_t* tmpbuff = (uint8_t*)malloc(len * 2);
    if(!tmpbuff) {log_e("out of memory"); return;}
    bool bitorder = false;
    uint16_t j = 0;
    uint16_t k = 0;
    uint16_t m = 0;
    uint8_t uni_h = 0;
    uint8_t uni_l = 0;

    while(m < len - 1) {
        if((buff[m] == 0xFE) && (buff[m + 1] == 0xFF)) {
            bitorder = true;
            j = m + 2;
        }  // LSB/MSB
        if((buff[m] == 0xFF) && (buff[m + 1] == 0xFE)) {
            bitorder = false;
            j = m + 2;
        }  // MSB/LSB
        m++;
    } // seek for last bitorder
    m = 0;
    if(j > 0) {
        for(k = j; k < len; k += 2) {
            if(bitorder == true) {
                uni_h = (uint8_t)buff[k];
                uni_l = (uint8_t)buff[k + 1];
            }
            else {
                uni_l = (uint8_t)buff[k];
                uni_h = (uint8_t)buff[k + 1];
            }

            uint16_t uni_hl = ((uni_h << 8) | uni_l);

            if (uni_hl < 0X80){
                tmpbuff[m] = uni_l;
                m++;
            }
            else if (uni_hl < 0X800) {
                tmpbuff[m]= ((uni_hl >> 6) | 0XC0);
                m++;
                tmpbuff[m] =((uni_hl & 0X3F) | 0X80);
                m++;
            }
            else {
                tmpbuff[m] = ((uni_hl >> 12) | 0XE0);
                m++;
                tmpbuff[m] = (((uni_hl >> 6) & 0X3F) | 0X80);
                m++;
                tmpbuff[m] = ((uni_hl & 0X3F) | 0X80);
                m++;
            }
        }
    }
    memcpy(buff, tmpbuff, m);
    buff[m] = 0;
    free(tmpbuff);
}
//---------------------------------------------------------------------------------------------------------------------
int Audio::read_ID3_Header(uint8_t *data, size_t len) {

    static size_t id3Size;
    static size_t   totalId3Size; // if we have more header, id3_1_size + id3_2_size + ....
    static size_t remainingHeaderBytes;
    static size_t universal_tmp = 0;
    static uint8_t ID3version;
    static int ehsz = 0;
    static char frameid[5];
    static size_t framesize = 0;
    static bool compressed = false;
    static size_t   APIC_size[3] = {0};
    static uint32_t APIC_pos[3] = {0};
    static bool SYLT_seen = false;
    static size_t SYLT_size = 0;
    static uint32_t SYLT_pos = 0;
    static uint8_t  numID3Header = 0;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 0){      /* read ID3 tag and ID3 header size */
        if(getDatamode() == AUDIO_LOCALFILE){
            m_contentlength = getFileSize();
            ID3version = 0;
            sprintf(m_chbuf, "Content-Length: %lu", m_contentlength);
            if(audio_info) audio_info(m_chbuf);
        }
        m_controlCounter ++;
        SYLT_seen = false;
        remainingHeaderBytes = 0;
        ehsz = 0;
        if(specialIndexOf(data, "ID3", 4) != 0) { // ID3 not found
            if(!m_f_m3u8data) if(audio_info) audio_info("file has no mp3 tag, skip metadata");
            m_audioDataSize = m_contentlength;
            sprintf(m_chbuf, "Audio-Length: %u", m_audioDataSize);
            //if(audio_progress) audio_progress(295903, m_audioDataSize);
            if(audio_progress) audio_progress(0, m_audioDataSize);
            if(!m_f_m3u8data) if(audio_info) audio_info(m_chbuf);
            return -1; // error, no ID3 signature found
        }
        ID3version = *(data + 3);
        switch(ID3version){
            case 2:
                m_f_unsync = (*(data + 5) & 0x80);
                m_f_exthdr = false;
                break;
            case 3:
            case 4:
                m_f_unsync = (*(data + 5) & 0x80); // bit7
                m_f_exthdr = (*(data + 5) & 0x40); // bit6 extended header
                break;
        };
        id3Size = bigEndian(data + 6, 4, 7); //  ID3v2 size  4 * %0xxxxxxx (shift left seven times!!)
        id3Size += 10;

        // Every read from now may be unsync'd
        sprintf(m_chbuf, "ID3 framesSize: %i", id3Size);
        if(!m_f_m3u8data) if(audio_info) audio_info(m_chbuf);

        sprintf(m_chbuf, "ID3 version: 2.%i", ID3version);
        if(!m_f_m3u8data) if(audio_info) audio_info(m_chbuf);

        if(ID3version == 2){
            m_controlCounter = 10;
        }
        remainingHeaderBytes = id3Size;
        remainingHeaderBytes -= 10;

        return 10;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 1){      // compute extended header size if exists
        m_controlCounter ++;
        if(m_f_exthdr) {
            if(audio_info) audio_info("ID3 extended header");
            ehsz =  bigEndian(data, 4);
            remainingHeaderBytes -= 4;
            ehsz -= 4;
            return 4;
        }
        else{
            if(!m_f_m3u8data) if(audio_info) audio_info("ID3 normal frames");
            return 0;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 2){      // skip extended header if exists
        if(ehsz > 256) {
            ehsz -=256;
            remainingHeaderBytes -= 256;
            return 256;} // Throw it away
        else           {
            m_controlCounter ++;
            remainingHeaderBytes -= ehsz;
            return ehsz;} // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 3){      // read a ID3 frame, get the tag
        if(remainingHeaderBytes == 0){
            m_controlCounter = 99;
            return 0;
        }
        m_controlCounter ++;
        frameid[0] = *(data + 0);
        frameid[1] = *(data + 1);
        frameid[2] = *(data + 2);
        frameid[3] = *(data + 3);
        frameid[4] = 0;
        remainingHeaderBytes -= 4;
        if(frameid[0] == 0 && frameid[1] == 0 && frameid[2] == 0 && frameid[3] == 0) {
            // We're in padding
            m_controlCounter = 98;  // all ID3 metadata processed
        }
        return 4;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 4){  // get the frame size
        m_controlCounter = 6;

        if(ID3version == 4){
            framesize = bigEndian(data, 4, 7); // << 7
        }
        else {
            framesize = bigEndian(data, 4);  // << 8
        }
        remainingHeaderBytes -= 4;
        uint8_t flag = *(data + 4); // skip 1st flag
        (void) flag;
        remainingHeaderBytes--;
        compressed = (*(data + 5)) & 0x80; // Frame is compressed using [#ZLIB zlib] with 4 bytes for 'decompressed
                                           // size' appended to the frame header.
        remainingHeaderBytes--;
        uint32_t decompsize = 0;
        if(compressed){
            log_d("iscompressed");
            decompsize = bigEndian(data + 6, 4);
            remainingHeaderBytes -= 4;
            (void) decompsize;
            log_d("decompsize=%u", decompsize);
            return 6 + 4;
        }
        return 6;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 5){      // If the frame is larger than 512 bytes, skip the rest
        if(framesize > 512){
            framesize -= 512;
            remainingHeaderBytes -= 512;
            return 512;
        }
        else {
            m_controlCounter = 3; // check next frame
            remainingHeaderBytes -= framesize;
            return framesize;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 6){      // Read the value
        m_controlCounter = 5;       // only read 256 bytes
        char ch = *(data + 0);
        bool isUnicode = (ch==1) ? true : false;

        if(startsWith(frameid, "APIC")) { // a image embedded in file, passing it to external function
            // log_d("framesize=%i", framesize);
            isUnicode = false;
            if(getDatamode() == AUDIO_LOCALFILE){
                APIC_pos[numID3Header] = totalId3Size + id3Size - remainingHeaderBytes;
                APIC_size[numID3Header] = framesize;
                //    log_e("APIC_pos %i APIC_size %i", APIC_pos[numID3Header], APIC_size[numID3Header]);
            }
            return 0;
        }

        if(// any lyrics embedded in file, passing it to external function
            startsWith(frameid, "SYLT")
            || startsWith(frameid, "TXXX")
            || startsWith(frameid, "USLT")
        ) {
            if(getDatamode() == AUDIO_LOCALFILE){
                SYLT_seen = true;
                SYLT_pos = id3Size - remainingHeaderBytes;
                SYLT_size = framesize;
            }
            return 0;
        }

        size_t fs = framesize;
        if(fs >512) fs = 512;
        for(int i=0; i<fs; i++){
            m_ibuff[i] = *(data + i);
        }
        framesize -= fs;
        remainingHeaderBytes -= fs;
        m_ibuff[fs] = 0;
        if(isUnicode && fs > 1) {
            unicode2utf8(m_ibuff, fs);   // convert unicode to utf-8 U+0020...U+07FF
        }
        if(!isUnicode){
            uint16_t j = 0, k = 0;
            while(j < fs) {
                if(m_ibuff[j] > 0x1F) {
                    m_ibuff[k] = m_ibuff[j]; //remove non printables
                    k++;
                }
                j++;
            }
            m_ibuff[k] = '\0';  // new termination
            latinToUTF8(m_ibuff, m_ibuffSize, false);
        }
        showID3Tag(frameid, m_ibuff);
        return fs;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // --- section V2.2 only , greater Vers above ----
    // see https://mutagen-specs.readthedocs.io/en/latest/id3/id3v2.2.html
    if(m_controlCounter == 10){ // frames in V2.2, 3bytes identifier, 3bytes size descriptor

        if(universal_tmp > 0){
            if( universal_tmp > 256) { universal_tmp -= 256; return 256;}
            else{ uint8_t t = universal_tmp; universal_tmp = 0; return t;}
        }

        frameid[0] = *(data + 0);
        frameid[1] = *(data + 1);
        frameid[2] = *(data + 2);
        frameid[3] = 0;
        remainingHeaderBytes -= 3;
        size_t len = bigEndian(data + 3, 3);
        universal_tmp = len;
        remainingHeaderBytes -= 3;
        char value[256];
        if(len > 249) {len = 249;   }
        memcpy(value, (data + 7), len);
        value[len + 1] = 0;
        m_chbuf[0] = 0;
        if(startsWith(frameid, "PIC")) { // image embedded in header
            if(getDatamode() == AUDIO_LOCALFILE){
                APIC_pos[numID3Header] = id3Size - remainingHeaderBytes;
                APIC_size[numID3Header] = universal_tmp;
                if(m_f_Log) log_i("Attached picture seen at pos %d length %d", APIC_pos[0], APIC_size[0]);
            }
        }
        else if(startsWith(frameid, "SLT")) { // lyrics embedded in header
            if(getDatamode() == AUDIO_LOCALFILE){
                SYLT_seen = true;                       // #460
                SYLT_pos = id3Size - remainingHeaderBytes;
                SYLT_size = universal_tmp;
                if(m_f_Log) log_i("Attached lyrics seen at pos %d length %d", SYLT_pos, SYLT_size);
            }
        }
        else{
            if(!m_f_m3u8data) showID3Tag(frameid, value);
        }
        remainingHeaderBytes -= universal_tmp;
        universal_tmp -= len;

        if(len == 0) m_controlCounter = 98;
        if(remainingHeaderBytes == 0) m_controlCounter = 98;

        return 3 + 3 + len;
    }
    // -- end section V2.2 -----------

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 98){ // skip all ID3 metadata (mostly spaces)
        if(remainingHeaderBytes > 256) {
            remainingHeaderBytes -=256;
            return 256;
        } // Throw it away
        else           {
            m_controlCounter = 99;
            return remainingHeaderBytes;
        } // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 99){ //  exist another ID3tag?
        m_audioDataStart += id3Size;
        vTaskDelay(30);
        if((*(data + 0) == 'I') && (*(data + 1) == 'D') && (*(data + 2) == '3')) {
            m_controlCounter = 0;
            numID3Header ++;
            totalId3Size += id3Size;
            return 0;
        }
        else {
            m_controlCounter = 100; // ok
            eofHeader = true;
            m_audioDataSize = m_contentlength - m_audioDataStart;
            if(audio_progress) audio_progress(m_audioDataStart, m_audioDataSize);
            sprintf(m_chbuf, "Audio-Length: %u", m_audioDataSize);
            if(!m_f_m3u8data) if(audio_info) audio_info(m_chbuf);
            if(APIC_pos[0] && audio_id3image) { // if we have more than one APIC, output the first only
                size_t pos = audiofile.position();
                audio_id3image(audiofile, APIC_pos[0], APIC_size[0]);
                audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
            }
            if(SYLT_seen && audio_id3lyrics){
                size_t pos = audiofile.position();
                audio_id3lyrics(audiofile, SYLT_pos, SYLT_size);
                audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
            }
            numID3Header = 0;
            totalId3Size = 0;
            for(int i = 0; i< 3; i++) APIC_pos[i] = 0; // delete all
            for(int i = 0; i< 3; i++) APIC_size[i] = 0; // delete all
            return 0;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::showID3Tag(const char* tag, const char* value){

    m_chbuf[0] = 0;
    // V2.2
    if(!strcmp(tag, "CNT")) sprintf(m_chbuf, "Play counter: %s", value);
    // if(!strcmp(tag, "COM")) sprintf(m_chbuf, "Comments: %s", value);
    if(!strcmp(tag, "CRA")) sprintf(m_chbuf, "Audio encryption: %s", value);
    if(!strcmp(tag, "CRM")) sprintf(m_chbuf, "Encrypted meta frame: %s", value);
    if(!strcmp(tag, "ETC")) sprintf(m_chbuf, "Event timing codes: %s", value);
    if(!strcmp(tag, "EQU")) sprintf(m_chbuf, "Equalization: %s", value);
    if(!strcmp(tag, "IPL")) sprintf(m_chbuf, "Involved people list: %s", value);
    if(!strcmp(tag, "PIC")) sprintf(m_chbuf, "Attached picture: %s", value);
    if(!strcmp(tag, "SLT")) sprintf(m_chbuf, "Synchronized lyric/text: %s", value);
    if(!strcmp(tag, "TAL")) sprintf(m_chbuf, "Album/Movie/Show title: %s", value);
    if(!strcmp(tag, "TBP")) sprintf(m_chbuf, "BPM (Beats Per Minute): %s", value);
    if(!strcmp(tag, "TCM")) sprintf(m_chbuf, "Composer: %s", value);
    if(!strcmp(tag, "TCO")) sprintf(m_chbuf, "Content type: %s", value);
    if(!strcmp(tag, "TCR")) sprintf(m_chbuf, "Copyright message: %s", value);
    if(!strcmp(tag, "TDA")) sprintf(m_chbuf, "Date: %s", value);
    if(!strcmp(tag, "TDY")) sprintf(m_chbuf, "Playlist delay: %s", value);
    if(!strcmp(tag, "TEN")) sprintf(m_chbuf, "Encoded by: %s", value);
    if(!strcmp(tag, "TFT")) sprintf(m_chbuf, "File type: %s", value);
    if(!strcmp(tag, "TIM")) sprintf(m_chbuf, "Time: %s", value);
    if(!strcmp(tag, "TKE")) sprintf(m_chbuf, "Initial key: %s", value);
    if(!strcmp(tag, "TLA")) sprintf(m_chbuf, "Language(s): %s", value);
    if(!strcmp(tag, "TLE")) sprintf(m_chbuf, "Length: %s", value);
    if(!strcmp(tag, "TMT")) sprintf(m_chbuf, "Media type: %s", value);
    if(!strcmp(tag, "TOA")) sprintf(m_chbuf, "Original artist(s)/performer(s): %s", value);
    if(!strcmp(tag, "TOF")) sprintf(m_chbuf, "Original filename: %s", value);
    if(!strcmp(tag, "TOL")) sprintf(m_chbuf, "Original Lyricist(s)/text writer(s): %s", value);
    if(!strcmp(tag, "TOR")) sprintf(m_chbuf, "Original release year: %s", value);
    if(!strcmp(tag, "TOT")) sprintf(m_chbuf, "Original album/Movie/Show title: %s", value);
    if(!strcmp(tag, "TP1")) sprintf(m_chbuf, "Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group: %s", value);
    if(!strcmp(tag, "TP2")) sprintf(m_chbuf, "Band/Orchestra/Accompaniment: %s", value);
    if(!strcmp(tag, "TP3")) sprintf(m_chbuf, "Conductor/Performer refinement: %s", value);
    if(!strcmp(tag, "TP4")) sprintf(m_chbuf, "Interpreted, remixed, or otherwise modified by: %s", value);
    if(!strcmp(tag, "TPA")) sprintf(m_chbuf, "Part of a set: %s", value);
    if(!strcmp(tag, "TPB")) sprintf(m_chbuf, "Publisher: %s", value);
    if(!strcmp(tag, "TRC")) sprintf(m_chbuf, "ISRC (International Standard Recording Code): %s", value);
    if(!strcmp(tag, "TRD")) sprintf(m_chbuf, "Recording dates: %s", value);
    if(!strcmp(tag, "TRK")) sprintf(m_chbuf, "Track number/Position in set: %s", value);
    if(!strcmp(tag, "TSI")) sprintf(m_chbuf, "Size: %s", value);
    if(!strcmp(tag, "TSS")) sprintf(m_chbuf, "Software/hardware and settings used for encoding: %s", value);
    if(!strcmp(tag, "TT1")) sprintf(m_chbuf, "Content group description: %s", value);
    if(!strcmp(tag, "TT2")) sprintf(m_chbuf, "Title/Songname/Content description: %s", value);
    if(!strcmp(tag, "TT3")) sprintf(m_chbuf, "Subtitle/Description refinement: %s", value);
    if(!strcmp(tag, "TXT")) sprintf(m_chbuf, "Lyricist/text writer: %s", value);
    if(!strcmp(tag, "TXX")) sprintf(m_chbuf, "User defined text information frame: %s", value);
    if(!strcmp(tag, "TYE")) sprintf(m_chbuf, "Year: %s", value);
    if(!strcmp(tag, "UFI")) sprintf(m_chbuf, "Unique file identifier: %s", value);
    if(!strcmp(tag, "ULT")) sprintf(m_chbuf, "Unsychronized lyric/text transcription: %s", value);
    if(!strcmp(tag, "WAF")) sprintf(m_chbuf, "Official audio file webpage: %s", value);
    if(!strcmp(tag, "WAR")) sprintf(m_chbuf, "Official artist/performer webpage: %s", value);
    if(!strcmp(tag, "WAS")) sprintf(m_chbuf, "Official audio source webpage: %s", value);
    if(!strcmp(tag, "WCM")) sprintf(m_chbuf, "Commercial information: %s", value);
    if(!strcmp(tag, "WCP")) sprintf(m_chbuf, "Copyright/Legal information: %s", value);
    if(!strcmp(tag, "WPB")) sprintf(m_chbuf, "Publishers official webpage: %s", value);
    if(!strcmp(tag, "WXX")) sprintf(m_chbuf, "User defined URL link frame: %s", value);

    // V2.3 V2.4 tags
    // if(!strcmp(tag, "COMM")) sprintf(m_chbuf, "Comment: %s", value);
    if(!strcmp(tag, "OWNE")) sprintf(m_chbuf, "Ownership: %s", value);
    // if(!strcmp(tag, "PRIV")) sprintf(m_chbuf, "Private: %s", value);
    if(!strcmp(tag, "SYLT")) sprintf(m_chbuf, "SynLyrics: %s", value);
    if(!strcmp(tag, "TALB")) { sprintf(m_chbuf, "Album: %s", value); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TBPM")) sprintf(m_chbuf, "BeatsPerMinute: %s", value);
    if(!strcmp(tag, "TCMP")) sprintf(m_chbuf, "Compilation: %s", value);
    if(!strcmp(tag, "TCOM")) sprintf(m_chbuf, "Composer: %s", value);
    if(!strcmp(tag, "TCON")) sprintf(m_chbuf, "ContentType: %s", value);
    if(!strcmp(tag, "TCOP")) sprintf(m_chbuf, "Copyright: %s", value);
    if(!strcmp(tag, "TDAT")) sprintf(m_chbuf, "Date: %s", value);
    if(!strcmp(tag, "TEXT")) sprintf(m_chbuf, "Lyricist: %s", value);
    if(!strcmp(tag, "TIME")) sprintf(m_chbuf, "Time: %s", value);
    if(!strcmp(tag, "TIT1")) sprintf(m_chbuf, "Grouping: %s", value);
    if(!strcmp(tag, "TIT2")) { sprintf(m_chbuf, "Title: %s", value); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TIT3")) sprintf(m_chbuf, "Subtitle: %s", value);
    if(!strcmp(tag, "TLAN")) sprintf(m_chbuf, "Language: %s", value);
    if(!strcmp(tag, "TLEN")) sprintf(m_chbuf, "Length (ms): %s", value);
    if(!strcmp(tag, "TMED")) sprintf(m_chbuf, "Media: %s", value);
    if(!strcmp(tag, "TOAL")) sprintf(m_chbuf, "OriginalAlbum: %s", value);
    if(!strcmp(tag, "TOPE")) sprintf(m_chbuf, "OriginalArtist: %s", value);
    if(!strcmp(tag, "TORY")) sprintf(m_chbuf, "OriginalReleaseYear: %s", value);
    if(!strcmp(tag, "TPE1")) { sprintf(m_chbuf, "Artist: %s", value); if(audio_id3artist) audio_id3artist(value); }
    if(!strcmp(tag, "TPE2")) sprintf(m_chbuf, "Band: %s", value);
    if(!strcmp(tag, "TPE3")) sprintf(m_chbuf, "Conductor: %s", value);
    if(!strcmp(tag, "TPE4")) sprintf(m_chbuf, "InterpretedBy: %s", value);
    if(!strcmp(tag, "TPOS")) sprintf(m_chbuf, "PartOfSet: %s", value);
    if(!strcmp(tag, "TPUB")) sprintf(m_chbuf, "Publisher: %s", value);
    if(!strcmp(tag, "TRCK")) sprintf(m_chbuf, "Track: %s", value);
    if(!strcmp(tag, "TSSE")) sprintf(m_chbuf, "SettingsForEncoding: %s", value);
    if(!strcmp(tag, "TRDA")) sprintf(m_chbuf, "RecordingDates: %s", value);
    if(!strcmp(tag, "TXXX")) sprintf(m_chbuf, "UserDefinedText: %s", value);
    if(!strcmp(tag, "TYER")) sprintf(m_chbuf, "Year: %s", value);
    if(!strcmp(tag, "USER")) sprintf(m_chbuf, "TermsOfUse: %s", value);
    if(!strcmp(tag, "USLT")) sprintf(m_chbuf, "Lyrics: %s", value);
    if(!strcmp(tag, "WOAR")) sprintf(m_chbuf, "OfficialArtistWebpage: %s", value);
    if(!strcmp(tag, "XDOR")) sprintf(m_chbuf, "OriginalReleaseTime: %s", value);

    latinToUTF8(m_chbuf, sizeof(m_chbuf));
    if(indexOf(m_chbuf, "?xml", 0) > 0) {showstreamtitle(m_chbuf); return;}
    if(m_chbuf[0] != 0){if(audio_id3data) audio_id3data(m_chbuf);}
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getFileSize(){
    if (!audiofile) return 0;
    uint32_t s = audiofile.size();
    return s;
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getFilePos(){
    if (!audiofile) return 0;
    uint32_t p = audiofile.position();
    return p;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::setFilePos(uint32_t pos){
    if (!audiofile) return false;
    bool s = audiofile.seek(pos);
    return s;
}
uint32_t Audio::getAudioFileDuration(){
  if(!audiofile) return 0;
  return m_audioFileDuration;
}

const uint16_t l3id012[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
const uint16_t l3id3[16]   = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
char           brbuf[32];
uint32_t Audio::getAudioCurrentTime(){
  uint16_t SCIStatus;
  uint32_t prev_bitrate = m_avr_bitrate;
  if(m_codec == CODEC_MP3){
    SCIStatus=read_register(SCI_HDAT1);
    uint8_t layer = SCIStatus>>1 & 0b11;
    uint8_t id    = SCIStatus>>3 & 0b11;
    SCIStatus = read_register(SCI_HDAT0);
    if(layer==1){ //layer3
      if(id==3){
        m_avr_bitrate = l3id3[SCIStatus>>12] * 1000;
      }else{
        m_avr_bitrate = l3id012[SCIStatus>>12] * 1000;
      }
      m_localBitrateSend = prev_bitrate==m_avr_bitrate;
      if(m_avr_bitrate==0) return 0;
      sprintf(brbuf, "%lu", m_avr_bitrate);
      if(audio_bitrate && !m_localBitrateSend) audio_bitrate(brbuf);
      m_localBitrateSend = true;
      m_audioFileDuration = 8 * ((float)m_audioDataSize / (m_avr_bitrate));
      return 8 * ((float)getFilePos() / (m_avr_bitrate));
    }
    return 0;
  }
  if(m_codec == CODEC_AAC || m_codec == CODEC_FLAC || m_codec == CODEC_WAV || m_codec == CODEC_M4A){
    SCIStatus=read_register(SCI_HDAT0);
    m_avr_bitrate = SCIStatus * 8;
    m_localBitrateSend = prev_bitrate==m_avr_bitrate;
    if(m_avr_bitrate==0) return 0;
    sprintf(brbuf, "%lu", m_avr_bitrate);
    if(audio_bitrate && !m_localBitrateSend) audio_bitrate(brbuf);
    m_localBitrateSend = true;
    m_audioFileDuration = 8 * ((float)m_audioDataSize / (m_avr_bitrate));
    return 8 * ((float)getFilePos() / (m_avr_bitrate));
  }
  return 0;
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getAudioDataStartPos() {
    if(!audiofile) return 0;
    return m_audioDataStart;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::urlencode(char* buff, uint16_t buffLen, bool spacesOnly){
    uint16_t len = strlen(buff);
    uint8_t* tmpbuff = (uint8_t*)malloc(buffLen);
    if(!tmpbuff) {log_e("out of memory"); return;}
    char c;
    char code0;
    char code1;
    uint16_t j = 0;
    for(int i = 0; i < len; i++) {
        c = buff[i];
        if(isalnum(c)) tmpbuff[j++] = c;
        else if(spacesOnly){
            if(c == ' '){
                tmpbuff[j++] = '%';
                tmpbuff[j++] = '2';
                tmpbuff[j++] = '0';
            }
            else{
                tmpbuff[j++] = c;
            }
        }
        else {
            code1 = (c & 0xf) + '0';
            if((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if(c > 9) code0 = c - 10 + 'A';
            tmpbuff[j++] = '%';
            tmpbuff[j++] = code0;
            tmpbuff[j++] = code1;
        }
        if(j == buffLen - 1){
            log_e("out of memory");
            break;
        }
    }
    memcpy(buff, tmpbuff, j);
    buff[j] ='\0';
    free(tmpbuff);
}
//----------------------------------------------------------------------------------------------------------------------
//    AAC - T R A N S P O R T S T R E A M
//----------------------------------------------------------------------------------------------------------------------
bool Audio::ts_parsePacket(uint8_t* packet, uint8_t* packetStart, uint8_t* packetLength) {

    const uint8_t TS_PACKET_SIZE = 188;
    const uint8_t PAYLOAD_SIZE = 184;
    const uint8_t PID_ARRAY_LEN = 4;

    (void) PAYLOAD_SIZE;  // suppress [-Wunused-variable]

    typedef struct{
        int number= 0;
        int pids[PID_ARRAY_LEN];
    } pid_array;

    static pid_array pidsOfPMT;
    static int PES_DataLength = 0;
    static int pidOfAAC = 0;

    if(packet == NULL){
        if(m_f_Log) log_i("parseTS reset");
        for(int i = 0; i < PID_ARRAY_LEN; i++) pidsOfPMT.pids[i] = 0;
        PES_DataLength = 0;
        pidOfAAC = 0;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------
    // 0. Byte SyncByte  | 0 | 1 | 0 | 0 | 0 | 1 | 1 | 1 | always bit pattern of 0x47
    //---------------------------------------------------------------------------------------------------------
    // 1. Byte           |PUSI|TP|   |PID|PID|PID|PID|PID|
    //---------------------------------------------------------------------------------------------------------
    // 2. Byte           |PID|PID|PID|PID|PID|PID|PID|PID|
    //---------------------------------------------------------------------------------------------------------
    // 3. Byte           |TSC|TSC|AFC|AFC|CC |CC |CC |CC |
    //---------------------------------------------------------------------------------------------------------
    // 4.-187. Byte      |Payload data if AFC==01 or 11  |
    //---------------------------------------------------------------------------------------------------------

    // PUSI Payload unit start indicator, set when this packet contains the first byte of a new payload unit.
    //      The first byte of the payload will indicate where this new payload unit starts.
    // TP   Transport priority, set when the current packet has a higher priority than other packets with the same PID.
    // PID  Packet Identifier, describing the payload data.
    // TSC  Transport scrambling control, '00' = Not scrambled.
    // AFC  Adaptation field control, 01 – no adaptation field, payload only, 10 – adaptation field only, no payload,
    //                                11 – adaptation field followed by payload, 00 – RESERVED for future use
    // CC   Continuity counter, Sequence number of payload packets (0x00 to 0x0F) within each stream (except PID 8191)

    if(packet[0] != 0x47) {
        log_e("ts SyncByte not found, first bytes are %X %X %X %X", packet[0], packet[1], packet[2], packet[3]);
        stopSong();
        return false;
    }
    int PID = (packet[1] & 0x1F) << 8 | (packet[2] & 0xFF);
    if(m_f_Log) log_i("PID: 0x%04X(%d)", PID, PID);
    int PUSI = (packet[1] & 0x40) >> 6;
    if(m_f_Log) log_i("Payload Unit Start Indicator: %d", PUSI);
    int AFC = (packet[3] & 0x30) >> 4;
    if(m_f_Log) log_i("Adaption Field Control: %d", AFC);

    int AFL = -1;
    if((AFC & 0b10) == 0b10) {  // AFC '11' Adaptation Field followed
        AFL = packet[4] & 0xFF; // Adaptation Field Length
        if(m_f_Log) log_i("Adaptation Field Length: %d", AFL);
    }
    int PLS = PUSI ? 5 : 4;     // PayLoadStart, Payload Unit Start Indicator
    if(AFL > 0) PLS += AFL + 1; // skip adaption field

    if(PID == 0) {
        // Program Association Table (PAT) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_f_Log)  log_i("PAT");
        pidsOfPMT.number = 0;
        pidOfAAC = 0;
    
        int startOfProgramNums = 8;
        int lengthOfPATValue = 4;
        int sectionLength = ((packet[PLS + 1] & 0x0F) << 8) | (packet[PLS + 2] & 0xFF);
        if(m_f_Log) log_i("Section Length: %d", sectionLength);
        int program_number, program_map_PID;
        int indexOfPids = 0;
        (void) program_number; // [-Wunused-but-set-variable]
        for(int i = startOfProgramNums; i <= sectionLength; i += lengthOfPATValue) {
            program_number = ((packet[PLS + i] & 0xFF) << 8) | (packet[PLS + i + 1] & 0xFF);
            program_map_PID = ((packet[PLS + i + 2] & 0x1F) << 8) | (packet[PLS + i + 3] & 0xFF);
            if(m_f_Log) log_i("Program Num: 0x%04X(%d) PMT PID: 0x%04X(%d)", program_number, program_number,
                   program_map_PID, program_map_PID);
            pidsOfPMT.pids[indexOfPids++] = program_map_PID;
        }
        pidsOfPMT.number = indexOfPids;
        *packetStart = 0;
        *packetLength = 0;
        return true;

    }
    else if(PID == pidOfAAC) {
        static uint8_t fillData = 0;
        if(m_f_Log) log_i("AAC");
        uint8_t posOfPacketStart = 4;
        if(AFL >= 0) {posOfPacketStart = 5 + AFL;
        if(m_f_Log) log_i("posOfPacketStart: %d", posOfPacketStart);}
        // Packetized Elementary Stream (PES) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_f_Log) log_i("PES_DataLength %i", PES_DataLength);
        if (PES_DataLength > 0) {
            *packetStart    = posOfPacketStart + fillData;
            *packetLength   = TS_PACKET_SIZE - posOfPacketStart - fillData;
            fillData = 0;
            PES_DataLength -= (*packetLength);
            return true;
        }
        else{
            int firstByte  = packet[posOfPacketStart] & 0xFF;
            int secondByte = packet[posOfPacketStart + 1] & 0xFF;
            int thirdByte  = packet[posOfPacketStart + 2] & 0xFF;
            if(m_f_Log) log_i("First 3 bytes: %02X %02X %02X", firstByte, secondByte, thirdByte);
            if(firstByte == 0x00 && secondByte == 0x00 && thirdByte == 0x01) { // Packet start code prefix
                // PES
                uint8_t StreamID = packet[posOfPacketStart + 3] & 0xFF;
                if(StreamID >= 0xC0 && StreamID <= 0xDF) {;} // okay ist audio stream
                if(StreamID >= 0xE0 && StreamID <= 0xEF) {log_e("video stream!"); return false;}
                uint8_t PES_HeaderDataLength = packet[posOfPacketStart + 8] & 0xFF;
                if(m_f_Log) log_i("PES_headerDataLength %d", PES_HeaderDataLength);
                int PES_PacketLength =
                    ((packet[posOfPacketStart + 4] & 0xFF) << 8) + (packet[posOfPacketStart + 5] & 0xFF);
                if(m_f_Log) log_i("PES Packet length: %d", PES_PacketLength);
                PES_DataLength = PES_PacketLength;
                int startOfData = PES_HeaderDataLength + 9;
                if(posOfPacketStart + startOfData >= 188){ // only fillers in packet
                    if(m_f_Log) log_e("posOfPacketStart + startOfData %i", posOfPacketStart + startOfData);
                    *packetStart = 0;
                    *packetLength = 0;
                    PES_DataLength -= (PES_HeaderDataLength + 3);
                    fillData = (posOfPacketStart + startOfData) - 188;
                    if(m_f_Log) log_i("fillData %i", fillData);
                    return true;
                }
                if(m_f_Log) log_i("First AAC data byte: %02X", packet[posOfPacketStart + startOfData]);
                if(m_f_Log) log_i("Second AAC data byte: %02X", packet[posOfPacketStart + startOfData + 1]);
                *packetStart = posOfPacketStart + startOfData;
                *packetLength = TS_PACKET_SIZE - posOfPacketStart - startOfData;
                PES_DataLength -= (*packetLength);
                PES_DataLength -= (PES_HeaderDataLength + 3);
                return true;
            }
        }
        *packetStart = 0;
        *packetLength = 0;
        log_e("PES not found");
        return false;
    }
    else if(pidsOfPMT.number) {
        //  Program Map Table (PMT) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        for(int i = 0; i < pidsOfPMT.number; i++) {
            if(PID == pidsOfPMT.pids[i]) {
                if(m_f_Log) log_i("PMT");
                int staticLengthOfPMT = 12;
                int sectionLength = ((packet[PLS + 1] & 0x0F) << 8) | (packet[PLS + 2] & 0xFF);
                if(m_f_Log) log_i("Section Length: %d", sectionLength);
                int programInfoLength = ((packet[PLS + 10] & 0x0F) << 8) | (packet[PLS + 11] & 0xFF);
                if(m_f_Log) log_i("Program Info Length: %d", programInfoLength);
                int cursor = staticLengthOfPMT + programInfoLength;
                while(cursor < sectionLength - 1) {
                    int streamType = packet[PLS + cursor] & 0xFF;
                    int elementaryPID = ((packet[PLS + cursor + 1] & 0x1F) << 8) | (packet[PLS + cursor + 2] & 0xFF);
                    if(m_f_Log) log_i("Stream Type: 0x%02X Elementary PID: 0x%04X", streamType, elementaryPID);

                    if(streamType == 0x0F || streamType == 0x11 || streamType == 0x04) {
                        if(m_f_Log) log_i("AAC PID discover");
                        pidOfAAC= elementaryPID;
                    }
                    int esInfoLength = ((packet[PLS + cursor + 3] & 0x0F) << 8) | (packet[PLS + cursor + 4] & 0xFF);
                    if(m_f_Log) log_i("ES Info Length: 0x%04X", esInfoLength);
                    cursor += 5 + esInfoLength;
                }
            }
        }
        *packetStart = 0;
        *packetLength = 0;
        return true;
    }
    // PES received before PAT and PMT seen
    *packetStart = 0;
    *packetLength = 0;
    return false;
}
//----------------------------------------------------------------------------------------------------------------------
//    W E B S T R E A M  -  H E L P   F U N C T I O N S
//----------------------------------------------------------------------------------------------------------------------
uint16_t Audio::readMetadata(uint16_t maxBytes, bool first) {

    static uint16_t pos_ml = 0;                          // determines the current position in metaline
    static uint16_t metalen = 0;
    uint16_t res = 0;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(first){
        pos_ml = 0;
        metalen = 0;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(!maxBytes) return 0;  // guard

    if(!metalen) {
        int b = _client->read();   // First byte of metadata?
        metalen = b * 16 ;                              // New count for metadata including length byte, max 4096
        pos_ml = 0; m_chbuf[pos_ml] = 0;                // Prepare for new line
        res = 1;
    }
    if(!metalen) {m_metacount = m_metaint; return res;} // metalen is 0
    if(metalen < m_chbufSize){
        uint16_t a = _client->readBytes(&m_chbuf[pos_ml], min((uint16_t)(metalen - pos_ml), (uint16_t)(maxBytes -1)));
        res += a;
        pos_ml += a;
    }
    else{ // metadata doesn't fit in m_chbuf
        uint8_t c = 0;
        int8_t i = 0;
        while(pos_ml != metalen){
            i = _client->read(&c, 1); // fake read
            if(i != -1) {pos_ml++; res++;}
            else {return res;}
        }
        m_metacount = m_metaint;
        metalen = 0;
        pos_ml = 0;
        return res;
    }
    if(pos_ml == metalen) {
        m_chbuf[pos_ml] = '\0';
        if(strlen(m_chbuf)) {                             // Any info present?
            // metaline contains artist and song name.  For example:
            // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
            // Sometimes it is just other info like:
            // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
            // Isolate the StreamTitle, remove leading and trailing quotes if present.
            latinToUTF8(m_chbuf, m_chbufSize); // convert to UTF-8 if necessary
            int pos = indexOf(m_chbuf, "song_spot", 0);    // remove some irrelevant infos
            if(pos > 3) {                                // e.g. song_spot="T" MediaBaseId="0" itunesTrackId="0"
                m_chbuf[pos] = 0;
            }
            showstreamtitle(m_chbuf);   // Show artist and title if present in metadata
        }
        m_metacount = m_metaint;
        metalen = 0;
        pos_ml = 0;
    }
    return res;
}
//----------------------------------------------------------------------------------------------------------------------
size_t Audio::chunkedDataTransfer(uint8_t* bytes){
    uint8_t byteCounter = 0;
    size_t chunksize = 0;
    int b = 0;
    uint32_t ctime = millis();
    uint32_t timeout = 2000; // ms
    while(true){
        if(ctime + timeout < millis()) {
            log_e("timeout");
            stopSong();
            return 0;
        }
        b = _client->read();
        byteCounter++;
        if(b < 0) continue;  // -1 no data available
        if(b == '\n') break;
        if(b < '0') continue;
        // We have received a hexadecimal character.  Decode it and add to the result.
        b = toupper(b) - '0';                       // Be sure we have uppercase
        if(b > 9) b = b - 7;                        // Translate A..F to 10..15
        chunksize = (chunksize << 4) + b;
    }
    if(m_f_Log) log_i("chunksize %d", chunksize);
    *bytes = byteCounter;
    return chunksize;
}
//----------------------------------------------------------------------------------------------------------------------
bool Audio::readID3V1Tag(){
    // this is an V1.x id3tag after an audio block, ID3 v1 tags are ASCII
    // Version 1.x is a fixed size at the end of the file (128 bytes) after a <TAG> keyword.
    if(m_codec != CODEC_MP3) return false;
    if(InBuff.bufferFilled() == 128 && startsWith((const char*)InBuff.getReadPtr(), "TAG")){ // maybe a V1.x TAG
        char title[31];
        memcpy(title,   InBuff.getReadPtr() + 3 +  0,  30);  title[30]  = '\0'; latinToUTF8(title, sizeof(title));
        char artist[31];
        memcpy(artist,  InBuff.getReadPtr() + 3 + 30,  30); artist[30]  = '\0'; latinToUTF8(artist, sizeof(artist));
        char album[31];
        memcpy(album,   InBuff.getReadPtr() + 3 + 60,  30);  album[30]  = '\0'; latinToUTF8(album, sizeof(album));
        char year[5];
        memcpy(year,    InBuff.getReadPtr() + 3 + 90,   4);  year[4]    = '\0'; latinToUTF8(year, sizeof(year));
        char comment[31];
        memcpy(comment, InBuff.getReadPtr() + 3 + 94,  30); comment[30] = '\0'; latinToUTF8(comment, sizeof(comment));
        uint8_t zeroByte = *(InBuff.getReadPtr() + 125);
        uint8_t track    = *(InBuff.getReadPtr() + 126);
        uint8_t genre    = *(InBuff.getReadPtr() + 127);
        if(zeroByte) {AUDIO_INFO("ID3 version: 1");} //[2]
        else         {AUDIO_INFO("ID3 Version 1.1");}
        if(strlen(title))  {sprintf(m_chbuf, "Title: %s",        title);   if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(artist)) {sprintf(m_chbuf, "Artist: %s",       artist);  if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(album))  {sprintf(m_chbuf, "Album: %s",        album);   if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(year))   {sprintf(m_chbuf, "Year: %s",         year);    if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(comment)){sprintf(m_chbuf, "Comment: %s",      comment); if(audio_id3data) audio_id3data(m_chbuf);}
        if(zeroByte == 0)  {sprintf(m_chbuf, "Track Number: %d", track);   if(audio_id3data) audio_id3data(m_chbuf);}
        if(genre < 192)    {sprintf(m_chbuf, "Genre: %d",        genre);   if(audio_id3data) audio_id3data(m_chbuf);} //[1]
        return true;
    }
    if(InBuff.bufferFilled() == 227 && startsWith((const char*)InBuff.getReadPtr(), "TAG+")){ // ID3V1EnhancedTAG
        AUDIO_INFO("ID3 version: 1 - Enhanced TAG");
        char title[61];
        memcpy(title,   InBuff.getReadPtr() + 4 +   0,  60);  title[60] = '\0'; latinToUTF8(title, sizeof(title));
        char artist[61];
        memcpy(artist,  InBuff.getReadPtr() + 4 +  60,  60); artist[60] = '\0'; latinToUTF8(artist, sizeof(artist));
        char album[61];
        memcpy(album,   InBuff.getReadPtr() + 4 + 120,  60);  album[60] = '\0'; latinToUTF8(album, sizeof(album));
        // one byte "speed" 0=unset, 1=slow, 2= medium, 3=fast, 4=hardcore
        char genre[31];
        memcpy(genre,   InBuff.getReadPtr() + 5 + 180,  30);  genre[30] = '\0'; latinToUTF8(genre, sizeof(genre));
        // six bytes "start-time", the start of the music as mmm:ss
        // six bytes "end-time",   the end of the music as mmm:ss
        if(strlen(title))  {sprintf(m_chbuf, "Title: %s",  title);  if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(artist)) {sprintf(m_chbuf, "Artist: %s", artist); if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(album))  {sprintf(m_chbuf, "Album: %s",  album);  if(audio_id3data) audio_id3data(m_chbuf);}
        if(strlen(genre))  {sprintf(m_chbuf, "Genre: %s",  genre);  if(audio_id3data) audio_id3data(m_chbuf);}
        return true;
    }
    return false;
    // [1] https://en.wikipedia.org/wiki/List_of_ID3v1_Genres
    // [2] https://en.wikipedia.org/wiki/ID3#ID3v1_and_ID3v1.1[5]
}
//----------------------------------------------------------------------------------------------------------------------
boolean Audio::streamDetection(uint32_t bytesAvail){
    static uint32_t tmr_slow = millis();
    static uint32_t tmr_lost = millis();
    static uint8_t  cnt_slow = 0;
    static uint8_t  cnt_lost = 0;

    // if within one second the content of the audio buffer falls below the size of an audio frame 100 times,
    // issue a message
    if(tmr_slow + 1000 < millis()){
        tmr_slow = millis();
        if(cnt_slow > 100) AUDIO_INFO("slow stream, dropouts are possible");
        cnt_slow = 0;
    }
    if(InBuff.bufferFilled() < InBuff.getMaxBlockSize()) cnt_slow++;
    if(bytesAvail) {tmr_lost = millis() + 1000; cnt_lost = 0;}
    if(InBuff.bufferFilled() > InBuff.getMaxBlockSize() * 2) return false; // enough data available to play

    // if no audio data is received within three seconds, a new connection attempt is started.
    if(tmr_lost < millis()){
        cnt_lost++;
        tmr_lost = millis() + 1000;
        if(cnt_lost == 5){ // 5s no data?
            cnt_lost = 0;
            AUDIO_INFO("Stream lost -> try new connection");
            connecttohost(m_lastHost);
            return true;
        }
    }
    return false;
}
//----------------------------------------------------------------------------------------------------------------------
uint8_t Audio::determineOggCodec(uint8_t* data, uint16_t len){
    // if we have contentType == application/ogg; codec cn be OPUS, FLAC or VORBIS
    // let's have a look, what it is
    int idx = specialIndexOf(data, "OggS", 6);
    if(idx != 0){
        if(specialIndexOf(data, "fLaC", 6)) return CODEC_FLAC;
        return CODEC_NONE;
    }
    data += 27;
    idx = specialIndexOf(data, "OpusHead", 40);
    if(idx >= 0) return CODEC_OPUS;
    idx = specialIndexOf(data, "fLaC", 40);
    if(idx >= 0) return CODEC_FLAC;
    idx = specialIndexOf(data, "vorbis", 40);
    if(idx >= 0){log_i("vorbis"); return CODEC_VORBIS;}
    return CODEC_NONE;
}
//----------------------------------------------------------------------------------------------------------------------

#endif  //  if I2S_DOUT==255
