#ifndef DNET_UI
#define DNET_UI

#include "float.h"
#include "smartptr.h"

#include <boost/noncopyable.h>

class UiGlobal : boost::noncopyable {
  UiWidget *focus;
public:
  
  void setFocus(UiWidget *focus);

  void keyinput(const Controller &cnt);
};

class UiWidget : boost::noncopyable {
public:

  // Accessors
  virtual Float2 getDesiredSize() const = 0;
  virtual void repoll_size();

  Float4 getPosition() const;
  void setPosition(const Float4 &pos);

  void setParent(UiWidget *parent);
  const UiWidget *getParent() const;
  UiWidget *getParent();

  // Render
  virtual void render() const = 0;

  // Events
  virtual void tick();
  virtual void keyinput(const Controller &cnt); // this is the main controller only
  virtual void mouseclick(Float2 pos, bool left, bool right);

  // State
  void acquireFocus(UiGlobal *gst);
  void setFocusDepth(int depth);
  int getFocusDepth(void) const;

  Widget();
  virtual ~Widget();
};

// Okay. Helper classes, like centering and scrollable, will simply be classes that do things.
class UiCentering {
  GfxWindow window;
public:
  UiCentering(const Float2 &size);
  ~UiCentering();
};

// Menu will need to scroll based on its actual size and its contents. Let's just do this internal to Menu.
class UiMenu : public UiWidget {
public:
  virtual Float2 getDesiredSize() const;
  virtual void repoll_size();

  virtual void render() const;

  virtual void tick();
  virtual void keyinput(const Controller &cnt);
  virtual void mouseclick(Float2 pos, bool left, bool right);

  void addVerticalItem(const smart_ptr<UiWidget> &widg);
  void addHorizontalItem(const smart_ptr<UiWidget> &widg);

  UiMenu();
};

class UiButton : public UiWidget {
public:
  virtual Float2 getDesiredSize() const;

  virtual void render() const;

  virtual void keyinput(const Controller &cnt);
  virtual void mouseclick(Float2 pos, bool left, bool right);

  UiButton(const string &text, const function<void (void)> &trigger);
};

#endif
