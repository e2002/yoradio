/*
 *  vs1053_ext.h
 *
 *  Created on: Jul 09.2017
 *  Updated on: Feb 11 2022
 *      Author: Wolle
 */

#ifndef _vs1053_ext
#define _vs1053_ext

#define AUDIOBUFFER_MULTIPLIER 13

#define VS1053VOLM 128				// 128 or 96 only
#define VS1053VOL(v) (VS1053VOLM==128?log10(((float)v+1)) * 50.54571334 + 128:log10(((float)v+1)) * 64.54571334 + 96)

#include "Arduino.h"
#include "libb64/cencode.h"
#include "SPI.h"
#include "SD.h"
#include "SD_MMC.h"
#include "SPIFFS.h"
#include "FS.h"
#include "FFat.h"
#include "WiFiClient.h"
#include "WiFiClientSecure.h"

#include "vs1053b-patches-flac.h"

extern __attribute__((weak)) void audio_info(const char*);
extern __attribute__((weak)) void audio_showstreamtitle(const char*);
extern __attribute__((weak)) void audio_showstation(const char*);
extern __attribute__((weak)) void audio_showstreaminfo(const char*);
extern __attribute__((weak)) void audio_id3data(const char*); //ID3 metadata
extern __attribute__((weak)) void audio_id3image(File& file, const size_t pos, const size_t size); //ID3 metadata image
extern __attribute__((weak)) void audio_eof_mp3(const char*);
extern __attribute__((weak)) void audio_eof_speech(const char*);
extern __attribute__((weak)) void audio_bitrate(const char*);
extern __attribute__((weak)) void audio_commercial(const char*);
extern __attribute__((weak)) void audio_icyurl(const char*);
extern __attribute__((weak)) void audio_icydescription(const char*);
extern __attribute__((weak)) void audio_lasthost(const char*);
extern __attribute__((weak)) void audio_eof_stream(const char*); // The webstream comes to an end

//----------------------------------------------------------------------------------------------------------------------

class AudioBuffer {
// AudioBuffer will be allocated in PSRAM, If PSRAM not available or has not enough space AudioBuffer will be
// allocated in FlashRAM with reduced size
//
//  m_buffer            m_readPtr                 m_writePtr                 m_endPtr
//   |                       |<------dataLength------->|<------ writeSpace ----->|
//   ▼                       ▼                         ▼                         ▼
//   ---------------------------------------------------------------------------------------------------------------
//   |                     <--m_buffSize-->                                      |      <--m_resBuffSize -->     |
//   ---------------------------------------------------------------------------------------------------------------
//   |<-----freeSpace------->|                         |<------freeSpace-------->|
//
//
//
//   if the space between m_readPtr and buffend < m_resBuffSize copy data from the beginning to resBuff
//   so that the mp3/aac/flac frame is always completed
//
//  m_buffer                      m_writePtr                 m_readPtr        m_endPtr
//   |                                 |<-------writeSpace------>|<--dataLength-->|
//   ▼                                 ▼                         ▼                ▼
//   ---------------------------------------------------------------------------------------------------------------
//   |                        <--m_buffSize-->                                    |      <--m_resBuffSize -->     |
//   ---------------------------------------------------------------------------------------------------------------
//   |<---  ------dataLength--  ------>|<-------freeSpace------->|
//
//

public:
    AudioBuffer(size_t maxBlockSize = 0);       // constructor
    ~AudioBuffer();                             // frees the buffer
    size_t   init();                            // set default values
    void     changeMaxBlockSize(uint16_t mbs);  // is default 1600 for mp3 and aac, set 16384 for FLAC
    uint16_t getMaxBlockSize();                 // returns maxBlockSize
    size_t   freeSpace();                       // number of free bytes to overwrite
    size_t   writeSpace();                      // space fom writepointer to bufferend
    size_t   bufferFilled();                    // returns the number of filled bytes
    void     bytesWritten(size_t bw);           // update writepointer
    void     bytesWasRead(size_t br);           // update readpointer
    uint8_t* getWritePtr();                     // returns the current writepointer
    uint8_t* getReadPtr();                      // returns the current readpointer
    uint32_t getWritePos();                     // write position relative to the beginning
    uint32_t getReadPos();                      // read position relative to the beginning
    void     resetBuffer();                     // restore defaults

protected:
    const size_t m_buffSizePSRAM    = 300000;   // most webstreams limit the advance to 100...300Kbytes
    const size_t m_buffSizeRAM      = 1600 * AUDIOBUFFER_MULTIPLIER;
    size_t       m_buffSize         = 0;
    size_t       m_freeSpace        = 0;
    size_t       m_writeSpace       = 0;
    size_t       m_dataLength       = 0;
    size_t       m_resBuffSizeRAM   = 4096;     // reserved buffspace, >= one mp3  frame
    size_t       m_resBuffSizePSRAM = 4096;
    size_t       m_maxBlockSize     = 1600;
    uint8_t*     m_buffer           = NULL;
    uint8_t*     m_writePtr         = NULL;
    uint8_t*     m_readPtr          = NULL;
    uint8_t*     m_endPtr           = NULL;
    bool         m_f_start          = true;
};
//----------------------------------------------------------------------------------------------------------------------

class Audio : private AudioBuffer{

    AudioBuffer InBuff; // instance of input buffer

private:
    WiFiClient client;
    WiFiClientSecure clientsecure;
    File audiofile;


private:
    enum : int { VS1053_NONE, VS1053_HEADER , VS1053_DATA, VS1053_METADATA, VS1053_PLAYLISTINIT,
                 VS1053_PLAYLISTHEADER,  VS1053_PLAYLISTDATA, VS1053_SWM, VS1053_OGG};
    enum : int { FORMAT_NONE = 0, FORMAT_M3U = 1, FORMAT_PLS = 2, FORMAT_ASX = 3};

    enum : int { CODEC_NONE, CODEC_WAV, CODEC_MP3, CODEC_AAC, CODEC_M4A, CODEC_FLAC, CODEC_OGG,
                 CODEC_OGG_FLAC, CODEC_OGG_OPUS};

private:
    uint8_t       cs_pin ;                        	// Pin where CS line is connected
    uint8_t       dcs_pin ;                       	// Pin where DCS line is connected
    uint8_t       dreq_pin ;                      	// Pin where DREQ line is connected
    uint8_t       curvol ;                        	// Current volume setting 0..100%

    const uint8_t vs1053_chunk_size = 32 ;
    int8_t          m_balance = 0;                  // -16 (mute left) ... +16 (mute right)
    // SCI Register
    const uint8_t SCI_MODE          = 0x0 ;
    const uint8_t SCI_STATUS        = 0x1 ;
    const uint8_t SCI_BASS          = 0x2 ;
    const uint8_t SCI_CLOCKF        = 0x3 ;
    const uint8_t SCI_DECODE_TIME   = 0x4 ;
    const uint8_t SCI_AUDATA        = 0x5 ;
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;
    const uint8_t SCI_HDAT0         = 0x8 ;
    const uint8_t SCI_HDAT1         = 0x9 ;
    const uint8_t SCI_AIADDR        = 0xA ;
    const uint8_t SCI_VOL           = 0xB ;
    const uint8_t SCI_AICTRL0       = 0xC ;
    const uint8_t SCI_AICTRL1       = 0xD ;
    const uint8_t SCI_AICTRL2       = 0xE ;
    const uint8_t SCI_AICTRL3       = 0xF ;
    // SCI_MODE bits
    const uint8_t SM_SDINEW         = 11 ;        	// Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;        	// Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;         	// Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;         	// Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;        	// Bitnumber in SCI_MODE for Line input

    SPIClass*       spi_VS1053 = NULL;
    SPISettings     VS1053_SPI;                     // SPI settings for this slave

    char            chbuf[512];
    char            m_lastHost[256];                // Store the last URL to a webstream
    uint8_t         m_codec = CODEC_NONE;           //
    uint8_t         m_rev=0;                        // Revision
    uint8_t         m_playlistFormat = 0;           // M3U, PLS, ASX
    size_t          m_file_size = 0;                // size of the file
    size_t          m_audioDataSize = 0;            //
    uint32_t        m_audioDataStart = 0;           // in bytes
    int             m_id3Size=0;                    // length id3 tag
    bool            m_f_ssl=false;
    uint8_t         m_endFillByte ;                 // Byte to send when stopping song
    uint16_t        m_datamode=0;                   // Statemaschine
    bool            m_f_chunked = false ;           // Station provides chunked transfer
    bool            m_f_ctseen=false;               // First line of header seen or not
    bool            m_f_firstchunk=true;            // First chunk as input
    bool            m_f_swm = true;                 // Stream without metadata
    bool            m_f_tts = false;                // text to speech
    bool            m_f_webfile = false;
    bool            m_f_firstCall = false;          // InitSequence for processWebstream and processLokalFile
    int             m_LFcount;                      // Detection of end of header
    uint32_t        m_chunkcount = 0 ;              // Counter for chunked transfer
    uint32_t        m_contentlength = 0;
    uint32_t        m_metaint = 0;                  // Number of databytes between metadata
    uint32_t        m_t0 = 0;                       // store millis(), is needed for a small delay
    uint16_t        m_bitrate = 0;                  // Bitrate in kb/sec
    int16_t         m_btp=0;                        // Bytes to play
    int             m_metacount=0;                  // Number of bytes in metadata
    int             m_controlCounter = 0;           // Status within readID3data() and readWaveHeader()
    bool            m_firstmetabyte=false;          // True if first metabyte (counter)
    bool            m_f_running = false;
    bool            m_f_localfile = false ;         // Play from local mp3-file
    bool            m_f_webstream = false ;         // Play from URL
    bool            m_f_ogg=false;                  // Set if oggstream
    bool            m_f_stream_ready=false;         // Set after connecttohost and first streamdata are available
    bool            m_f_unsync = false;
    bool            m_f_exthdr = false;             // ID3 extended header

    const char volumetable[22]={   0,50,60,65,70,75,80,82,84,86,
                                  88,90,91,92,93,94,95,96,97,98,99,100}; //22 elements
protected:
    inline void DCS_HIGH() {(dcs_pin&0x20) ? GPIO.out1_w1ts.data = 1 << (dcs_pin - 32) : GPIO.out_w1ts = 1 << dcs_pin;}
	inline void DCS_LOW()  {(dcs_pin&0x20) ? GPIO.out1_w1tc.data = 1 << (dcs_pin - 32) : GPIO.out_w1tc = 1 << dcs_pin;}
	inline void CS_HIGH()  {( cs_pin&0x20) ? GPIO.out1_w1ts.data = 1 << ( cs_pin - 32) : GPIO.out_w1ts = 1 <<  cs_pin;}
    inline void CS_LOW()   {( cs_pin&0x20) ? GPIO.out1_w1tc.data = 1 << ( cs_pin - 32) : GPIO.out_w1tc = 1 <<  cs_pin;}
    inline void await_data_request() {while(!digitalRead(dreq_pin)) NOP();}	  // Very short delay
    inline bool data_request()     {return(digitalRead(dreq_pin) == HIGH);}

    void     initInBuff();
    void     control_mode_on();
    void     control_mode_off();
    void     data_mode_on();
    void     data_mode_off();
    uint16_t read_register ( uint8_t _reg ) ;
    void     write_register ( uint8_t _reg, uint16_t _value );
    void     sdi_send_buffer ( uint8_t* data, size_t len ) ;
    size_t   sendBytes(uint8_t* data, size_t len);
    void     sdi_send_fillers ( size_t length ) ;
    void     wram_write ( uint16_t address, uint16_t data ) ;
    uint16_t wram_read ( uint16_t address ) ;
    void     showstreamtitle(const char* ml);
    void     startSong() ;                               // Prepare to start playing. Call this each
                                                         // time a new song starts.
    void     stopSong() ;                                // Finish playing a song. Call this after
                                                         // the last playChunk call.
    void     urlencode(char* buff, uint16_t buffLen, bool spacesOnly = false);
    int      read_MP3_Header(uint8_t *data, size_t len);
    void     showID3Tag(const char* tag, const char* value);
    void     processLocalFile();
    void     processWebStream();
    void     processPlayListData();
    bool     parseContentType(const char* ct);
    bool     latinToUTF8(char* buff, size_t bufflen);
    void     processAudioHeaderData();
    bool     readMetadata(uint8_t b, bool first = false);
    void     UTF8toASCII(char* str);
    void     unicode2utf8(char* buff, uint32_t len);
    void     setDefaults();
    void     loadUserCode();



public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    Audio ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi = VSPI, uint8_t mosi = 23, uint8_t miso = 19, uint8_t sclk = 18);
    ~Audio();

    void     begin() ;                                  // Begin operation.  Sets pins correctly and prepares SPI bus.
    void     stop_mp3client();
    void     setVolume(uint8_t vol);                    // Set the player volume.Level from 0-21, higher is louder.
    void     setTone(uint8_t* rtone);                   // Set the player baas/treble, 4 nibbles for treble gain/freq and bass gain/freq
    uint8_t  getVolume();                               // Get the current volume setting, higher is louder.
    void     printDetails(const char* str);             // Print configuration details to serial output.
    const char* printVersion();                         // Print ID and version of vs1053 chip
    void     softReset() ;                              // Do a soft reset
    void 	 loop();
    bool     connecttohost(String host);
    bool     connecttohost(const char* host, const char* user = "", const char* pwd = "");
    bool	 connecttoSD(String sdfile);
    bool     connecttoSD(const char* sdfile);
    bool     connecttoFS(fs::FS &fs, const char* path);
    bool     connecttospeech(const char* speech, const char* lang);
    uint32_t getFileSize();
    uint32_t getFilePos();
    uint32_t getAudioDataStartPos();
    bool     setFilePos(uint32_t pos);
    size_t   bufferFilled();
    size_t   bufferFree();
		bool isRunning() {/*Serial.printf("m_f_running=%d\n", m_f_running); */return m_f_running;}
		void setBalance(int8_t bal = 0);
		void setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass);
		
    // implement several function with respect to the index of string
    bool startsWith (const char* base, const char* str) { return (strstr(base, str) - base) == 0;}
    bool endsWith (const char* base, const char* str) {
        int blen = strlen(base);
        int slen = strlen(str);
        return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
    }
    int indexOf (const char* base, const char* str, int startIndex) {
        int result;
        int baselen = strlen(base);
        if (strlen(str) > baselen || startIndex > baselen) result = -1;
        else {
            char* pos = strstr(base + startIndex, str);
            if (pos == NULL) result = -1;
            else result = pos - base;
        }
        return result;
    }
    int lastIndexOf(const char* base, const char* str) {
        int res = -1, result = -1;
        int lenBase = strlen(base);
        int lenStr  = strlen(str);
        if(lenStr > lenBase) {return -1;} // str should not longer than base
        for(int i=0; i<(lenBase - lenStr); i++){
            res = indexOf(base, str, i);
            if(res > result) result = res;
        }
        return result;
    }
    int specialIndexOf (uint8_t* base, const char* str, int baselen, bool exact = false){
        int result;  // seek for str in buffer or in header up to baselen, not nullterninated
        if (strlen(str) > baselen) return -1; // if exact == true seekstr in buffer must have "\0" at the end
        for (int i = 0; i < baselen - strlen(str); i++){
            result = i;
            for (int j = 0; j < strlen(str) + exact; j++){
                if (*(base + i + j) != *(str + j)){
                    result = -1;
                    break;
                }
            }
            if (result >= 0) break;
        }
        return result;
    }
    size_t bigEndian(uint8_t* base, uint8_t numBytes, uint8_t shiftLeft = 8){
        size_t result = 0;
        if(numBytes < 1 or numBytes > 4) return 0;
        for (int i = 0; i < numBytes; i++) {
                result += *(base + i) << (numBytes -i - 1) * shiftLeft;
        }
        return result;
    }
    bool b64encode(const char* source, uint16_t sourceLength, char* dest){
        size_t size = base64_encode_expected_len(sourceLength) + 1;
        char * buffer = (char *) malloc(size);
        if(buffer) {
            base64_encodestate _state;
            base64_init_encodestate(&_state);
            int len = base64_encode_block(&source[0], sourceLength, &buffer[0], &_state);
            len = base64_encode_blockend((buffer + len), &_state);
            memcpy(dest, buffer, strlen(buffer));
            dest[strlen(buffer)] = '\0';
            free(buffer);
            return true;
        }
        return false;
    }
    size_t urlencode_expected_len(const char* source){
        size_t expectedLen = strlen(source);
        for(int i = 0; i < strlen(source); i++) {
            if(isalnum(source[i])){;}
            else expectedLen += 2;
        }
        return expectedLen;
    }
    void trim(char* s){
        uint8_t l = 0;
        while(isspace(*(s + l))) l++;
        for(uint16_t i = 0; i< strlen(s) - l; i++)  *(s + i) = *(s + i + l); // ltrim
        char* back = s + strlen(s);
        while(isspace(*--back));
        *(back + 1) = '\0';      // rtrim
    }


    inline uint8_t  getDatamode(){return m_datamode;}
    inline void     setDatamode(uint8_t dm){m_datamode=dm;}
    inline uint32_t streamavail() {if(m_f_ssl==false) return client.available(); else return clientsecure.available();}
};

#endif
