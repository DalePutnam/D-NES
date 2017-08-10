#pragma once

#include <wx/app.h>

class NESApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual bool Initialize(int& argc, wxChar **argv) override;
};

wxDECLARE_APP(NESApp);