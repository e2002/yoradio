/*
 *  vs1053_ext.cpp
 *
 *  Created on: Jul 09.2017
 *  Updated on: Feb 11 2022
 *      Author: Wolle
 */
#include "../../options.h"
#if I2S_DOUT==255

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
Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin,  uint8_t spi, uint8_t mosi, uint8_t miso, uint8_t sclk) :
        cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin)
{
    // default is VSPI (VSPI_MISO 19, VSPI_MOSI 23, VSPI_SCLK 18)
    //            HSPI (HSPI_MISO 12, HSPI_MOSI 13, HSPI_SCLK 14)
    
    if(spi == VSPI){
        spi_VS1053 = &SPI;
    }
    else if(spi == HSPI){
        spi_VS1053 = new SPIClass(HSPI);
        spi_VS1053->begin(sclk, miso, mosi, -1);
    }
    else{
        log_e("unknown SPI authority");
    }

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
    CS_HIGH();                                  // End control mode
    spi_VS1053->endTransaction();                       // Allow other SPI users
}
void Audio::control_mode_on()
{
    spi_VS1053->beginTransaction(VS1053_SPI);           // Prevent other SPI users
    DCS_HIGH();                                 // Bring slave in control mode
    CS_LOW();
}
void Audio::data_mode_on()
{
    spi_VS1053->beginTransaction(VS1053_SPI);           // Prevent other SPI users
    CS_HIGH();                                  // Bring slave in data mode
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
    DCS_HIGH();
    CS_HIGH();
    delay(100);

    // Init SPI in slow mode (0.2 MHz)
    VS1053_SPI = SPISettings(200000, MSBFIRST, SPI_MODE0);
//    printDetails("Right after reset/startup");
    delay(20);
    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write(0xC017, 3);                                  // GPIO DDR=3
    wram_write(0xC019, 0);                                  // GPIO ODATA=0
    delay(100);
//    printDetails("After test loop");
    softReset();                                            // Do a soft reset
    // Switch on the analog parts
    write_register(SCI_AUDATA, 44100 + 1);                  // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register(SCI_CLOCKF, 6 << 12);                    // Normal clock settings multiplyer 3.0=12.2 MHz
    //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
    VS1053_SPI=SPISettings(6700000, MSBFIRST, SPI_MODE0); // SPIDIV 12 -> 80/12=6.66 MHz
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_LINE1));
    //testComm("Fast SPI, Testing VS1053 read/write registers again... \n");
    delay(10);
    await_data_request();
    m_endFillByte=wram_read(0x1E06) & 0xFF;
//    sprintf(chbuf, "endFillByte is %X", endFillByte);
//    if(audio_info) audio_info(chbuf);
//    printDetails("After last clocksetting \n");
    loadUserCode(); // load in VS1053B if you want to play flac
    delay(100);
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
    // Input value is 0..254.  254 is the loudest.
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

		//uint8_t lgvolL = log10(((float)valueL+1)) * 64.54571334 + 96;
		//uint8_t lgvolR = log10(((float)valueR+1)) * 64.54571334 + 96;
		uint8_t lgvolL = VS1053VOL(valueL);
		uint8_t lgvolR = VS1053VOL(valueR);
		if(lgvolL==VS1053VOLM) lgvolL=0;
		if(lgvolR==VS1053VOLM) lgvolR=0;
		valueL=map(lgvolL, 0, 254, 0xF8, 0x00);
		valueR=map(lgvolR, 0, 254, 0xF8, 0x00);
		value=(valueL << 8) | valueR;
		write_register(SCI_VOL, value);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setTone(uint8_t *rtone){                       // Set bass/treble (4 nibbles)

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
	uint8_t rtone[] = {map(gainHighPass, -16, 16, -8, 7), 2+gainBandPass, gainLowPass, 15-gainBandPass};
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

    sdi_send_fillers(2052);
    delay(10);
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_CANCEL));
    for(i=0; i < 200; i++)
            {
        sdi_send_fillers(32);
        modereg=read_register(SCI_MODE);  // Read status
        if((modereg & _BV(SM_CANCEL)) == 0)
                {
            sdi_send_fillers(2052);
            sprintf(chbuf, "Song stopped correctly after %d msec", i * 10);
            m_f_running = false;
            if(audio_info) audio_info(chbuf);
            return;
        }
        delay(10);
    }
    if(audio_info) audio_info("Song stopped incorrectly!");
    printDetails("after sond stopped incorrectly");
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::softReset()
{
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_RESET));
    delay(10);
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
const char* Audio::printVersion(){
    boolean flag=true;
    uint16_t reg1 = 0, reg2 = 0, reg3 = 0;
    reg1 = wram_read(0x1E00);
    reg2 = wram_read(0x1E01);
    reg3 = wram_read(0x1E02) & 0xFF;
    if((reg1 ==0xFFFF)&&(reg2 == 0xFFFF)) {flag = false; log_e("all pins high?, VS1053 seems not connected");}
    if((reg1 ==0x0000)&&(reg2 == 0x0000)) {flag = false; log_e("all pins low?, VS1053 not proper connected (no SCK?)");}
    if( reg3 == 0xFF)                     {flag = false; log_e("VS1053 version too high");} 
    sprintf(chbuf, "chipID = %d%d, version = %d", reg1, reg2, reg3);
    if(audio_info) audio_info(chbuf);
    if(flag) return chbuf;
    return nullptr;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::showstreamtitle(const char* ml) {
    // example for ml:
    // StreamTitle='Oliver Frank - Mega Hitmix';StreamUrl='www.radio-welle-woerthersee.at';
    // or adw_ad='true';durationMilliseconds='10135';adId='34254';insertionType='preroll';

    int16_t idx1, idx2;
    uint16_t i = 0, hash = 0;
    static uint16_t sTit_remember = 0, sUrl_renember = 0;

    idx1 = indexOf(ml, "StreamTitle=", 0);
    if(idx1 >= 0){                                                              // Streamtitle found
        idx2 = indexOf(ml, ";", idx1);
        char *sTit;
        if(idx2 >= 0){sTit = strndup(ml + idx1, idx2 + 1); sTit[idx2] = '\0';}
        else          sTit =  strdup(ml);

        while(i < strlen(sTit)){hash += sTit[i] * i+1; i++;}

        if(sTit_remember != hash){
            sTit_remember = hash;
            if(audio_info) audio_info(sTit);
            uint8_t pos = 12;                                                   // remove "StreamTitle="
            if(sTit[pos] == '\'') pos++;                                        // remove leading  \'
            if(sTit[strlen(sTit) - 1] == '\'') sTit[strlen(sTit) -1] = '\0';    // remove trailing \'
            if(audio_showstreamtitle) audio_showstreamtitle(sTit + pos);
        }
        free(sTit);
    }

    idx1 = indexOf(ml, "StreamUrl=", 0);
    idx2 = indexOf(ml, ";", idx1);
    if(idx1 >= 0 && idx2 > idx1){                                               // StreamURL found
        uint16_t len = idx2 - idx1;
        char *sUrl;
        sUrl = strndup(ml + idx1, len + 1); sUrl[len] = '\0';

        while(i < strlen(sUrl)){hash += sUrl[i] * i+1; i++;}
        if(sUrl_renember != hash){
            sUrl_renember = hash;
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
    // - localfile - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_localfile) {                                      // Playing file fron SPIFFS or SD?
        processLocalFile();
    }
    // - webstream - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_webstream) {                                      // Playing file from URL?
        //if(!m_f_running) return;
        if(m_datamode == VS1053_PLAYLISTINIT || m_datamode == VS1053_PLAYLISTHEADER || m_datamode == VS1053_PLAYLISTDATA){
            processPlayListData();
            return;
        }
        if(m_datamode == VS1053_HEADER){
            processAudioHeaderData();
            return;
        }
        if(m_datamode == VS1053_DATA){
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

//    if(psramFound() && bytesAddedToBuffer >4096)
//        vTaskDelay(2);// PSRAM has a bottleneck in the queue, so wait a little bit

    if(bytesAddedToBuffer == -1) bytesAddedToBuffer = 0; // read error? eof?
    bytesCanBeRead = InBuff.bufferFilled();
    if(bytesCanBeRead > InBuff.getMaxBlockSize()) bytesCanBeRead = InBuff.getMaxBlockSize();
    if(bytesCanBeRead == InBuff.getMaxBlockSize()) { // mp3 or aac frame complete?
        if(!f_stream) {
            f_stream = true;
            if(audio_info) audio_info("stream ready");
        }
        if(m_controlCounter != 100){
            if(m_codec == CODEC_WAV){
            //     int res = read_WAV_Header(InBuff.getReadPtr(), bytesCanBeRead);
                m_controlCounter = 100;
            }
            if(m_codec == CODEC_MP3){
                int res = read_MP3_Header(InBuff.getReadPtr(), bytesCanBeRead);
                if(res >= 0) bytesDecoded = res;
                else{ // error, skip header
                    m_controlCounter = 100;
                }
            }
            if(m_codec == CODEC_M4A){
            //     int res = read_M4A_Header(InBuff.getReadPtr(), bytesCanBeRead);
            //     if(res >= 0) bytesDecoded = res;
            //     else{ // error, skip header
                    m_controlCounter = 100;
            //     }
            }
            if(m_codec == CODEC_AAC){
                // stream only, no header
                m_audioDataSize = getFileSize();
                m_controlCounter = 100;
            }

            if(m_codec == CODEC_FLAC){
            //     int res = read_FLAC_Header(InBuff.getReadPtr(), bytesCanBeRead);
            //     if(res >= 0) bytesDecoded = res;
            //     else{ // error, skip header
            //         stopSong();
                    m_controlCounter = 100;
            //     }
            }

            if(m_codec == CODEC_OGG){
                m_controlCounter = 100;
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

        char *afn =strdup(audiofile.name()); // store temporary the name


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

    if(m_f_ssl == false) availableBytes = client.available();            // available from stream
    if(m_f_ssl == true)  availableBytes = clientsecure.available();      // available from stream

    // if we have chunked data transfer: get the chunksize- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked && !m_chunkcount && availableBytes) { // Expecting a new chunkcount?
        int b;
        if(!m_f_ssl) b = client.read();
        else         b = clientsecure.read();

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
        int16_t b = 0;
        if(!m_f_ssl) b = client.read();
        else         b = clientsecure.read();
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

        if(m_f_ssl == false) bytesAddedToBuffer = client.read(InBuff.getWritePtr(), bytesCanBeWritten);
        else                 bytesAddedToBuffer = clientsecure.read(InBuff.getWritePtr(), bytesCanBeWritten);

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
        }
        if(m_codec == CODEC_MP3){
            int res = read_MP3_Header(InBuff.getReadPtr(), InBuff.bufferFilled());
            if(res >= 0) bytesDecoded = res;
            else{m_controlCounter = 100;} // error, skip header
        }
        if(m_codec == CODEC_M4A){
            m_controlCounter = 100;
        }
        if(m_codec == CODEC_FLAC){
            m_controlCounter = 100;
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
void Audio::processPlayListData() {

    static bool f_entry = false;                            // entryflag for asx playlist
    static bool f_title = false;                            // titleflag for asx playlist
    static bool f_ref   = false;                            // refflag   for asx playlist
    static bool f_begin = false;
    static bool f_end   = false;

    (void)f_title;  // is unused yet

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_datamode == VS1053_PLAYLISTINIT) {                  // Initialize for receive .m3u file
        // We are going to use metadata to read the lines from the .m3u file
        // Sometimes this will only contain a single line
        f_entry = false;
        f_title = false;
        f_ref   = false;
        f_begin = false;
        f_end   = false;
        m_datamode = VS1053_PLAYLISTHEADER;                  // Handle playlist data
        if(audio_info) audio_info("Read from playlist");
    } // end AUDIO_PLAYLISTINIT

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int av = 0;
    if(!m_f_ssl) av = client.available();
    else         av = clientsecure.available();
    if(av < 1){
        if(f_end) return;
        if(f_begin) {f_end = true;}
        else return;
    } 

    char pl[256]; // playlistline
    uint8_t b = 0;
    int16_t pos = 0;



    while(true){
        if(!m_f_ssl)  b = client.read();
        else          b = clientsecure.read();
        if(b == 0xff) b = '\n'; // no more to read? send new line
        if(b == '\n') {pl[pos] = 0; break;}
        if(b < 0x20 || b > 0x7E) continue;
        pl[pos] = b;
        pos++;
        if(pos == 255){pl[pos] = '\0'; log_e("playlistline oberflow"); break;}
    }

    // log_i("pl=%s", pl);


    if(strlen(pl) == 0 && m_datamode == VS1053_PLAYLISTHEADER) {
        if(audio_info) audio_info("Switch to PLAYLISTDATA");
        m_datamode = VS1053_PLAYLISTDATA;                    // Expecting data now
        return;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_datamode == VS1053_PLAYLISTHEADER) {                // Read header

        sprintf(chbuf, "Playlistheader: %s", pl);           // Show playlistheader
        if(audio_info) audio_info(chbuf);

            if(indexOf(pl, "Connection:close", 0) >= 0){        // not a playlist
            m_datamode = VS1053_HEADER;
        }

        int pos = indexOf(pl, "404 Not Found", 0);
        if(pos >= 0) {
            m_datamode = VS1053_NONE;
            if(audio_info) audio_info("Error 404 Not Found");
            stopSong();
            return;
        }

        pos = indexOf(pl, "404 File Not Found", 0);
        if(pos >= 0) {
            m_datamode = VS1053_NONE;
            if(audio_info) audio_info("Error 404 File Not Found");
            stopSong();
            return;
        }

        pos = indexOf(pl, ":", 0);                          // lowercase all letters up to the colon
        if(pos >= 0) {
            for(int i=0; i<pos; i++) {
                pl[i] = toLowerCase(pl[i]);
            }
        }
        if(startsWith(pl, "location:")) {
            const char* host;
            pos = indexOf(pl, "http", 0);
            host = (pl + pos);
            sprintf(chbuf, "redirect to new host %s", host);
            if(audio_info) audio_info(chbuf);
            connecttohost(host);
        }
        return;
    } // end AUDIO_PLAYLISTHEADER

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_datamode == VS1053_PLAYLISTDATA) {                  // Read next byte of .m3u file data
        sprintf(chbuf, "Playlistdata: %s", pl);             // Show playlistdata
        if(audio_info) audio_info(chbuf);
        if(!f_begin) f_begin = true;                        // first playlistdata received

        pos = indexOf(pl, "<!DOCTYPE", 0);                  // webpage found
        if(pos >= 0) {
            m_datamode = VS1053_NONE;
            if(audio_info) audio_info("Not Found");
            stopSong();
            return;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_M3U) {

            if(indexOf(pl, "#EXTINF:", 0) >= 0) {           // Info?
               pos = indexOf(pl, ",", 0);                   // Comma in this line?
               if(pos > 0) {
                   // Show artist and title if present in metadata
                   if(audio_info) audio_info(pl + pos + 1);
               }
               return;
           }
           if(startsWith(pl, "#")) {                        // Commentline?
               return;
           }

           pos = indexOf(pl, "http://:@", 0); // ":@"??  remove that!
           if(pos >= 0) {
               sprintf(chbuf, "Entry in playlist found: %s", (pl + pos + 9));
               connecttohost(pl + pos + 9);
               return;
           }
           //sprintf(chbuf, "Entry in playlist found: %s", pl);
           //if(audio_info) audio_info(chbuf);
           pos = indexOf(pl, "http", 0);                    // Search for "http"
           const char* host;
           if(pos >= 0) {                                   // Does URL contain "http://"?
               host = (pl + pos);
               connecttohost(host);
           }                                                // Yes, set new host
           return;
        } //m3u

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_PLS) {
            if(startsWith(pl, "File1")) {
                pos = indexOf(pl, "http", 0);                   // File1=http://streamplus30.leonex.de:14840/;
                if(pos >= 0) {                                  // yes, URL contains "http"?
                    memcpy(m_lastHost, pl + pos, strlen(pl) + 1);   // http://streamplus30.leonex.de:14840/;
                    // Now we have an URL for a stream in host.
                    f_ref = true;
                }
            }
            if(startsWith(pl, "Title1")) {                      // Title1=Antenne Tirol
                const char* plsStationName = (pl + 7);
                if(audio_showstation) audio_showstation(plsStationName);
                sprintf(chbuf, "StationName: \"%s\"", plsStationName);
                if(audio_info) audio_info(chbuf);
                f_title = true;
            }
            if(startsWith(pl, "Length1")) f_title = true;               // if no Title is available
            if((f_ref == true) && (strlen(pl) == 0)) f_title = true;

            if(indexOf(pl, "Invalid username", 0) >= 0){ // Unable to access account: Invalid username or password
                m_f_running = false;
                stopSong();
                m_datamode = VS1053_NONE;
                return;
            }

            if(f_end) {                                      // we have both StationName and StationURL
                log_d("connect to new host %s", m_lastHost);
                connecttohost(m_lastHost);                              // Connect to it
            }
            return;
        } // pls

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(m_playlistFormat == FORMAT_ASX) { // Advanced Stream Redirector
            if(indexOf(pl, "<entry>", 0) >= 0) f_entry = true;      // found entry tag (returns -1 if not found)

            if(f_entry) {
                if(indexOf(pl, "ref href", 0) > 0) {                // <ref href="http://87.98.217.63:24112/stream" />
                    pos = indexOf(pl, "http", 0);
                    if(pos > 0) {
                        char* plsURL = (pl + pos);                  // http://87.98.217.63:24112/stream" />
                        int pos1 = indexOf(plsURL, "\"", 0);        // http://87.98.217.63:24112/stream
                        if(pos1 > 0) {
                            plsURL[pos1] = '\0';
                        }
                        memcpy(m_lastHost, plsURL, strlen(plsURL) + 1); // save url in array
                        log_d("m_lastHost = %s",m_lastHost);
                        // Now we have an URL for a stream in host.
                        f_ref = true;
                    }
                }
                pos = indexOf(pl, "<title>", 0);
                if(pos < 0) pos = indexOf(pl, "<Title>", 0);
                if(pos >= 0) {
                    char* plsStationName = (pl + pos + 7);          // remove <Title>
                    pos = indexOf(plsStationName, "</", 0);
                    if(pos >= 0){
                            *(plsStationName +pos) = 0;             // remove </Title>
                    }
                    if(audio_showstation) audio_showstation(plsStationName);
                    sprintf(chbuf, "StationName: \"%s\"", plsStationName);
                    if(audio_info) audio_info(chbuf);
                    f_title = true;
                }
            } //entry
            if(indexOf(pl, "http", 0) == 0) { //url only in asx
                memcpy(m_lastHost, pl, strlen(pl)); // save url in array
                m_lastHost[strlen(pl)] = '\0';
                log_d("m_lastHost = %s",m_lastHost);
                connecttohost(pl);
            }
            if(f_end) {   //we have both StationName and StationURL
                connecttohost(m_lastHost);                          // Connect to it
            }
        }  //asx
        return;
    } // end AUDIO_PLAYLISTDATA
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
void Audio::processAudioHeaderData() {

    int av = 0;
    if(!m_f_ssl) av=client.available();
    else         av= clientsecure.available();
    if(av <= 0) return;

    char hl[512]; // headerline
    uint8_t b = 0;
    uint16_t pos = 0;
    int16_t idx = 0;
    static bool f_icyname = false;
    static bool f_icydescription = false;
    static bool f_icyurl = false;

    while(true){
        if(!m_f_ssl) b = client.read();
        else         b = clientsecure.read();
        if(b == '\n') break;
        if(b == '\r') hl[pos] = 0;
        if(b < 0x20) continue;
        hl[pos] = b;
        pos++;
        if(pos == 510){
            hl[pos] = '\0';
            log_e("headerline overflow");
            break;
        }
    }

    if(!pos && m_f_ctseen){  // audio header complete?
        m_datamode = VS1053_DATA;                         // Expecting data now
        sprintf(chbuf, "Switch to DATA, metaint is %d", m_metaint);
        if(audio_info) audio_info(chbuf);
        memcpy(chbuf, m_lastHost, strlen(m_lastHost)+1);
    //    uint idx = indexOf(chbuf, "?", 0);
    //    if(idx > 0) chbuf[idx] = 0;
        if(audio_lasthost) audio_lasthost(chbuf);
        if(!f_icyname){if(audio_showstation) audio_showstation("");}
        if(!f_icydescription){if(audio_icydescription) audio_icydescription("");}
        if(!f_icyurl){if(audio_icyurl) audio_icyurl("");}
        if(m_f_swm){if(audio_showstreamtitle) audio_showstreamtitle("");}
        f_icyname = false;
        f_icydescription = false;
        f_icyurl = false;
        delay(50);  // #77
        return;
    }
    if(!pos){
        stopSong();
        if(audio_showstation) audio_showstation("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
        f_icyname = false;
        f_icydescription = false;
        f_icyurl = false;
        log_e("can't see content in audioHeaderData");
        return;
    }

    idx = indexOf(hl, ":", 0); // lowercase all letters up to the colon
    if(idx >= 0) {
        for(int i=0; i< idx; i++) {
            hl[i] = toLowerCase(hl[i]);
        }
    }

        if(indexOf(hl, "HTTP/1.0 404", 0) >= 0) {
        m_f_running = false;
        stopSong();
        if(audio_info) audio_info("404 Not Found");
        return;
    }

    if(indexOf(hl, "HTTP/1.1 404", 0) >= 0) {
        m_f_running = false;
        stopSong();
        if(audio_info) audio_info("404 Not Found");
        return;
    }

    if(indexOf(hl, "ICY 401", 0) >= 0) {
        m_f_running = false;
        stopSong();
        if(audio_info) audio_info("ICY 401 Service Unavailable");
        return;
    }

    if(indexOf(hl, "content-type:", 0) >= 0) {
        if(parseContentType(hl)) m_f_ctseen = true;
    }
    else if(startsWith(hl, "location:")) {
        int pos = indexOf(hl, "http", 0);
        const char* c_host = (hl + pos);
        sprintf(chbuf, "redirect to new host \"%s\"", c_host);
        if(audio_info) audio_info(chbuf);
        connecttohost(c_host);
    }
    else if(startsWith(hl, "content-disposition:")) {
        int pos1, pos2; // pos3;
        // e.g we have this headerline:  content-disposition: attachment; filename=stream.asx
        // filename is: "stream.asx"
        pos1 = indexOf(hl, "filename=", 0);
        if(pos1 > 0){
            pos1 += 9;
            if(hl[pos1] == '\"') pos1++;  // remove '\"' around filename if present
            pos2 = strlen(hl);
            if(hl[pos2 - 1] == '\"') hl[pos2 - 1] = '\0';
        }
        log_d("Filename is %s", hl + pos1);
    }
    else if(startsWith(hl, "set-cookie:")    ||
            startsWith(hl, "pragma:")        ||
            startsWith(hl, "expires:")       ||
            startsWith(hl, "cache-control:") ||
            startsWith(hl, "icy-pub:")       ||
            startsWith(hl, "p3p:")           ||
            startsWith(hl, "accept-ranges:") ){
        ; // do nothing
    }
    else if(startsWith(hl, "connection:")) {
        if(indexOf(hl, "close", 0) >= 0) {; /* do nothing */}
    }
    else if(startsWith(hl, "icy-genre:")) {
        ; // do nothing Ambient, Rock, etc
    }
    else if(startsWith(hl, "icy-br:")) {
        const char* c_bitRate = (hl + 7);
        int32_t br = atoi(c_bitRate); // Found bitrate tag, read the bitrate in Kbit
        m_bitrate = br;
        sprintf(chbuf, "%d", m_bitrate*1000);
        if(audio_bitrate) audio_bitrate(chbuf);
    }
    else if(startsWith(hl, "icy-metaint:")) {
        const char* c_metaint = (hl + 12);
        int32_t i_metaint = atoi(c_metaint);
        m_metaint = i_metaint;
        if(m_metaint) m_f_swm = false;                                // Multimediastream
    }
    else if(startsWith(hl, "icy-name:")) {
        char* c_icyname = (hl + 9); // Get station name
        idx = 0;
        while(c_icyname[idx] == ' '){idx++;} c_icyname += idx;        // Remove leading spaces
        idx = strlen(c_icyname);
        while(c_icyname[idx] == ' '){idx--;} c_icyname[idx + 1] = 0;  // Remove trailing spaces

        if(strlen(c_icyname) > 0) {
            sprintf(chbuf, "icy-name: %s", c_icyname);
            if(audio_info) audio_info(chbuf);
            if(audio_showstation) audio_showstation(c_icyname);
            f_icyname = true;
        }
    }
    else if(startsWith(hl, "content-length:")) {
        const char* c_cl = (hl + 15);
        int32_t i_cl = atoi(c_cl);
        m_contentlength = i_cl;
        m_f_webfile = true; // Stream comes from a fileserver
        sprintf(chbuf, "content-length: %i", m_contentlength);
        if(audio_info) audio_info(chbuf);
    }
    else if(startsWith(hl, "icy-description:")) {
        const char* c_idesc = (hl + 16);
        while(c_idesc[0] == ' ') c_idesc++;

        latinToUTF8(hl, sizeof(hl)); // if already UTF-0 do nothing, otherwise convert to UTF-8

        if(audio_icydescription) audio_icydescription(c_idesc);
        f_icydescription = true;
    }
    else if((startsWith(hl, "transfer-encoding:"))){
        if(endsWith(hl, "chunked") || endsWith(hl, "Chunked") ) {     // Station provides chunked transfer
            m_f_chunked = true;
            if(audio_info) audio_info("chunked data transfer");
            m_chunkcount = 0;                                         // Expect chunkcount in DATA
        }
    }
    else if(startsWith(hl, "icy-url:")) {
        const char* icyurl = (hl + 8);
        idx = 0;
        while(icyurl[idx] == ' ') {idx ++;} icyurl += idx;            // remove leading blanks
        sprintf(chbuf, "icy-url: %s", icyurl);
        // if(audio_info) audio_info(chbuf);
        if(audio_icyurl) audio_icyurl(icyurl);
        f_icyurl = true;
    }
    else if(startsWith(hl, "www-authenticate:")) {
        if(audio_info) audio_info("authentification failed, wrong credentials?");
        m_f_running = false;
        stopSong();
    }
    else {
        if(isascii(hl[0]) && hl[0] >= 0x20) {  // all other
            sprintf(chbuf, "%s", hl);
            if(audio_info) audio_info(chbuf);
        }
    }
    return;
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
        if(pos_ml == 509) log_e("metaline overflow in AUDIO_METADATA! metaline=%s", chbuf) ;
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
bool Audio::parseContentType(const char* ct) {
    bool ct_seen = false;
    if(indexOf(ct, "audio", 0) >= 0) {        // Is ct audio?
        ct_seen = true;                       // Yes, remember seeing this
        if(indexOf(ct, "mpeg", 13) >= 0) {
            m_codec = CODEC_MP3;
            sprintf(chbuf, "%s, format is mp3", ct);
            if(audio_info) audio_info(chbuf); //ok is likely mp3
        }
        else if(indexOf(ct, "mp3", 13) >= 0) {
            m_codec = CODEC_MP3;
            sprintf(chbuf, "%s, format is mp3", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "aac", 13) >= 0) {
            m_codec = CODEC_AAC;
            sprintf(chbuf, "%s, format is aac", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "mp4", 13) >= 0) {      // audio/mp4a, audio/mp4a-latm
            m_codec = CODEC_M4A;
            sprintf(chbuf, "%s, format is aac", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "m4a", 13) >= 0) {      // audio/x-m4a
            m_codec = CODEC_M4A;
            sprintf(chbuf, "%s, format is aac", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "wav", 13) >= 0) {      // audio/x-wav
            m_codec = CODEC_WAV;
            sprintf(chbuf, "%s, format is wav", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "ogg", 13) >= 0) {
            m_codec = CODEC_OGG;
            sprintf(chbuf, "ContentType %s found", ct);
            if(audio_info) audio_info(chbuf);
        }
        else if(indexOf(ct, "flac", 13) >= 0) {     // audio/flac, audio/x-flac
            m_codec = CODEC_FLAC;
            sprintf(chbuf, "%s, format is flac", ct);
            if(audio_info) audio_info(chbuf);
        }
        else {
            m_f_running = false;
            sprintf(chbuf, "%s, unsupported audio format", ct);
            if(audio_info) audio_info(chbuf);
        }
    }
    if(indexOf(ct, "application", 0) >= 0) {  // Is ct application?
        ct_seen = true;                       // Yes, remember seeing this
        uint8_t pos = indexOf(ct, "application", 0);
        if(indexOf(ct, "ogg", 13) >= 0) {
            m_codec = CODEC_OGG;
            sprintf(chbuf, "ContentType %s found", ct + pos);
            if(audio_info) audio_info(chbuf);
        }
    }
    return ct_seen;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::stop_mp3client(){
    int v=read_register(SCI_VOL);
    audiofile.close();
    m_f_localfile=false;
    m_f_webstream=false;
    write_register(SCI_VOL, 0);                         // Mute while stopping

    client.flush();                                     // Flush stream client
    client.stop();                                      // Stop stream client
    write_register(SCI_VOL, v);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::setDefaults(){
    // initializationsequence
    stopSong();
    initInBuff();                                           // initialize InputBuffer if not already done
    InBuff.resetBuffer();
    client.stop();
    client.flush(); // release memory
    clientsecure.stop();
    clientsecure.flush();
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
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(String host){
    return connecttohost(host.c_str());
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::connecttohost(const char* host, const char* user, const char* pwd) {
    // user and pwd for authentification only, can be empty

    if(strlen(host) == 0) {
        if(audio_info) audio_info("Hostaddress is empty");
        return false;
    }
    setDefaults();

    log_d("free heap=%d", ESP.getFreeHeap());

    sprintf(chbuf, "Connect to new host: \"%s\"", host);
    if(audio_info) audio_info(chbuf);

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

    // initializationsequence
    int16_t pos_slash;                                        // position of "/" in hostname
    int16_t pos_colon;                                        // position of "/" in hostname
    int16_t pos_ampersand;                                    // position of "&" in hostname
    uint16_t port = 80;                                       // port number
    m_f_webstream = true;
    setDatamode(VS1053_HEADER);                                // Handle header

    if(startsWith(host, "http://")) {
        host = host + 7;
        m_f_ssl = false;
    }

    if(startsWith(host, "https://")) {
        host = host +8;
        m_f_ssl = true;
        port = 443;
    }

    // Is it a playlist?
    if(endsWith(host, ".m3u")) {m_playlistFormat = FORMAT_M3U; m_datamode = VS1053_PLAYLISTINIT;}
    if(endsWith(host, ".pls")) {m_playlistFormat = FORMAT_PLS; m_datamode = VS1053_PLAYLISTINIT;}
    if(endsWith(host, ".asx")) {m_playlistFormat = FORMAT_ASX; m_datamode = VS1053_PLAYLISTINIT;}
        // if url ...=asx   www.fantasyfoxradio.de/infusions/gr_radiostatus_panel/gr_radiostatus_player.php?id=2&p=asx
    if(endsWith(host, "=asx")) {m_playlistFormat = FORMAT_ASX; m_datamode = VS1053_PLAYLISTINIT;}
    if(endsWith(host, "=pls")) {m_playlistFormat = FORMAT_PLS; m_datamode = VS1053_PLAYLISTINIT;}

     // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
    pos_slash     = indexOf(host, "/", 0);
    pos_colon     = indexOf(host, ":", 0);
    pos_ampersand = indexOf(host, "&", 0);

    char *hostwoext = NULL;                                  // "skonto.ls.lv:8002" in "skonto.ls.lv:8002/mp3"
    char *extension = NULL;                                  // "/mp3" in "skonto.ls.lv:8002/mp3"

    if(pos_slash > 1) {
        uint8_t hostwoextLen = pos_slash;
        hostwoext = (char*)malloc(hostwoextLen + 1);
        memcpy(hostwoext, host, hostwoextLen);
        hostwoext[hostwoextLen] = '\0';
        uint8_t extLen =  urlencode_expected_len(host + pos_slash);
        extension = (char *)malloc(extLen);
        memcpy(extension, host  + pos_slash, extLen);
        trim(extension);
        urlencode(extension, extLen, true);
    }
    else{  // url has no extension
        hostwoext = strdup(host);
        extension = strdup("/");
    }

    if((pos_colon >= 0) && ((pos_ampersand == -1) or (pos_ampersand > pos_colon))){
        port = atoi(host+ pos_colon + 1);// Get portnumber as integer
        hostwoext[pos_colon] = '\0';// Host without portnumber
    }

    sprintf(chbuf, "Connect to \"%s\" on port %d, extension \"%s\"", hostwoext, port, extension);
    if(audio_info) audio_info(chbuf);

    char resp[strlen(host) + strlen(authorization) + 100];
    resp[0] = '\0';

    strcat(resp, "GET ");
    strcat(resp, extension);
    strcat(resp, " HTTP/1.1\r\n");
    strcat(resp, "Host: ");
    strcat(resp, hostwoext);
    strcat(resp, "\r\n");
    strcat(resp, "Icy-MetaData:1\r\n");
    strcat(resp, "Authorization: Basic ");
    strcat(resp, authorization);
    strcat(resp, "\r\n");
    strcat(resp, "Connection: keep-alive\r\n\r\n");

    const uint32_t TIMEOUT_MS{350};
    uint32_t wtf;
    if(m_f_ssl == false) {
        uint32_t t = millis();
        if(client.connect(hostwoext, port, TIMEOUT_MS)) {
            // client.setNoDelay(true);
            client.print(resp);
            uint32_t dt = millis() - t;
            sprintf(chbuf, "Connected to server in %u ms", dt);
            if(audio_info) audio_info(chbuf);
            memcpy(m_lastHost, host, strlen(host) + 1);               // Remember the current s_host
            trim(m_lastHost);
            m_f_running = true;
            if(hostwoext) free(hostwoext);
            if(extension) free(extension);
            wtf = millis();
            while(!client.connected()){
            	if(millis()-wtf>TIMEOUT_MS * 2){
            		sprintf(chbuf, "Request %s failed! with WTF", m_lastHost);
								if(audio_info) audio_info(chbuf);
								return false;
								break;
            	}
            } // wait until the connection is established
            return true;
        }
    }

    const uint32_t TIMEOUT_MS_SSL{3700};
    if(m_f_ssl == true) {
        uint32_t t = millis();
        if(clientsecure.connect(hostwoext, port, TIMEOUT_MS_SSL)) {
            // clientsecure.setNoDelay(true);
            // if(audio_info) audio_info("SSL/TLS Connected to server");
            clientsecure.print(resp);
            uint32_t dt = millis() - t;
            sprintf(chbuf, "SSL has been established in %u ms, free Heap: %u bytes", dt, ESP.getFreeHeap());
            if(audio_info) audio_info(chbuf);
            memcpy(m_lastHost, "https://", 8);
            memcpy(m_lastHost + 8, host, strlen(host) + 1);    // Remember the current s_host
            m_f_running = true;
            if(hostwoext) free(hostwoext);
            if(extension) free(extension);
            wtf = millis();
            while(!clientsecure.connected()){
            	if(millis()-wtf>TIMEOUT_MS_SSL){
            		sprintf(chbuf, "Request %s failed! with WTF", m_lastHost);
								if(audio_info) audio_info(chbuf);
								return false;
								break;
            	}
            }
            return true;
        }
    }
    sprintf(chbuf, "Request %s failed!", host);
    if(audio_info) audio_info(chbuf);
    if(audio_showstation) audio_showstation("");
    if(audio_showstreamtitle) audio_showstreamtitle("");
    if(audio_icydescription) audio_icydescription("");
    if(audio_icyurl) audio_icyurl("");
    m_lastHost[0] = 0;
    if(hostwoext) free(hostwoext);
    if(extension) free(extension);
    return false;
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
bool Audio::connecttoSD(String sdfile){
    return connecttoFS(SD, sdfile.c_str());
}

bool Audio::connecttoSD(const char* sdfile){
    return connecttoFS(SD, sdfile);
}

bool Audio::connecttoFS(fs::FS &fs, const char* path) {

    if(strlen(path)>255) return false;

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

    m_f_localfile = true;
    m_file_size = audiofile.size();//TEST loop

    char* afn = strdup(audiofile.name());                   // audioFileName
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
    sprintf(chbuf, "SSL has been established, free Heap: %u bytes", ESP.getFreeHeap());
    if(audio_info) audio_info(chbuf);

    m_f_webstream = true;
    m_f_running = true;
    m_f_ssl = true;
    m_f_tts = true;
    setDatamode(VS1053_HEADER);

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
            m_audioDataSize = m_contentlength - m_audioDataStart;
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
    if(!strcmp(tag, "TALB")) sprintf(chbuf, "Album: %s", value);
    if(!strcmp(tag, "TBPM")) sprintf(chbuf, "BeatsPerMinute: %s", value);
    if(!strcmp(tag, "TCMP")) sprintf(chbuf, "Compilation: %s", value);
    if(!strcmp(tag, "TCOM")) sprintf(chbuf, "Composer: %s", value);
    if(!strcmp(tag, "TCON")) sprintf(chbuf, "ContentType: %s", value);
    if(!strcmp(tag, "TCOP")) sprintf(chbuf, "Copyright: %s", value);
    if(!strcmp(tag, "TDAT")) sprintf(chbuf, "Date: %s", value);
    if(!strcmp(tag, "TEXT")) sprintf(chbuf, "Lyricist: %s", value);
    if(!strcmp(tag, "TIME")) sprintf(chbuf, "Time: %s", value);
    if(!strcmp(tag, "TIT1")) sprintf(chbuf, "Grouping: %s", value);
    if(!strcmp(tag, "TIT2")) sprintf(chbuf, "Title: %s", value);
    if(!strcmp(tag, "TIT3")) sprintf(chbuf, "Subtitle: %s", value);
    if(!strcmp(tag, "TLAN")) sprintf(chbuf, "Language: %s", value);
    if(!strcmp(tag, "TLEN")) sprintf(chbuf, "Length (ms): %s", value);
    if(!strcmp(tag, "TMED")) sprintf(chbuf, "Media: %s", value);
    if(!strcmp(tag, "TOAL")) sprintf(chbuf, "OriginalAlbum: %s", value);
    if(!strcmp(tag, "TOPE")) sprintf(chbuf, "OriginalArtist: %s", value);
    if(!strcmp(tag, "TORY")) sprintf(chbuf, "OriginalReleaseYear: %s", value);
    if(!strcmp(tag, "TPE1")) sprintf(chbuf, "Artist: %s", value);
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
    return audiofile.size();
}
//---------------------------------------------------------------------------------------------------------------------
uint32_t Audio::getFilePos(){
    if (!audiofile) return 0;
    return audiofile.position();
}
//---------------------------------------------------------------------------------------------------------------------
bool Audio::setFilePos(uint32_t pos){
    if (!audiofile) return false;
    return audiofile.seek(pos);
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
#endif
