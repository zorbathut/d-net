
#include "wx/wx.h"

class MyApp: public wxApp {
  virtual bool OnInit();
};

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

IMPLEMENT_APP(MyApp)

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
  
  wxPanel *panel = new wxPanel(this);
  
  CreateStatusBar();
  SetStatusText( "Welcome to wxWidgets!" );
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
  Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox("This is a wxWidgets Hello world sample", "About Hello World", wxOK | wxICON_INFORMATION, this);
}
