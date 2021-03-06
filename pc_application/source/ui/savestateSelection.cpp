#include "savestateSelection.hpp"

SavestateLister::SavestateLister(DataProcessing* input)
	: wxDialog(NULL, wxID_ANY, "Savestate Listing", wxDefaultPosition, wxDefaultSize) {
	inputInstance = input;

	mainSizer = new wxBoxSizer(wxVERTICAL);

	// 3 columns
	projectList       = new wxWrapSizer(wxVERTICAL);
	projectListHolder = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	// Add each individual savestate hook for viewing
	for(auto const& savestateHook : inputInstance->getAllSavestateHookBlocks()) {
		wxBoxSizer* dataSizer = new wxBoxSizer(wxVERTICAL);

		wxStaticText* dHash = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(savestateHook->dHash), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

		wxBitmap* screenshot = savestateHook->screenshot;
		// Size is reduced by 4 so the bitmap isn't massive
		DrawingCanvasBitmap* drawingCanvas = new DrawingCanvasBitmap(this, wxSize(1280 / 4, 720 / 4));
		drawingCanvas->setBitmap(screenshot);

		savestateScreenshots.push_back(drawingCanvas);

		drawingCanvas->Bind(wxEVT_LEFT_DOWN, &SavestateLister::onSavestateHookSelect, this);

		dataSizer->Add(dHash, 1);
		dataSizer->Add(drawingCanvas, 0, wxSHAPED);

		projectList->Add(dataSizer, 1);
	}

	projectListHolder->SetSizer(projectList);

	mainSizer->Add(projectListHolder, 1, wxEXPAND | wxALL);

	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);
	Layout();
	Fit();
	Center(wxBOTH);

	Layout();
}

void SavestateLister::onSavestateHookSelect(wxMouseEvent& event) {
	DrawingCanvasBitmap* drawingCanvas = (DrawingCanvasBitmap*)event.GetEventObject();
	for(std::size_t i = 0; i < savestateScreenshots.size(); i++) {
		if(drawingCanvas == savestateScreenshots[i]) {
			selectedSavestate   = i;
			operationSuccessful = true;
			Close(true);
			break;
		}
	}
}

SavestateSelection::SavestateSelection(rapidjson::Document* settings, std::shared_ptr<ProjectHandler> projHandler, bool isSavestateLoadDialog, std::shared_ptr<CommunicateWithNetwork> networkImp)
	: wxDialog(NULL, wxID_ANY, "Savestate Selection", wxDefaultPosition, wxDefaultSize) {
	// Parent is specifically null because this is a separate window that opens
	savestateLoadDialog = isSavestateLoadDialog;
	mainSettings        = settings;
	networkInstance     = networkImp;
	projectHandler      = projHandler;

	dhashWidth  = (*mainSettings)["dhashWidth"].GetInt();
	dhashHeight = (*mainSettings)["dhashHeight"].GetInt();

	imageSizer  = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	fullSizer   = new wxBoxSizer(wxVERTICAL);

	leftImageSizer  = new wxBoxSizer(wxVERTICAL);
	rightImageSizer = new wxBoxSizer(wxVERTICAL);

	leftDHash  = new wxStaticText(this, wxID_ANY, "null", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	rightDHash = new wxStaticText(this, wxID_ANY, "null", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

	hammingDistance = new wxStaticText(this, wxID_ANY, "null", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

	// To set the text, just use:
	// leftDHash->SetLabelText(aString);

	// Fill sizers
	// Have to have play, pause and frame advance buttons
	// Look to mainWindow for setting sizers
	// Gonna go a roundabout way for adding buttons, every button will be visible, but certain ones will be disabled as needed

	playButton         = HELPERS::getBitmapButton(this, mainSettings, "playButton");
	pauseButton        = HELPERS::getBitmapButton(this, mainSettings, "pauseButton");
	frameAdvanceButton = HELPERS::getBitmapButton(this, mainSettings, "frameAdvanceButton");
	okButton           = HELPERS::getBitmapButton(this, mainSettings, "okButton");

	selectFrameAutomatically = new wxSpinCtrl(this, wxID_ANY, "Select automatically at or below this hamming distance: ", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);

	playButton->Bind(wxEVT_BUTTON, &SavestateSelection::onPlay, this);
	pauseButton->Bind(wxEVT_BUTTON, &SavestateSelection::onPause, this);
	frameAdvanceButton->Bind(wxEVT_BUTTON, &SavestateSelection::onFrameAdvance, this);
	okButton->Bind(wxEVT_BUTTON, &SavestateSelection::onOk, this);

	// Divide by two because it's usually too big
	currentFrame = new DrawingCanvasBitmap(this, wxSize(1280 / 2, 720 / 2));
	if(savestateLoadDialog) {
		// Otherwise, just show the frame to save on
		goalFrame = new DrawingCanvasBitmap(this, wxSize(1280 / 2, 720 / 2));
	}

	leftImageSizer->Add(currentFrame, 0, wxSHAPED);

	if(savestateLoadDialog) {
		// Dhashes are only used in loading, not creating the first one
		leftImageSizer->Add(leftDHash, 1, wxEXPAND | wxALL);

		rightImageSizer->Add(goalFrame, 0, wxSHAPED);
		rightImageSizer->Add(rightDHash, 1, wxEXPAND | wxALL);
	}

	imageSizer->Add(leftImageSizer, 1, wxEXPAND | wxALL);
	if(savestateLoadDialog) {
		imageSizer->Add(rightImageSizer, 1, wxEXPAND | wxALL);
	}

	fullSizer->Add(imageSizer, 1, wxEXPAND | wxALL);

	if(savestateLoadDialog) {
		fullSizer->Add(hammingDistance, 1, wxEXPAND | wxALL);
	}

	buttonSizer->Add(playButton, 1);
	buttonSizer->Add(pauseButton, 1);
	buttonSizer->Add(frameAdvanceButton, 1);
	buttonSizer->Add(okButton, 1);

	fullSizer->Add(buttonSizer, 1, wxEXPAND | wxALL);

	SetSizer(fullSizer);
	fullSizer->SetSizeHints(this);
	Layout();
	Fit();
	Center(wxBOTH);

	Layout();

	// Pause at the beginning to get the framebuffer and stuff
	// clang-format off
    ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::PAUSE;
	})
	// clang-format on

	// When done with all the stuff, close with Close(true);
}

// clang-format off
BEGIN_EVENT_TABLE(SavestateSelection, wxDialog)
    EVT_IDLE(SavestateSelection::onIdle)
	EVT_CLOSE(SavestateSelection::onClose)
END_EVENT_TABLE()
// clang-format on

void SavestateSelection::setTargetFrame(wxBitmap* targetBitmap, std::string targetDhash) {
	// Called when it's a load dialog
	goalFrame->setBitmap(targetBitmap);
	rightDHash->SetLabel(wxString::FromUTF8(targetDhash));
}

void SavestateSelection::onIdle(wxIdleEvent& event) {
	/*
	CHECK_QUEUE(networkInstance, RecieveGameFramebuffer, {
		wxImage screenshot = HELPERS::getImageFromJPEGData(data.buf);
		currentFrame->setBitmap(new wxBitmap(screenshot));
		leftDHash->SetLabel(HELPERS::calculateDhash(screenshot, dhashWidth, dhashHeight));

		if(savestateLoadDialog) {
			uint16_t hamming = HELPERS::getHammingDistance(leftDHash->GetLabel(), rightDHash->GetLabel());
			hammingDistance->SetLabel(wxString::Format("%d", hamming));
			if(hamming <= selectFrameAutomatically->GetValue()) {
				// This frame might be identical, ask user if they want to use this frame
				wxMessageDialog useFrameDialog(this, "This frame is very similar to the target frame, use it?", "Use this frame", wxYES_NO | wxCANCEL | wxYES_DEFAULT);
				int res = useFrameDialog.ShowModal();
				if(res == wxID_YES) {
					// Use the frame
					callOk();
				}
			}
		}
	})
	*/
}

void SavestateSelection::onPlay(wxCommandEvent& event) {
	// Trigger automatic playing
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::UNPAUSE;
	})
	// clang-format on
}

void SavestateSelection::onPause(wxCommandEvent& event) {
	// Stop automatic playing
	// Do the same stuff as frameAdvance
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::PAUSE;
	})
	// clang-format on
}

void SavestateSelection::onFrameAdvance(wxCommandEvent& event) {
	// Send blank input for a single frame then recieve JPEG buffer
	// Blank input in this case is input that matches the inputs of the controller on the switch
	// Set the dHash for the currentFrame and calc the hamming distance
	// clang-format off
	ADD_TO_QUEUE(SendFlag, networkInstance, {
		data.actFlag = SendInfo::RUN_BLANK_FRAME;
	})
	// clang-format on
}

void SavestateSelection::onOk(wxCommandEvent& event) {
	callOk();
}

void SavestateSelection::onClose(wxCloseEvent& event) { }