#ifndef widgets_h
#define widgets_h

#include "Arduino.h"
//#include "../../core/config.h"
enum WidgetAlign { WA_LEFT, WA_CENTER, WA_RIGHT };
enum BitrateFormat { BF_UNCNOWN, BF_MP3, BF_AAC, BF_FLAC, BF_OGG, BF_WAV };

typedef struct clipArea {
  uint16_t left; 
  uint16_t top; 
  uint16_t width;  
  uint16_t height;
} clipArea;

struct WidgetConfig {
  uint16_t left; 
  uint16_t top; 
  uint16_t textsize;
  WidgetAlign align;
};

struct ScrollConfig {
  WidgetConfig widget;
  uint16_t buffsize;
  bool uppercase;
  uint16_t width;
  uint16_t startscrolldelay;
  uint8_t scrolldelta;
  uint16_t scrolltime;
};

struct FillConfig {
  WidgetConfig widget;
  uint16_t width;
  uint16_t height;
  bool outlined;
};

struct ProgressConfig {
  uint16_t speed;
  uint16_t width;
  uint16_t barwidth;
};

struct VUBandsConfig {
  uint16_t width;
  uint16_t height;
  uint8_t  space;
  uint8_t  vspace;
  uint8_t  perheight;
  uint8_t  fadespeed;
};

struct MoveConfig {
  uint16_t x;
  uint16_t y;
  int16_t width;
};

struct BitrateConfig {
  WidgetConfig widget;
  uint16_t dimension;
};

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
    uint16_t _realLeft();
};

class FillWidget: public Widget {
  public:
    FillWidget() {}
    FillWidget(FillConfig conf, uint16_t bgcolor) { init(conf, bgcolor); }
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
    VuWidget(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) { init(wconf, bands, vumaxcolor, vumincolor, bgcolor); }
    ~VuWidget();
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
    void draw();
  protected:
    void _draw();
    void _clear();
};

class BitrateWidget: public Widget {
  public:
    BitrateWidget() {}
    BitrateWidget(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor) { init(bconf, fgcolor, bgcolor); }
    ~BitrateWidget(){}
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
};

#endif
