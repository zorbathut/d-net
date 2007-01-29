
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/notebook.h>
#include <wx/toolbar.h>
#include <wx/tbarbase.h>
#include <wx/spinctrl.h>

#include "debug.h"
#include "gfx.h"
#include "vecedit.h"
#include "os.h"
#include "util.h"

/*************
 * VeceditGLC
 */
 
class VeceditGLC : public wxGLCanvas {
private:
  smart_ptr<Closure<> > render_callback;
  smart_ptr<Closure<const MouseInput &> > mouse_callback;

  smart_ptr<Callback<ScrollBounds, Float2> > scroll_callback;
  smart_ptr<Closure<Float2> > set_scroll_callback;

  MouseInput mstate;

public:
  
  VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &> > &mouse_callback, const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback, const smart_ptr<Closure<Float2> > &set_scroll_callback);
  
  void OnPaint(wxPaintEvent &event);
  void OnSize(wxSizeEvent &event);
  void OnEraseBackground(wxEraseEvent &event);

  void OnMouse(wxMouseEvent &event);
  void OnScroll(wxScrollWinEvent &event);

  void SetScrollBars();
  void SetGLCCursor(Cursor curse);

  ScrollBounds getSB() const;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(VeceditGLC, wxGLCanvas)
  EVT_PAINT(VeceditGLC::OnPaint)
  EVT_SIZE(VeceditGLC::OnSize)
  EVT_ERASE_BACKGROUND(VeceditGLC::OnEraseBackground) 
  EVT_MOUSE_EVENTS(VeceditGLC::OnMouse)
  EVT_SCROLLWIN(VeceditGLC::OnScroll)
END_EVENT_TABLE()

int gl_attribList[] = {
  WX_GL_RGBA,
  WX_GL_STENCIL_SIZE, 1,
  WX_GL_DOUBLEBUFFER,
  0
};
VeceditGLC::VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &> > &mouse_callback,  const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback, const smart_ptr<Closure<Float2> > &set_scroll_callback) : wxGLCanvas(wind, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxVSCROLL | wxHSCROLL | wxALWAYS_SHOW_SB, "GLCanvas", gl_attribList), render_callback(render_callback), mouse_callback(mouse_callback), scroll_callback(scroll_callback), set_scroll_callback(set_scroll_callback) {
  SetScrollbar(wxVERTICAL, 0, 40, 50);
  SetScrollbar(wxHORIZONTAL, 0, 40, 50);
};

void VeceditGLC::OnPaint(wxPaintEvent& event) {
  wxPaintDC dc(this);
  
  SetCurrent();
  
  {
    int w, h;
    GetClientSize(&w, &h);
    glViewport(0, 0, (GLint) w, (GLint) h); 
    
    updateResolution((float)w / h);
  }
  
  //wxYield();
  
  initFrame();
  clearFrame(Color(0, 0, 0));
  
  render_callback->Run();
  
  deinitFrame();
  SwapBuffers();
}

void VeceditGLC::OnSize(wxSizeEvent& event) {
  Refresh();
  SetScrollBars();
}

void VeceditGLC::OnEraseBackground(wxEraseEvent& event) {
}

void VeceditGLC::OnMouse(wxMouseEvent &event) {
  event.Skip();
  
  mstate.pos = Float2(event.GetX(), event.GetY());
  
  mstate.dw = event.GetWheelRotation();
  
  mstate.b[0].newState(event.LeftIsDown());
  mstate.b[1].newState(event.RightIsDown());
  mstate.b[2].newState(event.MiddleIsDown());
  
  mouse_callback->Run(mstate);
}

void VeceditGLC::OnScroll(wxScrollWinEvent &event) {
  const ScrollBounds bounds = getSB();
  
  int dir = event.GetOrientation();
  int cpos = GetScrollPos(dir);
  int thumbsize = GetScrollThumb(dir);
  
  if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP)
    cpos -= thumbsize * 4 / 5;
  if(event.GetEventType() == wxEVT_SCROLLWIN_PAGEDOWN)
    cpos += thumbsize * 4 / 5;
  
  if(event.GetEventType() == wxEVT_SCROLLWIN_THUMBTRACK || event.GetEventType() == wxEVT_SCROLLWIN_THUMBRELEASE)
    cpos = event.GetPosition();
  
  if(cpos < 0)
    cpos = 0;
  if(cpos + thumbsize > GetScrollRange(dir))
    cpos = GetScrollRange(dir) - thumbsize;
  
  if(cpos == GetScrollPos(dir))
    return;
  
  SetScrollbar(dir, cpos, thumbsize, GetScrollRange(dir));
  
  if(dir == wxVERTICAL) {
    set_scroll_callback->Run(Float2(bounds.currentwindow.midpoint().x, (GetScrollPos(wxVERTICAL) + GetScrollThumb(wxVERTICAL) / 2.0) / GetScrollRange(wxVERTICAL) * bounds.objbounds.span_y() + bounds.objbounds.sy));
  } else {
    set_scroll_callback->Run(Float2((GetScrollPos(wxHORIZONTAL) + GetScrollThumb(wxHORIZONTAL) / 2.0) / GetScrollRange(wxHORIZONTAL) * bounds.objbounds.span_x() + bounds.objbounds.sx, bounds.currentwindow.midpoint().y));
  }
  
  Refresh();
}

void VeceditGLC::SetScrollBars() {
  const ScrollBounds sb = getSB();
  
  const int maxv = 10000;
  
  const float xscale = maxv / sb.objbounds.span_x();
  const float yscale = maxv / sb.objbounds.span_y();
  
  SetScrollbar(wxHORIZONTAL, (int)((sb.currentwindow.sx - sb.objbounds.sx) * xscale), (int)(sb.currentwindow.span_x() * xscale), maxv);
  SetScrollbar(wxVERTICAL, (int)((sb.currentwindow.sy - sb.objbounds.sy) * yscale), (int)(sb.currentwindow.span_y() * yscale), maxv);
}
void VeceditGLC::SetGLCCursor(Cursor curse) {
  int cid;
  if(curse == CURSOR_NORMAL) {
    cid = wxCURSOR_ARROW;
  } else if(curse == CURSOR_CROSS) {
    cid = wxCURSOR_HAND;  // grr
  } else if(curse == CURSOR_HAND) {
    cid = wxCURSOR_HAND;
  } else {
    CHECK(0);
  }
  
  SetCursor(wxCursor(cid));
}

ScrollBounds VeceditGLC::getSB() const {
  int w, h;
  GetClientSize(&w, &h);
  return scroll_callback->Run(Float2(w, h));
}

/*************
 * VeceditWindow
 */

class VeceditWindow : public wxFrame {
private:
  Vecedit core;

  VeceditGLC *glc;
  string filename;

  wxToolBar *toolbar;
  wxSpinCtrl *grid;

public:
  
  VeceditWindow();
  ~VeceditWindow();

  void OnNew(wxCommandEvent &event);
  void OnOpen(wxCommandEvent &event);
  bool OnSave();
  bool OnSaveas();
  void OnQuit(wxCommandEvent &event);

  void OnUndo(wxCommandEvent &event);
  void OnRedo(wxCommandEvent &event);

  void OnAbout(wxCommandEvent &event);

  void OnClose(wxCloseEvent &event);

  void OnNewPath(wxCommandEvent &event);

  void OnGridToggle(wxCommandEvent &event);
  void OnGridUpdate(wxSpinEvent &event);
  void OnGridUp(wxSpinEvent &event);
  void OnGridDown(wxSpinEvent &event);

  void OnSave_dispatch(wxCommandEvent &event);
  void OnSaveas_dispatch(wxCommandEvent &event);

  void redraw();
  void mouse(const MouseInput &minp);
  void process(const OtherState &inp);
  bool maybeSaveChanges();
  
  DECLARE_EVENT_TABLE()
};

enum {
  ID_New = 1,
  ID_Open,
  ID_Save,
  ID_Saveas,
  ID_Quit,
  
  ID_Undo,
  ID_Redo,
  ID_Cut,
  ID_Copy,
  ID_Paste,
  ID_Delete,
  
  ID_About,
  
  ID_NewPath,
  
  ID_GridToggle,
  ID_GridSpinner
};

BEGIN_EVENT_TABLE(VeceditWindow, wxFrame)
  EVT_MENU(ID_New, VeceditWindow::OnNew)
  EVT_MENU(ID_Open, VeceditWindow::OnOpen)
  EVT_MENU(ID_Save, VeceditWindow::OnSave_dispatch)
  EVT_MENU(ID_Saveas, VeceditWindow::OnSaveas_dispatch)

  EVT_MENU(ID_Quit, VeceditWindow::OnQuit)

  EVT_MENU(ID_Undo, VeceditWindow::OnUndo)
  EVT_MENU(ID_Redo, VeceditWindow::OnRedo)

  EVT_MENU(ID_About, VeceditWindow::OnAbout)

  EVT_TOOL(ID_NewPath, VeceditWindow::OnNewPath)

  EVT_TOOL(ID_GridToggle, VeceditWindow::OnGridToggle)
  EVT_SPINCTRL(ID_GridSpinner, VeceditWindow::OnGridUpdate)
  EVT_SPIN_UP(ID_GridSpinner, VeceditWindow::OnGridUp)
  EVT_SPIN_DOWN(ID_GridSpinner, VeceditWindow::OnGridDown)

  EVT_CLOSE(VeceditWindow::OnClose)
END_EVENT_TABLE()

const string veceditname =  "D-Net Vecedit2";

VeceditWindow::VeceditWindow() : wxFrame((wxFrame *)NULL, -1, veceditname, wxDefaultPosition, wxSize(800, 600)) {
  wxMenuBar *menuBar = new wxMenuBar;
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append( ID_New, "&New" );
    menuFile->Append( ID_Open, "&Open..." );
    menuFile->Append( ID_Save, "&Save" );
    menuFile->Append( ID_Saveas, "Save &as..." );
    
    menuFile->AppendSeparator();
    
    menuFile->Append( ID_Quit, "E&xit" );
    
    menuBar->Append( menuFile, "&File" );
  }
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append( ID_Undo, "&Undo\tCtrl+Z" );
    menuFile->Append( ID_Redo, "&Redo\tCtrl+Y" );
    
    menuFile->AppendSeparator();
    
    menuFile->Append( ID_Cut, "Cu&t\tCtrl+X" );
    menuFile->Append( ID_Copy, "&Copy\tCtrl+C" );
    menuFile->Append( ID_Paste, "&Paste\tCtrl+V" );
    menuFile->Append( ID_Delete, "&Delete\tDel" );
    
    menuBar->Append( menuFile, "&Edit" );
  }
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append( ID_About, "&About..." );
    
    menuBar->Append( menuFile, "&Help" );
  }
  
  SetMenuBar( menuBar );

  // We make this first so it gets redrawn first, which reduces flicker a bit
  CreateStatusBar();
  SetStatusText("borf borf borf");
  
  glc = new VeceditGLC(this, NewFunctor(&core, &Vecedit::render), NewFunctor(this, &VeceditWindow::mouse), NewFunctor(&core, &Vecedit::getScrollBounds), NewFunctor(&core, &Vecedit::setScrollPos));
  wxNotebook *note = new wxNotebook(this, wxID_ANY);
  note->SetMinSize(wxSize(150, 0));
  note->AddPage(new wxNotebookPage(note, wxID_ANY), "Props");
  note->AddPage(new wxNotebookPage(note, wxID_ANY), "Globals");
  
  toolbar = new wxToolBar(this, wxID_ANY);
  toolbar->AddTool(ID_NewPath, "add shit", wxBitmap("vecedit/plus.png", wxBITMAP_TYPE_PNG), "Add a new path");
  toolbar->AddSeparator();
  toolbar->AddTool(ID_GridToggle, "toggle grid", wxBitmap("vecedit/grid.png", wxBITMAP_TYPE_PNG), "Activate grid lock", wxITEM_CHECK);
  toolbar->AddControl(grid = new wxSpinCtrl(toolbar, ID_GridSpinner, "16"));
  toolbar->Realize();
  toolbar->SetMinSize(wxSize(0, 25));  // this shouldn't be needed >:(
  
  wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
  sizer->Add(glc, 1, wxEXPAND);
  sizer->Add(note, 0, wxEXPAND);
  
  wxBoxSizer *vertsizer = new wxBoxSizer(wxVERTICAL);
  vertsizer->Add(toolbar, 0, wxEXPAND);
  vertsizer->Add(sizer, 1, wxEXPAND);
  
  SetSizer(vertsizer);
  
  redraw();
  
  core.registerEmergencySave();
}

VeceditWindow::~VeceditWindow() {
  core.unregisterEmergencySave();
}

void VeceditWindow::OnNew(wxCommandEvent& event) {
  if(!maybeSaveChanges())
    return;
  core.clear();
  filename = "";
  
  redraw();
}
void VeceditWindow::OnOpen(wxCommandEvent& event) {
  if(!maybeSaveChanges())
    return;
  wxFileDialog wxfd(this, "Open File", veceditname, "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_OPEN);
  if(wxfd.ShowModal() == wxID_OK) {
    filename = wxfd.GetPath();
    core.load(filename);
  }
  
  redraw();
}
bool VeceditWindow::OnSave() {
  if(filename.empty()) {
    return OnSaveas();
  } else {
    if(!core.save(filename))
      wxMessageBox("Error saving! File is not saved!", "ba-weep-gra-na-weep-ninny-bong", wxOK | wxICON_INFORMATION, this);
    return true;
  }
}
bool VeceditWindow::OnSaveas() {
  wxFileDialog wxfd(this, "Save File", veceditname, "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if(wxfd.ShowModal() == wxID_OK) {
    if(core.save((string)wxfd.GetPath())) {
      filename = wxfd.GetPath();
    } else {
      wxMessageBox("Error saving! File is not saved!", "ba-weep-gra-na-weep-ninny-bong", wxOK | wxICON_INFORMATION, this);
    }
    return true;
  } else {
    return false;
  }
}

void VeceditWindow::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  if(!maybeSaveChanges())
    return;
  Close(TRUE);
}

void VeceditWindow::OnUndo(wxCommandEvent &event) {
  dprintf("Undo\n");
}
void VeceditWindow::OnRedo(wxCommandEvent &event) {
  dprintf("Redo\n");
}

void VeceditWindow::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("What is D-Net Vecedit2? We just don't know.", "About D-Net Vecedit2", wxOK | wxICON_INFORMATION, this);
}
void VeceditWindow::OnClose(wxCloseEvent &event) {
  dprintf("enclose\n");
  if(!maybeSaveChanges()) {
    if(!event.CanVeto()) {
      wxMessageBox("Cannot cancel closing, saving to c:\\backup.dv2", "ba-weep-gra-na-weep-ninny-bong", wxOK);
      core.save("c:\\backup.dv2");
      this->Destroy();
    } else {
      event.Veto();
    }
  } else {
    dprintf("destroy!\n");
    bool rv = this->Destroy();
    dprintf("crush! %d\n", rv);
  }
}

void VeceditWindow::OnNewPath(wxCommandEvent &event) {
  dprintf("new path\n");
}

void VeceditWindow::OnGridToggle(wxCommandEvent &event) {
  if(event.IsSelection()) {
    process(core.gridupd(grid->GetValue()));
  } else {
    process(core.gridupd(-1));
  }
}
void VeceditWindow::OnGridUpdate(wxSpinEvent &event) {
  if(toolbar->GetToolState(ID_GridToggle)) {
    process(core.gridupd(event.GetPosition()));
  }
}
// The +'s and -'s are kind of dumb and shouldn't exist. Nevertheless, they do.
void VeceditWindow::OnGridUp(wxSpinEvent &event) {
  grid->SetValue(grid->GetValue() * 2 - 1);
}
void VeceditWindow::OnGridDown(wxSpinEvent &event) {
  if(grid->GetValue() <= 1)
    grid->SetValue(1 + 1);
  else
    grid->SetValue(grid->GetValue() / 2 + 1);
}

void VeceditWindow::OnSave_dispatch(wxCommandEvent& event) {
  OnSave();
}
void VeceditWindow::OnSaveas_dispatch(wxCommandEvent& event) {
  OnSaveas();
}

void VeceditWindow::redraw() {
  glc->Refresh();
  glc->SetScrollBars();
}

void VeceditWindow::mouse(const MouseInput &minp) {
  process(core.mouse(minp));
}

void VeceditWindow::process(const OtherState &ost) {
  if(ost.cursor != CURSOR_UNCHANGED)
    glc->SetGLCCursor(ost.cursor);
  if(ost.redraw)
    glc->Refresh();
}

bool VeceditWindow::maybeSaveChanges() {
  if(core.changed()) {
    int saveit = wxMessageBox(StringPrintf("The text in %s has changed.\n\nDo you want to save the changes?", filename.c_str()), veceditname, wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
    if(saveit == wxCANCEL)
      return false;
    if(saveit == wxYES)
      return OnSave();
  }
  return true;
}

/*************
 * VeceditMain
 */

class VeceditMain: public wxApp {
  virtual bool OnInit();
};

IMPLEMENT_APP(VeceditMain)

bool VeceditMain::OnInit() {
  set_exename("vecedit.exe");
  
  wxImage::AddHandler(new wxPNGHandler);
  
  initGfx();
  
  VeceditWindow *frame = new VeceditWindow();
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}
