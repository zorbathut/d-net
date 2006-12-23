
#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "debug.h"

class MyApp: public wxApp {
  virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

class MyFrame : public wxFrame {
public:
  
  MyFrame(const wxString& title);
  
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  
  DECLARE_EVENT_TABLE()
};

enum {
  ID_Quit = 1,
  ID_About,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_MENU(ID_Quit, MyFrame::OnQuit)
  EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()

class MyGLC : public wxGLCanvas {
public:
  
  MyGLC(wxWindow *wind);
  
  void OnPaint(wxPaintEvent& event);
  void OnEraseBackground(wxEraseEvent& event);

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MyGLC, wxGLCanvas)
  EVT_PAINT(MyGLC::OnPaint)
  EVT_ERASE_BACKGROUND(MyGLC::OnEraseBackground) 
END_EVENT_TABLE()

bool MyApp::OnInit() {
  MyFrame *frame = new MyFrame("D-Net Vecedit2");
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
}

MyFrame::MyFrame(const wxString& title) : wxFrame((wxFrame *)NULL, -1, title) {
  wxMenuBar *menuBar = new wxMenuBar;
  
  {
    wxMenu *menuFile = new wxMenu;
    
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

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("This is a level editor.", "About D-Net Vecedit2", wxOK | wxICON_INFORMATION, this);
}

MyGLC::MyGLC(wxWindow *wind) : wxGLCanvas(wind, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER) { };

void MyGLC::OnPaint(wxPaintEvent& event) {
  wxPaintDC dc(this);
  
  SetCurrent();
  
  static float foo = 0.3;
  foo += 0.1;
  if(foo > 0.9)
    foo = 0.1;

  dprintf("render, %f\n", foo);
  dprintf("size is %d,%d\n", GetSize().x, GetSize().y);
  
  {
    int w, h;
    GetClientSize(&w, &h);
    glViewport(0, 0, (GLint) w, (GLint) h); 
  }

  //wxSafeYield();
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  CHECK(glGetError() == GL_NO_ERROR);
  
  glColor3f(1.0, 1.0, 1.0);
  
  glBegin(GL_POLYGON);
    glVertex2f(-foo, -foo);
    glVertex2f(-foo, foo);
    glVertex2f(foo, foo);
    glVertex2f(foo, -foo);
  glEnd();
  glFlush();
  
  CHECK(glGetError() == GL_NO_ERROR);
  
  SwapBuffers();
}

void MyGLC::OnEraseBackground(wxEraseEvent& event) {
}
