#ifndef common_gfx_h
#define common_gfx_h
  public:
    uint16_t plItemHeight, plTtemsCount, plCurrentPos;
    int plYStart;
  public:
    DspCore();
    //char plMenu[PLMITEMS][PLMITEMLENGHT];
    void initDisplay();
    void drawLogo(uint16_t top);
    void clearDsp(bool black=false);
    void printClock(){}
    void printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw);
    void clearClock();
    char* utf8Rus(const char* str, bool uppercase);
    void drawPlaylist(uint16_t currentItem);
    void loop(bool force=false);
    void charSize(uint8_t textsize, uint8_t& width, uint16_t& height);
    #ifndef DSP_LCD
      #if DSP_MODEL==DSP_NOKIA5110
        virtual void command(uint8_t c);
        virtual void data(uint8_t c);
      #else
        virtual void startWrite(void);
        virtual void endWrite(void);
      #endif
      void setTextSize(uint8_t s);
    #else
      uint16_t width();
      uint16_t height();
      void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
      void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){}
      void setTextSize(uint8_t s){}
      void setTextSize(uint8_t sx, uint8_t sy){}
      void setTextColor(uint16_t c, uint16_t bg){}
      void setFont(){}
      void apScreen();
    #endif
    
    void flip();
    void invert();
    void sleep();
    void wake();
    void writePixel(int16_t x, int16_t y, uint16_t color);
    void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void setClipping(clipArea ca);
    void clearClipping();
    void setScrollId(void * scrollid) { _scrollid = scrollid; }
    void * getScrollId() { return _scrollid; }
    void setNumFont();
    uint16_t textWidth(const char *txt);
    #if DSP_MODEL==DSP_ILI9225
      uint16_t width(void) { return (int16_t)maxX(); }
      uint16_t height(void) { return (int16_t)maxY(); }
      void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
      uint16_t print(const char* s);
      void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
      void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
      void setFont(const GFXfont *f = NULL);
      void setFont(uint8_t* font, bool monoSp=false );
      void setTextColor(uint16_t fg, uint16_t bg);
      void setCursor(int16_t x, int16_t y);
      void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
      void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
      uint16_t drawChar(uint16_t x, uint16_t y, uint16_t ch, uint16_t color = COLOR_WHITE);
    #endif
    void printPLitem(uint8_t pos, const char* item, ScrollWidget& current);
  private:
    char  _timeBuf[20], _dateBuf[20], _oldTimeBuf[20], _oldDateBuf[20], _bufforseconds[4], _buffordate[40];
    uint16_t _timewidth, _timeleft, _datewidth, _dateleft, _oldtimeleft, _oldtimewidth, _olddateleft, _olddatewidth, clockTop, clockRightSpace, clockTimeHeight, _dotsLeft;
    bool _clipping, _printdots;
    clipArea _cliparea;
    void * _scrollid;
    void _getTimeBounds();
    void _clockSeconds();
    void _clockDate();
    void _clockTime();
    uint8_t _charWidth(unsigned char c);
    #if DSP_MODEL==DSP_ILI9225
      uint16_t _bgcolor, _fgcolor;
      int16_t  _cursorx, _cursory;
      bool _gFont/*, _started*/;
    #endif

#endif
