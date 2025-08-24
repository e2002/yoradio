#include "../../core/options.h"
#if DSP_MODEL!=DSP_DUMMY
#include "../dspcore.h"
#include "pages.h"
#include "widgets.h"

void Pager::begin(){

}

void Pager::loop(){
  for(const auto& p: _pages)
    if(p->isActive()) p->loop();
}

Page& Pager::addPage(Page* page, bool setNow){
  _pages.push_back(page);
  if(setNow) setPage(page);
  return *page;
}

bool Pager::removePage(Page* page){
  page->setActive(false);
  dsp.clearDsp();
  auto i = std::find_if(_pages.begin(), _pages.end(), [&page](const Page* pn){ return page == pn; });
  if (i != _pages.end()){
    delete (*i);
    (*i) = nullptr;
    _pages.erase(i);
    return true;
  }
  return false;
  //return _pages.remove(page);
}

void Pager::setPage(Page* page, bool black){
  for(const auto& p: _pages) p->setActive(false);
  dsp.clearDsp(black);
  page->setActive(true);
}


/*******************************************************/

//Page::Page() : _widgets(LinkedList<Widget * >([](Widget * wd) { delete wd;})), _pages(LinkedList<Page*>([](Page* pg){ delete pg; })) {
//  _active = false;
//}

Page::~Page() {
  for (const auto& w : _widgets) removeWidget(w);
  // what about deleting _pages ???
}

void Page::loop() {
  if(_active) for (const auto& w : _widgets) w->loop();
}

Widget& Page::addWidget(Widget* widget) {
  _widgets.push_back(widget);
  widget->setActive(_active, _active);
  return *widget;
}

bool Page::removeWidget(Widget* widget){
  widget->setActive(false, _active);
  auto i = std::find_if(_widgets.begin(), _widgets.end(), [&widget](const Widget* wn){ return widget == wn; });
  if (i != _widgets.end()){
    delete (*i);
    (*i) = nullptr;
    _widgets.erase(i);
    return true;
  }
  return false;

  //return _widgets.remove(widget);
}

Page& Page::addPage(Page* page){
  _pages.push_back(page);
  return *page;
}

bool Page::removePage(Page* page){
  auto i = std::find_if(_pages.begin(), _pages.end(), [&page](const Page* pn){ return page == pn; });
  if (i != _pages.end()){
    delete (*i);
    (*i) = nullptr;
    _pages.erase(i);
    return true;
  }
  return false;
//  return _pages.remove(page);
}

void Page::setActive(bool act) {
  for(const auto& w: _widgets) w->setActive(act);
  for(const auto& p: _pages) p->setActive(act);
  _active = act;
}

bool Page::isActive() {
  return _active;
}

#endif // #if DSP_MODEL!=DSP_DUMMY
