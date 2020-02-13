#pragma once

#include <functional>
#include <map>
#include <memory>
#include <unistd.h>
#include <utility>
#include <wx/dcbuffer.h>
#include <wx/grid.h>
#include <wx/wx.h>

#include "../dataHandling/buttonData.hpp"
#include "../dataHandling/dataProcessing.hpp"
#include "drawingCanvas.hpp"

class JoystickCanvas : public DrawingCanvas {
public:
	JoystickCanvas(wxFrame* parent);

	void draw(wxDC& dc) override;
};

// Simple way to render images in grid
class renderImageInGrid : public wxGridCellRenderer {
	// clang-format on
private:
	std::shared_ptr<wxBitmap> theBitmap;
	Btn button;

public:
	renderImageInGrid(std::shared_ptr<wxBitmap> bitmap, Btn btn);

	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);

	void setBitmap(std::shared_ptr<wxBitmap> bitmap);

	// Used by click handlers
	Btn getButton() {
		return button;
	}

	// Need to override
	wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) {
		return wxSize(theBitmap->GetWidth(), theBitmap->GetHeight());
	}

	wxGridCellRenderer* Clone() const {
		return new renderImageInGrid(theBitmap, button);
	}
};

class BottomUI {
private:
	// Location
	struct Location {
		int x;
		int y;
	};

	// https://www.tablesgenerator.com/text_tables
	//      0   1    2    3   4  5   6   7   8    9  10
	//   +----+---+----+----+---+--+---+---+----+---+---+
	// 0 | LS |   | ZL | L  |   |  |   | R | ZR |   |   |
	//   +----+---+----+----+---+--+---+---+----+---+---+
	// 1 |    | ^ |    | -  |   |  |   | + |    | X |   |
	//   +----+---+----+----+---+--+---+---+----+---+---+
	// 2 | <  |   | >  |    | C |  | H |   | Y  |   | A |
	//   +----+---+----+----+---+--+---+---+----+---+---+
	// 3 |    | v |    | RS |   |  |   |   |    | B |   |
	//   +----+---+----+----+---+--+---+---+----+---+---+
	std::map<Btn, Location> KeyLocs {
		{ Btn::A, { 10, 2 } },
		{ Btn::B, { 9, 3 } },
		{ Btn::X, { 9, 1 } },
		{ Btn::Y, { 8, 2 } },
		{ Btn::L, { 3, 0 } },
		{ Btn::R, { 7, 0 } },
		{ Btn::ZL, { 2, 0 } },
		{ Btn::ZR, { 8, 0 } },
		{ Btn::DUP, { 1, 1 } },
		{ Btn::DDOWN, { 1, 3 } },
		{ Btn::DLEFT, { 0, 2 } },
		{ Btn::DRIGHT, { 2, 2 } },
		{ Btn::PLUS, { 7, 1 } },
		{ Btn::MINUS, { 3, 1 } },
		{ Btn::HOME, { 6, 2 } },
		{ Btn::CAPT, { 4, 2 } },
		{ Btn::LS, { 0, 0 } },
		{ Btn::RS, { 3, 3 } },
	};

	// Input instance to get inputs and such
	std::shared_ptr<DataProcessing> inputInstance;

	std::shared_ptr<wxBoxSizer> horizontalBoxSizer;

	std::shared_ptr<JoystickCanvas> leftJoystickDrawer;
	std::shared_ptr<JoystickCanvas> rightJoystickDrawer;

	// Grid containing the button viewer
	std::shared_ptr<wxGrid> buttonGrid;

	// The button mapping instance
	std::shared_ptr<ButtonData> buttonData;

	// Listen for grid onclick
	void onGridClick(wxGridEvent& event);

protected:
	// Need to fix
	// bool onButtonPress(GdkEventButton* event, Btn button);

public:
	BottomUI(wxFrame* parentFrame, std::shared_ptr<ButtonData> buttons, wxBoxSizer* theGrid, std::shared_ptr<DataProcessing> input);

	void setIconState(Btn button, bool state);

	//~BottomUI();
};