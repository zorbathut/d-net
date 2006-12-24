
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "debug.h"
#include "gfx.h"

class MyApp: public wxApp {
  virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

class MyFrame : public wxFrame {
public:
  
  MyFrame(const wxString& title);
  
  void OnNew(wxCommandEvent& event);
  void OnOpen(wxCommandEvent& event);
  void OnSave(wxCommandEvent& event);
  void OnSaveas(wxCommandEvent& event);

  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  
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

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_MENU(ID_New, MyFrame::OnNew)
  EVT_MENU(ID_Open, MyFrame::OnOpen)
  EVT_MENU(ID_Save, MyFrame::OnSave)
  EVT_MENU(ID_Saveas, MyFrame::OnSaveas)
  EVT_MENU(ID_Quit, MyFrame::OnQuit)
  EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()

class MyGLC : public wxGLCanvas {
public:
  
  MyGLC(wxWindow *wind);
  
  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnEraseBackground(wxEraseEvent& event);

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MyGLC, wxGLCanvas)
  EVT_PAINT(MyGLC::OnPaint)
  EVT_SIZE(MyGLC::OnSize)
  EVT_ERASE_BACKGROUND(MyGLC::OnEraseBackground) 
END_EVENT_TABLE()

bool MyApp::OnInit() {
  initGfx();
  
  MyFrame *frame = new MyFrame("D-Net Vecedit2");
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}

MyFrame::MyFrame(const wxString& title) : wxFrame((wxFrame *)NULL, -1, title) {
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
  
  new MyGLC(this);

}

void MyFrame::OnNew(wxCommandEvent& event) {
  dprintf("New");
}
void MyFrame::OnOpen(wxCommandEvent& event) {
  wxFileDialog wxfd(this, "Open File", "", "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_OPEN);
  if(wxfd.ShowModal() == wxID_OK) {
    dprintf("Open %s", wxfd.GetPath().c_str());
  } else {
    dprintf("Open cancel");
  }
}
void MyFrame::OnSave(wxCommandEvent& event) {
  OnSaveas(event);
}
void MyFrame::OnSaveas(wxCommandEvent& event) {
  wxFileDialog wxfd(this, "Save File", "", "", "DVec2 Files (*.dv2)|*.dv2|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if(wxfd.ShowModal() == wxID_OK) {
    dprintf("Saveas %s", wxfd.GetPath().c_str());
  } else {
    dprintf("Saveas cancel");
  }
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("This is a level editor.", "About D-Net Vecedit2", wxOK | wxICON_INFORMATION, this);
}

int gl_attribList[] = {
  WX_GL_RGBA,
  WX_GL_STENCIL_SIZE, 1,
  WX_GL_DOUBLEBUFFER,
  0
};
MyGLC::MyGLC(wxWindow *wind) : wxGLCanvas(wind, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER, "GLCanvas", gl_attribList) { };

void MyGLC::OnPaint(wxPaintEvent& event) {
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

void MyGLC::OnSize(wxSizeEvent& event) {
  Refresh();
}

void MyGLC::OnEraseBackground(wxEraseEvent& event) {
}
