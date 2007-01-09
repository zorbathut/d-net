
#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/notebook.h>
#include <wx/toolbar.h>
#include <wx/tbarbase.h>

#include "debug.h"
#include "gfx.h"
#include "vecedit.h"

/*************
 * VeceditGLC
 */
 
class VeceditGLC : public wxGLCanvas {
private:
  smart_ptr<Closure<> > render_callback;
  smart_ptr<Closure<const MouseInput &> > mouse_callback;
  smart_ptr<Callback<ScrollBounds, Float2> > scroll_callback;
  MouseInput mstate;

public:
  
  VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &> > &mouse_callback, const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback);
  
  void OnPaint(wxPaintEvent &event);
  void OnSize(wxSizeEvent &event);
  void OnEraseBackground(wxEraseEvent &event);

  void OnMouse(wxMouseEvent &event);

  void SetScrollBars();

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(VeceditGLC, wxGLCanvas)
  EVT_PAINT(VeceditGLC::OnPaint)
  EVT_SIZE(VeceditGLC::OnSize)
  EVT_ERASE_BACKGROUND(VeceditGLC::OnEraseBackground) 
  EVT_MOUSE_EVENTS(VeceditGLC::OnMouse)
END_EVENT_TABLE()

int gl_attribList[] = {
  WX_GL_RGBA,
  WX_GL_STENCIL_SIZE, 1,
  WX_GL_DOUBLEBUFFER,
  0
};
VeceditGLC::VeceditGLC(wxWindow *wind, const smart_ptr<Closure<> > &render_callback, const smart_ptr<Closure<const MouseInput &> > &mouse_callback,  const smart_ptr<Callback<ScrollBounds, Float2> > &scroll_callback) : wxGLCanvas(wind, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxVSCROLL | wxHSCROLL | wxALWAYS_SHOW_SB, "GLCanvas", gl_attribList), render_callback(render_callback), mouse_callback(mouse_callback), scroll_callback(scroll_callback) {
  SetScrollbar(wxVERTICAL, 0, 50, 50);
  SetScrollbar(wxHORIZONTAL, 0, 50, 50);
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
  
  
  //SetScrollbarInfo(wxVERTICAL, 
}

void VeceditGLC::OnSize(wxSizeEvent& event) {
  Refresh();
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

/*************
 * VeceditWindow
 */

class VeceditWindow : public wxFrame {
private:
  Vecedit core;

  VeceditGLC *glc;
  string filename;

public:
  
  VeceditWindow();
  
  void OnNew(wxCommandEvent &event);
  void OnOpen(wxCommandEvent &event);
  bool OnSave();
  bool OnSaveas();

  void OnQuit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
  void OnClose(wxCloseEvent &event);

  void OnSave_dispatch(wxCommandEvent &event);
  void OnSaveas_dispatch(wxCommandEvent &event);

  void redraw();
  bool maybeSaveChanges();
  
  DECLARE_EVENT_TABLE()
};

enum {
  ID_New = 1,
  ID_Open,
  ID_Save,
  ID_Saveas,
  ID_Quit,
  ID_About,
};

BEGIN_EVENT_TABLE(VeceditWindow, wxFrame)
  EVT_MENU(ID_New, VeceditWindow::OnNew)
  EVT_MENU(ID_Open, VeceditWindow::OnOpen)
  EVT_MENU(ID_Save, VeceditWindow::OnSave_dispatch)
  EVT_MENU(ID_Saveas, VeceditWindow::OnSaveas_dispatch)

  EVT_MENU(ID_Quit, VeceditWindow::OnQuit)
  EVT_MENU(ID_About, VeceditWindow::OnAbout)
  EVT_CLOSE(VeceditWindow::OnClose)
END_EVENT_TABLE()

const string veceditname =  "D-Net Vecedit2";

VeceditWindow::VeceditWindow() : wxFrame((wxFrame *)NULL, -1, veceditname, wxDefaultPosition, wxSize(800, 600)), core(NewFunctor(this, &VeceditWindow::redraw)) {
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
    
    menuFile->Append( ID_About, "&About..." );
    
    menuBar->Append( menuFile, "&Help" );
  }
  
  SetMenuBar( menuBar );

  // We make this first so it gets redrawn first, which reduces flicker a bit
  CreateStatusBar();
  SetStatusText("borf borf borf");
  
  glc = new VeceditGLC(this, NewFunctor(&core, &Vecedit::render), NewFunctor(&core, &Vecedit::mouse), NewFunctor(&core, &Vecedit::getScrollBounds));
  wxNotebook *note = new wxNotebook(this, wxID_ANY);
  note->SetMinSize(wxSize(150, 0));
  note->AddPage(new wxNotebookPage(this, wxID_ANY), "Props");
  note->AddPage(new wxNotebookPage(this, wxID_ANY), "Globals");
  
  wxToolBar *tool = new wxToolBar(this, wxID_ANY);
  tool->AddTool(wxID_ANY, "add shit", wxBitmap("vecedit/plus.png", wxBITMAP_TYPE_PNG));
  tool->Realize();
  tool->SetMinSize(wxSize(0, 25));  // this shouldn't be needed >:(
  
  wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
  sizer->Add(glc, 1, wxEXPAND);
  sizer->Add(note, 0, wxEXPAND);
  
  wxBoxSizer *vertsizer = new wxBoxSizer(wxVERTICAL);
  vertsizer->Add(tool, 0, wxEXPAND);
  vertsizer->Add(sizer, 1, wxEXPAND);
  
  SetSizer(vertsizer);
  
}

void VeceditWindow::OnNew(wxCommandEvent& event) {
  if(!maybeSaveChanges())
    return;
  core.clear();
  filename = "";
}
void VeceditWindow::OnOpen(wxCommandEvent& event) {
  if(!maybeSaveChanges())
    return;
  wxFileDialog wxfd(this, "Open File", veceditname, "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_OPEN);
  if(wxfd.ShowModal() == wxID_OK) {
    filename = wxfd.GetPath();
    core.load(filename);
  }
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

void VeceditWindow::OnSave_dispatch(wxCommandEvent& event) {
  OnSave();
}
void VeceditWindow::OnSaveas_dispatch(wxCommandEvent& event) {
  OnSaveas();
}

void VeceditWindow::redraw() {
  dprintf("Redraw\n");
  
  glc->Refresh();
}

bool VeceditWindow::maybeSaveChanges() {
  if(core.changed()) {
    int saveit = wxMessageBox("The text in the unknown file has changed.\n\nDo you want to save the changes?", veceditname, wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
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
  wxImage::AddHandler(new wxPNGHandler);
  
  initGfx();
  
  VeceditWindow *frame = new VeceditWindow();
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}
