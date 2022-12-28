#include "../core/options.h"
#if I2S_DOUT==255
/*
 *  vs1053_ext.cpp
 *
 *  Created on: Jul 09.2017
 *  Updated on: Aug 15.2022
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
        m_buffSize = m_buffSizeRAM;
        m_buffer = (uint8_t*) calloc(m_buffSize, sizeof(uint8_t));
        m_buffSize = m_buffSizeRAM - m_resBuffSizeRAM;
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
Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi, uint8_t mosi, uint8_t miso, uint8_t sclk) :
        cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin)
{
    spi_VS1053 = new SPIClass(spi);
    spi_VS1053->begin(sclk, miso, mosi, -1);

    clientsecure.setInsecure();                 // update to ESP32 Arduino version 1.0.5-rc05 or higher
    m_endFillByte=0;
    curvol=50;
    m_LFcount=0;
}
Audio::~Audio(){
    // destructor
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::initInBuff() {
    static bool f_already_done = false;
    if(!f_already_done) {
        size_t size = InBuff.init();
        if(size == m_buffSizeRAM - m_resBuffSizeRAM) {
            sprintf(chbuf, "PSRAM not found, inputBufferSize: %u bytes", size - 1);
            if(audio_info)  audio_info(chbuf);
            f_already_done = true;
        }
        if(size == m_buffSizePSRAM - m_resBuffSizePSRAM) {
            sprintf(chbuf, "PSRAM found, inputBufferSize: %u bytes", size - 1);
            if(audio_info) audio_info(chbuf);
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
    xSemaphoreGive(mutex_pl);
}
void Audio::control_mode_on()
{
    xSemaphoreTake(mutex_pl, portMAX_DELAY);
    spi_VS1053->beginTransaction(VS1053_SPI_CTL);   // Prevent other SPI users
    DCS_HIGH();                                     // Bring slave in control mode
    CS_LOW();
}
void Audio::data_mode_on()
{
    xSemaphoreTake(mutex_pl, portMAX_DELAY);
    spi_VS1053->beginTransaction(VS1053_SPI_DATA);  // Prevent other SPI users
    CS_HIGH();                                      // Bring slave in data mode
    DCS_LOW();
}
void Audio::data_mode_off()
{
    //digitalWrite(dcs_pin, HIGH);              // End data mode
    DCS_HIGH();
    spi_VS1053->endTransaction();                       // Allow other SPI users
    xSemaphoreGive(mutex_pl);
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

    size_t chunk_length;                                    // Length of chunk 32 byte or shorter

    data_mode_on();
    while(len)                                              // More to do?
    {
        await_data_request();                               // Wait for space available
        chunk_length=len;
        if(len > vs1053_chunk_size){
            chunk_length=vs1053_chunk_size;
        }
        len-=chunk_length;
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

    pinMode(dreq_pin, INPUT);                               // DREQ is an input
    pinMode(cs_pin, OUTPUT);                                // The SCI and SDI signals
    pinMode(dcs_pin, OUTPUT);
    mutex_pl = xSemaphoreCreateMutex();
    DCS_HIGH();
    CS_HIGH();
    delay(170);

    VS1053_SPI_CTL   = SPISettings( 250000, MSBFIRST, SPI_MODE0);
    VS1053_SPI_DATA  = SPISettings(8000000, MSBFIRST, SPI_MODE0); // SPIDIV 10 -> 80/10=8.00 MHz
    // printDetails("Right after reset/startup");
    //loadUserCode(); // load in VS1053B if you want to play flac
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
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_LINE1));
    // testComm("Fast SPI, Testing VS1053 read/write registers again... \n");
    await_data_request();
    //set vu meter
    setVUmeter();
    m_endFillByte = wram_read(0x1E06) & 0xFF;
    //  printDetails("After last clocksetting \n");
    if(VS_PATCH_ENABLE) loadUserCode(); // load in VS1053B if you want to play flac
    startSong();
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
Name						Bits			Description
ST AMPLITUDE		15:12			Treble Control in 1.5 dB steps (-8..7, 0 = off)
ST FREQLIMIT		11:8			Lower limit frequency in 1000 Hz steps (1..15) 			// 1000..15000
SB AMPLITUDE		7:4				Bass Enhancement in 1 dB steps (0..15, 0 = off)
SB FREQLIMIT		3:0				Lower limit frequency in 10 Hz steps (2..15)				// 20..150
*/
void Audio::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){ 
	if(gainLowPass<0) gainLowPass=0;
	if(gainLowPass>15) gainLowPass=15;
	if(gainBandPass<0) gainBandPass=0;
	if(gainBandPass>13) gainBandPass=13;
	int8_t rtone[] = {(int8_t)map(gainHighPass, -16, 16, -8, 7), (int8_t)(2+gainBandPass), gainLowPass, (int8_t)(15-gainBandPass)};
	setTone(rtone);
}
void Audio::setBalance(int8_t bal){ 
	m_balance = bal;
	setVolume(curvol);
}
//---------------------------------------------------------------------------------------------------------------------
uint8_t Audio::getVolume()                                 // Get the currenet volume setting.
{
    return curvol;
}
//----------------------------------------------------------------------------------------------------------------------
void Audio::startSong()
{
    sdi_send_fillers(2052);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::stopSong()
{
    uint16_t modereg;                                       // Read from mode register
    int i;                                                  // Loop control

    m_f_localfile = false;
    m_f_webfile = false;
    m_f_webstream = false;
    m_f_running = false;

    sdi_send_fillers(2052);
    delay(10);
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_CANCEL));
    for(i=0; i < 200; i++) {
        sdi_send_fillers(32);
        modereg = read_register(SCI_MODE);  // Read status
        if((modereg & _BV(SM_CANCEL)) == 0) {
            sdi_send_fillers(2052);
            sprintf(chbuf, "Song stopped correctly after %d msec", i * 10);
            m_f_running = false;
            if(audio_info) audio_info(chbuf);
            return;
        }
        delay(10);
    }
    if(audio_info) audio_info("Song stopped incorrectly!");
    printDetails("after song stopped incorrectly");
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
        sprintf(chbuf, "%s   %s   %s   %s", regName[i], decbuf[i], binbuf[i], hexbuf[i]);
        if(audio_info) audio_info(chbuf);
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
void Audio::showstreamtitle(const char* ml) {
    // example for ml:
    // StreamTitle='Oliver Frank - Mega Hitmix';StreamUrl='www.radio-welle-woerthersee.at';
    // or adw_ad='true';durationMilliseconds='10135';adId='34254';insertionType='preroll';

    int16_t idx1, idx2;
    uint16_t i = 0, hash = 0;

    idx1 = indexOf(ml, "StreamTitle=", 0);
    if(idx1 >= 0){                                                              // Streamtitle found
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
void Audio::cardLock(bool lock){
#if (SDC_CS!=255)
  if(lock){
    xSemaphoreTake(mutex_pl, portMAX_DELAY);
  }else{
//    digitalWrite(SDC_CS, HIGH);
    xSemaphoreGive(mutex_pl);
  }
#endif
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::loop(){
    // - localfile - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_localfile) {                                      // Playing file fron SPIFFS or SD?
        processLocalFile();
    }
    // - webstream - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_webstream) {                                      // Playing file from URL?
        //if(!m_f_running) return;
        if(m_datamode == AUDIO_PLAYLISTINIT){
            readPlayListData();
            return;
        }
        if(m_datamode == AUDIO_PLAYLISTDATA){
            if(m_playlistFormat == FORMAT_M3U)  connecttohost(parsePlaylist_M3U());
            if(m_playlistFormat == FORMAT_PLS)  connecttohost(parsePlaylist_PLS());
            if(m_playlistFormat == FORMAT_ASX)  connecttohost(parsePlaylist_ASX());
        }
        if(m_datamode == HTTP_RESPONSE_HEADER){
            parseHttpResponseHeader();
            return;
        }
        if(m_datamode == AUDIO_DATA){
            processWebStream();
            return;
        }
    }
    return;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processLocalFile() {

    if(!(audiofile && m_f_running && m_f_localfile)) return;

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
    cardLock(true); bytesAddedToBuffer = audiofile.read(InBuff.getWritePtr(), bytesCanBeWritten); cardLock(false);
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
                int res = read_MP3_Header(InBuff.getReadPtr(), bytesCanBeRead);
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
        //     sprintf(chbuf, "loop from: %u to: %u", getFilePos(), m_audioDataStart);  //TEST loop
        //     if(audio_info) audio_info(chbuf);
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
        m_f_localfile = false;
        cardLock(true);
        char *afn =strdup(audiofile.name()); // store temporary the name
        cardLock(false);
        stopSong();
        sprintf(chbuf, "End of file \"%s\"", afn);
        if(audio_info) audio_info(chbuf);
        if(audio_eof_mp3) audio_eof_mp3(afn);
        if(afn) free(afn);
    }
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::processWebStream(){
    const uint16_t  maxFrameSize = InBuff.getMaxBlockSize();
    int32_t         availableBytes = 0;                         // available bytes in stream
    static bool     f_tmr_1s;
    static bool     f_stream;                                   // first audio data received
    static int      bytesDecoded;
    static uint32_t byteCounter;                                // count received data
    static uint32_t chunksize;                                  // chunkcount read from stream
    static uint32_t tmr_1s;                                     // timer 1 sec
    static uint32_t loopCnt;                                    // count loops if clientbuffer is empty
    static uint32_t metacount;                                  // counts down bytes between metadata


    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        f_stream = false;
        byteCounter = 0;
        chunksize = 0;
        bytesDecoded = 0;
        loopCnt = 0;
        tmr_1s = millis();
        m_t0 = millis();
        metacount = m_metaint;
        readMetadata(0, true); // reset all static vars
    }

    // timer, triggers every second - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if((tmr_1s + 1000) < millis()) {
        f_tmr_1s = true;                                        // flag will be set every second for one loop only
        tmr_1s = millis();
    }

    availableBytes = _client->available();            // available from stream

    // if we have chunked data transfer: get the chunksize- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked && !m_chunkcount && availableBytes) { // Expecting a new chunkcount?
        int b = _client->read();
        if(b == '\r') return;
        if(b == '\n'){
            m_chunkcount = chunksize;
            chunksize = 0;
            if(m_f_tts){
                m_contentlength = m_chunkcount; // tts has one chunk only
                m_f_webfile = true;
                m_f_chunked = false;
            }
            return;
        }
        // We have received a hexadecimal character.  Decode it and add to the result.
        b = toupper(b) - '0';                       // Be sure we have uppercase
        if(b > 9) b = b - 7;                        // Translate A..F to 10..15
        chunksize = (chunksize << 4) + b;
        return;
    }

    // if we have metadata: get them - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(!metacount && !m_f_swm && availableBytes){
        int16_t b = _client->read();
        if(b >= 0) {
            if(m_f_chunked) m_chunkcount--;
            if(readMetadata(b)) metacount = m_metaint;
        }
        return;
    }

    // if the buffer is often almost empty issue a warning  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(InBuff.bufferFilled() < maxFrameSize && f_stream){
        static uint8_t cnt_slow = 0;
        cnt_slow ++;
        if(f_tmr_1s) {
            if(cnt_slow > 25 && audio_info) audio_info("slow stream, dropouts are possible");
            f_tmr_1s = false;
            cnt_slow = 0;
        }
    }

    // if the buffer can't filled for several seconds try a new connection  - - - - - - - - - - - - - - - - - - - - - -
    if(f_stream && !availableBytes){
        loopCnt++;
        if(loopCnt > 200000) {              // wait several seconds
            loopCnt = 0;
            if(audio_info) audio_info("Stream lost -> try new connection");
            connecttohost(m_lastHost);
        }
    }
    if(availableBytes) loopCnt = 0;


    // buffer fill routine  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(true) { // statement has no effect
        uint32_t bytesCanBeWritten = InBuff.writeSpace();
        if(!m_f_swm)    bytesCanBeWritten = min(metacount,  bytesCanBeWritten);
        if(m_f_chunked) bytesCanBeWritten = min(m_chunkcount, bytesCanBeWritten);

        int16_t bytesAddedToBuffer = 0;

        if(psramFound()) if(bytesCanBeWritten > 4096) bytesCanBeWritten = 4096; // PSRAM throttle

        if(m_f_webfile){
            // normally there is nothing to do here, if byteCounter == contentLength
            // then the file is completely read, but:
            // m4a files can have more data  (e.g. pictures ..) after the audio Block
            // therefore it is bad to read anything else (this can generate noise)
            if(byteCounter + bytesCanBeWritten >= m_contentlength) bytesCanBeWritten = m_contentlength - byteCounter;
        }

        bytesAddedToBuffer = _client->read(InBuff.getWritePtr(), bytesCanBeWritten);

        if(bytesAddedToBuffer > 0) {
            if(m_f_webfile)             byteCounter  += bytesAddedToBuffer;  // Pull request #42
            if(!m_f_swm)                metacount  -= bytesAddedToBuffer;
            if(m_f_chunked)             m_chunkcount -= bytesAddedToBuffer;
            InBuff.bytesWritten(bytesAddedToBuffer);
        }

        if(InBuff.bufferFilled() > maxFrameSize && !f_stream) {  // waiting for buffer filled
            f_stream = true;  // ready to play the audio data
            uint16_t filltime = millis() - m_t0;
            if(audio_info) audio_info("stream ready");
            sprintf(chbuf, "buffer filled in %d ms", filltime);
            if(audio_info) audio_info(chbuf);
        }
        if(!f_stream) return;
    }

    // // if we have a webfile, read the file header first - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_webfile && m_controlCounter != 100){
        if(InBuff.bufferFilled() < maxFrameSize) return;
         if(m_codec == CODEC_WAV){
            m_controlCounter = 100;
            eofHeader = true;
        }
        if(m_codec == CODEC_MP3){
            int res = read_MP3_Header(InBuff.getReadPtr(), InBuff.bufferFilled());
            if(res >= 0) bytesDecoded = res;
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
        InBuff.bytesWasRead(bytesDecoded);
        return;
    }

    // play audio data - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if((InBuff.bufferFilled() >= maxFrameSize) && (f_stream == true)) { // fill > framesize?
        bytesDecoded = sendBytes(InBuff.getReadPtr(), maxFrameSize);
        InBuff.bytesWasRead(bytesDecoded);
    }

    // have we reached the end of the webfile?  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_webfile && byteCounter == m_contentlength){
        while(InBuff.bufferFilled() > 0){
            if(InBuff.bufferFilled() == 128){ // post tag?
                if(indexOf((const char*)InBuff.getReadPtr(), "TAG", 0) == 0){
                   //  log_d("%s", InBuff.getReadPtr() + 3);
                    break;
                }
                // else log_v("%s", InBuff.getReadPtr());
            }
            bytesDecoded = sendBytes(InBuff.getReadPtr(), InBuff.bufferFilled());
            if(bytesDecoded < 0) break;
            InBuff.bytesWasRead(bytesDecoded);
        }
        stopSong(); // Correct close when play known length sound #74 and before callback #112
        if(m_f_tts){
            sprintf(chbuf, "End of speech: \"%s\"", m_lastHost);
            if(audio_info) audio_info(chbuf);
            if(audio_eof_speech) audio_eof_speech(m_lastHost);
        }
        else{
            sprintf(chbuf, "End of webstream: \"%s\"", m_lastHost);
            if(audio_info) audio_info(chbuf);
            if(audio_eof_stream) audio_eof_stream(m_lastHost);
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------
size_t Audio::chunkedDataTransfer(){
    size_t chunksize = 0;
    int b = 0;
    while(true){
        b = _client->read();
        if(b < 0) break;
        if(b == '\n') break;
        if(b < '0') continue;
        // We have received a hexadecimal character.  Decode it and add to the result.
        b = toupper(b) - '0';                       // Be sure we have uppercase
        if(b > 9) b = b - 7;                        // Translate A..F to 10..15
        chunksize = (chunksize << 4) + b;
    }
    if(m_f_Log) log_i("chunksize %d", chunksize);
    return chunksize;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::readPlayListData() {

    if(getDatamode() != AUDIO_PLAYLISTINIT) return false;
    if(_client->available() == 0) return false;

    uint32_t chunksize = 0;
    if(m_f_chunked) chunksize = chunkedDataTransfer();

    // reads the content of the playlist and stores it in the vector m_contentlength
    // m_contentlength is a table of pointers to the lines
    char pl[512]; // playlistLine
    uint32_t ctl  = 0;
    int lines = 0;
    // delete all memory in m_playlistContent
    if(!psramFound()){log_e("m3u8 playlists requires PSRAM enabled!");}
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
                if(pos == 511){ pos--; continue;}
                if(pos == 510) {pl[pos] = '\0';}
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
        if(strlen(pl) > 0) m_playlistContent.push_back(strdup((const char*)pl));
        if(m_playlistContent.size() == 100){
            if(m_f_Log) log_i("the maximum number of lines in the playlist has been reached");
            break;
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
    //    log_e("%s pos=%i", m_playlistContent[i], pos);
            host = m_playlistContent[i] + pos;                        // Yes, set new host
            break;
        }
    }
    vector_clear_and_shrink(m_playlistContent);
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
//---------------------------------------------------------------------------------------------------------------------
bool Audio::latinToUTF8(char* buff, size_t bufflen){
    // most stations send  strings in UTF-8 but a few sends in latin. To standardize this, all latin strings are
    // converted to UTF-8. If UTF-8 is already present, nothing is done and true is returned.
    // A conversion to UTF-8 extends the string. Therefore it is necessary to know the buffer size. If the converted
    // string does not fit into the buffer, false is returned
    // utf8 bytelength: >=0xF0 3 bytes, >=0xE0 2 bytes, >=0xC0 1 byte, e.g. e293ab is ⓫

    uint16_t pos = 0;
    uint8_t  ext_bytes = 0;
    uint16_t len = strlen(buff);
    uint8_t  c;

    while(pos < len){
        c = buff[pos];
        if(c >= 0xC2) {    // is UTF8 char
            pos++;
            if(c >= 0xC0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
            if(c >= 0xE0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
            if(c >= 0xF0 && buff[pos] < 0x80) {ext_bytes++; pos++;}
        }
        else pos++;
    }
    if(!ext_bytes) return true; // is UTF-8, do nothing

    pos = 0;

    while(buff[pos] != 0){
        len = strlen(buff);
        if(buff[pos] >= 0x80 && buff[pos+1] < 0x80){       // is not UTF8, is latin?
            for(int i = len+1; i > pos; i--){
                buff[i+1] = buff[i];
            }
            uint8_t c = buff[pos];
            buff[pos++] = 0xc0 | ((c >> 6) & 0x1f);      // 2+1+5 bits
            buff[pos++] = 0x80 | ((char)c & 0x3f);       // 1+1+6 bits
        }
        pos++;
        if(pos > bufflen -3){
            buff[bufflen -1] = '\0';
            return false; // do not overwrite
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool Audio::parseHttpResponseHeader() { // this is the response to a GET / request

    if(getDatamode() != HTTP_RESPONSE_HEADER) return false;
    if(_client->available() == 0)  return false;

    char rhl[512]; // responseHeaderline
    bool ct_seen = false;
    uint32_t ctime = millis();
    uint32_t timeout = 2500; // ms

    while(true){  // outer while
        uint16_t pos = 0;
        if((millis() - ctime) > timeout) {
            log_e("timeout");
            goto exit;
        }
        while(_client->available()){
            uint8_t b = _client->read();
            if(b == '\n') {
                if(!pos){ // empty line received, is the last line of this responseHeader
                    if(ct_seen) goto lastToDo;
                    else goto exit;
                }
                break;
            }
            if(b == '\r') rhl[pos] = 0;
            if(b < 0x20) continue;
            rhl[pos] = b;
            pos++;
            if(pos == 511){pos = 510; continue;}
            if(pos == 510){
                rhl[pos] = '\0';
                if(m_f_Log) log_i("responseHeaderline overflow");
            }
        } // inner while

        if(!pos){vTaskDelay(3); continue;}

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
                        // if(!strncmp(c_host, m_lastHost, pos_slash)){
                        //     AUDIO_INFO("redirect to new extension at existing host \"%s\"", c_host);
                        //     if(m_playlistFormat == FORMAT_M3U8) {
                        //         strcpy(m_lastHost, c_host);
                        //         m_f_m3u8data = true;
                        //     }
                        //     httpPrint(c_host);
                        //     while(_client->available()) _client->read(); // empty client buffer
                        //     return true;
                        // }
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

        else if(startsWith(rhl, "icy-br:")) {
            const char* c_bitRate = (rhl + 7);
            int32_t br = atoi(c_bitRate); // Found bitrate tag, read the bitrate in Kbit
            br = br * 1000;
            m_bitrate= br;
            sprintf(chbuf, "%d", br);
            if(audio_bitrate) audio_bitrate(chbuf);
        }

        else if(startsWith(rhl, "icy-metaint:")) {
            const char* c_metaint = (rhl + 12);
            int32_t i_metaint = atoi(c_metaint);
            m_metaint = i_metaint;
            if(m_metaint) m_f_swm = false     ;                            // Multimediastream
        }

        else if(startsWith(rhl, "icy-name:")) {
            char* c_icyname = (rhl + 9); // Get station name
            trim(c_icyname);
            if(strlen(c_icyname) > 0) {
                if(!m_f_Log) AUDIO_INFO("icy-name: %s", c_icyname);
                if(audio_showstation) audio_showstation(c_icyname);
            }
        }

        else if(startsWith(rhl, "content-length:")) {
            const char* c_cl = (rhl + 15);
            int32_t i_cl = atoi(c_cl);
            m_contentlength = i_cl;
            m_streamType = ST_WEBFILE; // Stream comes from a fileserver
            if(m_f_Log) AUDIO_INFO("content-length: %i", m_contentlength);
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
                if(!m_f_Log) AUDIO_INFO("chunked data transfer");
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
        m_lastHost[0] = '\0';
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
            goto exit;
        }
        return true;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::readMetadata(uint8_t b, bool first) {

    static uint16_t pos_ml = 0;                          // determines the current position in metaline
    static uint16_t metalen = 0;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(first){
        pos_ml = 0;
        metalen = 0;
        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(!metalen) {                                       // First byte of metadata?
        metalen = b * 16 + 1;                            // New count for metadata including length byte
        if(metalen >512){
            if(audio_info) audio_info("Metadata block to long! Skipping all Metadata from now on.");
            m_f_swm = true;                              // expect stream without metadata
        }
        pos_ml = 0; chbuf[pos_ml] = 0;                   // Prepare for new line
    }
    else {

        chbuf[pos_ml] = (char) b;                        // Put new char in metaline
        if(pos_ml < 510) pos_ml ++;
        chbuf[pos_ml] = 0;
        if(pos_ml == 509) log_e("metaline overflow in AUDIO_METADATA!") ;
        if(pos_ml == 510) { ; /* last current char in b */}
    }

    if(--metalen == 0) {
        if(strlen(chbuf)) {                             // Any info present?
            // metaline contains artist and song name.  For example:
            // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
            // Sometimes it is just other info like:
            // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
            // Isolate the StreamTitle, remove leading and trailing quotes if present.
            // log_i("ST %s", metaline);

            latinToUTF8(chbuf, sizeof(chbuf)); // convert to UTF-8 if necessary

            int pos = indexOf(chbuf, "song_spot", 0);    // remove some irrelevant infos
            if(pos > 3) {                                // e.g. song_spot="T" MediaBaseId="0" itunesTrackId="0"
                chbuf[pos] = 0;
            }

            if(!m_f_localfile) showstreamtitle(chbuf);   // Show artist and title if present in metadata
        }
        return true ;
    }
    return false; // end_METADATA
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::parseContentType(char* ct) {

    enum : int {CT_NONE, CT_MP3, CT_AAC, CT_M4A, CT_WAV, CT_OGG, CT_FLAC, CT_PLS, CT_M3U, CT_ASX,
                CT_M3U8, CT_TXT, CT_AACP};

    strlwr(ct);
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
    else if(!strcmp(ct, "audio/aacp")){      ct_val = CT_AAC; if(m_playlistFormat == FORMAT_M3U8) m_f_ts = true;}
    else if(!strcmp(ct, "video/mp2t")){      ct_val = CT_AAC; m_f_ts = true;} // assume AAC transport stream
    else if(!strcmp(ct, "audio/mp4"))        ct_val = CT_M4A;
    else if(!strcmp(ct, "audio/m4a"))        ct_val = CT_M4A;

    else if(!strcmp(ct, "audio/wav"))        ct_val = CT_WAV;
    else if(!strcmp(ct, "audio/x-wav"))      ct_val = CT_WAV;

    else if(!strcmp(ct, "audio/flac"))       ct_val = CT_FLAC;

    else if(!strcmp(ct, "audio/scpls"))      ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/x-scpls"))    ct_val = CT_PLS;
    else if(!strcmp(ct, "application/pls+xml")) ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/mpegurl"))    ct_val = CT_M3U;
    else if(!strcmp(ct, "audio/x-mpegurl"))  ct_val = CT_M3U;
    else if(!strcmp(ct, "audio/ms-asf"))     ct_val = CT_ASX;
    else if(!strcmp(ct, "video/x-ms-asf"))   ct_val = CT_ASX;

    else if(!strcmp(ct, "application/ogg"))  ct_val = CT_OGG;
    else if(!strcmp(ct, "application/vnd.apple.mpegurl")) ct_val = CT_M3U8;
    else if(!strcmp(ct, "application/x-mpegurl")) ct_val =CT_M3U8;

    else if(!strcmp(ct, "application/octet-stream")) ct_val = CT_TXT; // ??? listen.radionomy.com/1oldies before redirection
    else if(!strcmp(ct, "text/html"))        ct_val = CT_TXT;
    else if(!strcmp(ct, "text/plain"))       ct_val = CT_TXT;

    else if(ct_val == CT_NONE){
        AUDIO_INFO("ContentType %s not supported", ct);
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
    if(m_f_localfile){
        pos = getFilePos() - InBuff.bufferFilled();
        cardLock(true); audiofile.close(); cardLock(false);
        m_f_localfile=false;
    }
    int v=read_register(SCI_VOL);
    m_f_webstream=false;
    write_register(SCI_VOL, 0);                         // Mute while stopping

    _client->flush();                                     // Flush stream client
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
    vector_clear_and_shrink(m_playlistURL);
    vector_clear_and_shrink(m_playlistContent);
    if(config.store.play_mode!=PM_SDCARD){
      client.stop();
      client.flush(); // release memory
      clientsecure.stop();
      clientsecure.flush();
      _client = static_cast<WiFiClient*>(&client); /* default to *something* so that no NULL deref can happen */
    }
    m_f_ctseen=false;                                       // Contents type not seen yet
    m_metaint=0;                                            // No metaint yet
    m_LFcount=0;                                            // For detection end of header
    m_bitrate=0;                                            // Bitrate still unknown
    m_f_firstCall = true;                                   // InitSequence for processWebstream and processLokalFile
    m_controlCounter = 0;
    m_f_firstchunk=true;                                    // First chunk expected
    m_f_chunked=false;                                      // Assume not chunked
    m_f_ssl=false;
    m_f_swm = true;
    m_f_webfile = false;
    m_f_webstream = false;
    m_f_tts = false;                                        // text to speech
    m_f_localfile = false;
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
void Audio::getVUlevel() {
  if(!VS_PATCH_ENABLE) return;
  if(!_vuInitalized) return;
  int16_t reg = read_register(SCI_AICTRL3);
  uint8_t rl = map((uint8_t)reg, 85, 92, 0, 255);
  uint8_t rr = map((uint8_t)(reg >> 8), 85, 92, 0, 255);
  //if(rl>30 || !isRunning()) vuLeft = rl;
  //if(rr>30 || !isRunning()) vuRight = rr;
  vuLeft = rl;
  vuRight = rr;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl){
    if(timeout_ms)     m_timeout_ms     = timeout_ms;
    if(timeout_ms_ssl) m_timeout_ms_ssl = timeout_ms_ssl;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(String host){
    return connecttohost(host.c_str());
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(const char* host, const char* user, const char* pwd) {
    // user and pwd for authentification only, can be empty

     if(host == NULL) {
        AUDIO_INFO("Hostaddress is empty");
        return false;
    }

    uint16_t lenHost = strlen(host);

    if(lenHost >= 512 - 10) {
        AUDIO_INFO("Hostaddress is too long");
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

    if((pos_colon >= 0) && ((pos_ampersand == -1) or (pos_ampersand > pos_colon))){
        port = atoi(h_host + pos_colon + 1);// Get portnumber as integer
        hostwoext[pos_colon] = '\0';// Host without portnumber
    }

    AUDIO_INFO("Connect to new host: \"%s\"", l_host);
    setDefaults(); // no need to stop clients if connection is established (default is true)

    if(startsWith(l_host, "https")) m_f_ssl = true;
    else                            m_f_ssl = false;

    // authentification
    uint8_t auth = strlen(user) + strlen(pwd);
    char toEncode[auth + 4];
    toEncode[0] = '\0';
    strcat(toEncode, user);
    strcat(toEncode, ":");
    strcat(toEncode, pwd);
    char authorization[base64_encode_expected_len(strlen(toEncode)) + 1];
    authorization[0] = '\0';
    b64encode((const char*)toEncode, strlen(toEncode), authorization);

    //  AUDIO_INFO("Connect to \"%s\" on port %d, extension \"%s\"", hostwoext, port, extension);

    char rqh[strlen(h_host) + strlen(authorization) + 200]; // http request header
    rqh[0] = '\0';

    strcat(rqh, "GET ");
    strcat(rqh, extension);
    strcat(rqh, " HTTP/1.1\r\n");
    strcat(rqh, "Host: ");
    strcat(rqh, hostwoext);
    strcat(rqh, "\r\n");
    strcat(rqh, "Icy-MetaData:1\r\n");
    strcat(rqh, "Authorization: Basic ");
    strcat(rqh, authorization);
    strcat(rqh, "\r\n");
    strcat(rqh, "Accept-Encoding: identity;q=1,*;q=0\r\n");
    strcat(rqh, "User-Agent: Mozilla/5.0\r\n");
    strcat(rqh, "Connection: keep-alive\r\n\r\n");

    if(ESP_ARDUINO_VERSION_MAJOR == 2 && ESP_ARDUINO_VERSION_MINOR == 0 && ESP_ARDUINO_VERSION_PATCH >= 3){
        m_timeout_ms_ssl = UINT16_MAX;  // bug in v2.0.3 if hostwoext is a IPaddr not a name
        m_timeout_ms = UINT16_MAX;  // [WiFiClient.cpp:253] connect(): select returned due to timeout 250 ms for fd 48
    }
    bool res = true; // no need to reconnect if connection exists

    if(m_f_ssl){ _client = static_cast<WiFiClient*>(&clientsecure); if(port == 80) port = 443;}
    else       { _client = static_cast<WiFiClient*>(&client);}

    uint32_t t = millis();
    if(m_f_Log) AUDIO_INFO("connect to %s on port %d path %s", hostwoext, port, extension);
    res = _client->connect(hostwoext, port, m_f_ssl ? m_timeout_ms_ssl : m_timeout_ms);

    if(res){
        uint32_t dt = millis() - t;
        strcpy(m_lastHost, l_host);
        AUDIO_INFO("%s has been established in %u ms, free Heap: %u bytes",
                    m_f_ssl?"SSL":"Connection", dt, ESP.getFreeHeap());
        m_f_running = true;
    }

    m_expectedCodec = CODEC_NONE;
    m_expectedPlsFmt = FORMAT_NONE;

    if(res){
        _client->print(rqh);
        if(endsWith(extension, ".mp3"))   m_expectedCodec = CODEC_MP3;
        if(endsWith(extension, ".aac"))   m_expectedCodec = CODEC_AAC;
        if(endsWith(extension, ".wav"))   m_expectedCodec = CODEC_WAV;
        if(endsWith(extension, ".m4a"))   m_expectedCodec = CODEC_M4A;
        if(endsWith(extension, ".flac"))  m_expectedCodec = CODEC_FLAC;
        if(endsWith(extension, ".asx"))  m_expectedPlsFmt = FORMAT_ASX;
        if(endsWith(extension, ".m3u"))  m_expectedPlsFmt = FORMAT_M3U;
        if(endsWith(extension, ".m3u8")) m_expectedPlsFmt = FORMAT_M3U8;
        if(endsWith(extension, ".pls"))  m_expectedPlsFmt = FORMAT_PLS;

        setDatamode(HTTP_RESPONSE_HEADER);   // Handle header
        m_streamType = ST_WEBSTREAM;
        m_f_webstream = true;
    }
    else{
        AUDIO_INFO("Request %s failed!", l_host);
        if(audio_showstation) audio_showstation("");
        if(audio_showstreamtitle) audio_showstreamtitle("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
        m_lastHost[0] = 0;
    }
    if(hostwoext) {free(hostwoext); hostwoext = NULL;}
    if(extension) {free(extension); extension = NULL;}
    if(l_host   ) {free(l_host);    l_host    = NULL;}
    if(h_host   ) {free(h_host);    h_host    = NULL;}
    return res;
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

    setDefaults(); // free buffers an set defaults

    memcpy(audioName, path, strlen(path)+1);
    if(audioName[0] != '/'){
        for(int i = 255; i > 0; i--){
            audioName[i] = audioName[i-1];
        }
        audioName[0] = '/';
    }
    if(endsWith(audioName, "\n")) audioName[strlen(audioName) -1] = 0;

    sprintf(chbuf, "Reading file: \"%s\"", audioName);
    if(audio_info) {vTaskDelay(2); audio_info(chbuf);}
    if(audio_beginSDread) audio_beginSDread();
    cardLock(true); 
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
        cardLock(false);
        return false;
    }

    m_f_localfile = true;
    m_file_size = audiofile.size();//TEST loop
    
    char* afn = strdup(audiofile.name());                   // audioFileName
    cardLock(false);
    uint8_t dotPos = lastIndexOf(afn, ".");
    for(uint8_t i = dotPos + 1; i < strlen(afn); i++){
        afn[i] = toLowerCase(afn[i]);
    }

    if(endsWith(afn, ".mp3")){      // MP3 section
        m_codec = CODEC_MP3;
        m_f_running = true;
        return true;
    } // end MP3 section

    if(endsWith(afn, ".m4a")){      // M4A section, iTunes
        m_codec = CODEC_M4A;
        m_f_running = true;
        return true;
    } // end M4A section

    if(endsWith(afn, ".aac")){      // AAC section, without FileHeader
        m_codec = CODEC_AAC;
        m_f_running = true;
        return true;
    } // end AAC section

    if(endsWith(afn, ".wav")){      // WAVE section
        m_codec = CODEC_WAV;
        m_f_running = true;
        return true;
    } // end WAVE section

    if(endsWith(afn, ".flac")) {     // FLAC section
        m_codec = CODEC_FLAC;
        m_f_running = true;
        return true;
    } // end FLAC section

    if(endsWith(afn, ".ogg")) {     // FLAC section
        m_codec = CODEC_OGG;
        m_f_running = true;
        return true;
    } // end FLAC section

    sprintf(chbuf, "The %s format is not supported", afn + dotPos);
    if(audio_info) audio_info(chbuf);
    cardLock(true); audiofile.close(); cardLock(false);
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
    sprintf(chbuf, "SSL has been established, free Heap: %u bytes", ESP.getFreeHeap());
    if(audio_info) audio_info(chbuf);

    m_f_webstream = true;
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
    buff[m] = 0;
    memcpy(buff, tmpbuff, m);
    free(tmpbuff);
}
//---------------------------------------------------------------------------------------------------------------------
int Audio::read_MP3_Header(uint8_t *data, size_t len) {

    static size_t headerSize;
    static size_t id3Size;
    static uint8_t ID3version;
    static int ehsz = 0;
    static char frameid[5];
    static size_t framesize = 0;
    static bool compressed = false;
    static bool APIC_seen = false;
    static size_t APIC_size = 0;
    static uint32_t APIC_pos = 0;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 0){      /* read ID3 tag and ID3 header size */
        if(m_f_localfile){
            m_contentlength = getFileSize();
            ID3version = 0;
            sprintf(chbuf, "Content-Length: %u", m_contentlength);
            if(audio_info) audio_info(chbuf);
        }
        m_controlCounter ++;
        APIC_seen = false;
        headerSize = 0;
        ehsz = 0;
        if(specialIndexOf(data, "ID3", 4) != 0) { // ID3 not found
            if(audio_info) audio_info("file has no mp3 tag, skip metadata");
            m_audioDataSize = m_contentlength;
            sprintf(chbuf, "Audio-Length: %u", m_audioDataSize);
            if(audio_progress) audio_progress(295903, m_audioDataSize);
            if(audio_info) audio_info(chbuf);
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
        sprintf(chbuf, "ID3 framesSize: %i", id3Size);
        if(audio_info) audio_info(chbuf);

        sprintf(chbuf, "ID3 version: 2.%i", ID3version);
        if(audio_info) audio_info(chbuf);

        if(ID3version == 2){
            m_controlCounter = 10;
        }
        headerSize = id3Size;
        headerSize -= 10;

        return 10;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 1){      // compute extended header size if exists
        m_controlCounter ++;
        if(m_f_exthdr) {
            if(audio_info) audio_info("ID3 extended header");
            ehsz =  bigEndian(data, 4);
            headerSize -= 4;
            ehsz -= 4;
            return 4;
        }
        else{
            if(audio_info) audio_info("ID3 normal frames");
            return 0;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 2){      // skip extended header if exists
        if(ehsz > 256) {
            ehsz -=256;
            headerSize -= 256;
            return 256;} // Throw it away
        else           {
            m_controlCounter ++;
            headerSize -= ehsz;
            return ehsz;} // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 3){      // read a ID3 frame, get the tag
        if(headerSize == 0){
            m_controlCounter = 99;
            return 0;
        }
        m_controlCounter ++;
        frameid[0] = *(data + 0);
        frameid[1] = *(data + 1);
        frameid[2] = *(data + 2);
        frameid[3] = *(data + 3);
        frameid[4] = 0;
        headerSize -= 4;
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
        headerSize -= 4;
        uint8_t flag = *(data + 4); // skip 1st flag
        (void) flag;
        headerSize--;
        compressed = (*(data + 5)) & 0x80; // Frame is compressed using [#ZLIB zlib] with 4 bytes for 'decompressed
                                           // size' appended to the frame header.
        headerSize--;
        uint32_t decompsize = 0;
        if(compressed){
            log_d("iscompressed");
            decompsize = bigEndian(data + 6, 4);
            headerSize -= 4;
            (void) decompsize;
            log_d("decompsize=%u", decompsize);
            return 6 + 4;
        }
        return 6;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 5){      // If the frame is larger than 256 bytes, skip the rest
        if(framesize > 256){
            framesize -= 256;
            headerSize -= 256;
            return 256;
        }
        else {
            m_controlCounter = 3; // check next frame
            headerSize -= framesize;
            return framesize;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 6){      // Read the value
        m_controlCounter = 5;       // only read 256 bytes
        char value[256];
        char ch = *(data + 0);
        bool isUnicode = (ch==1) ? true : false;

        if(startsWith(frameid, "APIC")) { // a image embedded in file, passing it to external function
            // log_d("framesize=%i", framesize);
            isUnicode = false;
            if(m_f_localfile){
                APIC_seen = true;
                APIC_pos = id3Size - headerSize;
                APIC_size = framesize;
            }
            return 0;
        }

        size_t fs = framesize;
        if(fs >255) fs = 255;
        for(int i=0; i<fs; i++){
            value[i] = *(data + i);
        }
        framesize -= fs;
        headerSize -= fs;
        value[fs] = 0;
        if(isUnicode && fs > 1) {
            unicode2utf8(value, fs);   // convert unicode to utf-8 U+0020...U+07FF
        }
        if(!isUnicode){
            uint16_t j = 0, k = 0;
            j = 0;
            k = 0;
            while(j < fs) {
                if(value[j] == 0x0A) value[j] = 0x20; // replace LF by space
                if(value[j] > 0x1F) {
                    value[k] = value[j];
                    k++;
                }
                j++;
            } //remove non printables
            if(k>0) value[k] = 0; else value[0] = 0; // new termination
        }
        showID3Tag(frameid, value);
        return fs;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // -- section V2.2 only , greater Vers above ----
    if(m_controlCounter == 10){ // frames in V2.2, 3bytes identifier, 3bytes size descriptor
        frameid[0] = *(data + 0);
        frameid[1] = *(data + 1);
        frameid[2] = *(data + 2);
        frameid[3] = 0;
        headerSize -= 3;
        size_t len = bigEndian(data + 3, 3);
        headerSize -= 3;
        headerSize -= len;
        char value[256];
        size_t tmp = len;
        if(tmp > 254) tmp = 254;
        memcpy(value, (data + 7), tmp);
        value[tmp+1] = 0;
        chbuf[0] = 0;

        showID3Tag(frameid, value);
        if(len == 0) m_controlCounter = 98;

        return 3 + 3 + len;
    }
    // -- end section V2.2 -----------

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 98){ // skip all ID3 metadata (mostly spaces)
        if(headerSize > 256) {
            headerSize -=256;
            return 256;
        } // Throw it away
        else           {
            m_controlCounter = 99;
            return headerSize;
        } // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 99){ //  exist another ID3tag?
        m_audioDataStart += id3Size;
        vTaskDelay(30);
        if((*(data + 0) == 'I') && (*(data + 1) == 'D') && (*(data + 2) == '3')) {
            m_controlCounter = 0;
            return 0;
        }
        else {
            m_controlCounter = 100; // ok
            eofHeader = true;
            m_audioDataSize = m_contentlength - m_audioDataStart;
            if(audio_progress) audio_progress(m_audioDataStart, m_audioDataSize);
            sprintf(chbuf, "Audio-Length: %u", m_audioDataSize);
            if(audio_info) audio_info(chbuf);
            if(APIC_seen && audio_id3image) audio_id3image(audiofile, APIC_pos, APIC_size);
            return 0;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::showID3Tag(const char* tag, const char* value){

    chbuf[0] = 0;
    // V2.2
    if(!strcmp(tag, "CNT")) sprintf(chbuf, "Play counter: %s", value);
    // if(!strcmp(tag, "COM")) sprintf(chbuf, "Comments: %s", value);
    if(!strcmp(tag, "CRA")) sprintf(chbuf, "Audio encryption: %s", value);
    if(!strcmp(tag, "CRM")) sprintf(chbuf, "Encrypted meta frame: %s", value);
    if(!strcmp(tag, "ETC")) sprintf(chbuf, "Event timing codes: %s", value);
    if(!strcmp(tag, "EQU")) sprintf(chbuf, "Equalization: %s", value);
    if(!strcmp(tag, "IPL")) sprintf(chbuf, "Involved people list: %s", value);
    if(!strcmp(tag, "PIC")) sprintf(chbuf, "Attached picture: %s", value);
    if(!strcmp(tag, "SLT")) sprintf(chbuf, "Synchronized lyric/text: %s", value);
    // if(!strcmp(tag, "TAL")) sprintf(chbuf, "Album/Movie/Show title: %s", value);
    if(!strcmp(tag, "TBP")) sprintf(chbuf, "BPM (Beats Per Minute): %s", value);
    if(!strcmp(tag, "TCM")) sprintf(chbuf, "Composer: %s", value);
    if(!strcmp(tag, "TCO")) sprintf(chbuf, "Content type: %s", value);
    if(!strcmp(tag, "TCR")) sprintf(chbuf, "Copyright message: %s", value);
    if(!strcmp(tag, "TDA")) sprintf(chbuf, "Date: %s", value);
    if(!strcmp(tag, "TDY")) sprintf(chbuf, "Playlist delay: %s", value);
    if(!strcmp(tag, "TEN")) sprintf(chbuf, "Encoded by: %s", value);
    if(!strcmp(tag, "TFT")) sprintf(chbuf, "File type: %s", value);
    if(!strcmp(tag, "TIM")) sprintf(chbuf, "Time: %s", value);
    if(!strcmp(tag, "TKE")) sprintf(chbuf, "Initial key: %s", value);
    if(!strcmp(tag, "TLA")) sprintf(chbuf, "Language(s): %s", value);
    if(!strcmp(tag, "TLE")) sprintf(chbuf, "Length: %s", value);
    if(!strcmp(tag, "TMT")) sprintf(chbuf, "Media type: %s", value);
    if(!strcmp(tag, "TOA")) sprintf(chbuf, "Original artist(s)/performer(s): %s", value);
    if(!strcmp(tag, "TOF")) sprintf(chbuf, "Original filename: %s", value);
    if(!strcmp(tag, "TOL")) sprintf(chbuf, "Original Lyricist(s)/text writer(s): %s", value);
    if(!strcmp(tag, "TOR")) sprintf(chbuf, "Original release year: %s", value);
    if(!strcmp(tag, "TOT")) sprintf(chbuf, "Original album/Movie/Show title: %s", value);
    if(!strcmp(tag, "TP1")) sprintf(chbuf, "Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group: %s", value);
    if(!strcmp(tag, "TP2")) sprintf(chbuf, "Band/Orchestra/Accompaniment: %s", value);
    if(!strcmp(tag, "TP3")) sprintf(chbuf, "Conductor/Performer refinement: %s", value);
    if(!strcmp(tag, "TP4")) sprintf(chbuf, "Interpreted, remixed, or otherwise modified by: %s", value);
    if(!strcmp(tag, "TPA")) sprintf(chbuf, "Part of a set: %s", value);
    if(!strcmp(tag, "TPB")) sprintf(chbuf, "Publisher: %s", value);
    if(!strcmp(tag, "TRC")) sprintf(chbuf, "ISRC (International Standard Recording Code): %s", value);
    if(!strcmp(tag, "TRD")) sprintf(chbuf, "Recording dates: %s", value);
    if(!strcmp(tag, "TRK")) sprintf(chbuf, "Track number/Position in set: %s", value);
    if(!strcmp(tag, "TSI")) sprintf(chbuf, "Size: %s", value);
    if(!strcmp(tag, "TSS")) sprintf(chbuf, "Software/hardware and settings used for encoding: %s", value);
    if(!strcmp(tag, "TT1")) sprintf(chbuf, "Content group description: %s", value);
    if(!strcmp(tag, "TT2")) sprintf(chbuf, "Title/Songname/Content description: %s", value);
    if(!strcmp(tag, "TT3")) sprintf(chbuf, "Subtitle/Description refinement: %s", value);
    if(!strcmp(tag, "TXT")) sprintf(chbuf, "Lyricist/text writer: %s", value);
    if(!strcmp(tag, "TXX")) sprintf(chbuf, "User defined text information frame: %s", value);
    if(!strcmp(tag, "TYE")) sprintf(chbuf, "Year: %s", value);
    if(!strcmp(tag, "UFI")) sprintf(chbuf, "Unique file identifier: %s", value);
    if(!strcmp(tag, "ULT")) sprintf(chbuf, "Unsychronized lyric/text transcription: %s", value);
    if(!strcmp(tag, "WAF")) sprintf(chbuf, "Official audio file webpage: %s", value);
    if(!strcmp(tag, "WAR")) sprintf(chbuf, "Official artist/performer webpage: %s", value);
    if(!strcmp(tag, "WAS")) sprintf(chbuf, "Official audio source webpage: %s", value);
    if(!strcmp(tag, "WCM")) sprintf(chbuf, "Commercial information: %s", value);
    if(!strcmp(tag, "WCP")) sprintf(chbuf, "Copyright/Legal information: %s", value);
    if(!strcmp(tag, "WPB")) sprintf(chbuf, "Publishers official webpage: %s", value);
    if(!strcmp(tag, "WXX")) sprintf(chbuf, "User defined URL link frame: %s", value);

    // V2.3 V2.4 tags
    // if(!strcmp(tag, "COMM")) sprintf(chbuf, "Comment: %s", value);
    if(!strcmp(tag, "OWNE")) sprintf(chbuf, "Ownership: %s", value);
    // if(!strcmp(tag, "PRIV")) sprintf(chbuf, "Private: %s", value);
    if(!strcmp(tag, "SYLT")) sprintf(chbuf, "SynLyrics: %s", value);
    if(!strcmp(tag, "TALB")) { sprintf(chbuf, "Album: %s", value); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TBPM")) sprintf(chbuf, "BeatsPerMinute: %s", value);
    if(!strcmp(tag, "TCMP")) sprintf(chbuf, "Compilation: %s", value);
    if(!strcmp(tag, "TCOM")) sprintf(chbuf, "Composer: %s", value);
    if(!strcmp(tag, "TCON")) sprintf(chbuf, "ContentType: %s", value);
    if(!strcmp(tag, "TCOP")) sprintf(chbuf, "Copyright: %s", value);
    if(!strcmp(tag, "TDAT")) sprintf(chbuf, "Date: %s", value);
    if(!strcmp(tag, "TEXT")) sprintf(chbuf, "Lyricist: %s", value);
    if(!strcmp(tag, "TIME")) sprintf(chbuf, "Time: %s", value);
    if(!strcmp(tag, "TIT1")) sprintf(chbuf, "Grouping: %s", value);
    if(!strcmp(tag, "TIT2")) { sprintf(chbuf, "Title: %s", value); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TIT3")) sprintf(chbuf, "Subtitle: %s", value);
    if(!strcmp(tag, "TLAN")) sprintf(chbuf, "Language: %s", value);
    if(!strcmp(tag, "TLEN")) sprintf(chbuf, "Length (ms): %s", value);
    if(!strcmp(tag, "TMED")) sprintf(chbuf, "Media: %s", value);
    if(!strcmp(tag, "TOAL")) sprintf(chbuf, "OriginalAlbum: %s", value);
    if(!strcmp(tag, "TOPE")) sprintf(chbuf, "OriginalArtist: %s", value);
    if(!strcmp(tag, "TORY")) sprintf(chbuf, "OriginalReleaseYear: %s", value);
    if(!strcmp(tag, "TPE1")) { sprintf(chbuf, "Artist: %s", value); if(audio_id3artist) audio_id3artist(value); }
    if(!strcmp(tag, "TPE2")) sprintf(chbuf, "Band: %s", value);
    if(!strcmp(tag, "TPE3")) sprintf(chbuf, "Conductor: %s", value);
    if(!strcmp(tag, "TPE4")) sprintf(chbuf, "InterpretedBy: %s", value);
    if(!strcmp(tag, "TPOS")) sprintf(chbuf, "PartOfSet: %s", value);
    if(!strcmp(tag, "TPUB")) sprintf(chbuf, "Publisher: %s", value);
    if(!strcmp(tag, "TRCK")) sprintf(chbuf, "Track: %s", value);
    if(!strcmp(tag, "TSSE")) sprintf(chbuf, "SettingsForEncoding: %s", value);
    if(!strcmp(tag, "TRDA")) sprintf(chbuf, "RecordingDates: %s", value);
    if(!strcmp(tag, "TXXX")) sprintf(chbuf, "UserDefinedText: %s", value);
    if(!strcmp(tag, "TYER")) sprintf(chbuf, "Year: %s", value);
    if(!strcmp(tag, "USER")) sprintf(chbuf, "TermsOfUse: %s", value);
    if(!strcmp(tag, "USLT")) sprintf(chbuf, "Lyrics: %s", value);
    if(!strcmp(tag, "WOAR")) sprintf(chbuf, "OfficialArtistWebpage: %s", value);
    if(!strcmp(tag, "XDOR")) sprintf(chbuf, "OriginalReleaseTime: %s", value);

    latinToUTF8(chbuf, sizeof(chbuf));
    if(chbuf[0] != 0) if(audio_id3data) audio_id3data(chbuf);
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getFileSize(){
    if (!audiofile) return 0;
    cardLock(true);
    uint32_t s = audiofile.size();
    cardLock(false);
    return s;
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getFilePos(){
    if (!audiofile) return 0;
    cardLock(true);
    uint32_t p = audiofile.position();
    cardLock(false);
    return p;
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::setFilePos(uint32_t pos){
    if (!audiofile) return false;
    cardLock(true);
    bool s = audiofile.seek(pos);
    cardLock(false);
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
      sprintf(brbuf, "%d", m_avr_bitrate);
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
    sprintf(brbuf, "%d", m_avr_bitrate);
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
#endif  //  if I2S_DOUT==255
