#pragma once

#include <string.h>
#include <switch.h>
#include <stdlib.h>

#include "thirdParty/lvgl/lvgl.h"

#include "decodeAndResize.hpp"
#include "ui.hpp"
#include "writeToScreen.hpp"
#include "utils.hpp"
#include "getGameScreenshot.hpp"
#include "menuKeybinds.hpp"

class UI {
	private:
	// Game buffer recieved from the screenshot service
	u8* gameBuffer;
	// Dimensions of the screen
	constexpr int width = 1280;
	constexpr int height = 720;
	// Dimensions of the game window in the corner
	constexpr int gameWidth = 960;
	constexpr int gameHeight = 540;
	// Current display instance
	ViDisplay disp;
	// VSync event of display
	Event vsyncEvent;
	// RGBA buffer of game window in corner
	u8* gameImage;
	// This is returned by the other class
	//u8* outputRgbaBuffer = malloc(gameWidth * gameHeight * 4);
	// Debug handle (for pausing)
	Handle debugHandle;
	// Wether the game is currently disableGameRendering
	constexpr bool gameIsRendering = true;
	// Global result variable for all functions
	Result rc;
    // The function instances that do most of the work
	// UI overlay instance
	AppUI* appUI;
	// WriteToScreen instance
	WriteToScreen* writeToScreen;
    // GetGameScreenshot instance
    GetGameScreenshot* getGameScreenshot;
    // Decoding and resizing instance
	DecodeAndResize* decodeAndResize;
	// whether the UI is open
	constexpr bool UIOpen = false;
    // Whether to render the game window with the overlay ui
    constexpr bool gameShouldBeRunning = true;
    // Whether to render the game for this one frame
    // Used for frame-by-frame editing
    constexpr bool runGameThisFrame = false;
    // Whether the app should be running
    constexpr bool appShouldRun = true;

    void waitForVsync() {
        // Wait for this vsync to get a single frame
		rc = eventWait(&vsync_event, 0xFFFFFFFFFFF);
        if(R_FAILED(rc)) fatalSimple(rc);
    }

    void openUIToggle() {
        // If all triggers have been pressed at the same time, return true
        return allTriggersKeybind();
    }

	public:
	NxTASUI() {
		// Create neccessary instances
		writeToScreen = new WriteToScreen(width, height);
		// All the code neccessary to make sure the L UI is right
		appUI = new AppUI(width - gameWidth, height, gameWidth, height - gameHeight);
        // Game screenshot instance
        getGameScreenshot = new GetGameScreenshot();
        // Create decoding and resizing instance
		decodeAndResize = new DecodeAndResize(width, heght, gameWidth, gameHeight);
		// Create display and vsync instances
    	rc = viOpenDefaultDisplay(&disp);
    	if(R_FAILED(rc)) fatalSimple(rc);
    	rc = viGetDisplayVsyncEvent(&disp, &vsyncEvent);
    	if(R_FAILED(rc)) fatalSimple(rc);
	}

	void enableGameRendering() {
		// I believe it closes the debug process
		if (!gameIsRendering) {
			svcCloseHandle(debugHandle);
			gameIsRendering = true;
		}
	}

	void disableGameRendering() {
		if (gameIsRendering) {
			u64 pid = 0;
			// Get the PID of the currently running game, I believe...
        	pmdmntGetApplicationPid(&pid);
			// Pauses the game through debugging
			// Apparently applications pause when they are debugging, so I am using this to my advantage
    		svcDebugActiveProcess(&debugHandle, pid);
			gameIsRendering = false;
		}
	}

    void drawGameWindow() {
        // Make sure game is rendering
		enableGameRendering();
		
		waitForVsync();
        // Read the frame right now, quickly
		gameBuffer = getGameScreenshot->readFrame();
		// Stop rendering now, before another frame is written
		disableGameRendering();
		// Decode and resizing the H264 frame with the other function
		gameImage = decodeAndResize->decodeAndResize(gameBuffer, 0x32000);
		// gameImage is an RGBA array
		writeToScreen->startFrame();
		for (int x = 0; x < gameWidth; x++) {
			for (int y = 0; y < gameHeight; y++) {
				// Assign the pixel now
				int pixelOffset = y * gameWidth * 4 + x * 4;
				writeToScreen->setPixel(x + AppUI->getLeftWidth(), y,
					gameImage[pixelOffset],     // R
					gameImage[pixelOffset + 1], // G
					gameImage[pixelOffset + 2]);// B
			}
		}
        writeToScreen->endFrame();
    }

    void drawUI() {
        writeToScreen->startFrame();
        // Deal with LVGL UI
		for (int x = 0; x < AppUI->getLeftWidth(); X++) {
			for (int y = 0; y < appUI->getLeftHeight(); y++) {
				int pixelOffset = y * AppUI->getLeftWidth() + x;
				lv_color_t pixelColor = AppUI->getLeftBuf()[pixelOffset];
				writeToScreen->setPixel(x, y,
					pixelColor.red,
					pixelColor.green,
					pixelColor.blue);
			}
		}
		for (int x = 0; x < AppUI->getBottomWidth(); X++) {
			for (int y = 0; y < appUI->getBottomHeight(); y++) {
				int pixelOffset = y * AppUI->getBottomWidth() + x;
				lv_color_t pixelColor = AppUI->getBottomBuf()[pixelOffset];
				writeToScreen->setPixel(x + AppUI->getLeftWidth(),
					y + gameHeight,
					pixelColor.red,
					pixelColor.green,
					pixelColor.blue);
			}
		}
		// Finally, write to framebuffer
		writeToScreen->endFrame();
		// UI and game window are now successfully written to screen
    }

	void openUI() {
		// The good stuff where everything happens
		UIOpen = true
        // Open framebuffer
        writeToScreen->makeOpaque();
		disableGameRendering();
		appUI->enableDrawing();
		// Do the UI thing as much as you can
		while (UIOpen) {
			// Draw the game window in the corner and leave rendering open for the other UI
            if (gameShouldBeRunning || runGameThisFrame) {
                drawGameWindow();
                // Draw the UI right after
                drawUI();
                // Dont increment another frame
                runGameThisFrame = false;
            } else {
                drawUI();
                // Wait for vsync so that the UI is not drawn too quickly
                waitForVsync();
            }
            if (KB::closeMenu()) {
                // Close UI because close UI keybind has been used
                UIOpen = false;
            }
			// Some condition to close the UI by setting UIOpen to false
			if (!UIOpen) {
                // Make sure loop doesn't go again
				closeUI();
			}
		}
	}

	void closeUI() {
		// Called from openUI in the while loop
		appUI->enableDrawing();
		enableGameRendering();
        // Clear framebuffer
        writeToScreen->makeClear();
	}

    // The main loop where the UI is either opened or closed
    void mainUILoop() {
        while(appShouldRun) {
            // Check if the UI open toggle has been used
            if (KB::openMenu()) {
                // This function will return when the UI has been closed
                openUI();
            }

            // Sleep for a little bit to not overwhelm the system
            sleepMs(1);
        }
    }

	~NxTASUI() {
		// All instances will automatically be deleted
	}
}