#ifndef widgets_h
#define widgets_h
#if DSP_MODEL!=DSP_DUMMY
#include "widgetsconfig.h"

#ifndef DSP_LCD
  #define CHARWIDTH   6
  #define CHARHEIGHT  8
#else
  #define CHARWIDTH   1
  #define CHARHEIGHT  1
#endif

class psFrameBuffer;

class Widget{
  public:
    Widget(){ _active   = false; }
    virtual ~Widget(){}
    virtual void loop(){}
    virtual void init(WidgetConfig conf, uint16_t fgcolor, uint16_t bgcolor){
      _config = conf;
      _fgcolor  = fgcolor;
      _bgcolor  = bgcolor;
      _width = _backMove.width = 0;
      _backMove.x = _config.left;
      _backMove.y = _config.top;
      _moved = _locked = false;
    }
    void setAlign(WidgetAlign align){
      _config.align = align;
    }
    void setActive(bool act, bool clr=false) { _active = act; if(_active && !_locked) _draw(); if(clr && !_locked) _clear(); }
    void lock(bool lck=true) { _locked = lck; if(_locked) _reset(); if(_locked && _active) _clear();  }
    void unlock() { _locked = false; }
    bool locked() { return _locked; }
    void moveTo(MoveConfig mv){
      if(mv.width<0) return;
      _moved = true;
      if(_active && !_locked) _clear();
      _config.left = mv.x;
      _config.top = mv.y;
      if(mv.width>0) _width = mv.width;
      _reset();
      _draw();
    }
    void moveBack(){
      if(!_moved) return;
      if(_active && !_locked) _clear();
      _config.left = _backMove.x;
      _config.top = _backMove.y;
      _width = _backMove.width;
      _moved = false;
      _reset();
      _draw();
    }
  protected:
    bool _active, _moved, _locked;
    uint16_t _fgcolor, _bgcolor, _width;
    WidgetConfig _config;
    MoveConfig   _backMove;
    virtual void _draw() {}
    virtual void _clear() {}
    virtual void _reset() {}
};

class TextWidget: public Widget {
  public:
    TextWidget() {}
    TextWidget(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) { init(wconf, buffsize, uppercase, fgcolor, bgcolor); }
    ~TextWidget();
    using Widget::init;
    void init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor);
    void setText(const char* txt);
    void setText(int val, const char *format);
    void setText(const char* txt, const char *format);
    bool uppercase() { return _uppercase; }
  protected:
    char *_text;
    char *_oldtext;
    bool _uppercase;
    uint16_t  _buffsize, _textwidth, _oldtextwidth, _oldleft, _textheight;
    uint8_t _charWidth;
  protected:
    void _draw();
    uint16_t _realLeft(bool w_fb=false);
    void _charSize(uint8_t textsize, uint8_t& width, uint16_t& height);
};

class FillWidget: public Widget {
  public:
    FillWidget() {}
    FillWidget(FillConfig conf, uint16_t bgcolor) { init(conf, bgcolor); }
    using Widget::init;
    void init(FillConfig conf, uint16_t bgcolor);
    void setHeight(uint16_t newHeight);
  protected:
    uint16_t _height;
    void _draw();
};

class ScrollWidget: public TextWidget {
  public:
    ScrollWidget(){}
    ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor);
    ~ScrollWidget();
    using Widget::init;
    void init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor);
    void loop();
    void setText(const char* txt);
    void setText(const char* txt, const char *format);
  private:
    char *_sep;
    char *_window;
    int16_t _x;
    bool _doscroll;
    uint8_t _scrolldelta;
    uint16_t _scrolltime;
    uint32_t _scrolldelay;
    uint16_t _sepwidth, _startscrolldelay;
    uint8_t _charWidth;
    psFrameBuffer* _fb=nullptr;
  private:
    void _setTextParams();
    void _calcX();
    void _drawFrame();
    void _draw();
    bool _checkIsScrollNeeded();
    bool _checkDelay(int m, uint32_t &tstamp);
    void _clear();
    void _reset();
};

class SliderWidget: public Widget {
  public:
    SliderWidget(){}
    SliderWidget(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor=0){
      init(conf, fgcolor, bgcolor, maxval, oucolor);
    }
    using Widget::init;
    void init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor=0);
    void setValue(uint32_t val);
  protected:
    uint16_t _height, _oucolor, _oldvalwidth;
    uint32_t _max, _value;
    bool _outlined;
    void _draw();
    void _drawslider();
    void _clear();
    void _reset();
};

class VuWidget: public Widget {
  public:
    VuWidget() {}
    VuWidget(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor)
            { init(wconf, bands, vumaxcolor, vumincolor, bgcolor); }
    ~VuWidget();
    using Widget::init;
    void init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor);
    void loop();
  protected:
    #if !defined(DSP_LCD) && !defined(DSP_OLED)
      Canvas *_canvas;
    #endif
    VUBandsConfig _bands;
    uint16_t _vumaxcolor, _vumincolor;
    void _draw();
    void _clear();
};

class NumWidget: public TextWidget {
  public:
    using Widget::init;
    void init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor);
    void setText(const char* txt);
    void setText(int val, const char *format);
  protected:
    void _getBounds();
    void _draw();
};

class ProgressWidget: public TextWidget {
  public:
    ProgressWidget() {}
    ProgressWidget(WidgetConfig conf, ProgressConfig pconf, uint16_t fgcolor, uint16_t bgcolor) { 
      init(conf, pconf, fgcolor, bgcolor);
    }
    using Widget::init;
    void init(WidgetConfig conf, ProgressConfig pconf, uint16_t fgcolor, uint16_t bgcolor){
      TextWidget::init(conf, pconf.width, false, fgcolor, bgcolor);
      _speed = pconf.speed; _width = pconf.width; _barwidth = pconf.barwidth;
      _pg = 0; 
    }
    void loop();
  private:
    uint8_t _pg;
    uint16_t _speed, _barwidth;
    uint32_t _scrolldelay;
    void _progress();
    bool _checkDelay(int m, uint32_t &tstamp);
};

class ClockWidget: public Widget {
  public:
    using Widget::init;
    void init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor);
    void draw(bool force=false);
    uint8_t textsize(){ return _config.textsize; }
    void clear(){ _clearClock(); }
    inline uint16_t dateSize(){ return _space+ _dateheight; }
    inline uint16_t clockWidth(){ return _clockwidth; }
  private:
  #ifndef DSP_LCD
    Adafruit_GFX &getRealDsp();
  #endif
  protected:
    char  _timebuffer[20]="00:00";
    char _tmp[30], _datebuf[30];
    uint8_t _superfont;
    uint16_t _clockleft, _clockwidth, _timewidth, _dotsleft, _linesleft;
    uint8_t  _clockheight, _timeheight, _dateheight, _space;
    uint16_t _forceflag = 0;
    bool dots = true;
    bool _fullclock;
    psFrameBuffer* _fb=nullptr;
    void _draw();
    void _clear();
    void _reset();
    void _getTimeBounds();
    void _printClock(bool force=false);
    void _clearClock();
    bool _getTime();
    uint16_t _left();
    uint16_t _top();
    void _begin();
};

class BitrateWidget: public Widget {
  public:
    BitrateWidget() {}
    BitrateWidget(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor) { init(bconf, fgcolor, bgcolor); }
    ~BitrateWidget(){}
    using Widget::init;
    void init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor);
    void setBitrate(uint16_t bitrate);
    void setFormat(BitrateFormat format);
  protected:
    BitrateFormat _format;
    char _buf[6];
    uint8_t _charWidth;
    uint16_t _dimension, _bitrate, _textheight;
    void _draw();
    void _clear();
    void _charSize(uint8_t textsize, uint8_t& width, uint16_t& height);
};

class PlayListWidget: public Widget {
  public:
    using Widget::init;
    void init(ScrollWidget* current);
    void drawPlaylist(uint16_t currentItem);
    inline uint16_t itemHeight(){ return _plItemHeight; }
    inline uint16_t currentTop(){ return _plYStart+_plCurrentPos*_plItemHeight; }
  private:
    ScrollWidget* _current;
    uint16_t _plItemHeight, _plTtemsCount, _plCurrentPos;
    int _plYStart;
    uint8_t _fillPlMenu(int from, uint8_t count);
    void _printPLitem(uint8_t pos, const char* item);
    
};


#endif
#endif




