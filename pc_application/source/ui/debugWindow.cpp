#include "debugWindow.hpp"

DebugWindow::DebugWindow(wxFrame* parent, std::shared_ptr<CommunicateWithNetwork> networkImp)
	: wxFrame(parent, wxID_ANY, "Debug Menu", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {
	// Start hidden
	Hide();
	networkInstance = networkImp;

	buttonSizer = new wxBoxSizer(wxVERTICAL);

	pauseButton          = new wxButton(this, wxID_ANY, "Pause");
	unpauseButton        = new wxButton(this, wxID_ANY, "Unpause");
	getFramebufferButton = new wxButton(this, wxID_ANY, "Getframebuffer");

	pauseButton->Bind(wxEVT_BUTTON, &DebugWindow::onPausePressed, this);
	unpauseButton->Bind(wxEVT_BUTTON, &DebugWindow::onUnpausePressed, this);
	getFramebufferButton->Bind(wxEVT_BUTTON, &DebugWindow::onGetFramebufferPressed, this);

	buttonSizer->Add(pauseButton, 1);
	buttonSizer->Add(unpauseButton, 1);
	buttonSizer->Add(getFramebufferButton, 1);

	SetSizer(buttonSizer);
	buttonSizer->SetSizeHints(this);
	Layout();
	Fit();
	Center(wxBOTH);

	Layout();
}

// clang-format off
BEGIN_EVENT_TABLE(DebugWindow, wxFrame)
	EVT_CLOSE(DebugWindow::onClose)
END_EVENT_TABLE()
// clang-format on

void DebugWindow::onClose(wxCloseEvent& event) {
	// Only hide, not delete
	// This prevents the entire application from closing, but oh well
	Show(false);
}

void DebugWindow::onPausePressed(wxCommandEvent& event) {
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::PAUSE_DEBUG;
	})
	// clang-format on
}
void DebugWindow::onUnpausePressed(wxCommandEvent& event) {
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::UNPAUSE_DEBUG;
	})
	// clang-format on
}

void DebugWindow::onGetFramebufferPressed(wxCommandEvent& event) {
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::GET_FRAMEBUFFER;
	})
	// clang-format on
}