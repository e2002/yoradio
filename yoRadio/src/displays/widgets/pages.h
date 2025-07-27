#ifndef pages_h
#define pages_h

#include <list>


class Page {
  protected:
    std::list<Widget*> _widgets;
    std::list<Page*> _pages;
    bool _active;
  public:
    //Page();
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
    //Pager() : _pages(std::list<Page*>([](Page* pg){ delete pg; })) {}
    void begin();
    void loop();
    Page& addPage(Page* page, bool setNow = false);
    bool removePage(Page* page);
    void setPage(Page* page, bool black=false);
  private:
    std::list<Page*> _pages;
    
};


#endif
