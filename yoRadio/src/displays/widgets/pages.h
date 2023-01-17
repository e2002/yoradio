#ifndef pages_h
#define pages_h

#include "Arduino.h"
#include "../../AsyncWebServer/StringArray.h"

class Page {
  protected:
    LinkedList<Widget*> _widgets;
    LinkedList<Page*> _pages;
    bool _active;
  public:
    Page();
    ~Page();
    void loop();
    Widget& addWidget(Widget* widget);
    bool removeWidget(Widget* widget);
    Page& addPage(Page* page);
    bool removePage(Page* page);
    void setActive(bool act);
    bool isActive();
};

class Pager{
  public:
    Pager() : _pages(LinkedList<Page*>([](Page* pg){ delete pg; })) {}
    void begin();
    void loop();
    Page& addPage(Page* page, bool setNow = false);
    bool removePage(Page* page);
    void setPage(Page* page, bool black=false);
  private:
    LinkedList<Page*> _pages;
    
};


#endif
