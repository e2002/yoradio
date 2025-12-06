/*
 *  vs1053_ext.h
 *
 *  Created on: Jul 09.2017
 *  Updated on: Oct 22.2023
 *      Author: Wolle
 */

#ifndef _vs1053_ext
#define _vs1053_ext

#define VS1053VOLM 128        // 128 or 96 only
#define VS1053VOL(v) (VS1053VOLM==128?log10(((float)v+1)) * 50.54571334 + 128:log10(((float)v+1)) * 64.54571334 + 96)


#include "Arduino.h"
#include <vector>
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

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include "hal/gpio_ll.h"
#endif
extern __attribute__((weak)) void audio_info(const char*);
extern __attribute__((weak)) void audio_showstreamtitle(const char*);
extern __attribute__((weak)) void audio_showstation(const char*);
extern __attribute__((weak)) void audio_showstreaminfo(const char*);
extern __attribute__((weak)) void audio_id3data(const char*); //ID3 metadata
extern __attribute__((weak)) void audio_id3artist(const char*);
extern __attribute__((weak)) void audio_id3album(const char*);
extern __attribute__((weak)) void audio_id3title(const char*);
extern __attribute__((weak)) void audio_beginSDread();
extern __attribute__((weak)) void audio_id3image(File& file, const size_t pos, const size_t size); //ID3 metadata image
extern __attribute__((weak)) void audio_id3lyrics(File& file, const size_t pos, const size_t size); //ID3 metadata lyrics
extern __attribute__((weak)) void audio_eof_mp3(const char*);
extern __attribute__((weak)) void audio_eof_speech(const char*);
extern __attribute__((weak)) void audio_bitrate(const char*);
extern __attribute__((weak)) void audio_commercial(const char*);
extern __attribute__((weak)) void audio_icyurl(const char*);
extern __attribute__((weak)) void audio_icylogo(const char*);
extern __attribute__((weak)) void audio_icydescription(const char*);
extern __attribute__((weak)) void audio_lasthost(const char*);
extern __attribute__((weak)) void audio_eof_stream(const char*); // The webstream comes to an end
extern __attribute__((weak)) void audio_progress(uint32_t start, uint32_t durarion);
extern __attribute__((weak)) void audio_error(const char*);

#define AUDIO_ERROR(...) {char buff[512 + 64]; sprintf(buff,__VA_ARGS__); if(audio_error) audio_error(buff);}

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
    //const size_t m_buffSizeRAM      = 1600 * 10;
    const size_t m_buffSizeRAM      = 1600;
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

struct Audio;

class Audio : private AudioBuffer{

    AudioBuffer InBuff; // instance of input buffer

private:
    WiFiClient            client;       // @suppress("Abstract class cannot be instantiated")
    WiFiClientSecure      clientsecure; // @suppress("Abstract class cannot be instantiated")
    WiFiClient*          _client = nullptr;
    File audiofile;
    std::vector<char*>    m_playlistContent; // m3u8 playlist buffer
    std::vector<char*>    m_playlistURL;     // m3u8 streamURLs buffer
    std::vector<uint32_t> m_hashQueue;

    struct ConnectParams {
      char *hostwoext = nullptr;
      uint16_t port = 80;
      Audio* instance = nullptr;
      
      ConnectParams(char* h, uint16_t p, Audio* a)
        : hostwoext(h), port(p), instance(a) {}
    };
    volatile bool _connectionResult;
    TaskHandle_t _connectTaskHandle = nullptr;
    static void connectTask(void* pvParams);
    
private:
    const char *codecname[10] = {"unknown", "WAV", "MP3", "AAC", "M4A", "FLAC", "AACP", "OPUS", "OGG", "VORBIS" };
    enum : int { AUDIO_NONE, HTTP_RESPONSE_HEADER , AUDIO_DATA, AUDIO_LOCALFILE, AUDIO_PLAYLISTINIT,
                AUDIO_PLAYLISTDATA};
    enum : int { FORMAT_NONE = 0, FORMAT_M3U = 1, FORMAT_PLS = 2, FORMAT_ASX = 3, FORMAT_M3U8 = 4};

    enum : int { CODEC_NONE = 0, CODEC_WAV = 1, CODEC_MP3 = 2, CODEC_AAC = 3, CODEC_M4A = 4, CODEC_FLAC = 5,
                 CODEC_AACP = 6, CODEC_OPUS = 7, CODEC_OGG = 8, CODEC_VORBIS = 9};
    enum : int { ST_NONE = 0, ST_WEBFILE = 1, ST_WEBSTREAM = 2};

private:
    uint8_t       cs_pin ;                          // Pin where CS line is connected
    uint8_t       dcs_pin ;                         // Pin where DCS line is connected
    uint8_t       dreq_pin ;                        // Pin where DREQ line is connected
    uint8_t       curvol ;                          // Current volume setting 0..100%

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
    const uint8_t SM_SDINEW         = 11 ;          // Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;          // Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;           // Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;           // Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;          // Bitnumber in SCI_MODE for Line input

    SPIClass*       spi_VS1053 = NULL;
    SPISettings     VS1053_SPI;

    char*           m_ibuff = nullptr;              // used in audio_info()
    char*           m_chbuf = NULL;
    uint16_t        m_chbufSize = 0;                // will set in constructor (depending on PSRAM)
    uint16_t        m_ibuffSize = 0;                // will set in constructor (depending on PSRAM)
    char*           m_lastHost = NULL;              // Store the last URL to a webstream
    char*           m_lastM3U8host = NULL;          // Store the last M3U8-URL to a webstream
    char*           m_playlistBuff = NULL;          // stores playlistdata
    uint8_t         m_codec = CODEC_NONE;           //
    uint8_t         m_m3u8Codec = CODEC_AAC;        // codec of m3u8 stream
    uint8_t         m_expectedCodec = CODEC_NONE;   // set in connecttohost (e.g. http://url.mp3 -> CODEC_MP3)
    uint8_t         m_expectedPlsFmt = FORMAT_NONE; // set in connecttohost (e.g. streaming01.m3u) -> FORMAT_M3U)
    uint8_t         m_streamType = ST_NONE;
    uint8_t         m_rev=0;                        // Revision
    uint8_t         m_playlistFormat = 0;           // M3U, PLS, ASX
    size_t          m_file_size = 0;                // size of the file
    uint32_t        m_audioFileDuration = 0;
    size_t          m_audioDataSize = 0;            //
    uint32_t        m_avr_bitrate = 0;
    bool            m_localBitrateSend = true;
    uint32_t        m_audioDataStart = 0;           // in bytes
    int             m_id3Size=0;                    // length id3 tag
    bool            m_f_ssl=false;
    uint8_t         m_endFillByte = 0;              // Byte to send when stopping song
    uint16_t        m_datamode=0;                   // Statemaschine
    bool            m_f_chunked = false ;           // Station provides chunked transfer
    bool            m_f_ctseen=false;               // First line of header seen or not
    bool            m_f_firstchunk=true;            // First chunk as input
    bool            m_f_metadata = false;           // Stream without metadata
    bool            m_f_tts = false;                // text to speech
    bool            m_f_Log = false;                // set in platformio.ini  -DAUDIO_LOG and -DCORE_DEBUG_LEVEL=3 or 4
    bool            m_f_continue = false;           // next m3u8 chunk is available
    bool            m_f_ts = true;                  // transport stream
    bool            m_f_firstCall = false;          // InitSequence for processWebstream and processLokalFile
    bool            m_f_firstM3U8call = false;      // InitSequence for m3u8 parsing
    bool            m_f_m3u8data = false;           // used in processM3U8entries
    bool            m_f_psramFound = false;         // set in constructor, result of psramInit()
    bool            m_f_timeout = false;            //
    int             m_LFcount;                      // Detection of end of header
    uint32_t        m_chunkcount = 0 ;              // Counter for chunked transfer
    uint32_t        m_contentlength = 0;
    uint32_t        m_resumeFilePos = 0;
    uint32_t        m_metaint = 0;                  // Number of databytes between metadata
    uint32_t        m_t0 = 0;                       // store millis(), is needed for a small delay
    uint16_t        m_bitrate = 0;                  // Bitrate in kb/sec
    int16_t         m_btp=0;                        // Bytes to play
    uint16_t        m_streamTitleHash = 0;          // remember streamtitle, ignore multiple occurence in metadata
    uint16_t        m_streamUrlHash = 0;            // remember streamURL, ignore multiple occurence in metadata
    uint16_t        m_timeout_ms = 250;
    uint16_t        m_timeout_ms_ssl = 2700;
    uint32_t        m_metacount=0;                  // Number of bytes in metadata
    uint16_t        m_m3u8_targetDuration = 10;     //
    int             m_controlCounter = 0;           // Status within readID3data() and readWaveHeader()
    bool            m_f_running = false;
    bool            m_f_ogg=false;                  // Set if oggstream
    bool            m_f_stream_ready=false;         // Set after connecttohost and first streamdata are available
    bool            m_f_unsync = false;
    bool            m_f_exthdr = false;             // ID3 extended header
    bool            _vuInitalized;
    
    const char volumetable[22]={   0,50,60,65,70,75,80,82,84,86,
                                  88,90,91,92,93,94,95,96,97,98,99,100}; //22 elements
    uint8_t  vuLeft, vuRight;
protected:
    inline void DCS_HIGH() {(dcs_pin&0x20) ? GPIO.out1_w1ts.data = 1 << (dcs_pin - 32) : GPIO.out_w1ts = 1 << dcs_pin;}
    inline void DCS_LOW()  {(dcs_pin&0x20) ? GPIO.out1_w1tc.data = 1 << (dcs_pin - 32) : GPIO.out_w1tc = 1 << dcs_pin;}
    inline void CS_HIGH()  {( cs_pin&0x20) ? GPIO.out1_w1ts.data = 1 << ( cs_pin - 32) : GPIO.out_w1ts = 1 <<  cs_pin;}
    inline void CS_LOW()   {( cs_pin&0x20) ? GPIO.out1_w1tc.data = 1 << ( cs_pin - 32) : GPIO.out_w1tc = 1 <<  cs_pin;}
    inline void await_data_request() {while(!digitalRead(dreq_pin)) NOP();}    // Very short delay
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
    void     stopSong() ;                                // Finish playing a song. Call this after
                                                         // the last playChunk call.
    void     urlencode(char* buff, uint16_t buffLen, bool spacesOnly = false);
    int      read_ID3_Header(uint8_t *data, size_t len);
    void     showID3Tag(const char* tag, const char* value);
    bool     httpPrint(const char* host);
    void     processLocalFile();
    void     processWebStream();
    void     processWebStreamTS();
    void     processWebStreamHLS();
    void     processWebFile();
    void     playAudioData();
    bool     readPlayListData();
    const char* parsePlaylist_M3U();
    const char* parsePlaylist_PLS();
    const char* parsePlaylist_ASX();
    const char* parsePlaylist_M3U8();
    const char* m3u8redirection(uint8_t* codec);
    uint64_t m3u8_findMediaSeqInURL();
    bool     STfromEXTINF(char* str);
    size_t   process_m3u8_ID3_Header(uint8_t* packet);
    bool     parseContentType(char* ct);
    bool     latinToUTF8(char* buff, size_t bufflen, bool UTF8check = true);
    bool     parseHttpResponseHeader();
    void     UTF8toASCII(char* str);
    void     unicode2utf8(char* buff, uint32_t len);
    //void     setDefaults();
    bool     ts_parsePacket(uint8_t* packet, uint8_t* packetStart, uint8_t* packetLength);
    uint16_t readMetadata(uint16_t maxBytes, bool first = false);
    size_t   chunkedDataTransfer(uint8_t* bytes);
    bool     readID3V1Tag();
    boolean  streamDetection(uint32_t bytesAvail);
    uint8_t  determineOggCodec(uint8_t* data, uint16_t len);

public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    //Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi, uint8_t mosi, uint8_t miso, uint8_t sclk);
    //Audio ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi = VSPI, uint8_t mosi = 23, uint8_t miso = 19, uint8_t sclk = 18);
    Audio ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, SPIClass *spi=&SPI);
    ~Audio();

    void     begin() ;                                  // Begin operation.  Sets pins correctly and prepares SPI bus.
    uint32_t stop_mp3client();
    void     setVolume(uint8_t vol);                    // Set the player volume.Level from 0-21, higher is louder.
    void     setTone(int8_t* rtone);                   // Set the player baas/treble, 4 nibbles for treble gain/freq and bass gain/freq
    uint8_t  getVolume();                               // Get the current volume setting, higher is louder.
    void     printDetails(const char* str);             // Print configuration details to serial output.
    uint8_t  printVersion();                            // Returns version of vs1053 chip
    uint32_t printChipID();                             // Returns chipID of vs1053 chip
    uint32_t getBitRate();                              // average br from WRAM register
    void     softReset() ;                              // Do a soft reset
    void     loop();
    void     setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl);
    bool     connecttohost(String host);
    bool     connecttohost(const char* host, const char* user = "", const char* pwd = "");
    bool     connecttoSD(String sdfile, uint32_t resumeFilePos = 0);
    bool     connecttoSD(const char* sdfile, uint32_t resumeFilePos = 0);
    bool     connecttoFS(fs::FS &fs, const char* path, uint32_t resumeFilePos = 0);
    bool     connecttospeech(const char* speech, const char* lang);
    bool     isRunning() {return m_f_running;}
    bool     pauseResume();
    uint32_t getFileSize();
    uint32_t getFilePos();
    uint32_t getAudioDataStartPos();
    bool     setFilePos(uint32_t pos);
    uint32_t getAudioFileDuration();
    uint32_t getAudioCurrentTime();
    size_t   bufferFilled();
    size_t   bufferFree();
    void     loadUserCode();
    int getCodec() {return m_codec;}
    const char *getCodecname() {return codecname[m_codec];}
    size_t   inBufferFilled(){ return bufferFilled(); }
    size_t   inBufferFree(){ return bufferFree(); }
    void     setBalance(int8_t bal = 0);
    void     setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass);
    void     setDefaults();
    void     forceMono(bool m) {}                        // TODO
    /* VU METER */
    void     setVUmeter();
    uint16_t get_VUlevel(uint16_t dimension);
    void     computeVUlevel();
    bool     eofHeader;
    // implement several function with respect to the index of string
    void strlower(char *str){
        unsigned char *p = (unsigned char *)str;
        while (*p) {
           *p = tolower((unsigned char)*p);
            p++;
        }
    }

    bool startsWith (const char* base, const char* str) { return (strstr(base, str) - base) == 0;}

    bool endsWith(const char *base, const char *searchString) {
        int32_t slen = strlen(searchString);
        if(slen == 0) return false;
        const char *p = base + strlen(base);
    //  while(p > base && isspace(*p)) p--;  // rtrim
        p -= slen;
        if(p < base) return false;
        return (strncmp(p, searchString, slen) == 0);
    }

    int indexOf (const char* base, const char* str, int startIndex = 0) {
    //fb
        const char *p = base;
        for (; startIndex > 0; startIndex--)
            if (*p++ == '\0') return -1;
        char* pos = strstr(p, str);
        if (pos == nullptr) return -1;
        return pos - base;
    }

    int indexOf (const char* base, char ch, int startIndex = 0) {
    //fb
        const char *p = base;
        for (; startIndex > 0; startIndex--)
            if (*p++ == '\0') return -1;
        char *pos = strchr(p, ch);
        if (pos == nullptr) return -1;
        return pos - base;
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
        int result = 0;  // seek for str in buffer or in header up to baselen, not nullterninated
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
    
    int32_t min3(int32_t a, int32_t b, int32_t c){
        uint32_t min_val = a;
        if (b < min_val) min_val = b;
        if (c < min_val) min_val = c;
        return min_val;
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

    void trim(char *s) {
    //fb   trim in place
        char *pe;
        char *p = s;
        while ( isspace(*p) ) p++; //left
        pe = p; //right
        while ( *pe != '\0' ) pe++;
        do {
            pe--;
        } while ( (pe > p) && isspace(*pe) );
        if (p == s) {
            *++pe = '\0';
        } else {  //move
            while ( p <= pe ) *s++ = *p++;
            *s = '\0';
        }
    }

    void vector_clear_and_shrink(std::vector<char*>&vec){
        uint size = vec.size();
        for (int i = 0; i < size; i++) {
            if(vec[i]){
                free(vec[i]);
                vec[i] = NULL;
            }
        }
        vec.clear();
        vec.shrink_to_fit();
    }

    uint32_t simpleHash(const char* str){
        if(str == NULL) return 0;
        uint32_t hash = 0;
        for(int i=0; i<strlen(str); i++){
		    if(str[i] < 32) continue; // ignore control sign
		    hash += (str[i] - 31) * i * 32;
        }
        return hash;
	}

    char* x_strdup(const char* str){
        if(m_f_psramFound){
            char* s = (char*)ps_malloc(strlen(str) + 1);
            strcpy(s, str);
            return s;
        }
        else{
            return strdup(str);
        }
    }

    inline uint8_t  getDatamode(){return m_datamode;}
    inline void     setDatamode(uint8_t dm){m_datamode=dm;}
    inline uint32_t streamavail(){ return _client ? _client->available() : 0;}
};

#endif
