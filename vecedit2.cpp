
#include <wx/wx.h>
#include <wx/glcanvas.h>

class MyApp: public wxApp {
  virtual bool OnInit();
};

class MyFrame : public wxFrame {
private:
  wxGLCanvas *canvas;
public:
  
  MyFrame(const wxString& title);
  
  void OnQuit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);

  void render();
  
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

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit() {
  MyFrame *frame = new MyFrame("D-Net Vecedit2");
  frame->Show(TRUE);
  SetTopWindow(frame);
  
  frame->render();
  
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
  
  wxPanel *panel = new wxPanel(this);

  canvas = new wxGLCanvas(panel, -1, wxDefaultPosition, wxDefaultSize);
  canvas->SetSize(GetClientSize());
  
  CreateStatusBar();
  SetStatusText( "Welcome to wxWidgets!" );
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("This is a level editor.", "About D-Net Vecedit2", wxOK | wxICON_INFORMATION, this);
}

void MyFrame::render() {
  canvas->SetCurrent();

  wxSafeYield();
  glClearColor(0.0, 0.0, 0.0, 0.0);
  //glViewport(0, 0, (GLint)200, (GLint)200);
  glColor3f(1.0, 1.0, 1.0);
  
  glBegin(GL_POLYGON);
    glVertex2f(-0.5, -0.5);
    glVertex2f(-0.5, 0.5);
    glVertex2f(0.5, 0.5);
    glVertex2f(0.5, -0.5);
  glEnd();
  glFlush();
  
  canvas->SwapBuffers();
};
