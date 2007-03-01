
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
#include "itemdb.h"

/*************
 * VeceditGLC
 */
 
class VeceditGLC : public wxGLCanvas {
private:
  smart_ptr<Closure<> > render_callback;
  smart_ptr<Closure<const MouseInput &, int> > mouse_callback;

  smart_ptr<Callback<ScrollBounds, Float2> > scroll_callback;
  smart_ptr<Closure<Float2> > set_scroll_callback;

  Float2 center;
  float zpp;

  MouseInput mstate;

public:
  
  VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &, int> > &mouse_callback, const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback, const smart_ptr<Closure<Float2> > &set_scroll_callback);
  
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
VeceditGLC::VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &, int> > &mouse_callback,  const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback, const smart_ptr<Closure<Float2> > &set_scroll_callback) : wxGLCanvas(wind, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxVSCROLL | wxHSCROLL | wxALWAYS_SHOW_SB, "GLCanvas", gl_attribList), render_callback(render_callback), mouse_callback(mouse_callback), scroll_callback(scroll_callback), set_scroll_callback(set_scroll_callback) {
  SetScrollbar(wxVERTICAL, 0, 40, 50);
  SetScrollbar(wxHORIZONTAL, 0, 40, 50);
  center = Float2(0, 0);
  zpp = 0.1;
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
  
  render_callback->Run();
  
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
  
  mstate.b[0].newState(event.LeftIsDown());
  mstate.b[1].newState(event.RightIsDown());
  
  mouse_callback->Run(mstate, event.GetWheelRotation());
  
  Refresh();
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

  // toolbar
  wxToolBar *toolbar;
  wxSpinCtrl *grid;
  wxSpinCtrl *rotgrid;

  // properties panel
  wxSpinCtrl *reflects;
  wxRadioBox *snowflake;

  // while idle, the top item on undostack == core, since otherwise there's no way to intercept it *before* a change occurs
  vector<Vecedit> undostack;
  vector<Vecedit> redostack;

  WrapperState wstate;

  void redraw();
  void renderCore() const;
  void setScrollPos(Float2 nps);
  ScrollBounds getScrollBounds(Float2 screenres) const;

  void mouseCore(const MouseInput &mstate, int wheel);

  WrapperState &genwrap();
  void process(const OtherState &inp);
  bool maybeSaveChanges();

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
  void OnDelete(wxCommandEvent &event);

  void OnAbout(wxCommandEvent &event);

  void OnClose(wxCloseEvent &event);

  void OnNewPath(wxCommandEvent &event);
  void OnNewNode(wxCommandEvent &event);
  void OnNewTank(wxCommandEvent &event);
  
  void OnNewPathMenu(wxCommandEvent &event);
  void OnNewNodeMenu(wxCommandEvent &event);
  void OnNewTankMenu(wxCommandEvent &event);

  void OnGridToggle(wxCommandEvent &event);
  void OnGridUpdate(wxCommandEvent &event);
  void OnGridUp(wxSpinEvent &event);
  void OnGridDown(wxSpinEvent &event);
  
  void OnRotGridToggle(wxCommandEvent &event);
  void OnRotGridUpdate(wxCommandEvent &event);
  
  void OnPathReflects(wxCommandEvent &event);
  void OnPathRotation(wxCommandEvent &event);

  void OnSave_dispatch(wxCommandEvent &event);
  void OnSaveas_dispatch(wxCommandEvent &event);
  
  DECLARE_EVENT_TABLE()
};

enum {
  // Menu items
    // Main menu
      ID_New = 1,
      ID_Open,
      ID_Save,
      ID_Saveas,
      ID_Quit,

    // Edit menu
      ID_Undo,
      ID_Redo,
      ID_Cut,
      ID_Copy,
      ID_Paste,
      ID_Delete,

    // Help menu
      ID_About,
  
  // Toolbar items
    ID_NewPath,
    ID_NewPathMenu,
    ID_NewNode,
    ID_NewNodeMenu,
    ID_NewTank,
    ID_NewTankMenu,
    
    ID_GridToggle,
    ID_GridSpinner,
    
    ID_RotGridToggle,
    ID_RotGridSpinner,

  // Property pane items
    ID_PathReflects,
    ID_PathRotation
};

BEGIN_EVENT_TABLE(VeceditWindow, wxFrame)
  EVT_MENU(ID_New, VeceditWindow::OnNew)
  EVT_MENU(ID_Open, VeceditWindow::OnOpen)
  EVT_MENU(ID_Save, VeceditWindow::OnSave_dispatch)
  EVT_MENU(ID_Saveas, VeceditWindow::OnSaveas_dispatch)

  EVT_MENU(ID_Quit, VeceditWindow::OnQuit)

  EVT_MENU(ID_Undo, VeceditWindow::OnUndo)
  EVT_MENU(ID_Redo, VeceditWindow::OnRedo)
  EVT_MENU(ID_Delete, VeceditWindow::OnDelete)

  EVT_MENU(ID_About, VeceditWindow::OnAbout)

  EVT_TOOL(ID_NewPath, VeceditWindow::OnNewPath)
  EVT_TOOL(ID_NewNode, VeceditWindow::OnNewNode)
  EVT_TOOL(ID_NewTank, VeceditWindow::OnNewTank)

  EVT_MENU(ID_NewPathMenu, VeceditWindow::OnNewPathMenu)
  EVT_MENU(ID_NewNodeMenu, VeceditWindow::OnNewNodeMenu)
  EVT_MENU(ID_NewTankMenu, VeceditWindow::OnNewTankMenu)

  EVT_TOOL(ID_GridToggle, VeceditWindow::OnGridToggle)
  EVT_TEXT(ID_GridSpinner, VeceditWindow::OnGridUpdate)
  EVT_SPIN_UP(ID_GridSpinner, VeceditWindow::OnGridUp)
  EVT_SPIN_DOWN(ID_GridSpinner, VeceditWindow::OnGridDown)
  
  EVT_TOOL(ID_RotGridToggle, VeceditWindow::OnRotGridToggle)
  EVT_TEXT(ID_RotGridSpinner, VeceditWindow::OnRotGridUpdate)

  EVT_TEXT(ID_PathReflects, VeceditWindow::OnPathReflects)
  EVT_RADIOBOX(ID_PathRotation, VeceditWindow::OnPathRotation)

  EVT_CLOSE(VeceditWindow::OnClose)
END_EVENT_TABLE()

const string veceditname =  "D-Net Vecedit2";

void VeceditWindow::renderCore() const {
  initFrame();
  clearFrame(Color(0, 0, 0));

  WrapperState ws = wstate; // stupid const >:( TODO: suck less
  ws.ui.newPath = toolbar->GetToolState(ID_NewPath);
  ws.ui.newNode = toolbar->GetToolState(ID_NewNode);
  ws.ui.newTank = toolbar->GetToolState(ID_NewTank);
  core.render(ws);
  
  deinitFrame();
}

void VeceditWindow::setScrollPos(Float2 nps) {
  wstate.center = nps;
}

ScrollBounds VeceditWindow::getScrollBounds(Float2 screenres) const {
  return core.getScrollBounds(screenres, wstate);
}

void VeceditWindow::mouseCore(const MouseInput &mstate, int wheel) {
  MouseInput ms = mstate;
  
  ms.pos = (ms.pos - Float2(getResolutionX() / 2, getResolutionY() / 2)) * wstate.zpp + wstate.center;
  
  bool zoomed = false;
  
  if(wheel != 0) {
    const float mult = pow(1.2, wheel / 120.0);
    
    wstate.zpp /= mult;
    
    if(mult > 1.0)
      wstate.center = ms.pos - (ms.pos - wstate.center) / mult;
    
    zoomed = true;
  }
  
  OtherState ost = core.mouse(ms, genwrap());
  if(zoomed)
    ost.redraw = true;
  process(ost);
}

VeceditWindow::VeceditWindow() : wxFrame((wxFrame *)NULL, -1, veceditname, wxDefaultPosition, wxSize(800, 600)) {
  wxMenuBar *menuBar = new wxMenuBar;
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append(ID_New, "&New\tCtrl-N");
    menuFile->Append(ID_Open, "&Open...\tCtrl-O");
    menuFile->Append(ID_Save, "&Save\tCtrl+S");
    menuFile->Append(ID_Saveas, "Save &as...\tCtrl+Shift+S");
    
    menuFile->AppendSeparator();
    
    menuFile->Append(ID_Quit, "E&xit");
    
    menuBar->Append(menuFile, "&File");
  }
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append(ID_Undo, "&Undo\tCtrl+Z");
    menuFile->Append(ID_Redo, "&Redo\tCtrl+Y");
    
    menuFile->AppendSeparator();
    
    menuFile->Append(ID_Cut, "Cu&t\tCtrl+X");
    menuFile->Append(ID_Copy, "&Copy\tCtrl+C");
    menuFile->Append(ID_Paste, "&Paste\tCtrl+V");
    menuFile->Append(ID_Delete, "&Delete\tDel");
    
    menuFile->AppendSeparator();
    
    menuFile->Append(ID_NewPathMenu, "Add path\tP");
    menuFile->Append(ID_NewNodeMenu, "Add node\tA");
    menuFile->Append(ID_NewTankMenu, "Add tank\tT");
    
    menuBar->Append(menuFile, "&Edit");
  }
  
  {
    wxMenu *menuFile = new wxMenu;
    
    menuFile->Append(ID_About, "&About...");
    
    menuBar->Append(menuFile, "&Help");
  }
  
  SetMenuBar(menuBar);
  
  // We make this first so it gets redrawn first, which reduces flicker a bit
  CreateStatusBar();
  SetStatusText("borf borf borf");
  
  glc = new VeceditGLC(this, NewFunctor(this, &VeceditWindow::renderCore), NewFunctor(this, &VeceditWindow::mouseCore), NewFunctor(this, &VeceditWindow::getScrollBounds), NewFunctor(this, &VeceditWindow::setScrollPos));
  wxNotebook *note = new wxNotebook(this, wxID_ANY);
  note->SetMinSize(wxSize(150, 150));
  
  {
    wxNotebookPage *pathprops = new wxPanel(note, wxID_ANY);
    
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(pathprops, wxID_ANY, "Path reflections"));
    sizer->Add(reflects = new wxSpinCtrl(pathprops, ID_PathReflects, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 16384));
    
    sizer->Add(0, 20);
    
    {
      wxString options[] = { "Buzzsaw", "Snowflake" };
      
      sizer->Add(snowflake = new wxRadioBox(pathprops, ID_PathRotation, "Rotation mode", wxDefaultPosition, wxDefaultSize, ARRAY_SIZE(options), options, 1));
    }
    
    wxBoxSizer *internal = new wxBoxSizer(wxVERTICAL);
    internal->Add(sizer, wxSizerFlags().Border());
    
    pathprops->SetSizer(internal);
    
    note->AddPage(pathprops, "Props");
  }

  note->AddPage(new wxNotebookPage(note, wxID_ANY), "Globals");
  
  toolbar = new wxToolBar(this, wxID_ANY);
  toolbar->AddTool(ID_NewPath, "add path", wxBitmap("vecedit/addpath.png", wxBITMAP_TYPE_PNG), "Add a new path", wxITEM_CHECK);
  toolbar->AddTool(ID_NewNode, "add node", wxBitmap("vecedit/addnode.png", wxBITMAP_TYPE_PNG), "Add a new node", wxITEM_CHECK);
  toolbar->AddTool(ID_NewTank, "add tank", wxBitmap("vecedit/addtank.png", wxBITMAP_TYPE_PNG), "Add a new tank", wxITEM_CHECK);
  
  toolbar->AddSeparator();
  toolbar->AddTool(ID_GridToggle, "toggle grid", wxBitmap("vecedit/grid.png", wxBITMAP_TYPE_PNG), "Activate grid lock", wxITEM_CHECK);
  toolbar->AddControl(grid = new wxSpinCtrl(toolbar, ID_GridSpinner, "16", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 16384));
  wstate.grid = -1;
  
  toolbar->AddSeparator();
  toolbar->AddTool(ID_RotGridToggle, "toggle rotation grid", wxBitmap("vecedit/rotgrid.png", wxBITMAP_TYPE_PNG), "Activate rotation grid lock", wxITEM_CHECK);
  toolbar->ToggleTool(ID_RotGridToggle, 1);
  toolbar->AddControl(rotgrid = new wxSpinCtrl(toolbar, ID_RotGridSpinner, "8", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 16384));
  wstate.rotgrid = 8;
  
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
  undostack.clear();
  redostack.clear();
  undostack.push_back(core);
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
    undostack.clear();
    redostack.clear();
    undostack.push_back(core);
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
  if(undostack.size() >= 2) {
    undostack.pop_back();
    redostack.push_back(core);
    core = undostack.back();
    glc->Refresh();
  }
}
void VeceditWindow::OnRedo(wxCommandEvent &event) {
  if(redostack.size()) {
    core = redostack.back();
    redostack.pop_back();
    undostack.push_back(core);
    glc->Refresh();
  }
}

void VeceditWindow::OnDelete(wxCommandEvent &event) {
  process(core.del(genwrap()));
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
  if(event.IsSelection()) {
    toolbar->ToggleTool(ID_NewNode, 0);
  }
}

void VeceditWindow::OnNewNode(wxCommandEvent &event) {
  if(event.IsSelection()) {
    toolbar->ToggleTool(ID_NewPath, 0);
  }
}

void VeceditWindow::OnNewTank(wxCommandEvent &event) {
  if(event.IsSelection()) {
    toolbar->ToggleTool(ID_NewTank, 0);
  }
}

void VeceditWindow::OnNewPathMenu(wxCommandEvent &event) {
  toolbar->ToggleTool(ID_NewPath, !toolbar->GetToolState(ID_NewPath));
  toolbar->ToggleTool(ID_NewNode, 0);
  toolbar->ToggleTool(ID_NewTank, 0);
}

void VeceditWindow::OnNewNodeMenu(wxCommandEvent &event) {
  toolbar->ToggleTool(ID_NewNode, !toolbar->GetToolState(ID_NewNode));
  toolbar->ToggleTool(ID_NewPath, 0);
  toolbar->ToggleTool(ID_NewTank, 0);
}

void VeceditWindow::OnNewTankMenu(wxCommandEvent &event) {
  toolbar->ToggleTool(ID_NewTank, !toolbar->GetToolState(ID_NewTank));
  toolbar->ToggleTool(ID_NewNode, 0);
  toolbar->ToggleTool(ID_NewPath, 0);
}

void VeceditWindow::OnGridToggle(wxCommandEvent &event) {
  if(event.IsSelection()) {
    wstate.grid = grid->GetValue();
  } else {
    wstate.grid = -1;
  }
  glc->Refresh();
}
void VeceditWindow::OnGridUpdate(wxCommandEvent &event) {
  if(toolbar->GetToolState(ID_GridToggle)) {
    wstate.grid = event.GetInt();
    glc->Refresh();
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

void VeceditWindow::OnRotGridToggle(wxCommandEvent &event) {
  if(event.IsSelection()) {
    wstate.rotgrid = rotgrid->GetValue();
  } else {
    wstate.rotgrid = -1;
  }
  glc->Refresh();
}
void VeceditWindow::OnRotGridUpdate(wxCommandEvent &event) {
  if(toolbar->GetToolState(ID_RotGridToggle)) {
    wstate.rotgrid = event.GetInt();
    glc->Refresh();
  }
}


void VeceditWindow::OnPathReflects(wxCommandEvent &event) {
  process(core.rotate(event.GetInt(), genwrap()));
}
void VeceditWindow::OnPathRotation(wxCommandEvent &event) {
  process(core.snowflake(event.GetInt(), genwrap()));
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

WrapperState &VeceditWindow::genwrap() {
  wstate.ui.newPath = toolbar->GetToolState(ID_NewPath);
  wstate.ui.newNode = toolbar->GetToolState(ID_NewNode);
  wstate.ui.newTank = toolbar->GetToolState(ID_NewTank);
  return wstate;
}
void VeceditWindow::process(const OtherState &ost) {
  if(ost.cursor != CURSOR_UNCHANGED)
    glc->SetGLCCursor(ost.cursor);
  if(ost.redraw)
    redraw();
  if(ost.snapshot) {
    redostack.clear();
    undostack.push_back(core);
  }
  
  toolbar->ToggleTool(ID_NewPath, ost.ui.newPath);
  toolbar->ToggleTool(ID_NewNode, ost.ui.newNode);
  toolbar->ToggleTool(ID_NewTank, ost.ui.newTank);
  
  if(!ost.hasPathProperties) {
    reflects->Disable();
    snowflake->Disable();
  } else {
    reflects->Enable();
    snowflake->Enable();
    reflects->SetValue(ost.divisions);
    snowflake->SetSelection(ost.snowflakey);
  }
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
  virtual int OnExit();
};

IMPLEMENT_APP(VeceditMain)

bool VeceditMain::OnInit() {
  set_exename("vecedit.exe");
  
  initItemdb();
  initGfx();
  
  wxImage::AddHandler(new wxPNGHandler);
  
  wxFrame *frame = new VeceditWindow();
  frame->Show(true);
  SetTopWindow(frame);
  return true;
}

int VeceditMain::OnExit() {
  return wxApp::OnExit();
}

