
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "debug.h"
#include "gfx.h"
#include "vecedit.h"

/*************
 * VeceditGLC
 */
 
class VeceditGLC : public wxGLCanvas {
public:
  
  VeceditGLC(wxWindow *wind);
  
  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnEraseBackground(wxEraseEvent& event);

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(VeceditGLC, wxGLCanvas)
  EVT_PAINT(VeceditGLC::OnPaint)
  EVT_SIZE(VeceditGLC::OnSize)
  EVT_ERASE_BACKGROUND(VeceditGLC::OnEraseBackground) 
END_EVENT_TABLE()

int gl_attribList[] = {
  WX_GL_RGBA,
  WX_GL_STENCIL_SIZE, 1,
  WX_GL_DOUBLEBUFFER,
  0
};
VeceditGLC::VeceditGLC(wxWindow *wind) : wxGLCanvas(wind, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER, "GLCanvas", gl_attribList) { };

void VeceditGLC::OnPaint(wxPaintEvent& event) {
  wxPaintDC dc(this);
  
  SetCurrent();
  
  {
    int w, h;
    GetClientSize(&w, &h);
    glViewport(0, 0, (GLint) w, (GLint) h); 
    
    updateResolution((float)w / h);
  }
  
  wxYield();
  initFrame();

  clearFrame(Color(0, 0, 0));
  setZoomCenter(0, 0, 1);
  setColor(Color(1.0, 1.0, 1.0));
  drawJustifiedText("THIS IS A TEST", 0.1, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
  
  deinitFrame();
  
  SwapBuffers();

}

void VeceditGLC::OnSize(wxSizeEvent& event) {
  Refresh();
}

void VeceditGLC::OnEraseBackground(wxEraseEvent& event) {
}

/*************
 * VeceditWindow
 */

class VeceditWindow : public wxFrame {
private:
  Vecedit core;

  VeceditGLC *glc;

public:
  
  VeceditWindow(const wxString& title);
  
  void OnNew(wxCommandEvent& event);
  void OnOpen(wxCommandEvent& event);
  void OnSave(wxCommandEvent& event);
  void OnSaveas(wxCommandEvent& event);

  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);

  void redraw();
  
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
  EVT_MENU(ID_Save, VeceditWindow::OnSave)
  EVT_MENU(ID_Saveas, VeceditWindow::OnSaveas)
  EVT_MENU(ID_Quit, VeceditWindow::OnQuit)
  EVT_MENU(ID_About, VeceditWindow::OnAbout)
END_EVENT_TABLE()

VeceditWindow::VeceditWindow(const wxString& title) : wxFrame((wxFrame *)NULL, -1, title), core(NewFunctor(this, &VeceditWindow::redraw)) {
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
  
  glc = new VeceditGLC(this);

}

void VeceditWindow::OnNew(wxCommandEvent& event) {
  dprintf("New");
}
void VeceditWindow::OnOpen(wxCommandEvent& event) {
  wxFileDialog wxfd(this, "Open File", "", "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_OPEN);
  if(wxfd.ShowModal() == wxID_OK) {
    dprintf("Open %s", wxfd.GetPath().c_str());
  } else {
    dprintf("Open cancel");
  }
}
void VeceditWindow::OnSave(wxCommandEvent& event) {
  OnSaveas(event);
}
void VeceditWindow::OnSaveas(wxCommandEvent& event) {
  wxFileDialog wxfd(this, "Save File", "", "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if(wxfd.ShowModal() == wxID_OK) {
    dprintf("Saveas %s", wxfd.GetPath().c_str());
  } else {
    dprintf("Saveas cancel");
  }
}

void VeceditWindow::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  Close(TRUE);
}

void VeceditWindow::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("What is D-Net Vecedit2? We just don't know.", "About D-Net Vecedit2", wxOK | wxICON_INFORMATION, this);
}

void VeceditWindow::redraw() {
  dprintf("Redraw\n");
  
  glc->Refresh();
}

/*************
 * VeceditMain
 */

class VeceditMain: public wxApp {
  virtual bool OnInit();
};

IMPLEMENT_APP(VeceditMain)

bool VeceditMain::OnInit() {
  initGfx();
  
  VeceditWindow *frame = new VeceditWindow("D-Net Vecedit2");
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}
