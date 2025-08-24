#ifndef psframebuffer_h
#define psframebuffer_h

#ifdef PSFBUFFER
#include <Adafruit_GFX.h>

class  psFrameBuffer : public Adafruit_GFX {
  public:
    psFrameBuffer(int16_t w, int16_t h):Adafruit_GFX(w, h){ setTextWrap(false); cp437(true); }
    ~psFrameBuffer(){ freeBuffer(); }
    bool ready() { return _ready; }
    
    void freeBuffer(){
      _ready = false;
      if(buffer) {
        _dspl->fillRect(_ll, _tt, _ww, _hh, _bgcolor);
        free(buffer);
      }
      buffer = nullptr;
    }
    bool begin(yoDisplay *dspl, int16_t l, int16_t t, int16_t w, int16_t h, uint16_t bgcolor = 0){
      _dspl = dspl; _ll = l; _tt = t; _ww = w; _hh = h; _bgcolor = bgcolor;
      _createBuffer();
      return _ready;
    }
    void move(int16_t l, int16_t t, int16_t w, int16_t h){
      _ll = l; _tt = t; _ww = w; _hh = h;
      freeBuffer();
      _createBuffer();
    }
    void drawPixel( int16_t x, int16_t y, uint16_t color){
      if (x < 0 || x >= _ww || y < 0 || y >= _hh) return;
      if(!buffer) return;
      buffer[x + y * _ww] = color;
    }
    void display(){
      if(!buffer) return;
      _dspl->startWrite();
      _dspl->setAddrWindow(_ll, _tt, _ww, _hh);
      _dspl->writePixels((uint16_t*)buffer,  _ww * _hh);
      _dspl->endWrite();
    }
    void clear(){
      if(!buffer) return;
      fillRect(0, 0, _ww, _hh, _bgcolor);
    }
    int16_t width(){ return _ww; }
    int16_t height(){ return _hh; }
  private:
    int16_t _ll, _tt, _ww, _hh;
    yoDisplay *_dspl;
    uint16_t *buffer=nullptr;
    bool _ready = false;
    uint16_t _bgcolor;
    void _createBuffer(){
      if(USE_FBUFFER){
        if(psramInit())
          buffer = (uint16_t*) ps_calloc(_hh * _ww, sizeof(uint16_t));
        #ifdef SFBUFFER
        else
          buffer = (uint16_t*) calloc(_hh * _ww, sizeof(uint16_t));
        #endif
      }
      if(buffer){
        for (int i = 0; i < _hh * _ww; i++)
          buffer[i] = 0;
        _ready = true;
      }
    }
};
#else
struct psFrameBuffer {
  bool ready() { return false; }
  void display() {}
};
#endif //#ifdef PSFBUFFER


#endif
