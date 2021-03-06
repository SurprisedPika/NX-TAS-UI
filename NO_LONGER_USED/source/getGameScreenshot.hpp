#pragma once

extern "C" {
#include <switch.h>
}

class GetGameScreenshot {
private:
	// Global result variable for all functions
	Result rc;
	// 0x32000 is the size of the video buffer
	// I'm not quite sure why it is that, but it is
	u8* Vbuf;
	Service grcdVideo;

	// Borrowed from sysDVR
	Result grcdServiceOpen (Service* out) {
		if (serviceIsActive (out))
			return 0;

		rc = smGetService (out, "grc:d");

		if (R_FAILED (rc))
			grcdExit ();

		return rc;
	}

	// Borrowed from sysDVR
	void grcdServiceClose (Service* svc) {
		serviceClose (svc);
	}

	// Borrowed from sysDVR
	Result grcdServiceBegin (Service* svc) {
		return _grcCmdNoIO (svc, 1);
	}

	// Borrowed from sysDVR
	Result grcdServiceRead (Service* svc, GrcStream stream, void* buffer,
		size_t size, u32* unk, u32* data_size,
		u64* timestamp) {
		IpcCommand c;
		ipcInitialize (&c);

		ipcAddRecvBuffer (&c, buffer, size, BufferType_Normal);

		struct {
			u64 magic;
			u64 cmd_id;
			u32 stream;
		} * raw;

		raw = serviceIpcPrepareHeader (svc, &c, sizeof (*raw));

		raw->magic  = SFCI_MAGIC;
		raw->cmd_id = 2;
		raw->stream = stream;

		rc = serviceIpcDispatch (svc);

		if (R_SUCCEEDED (rc)) {
			IpcParsedCommand r;
			struct {
				u64 magic;
				u64 result;
				u32 unk;
				u32 data_size;
				u64 timestamp;
			} * resp;

			serviceIpcParse (svc, &r, sizeof (*resp));
			resp = r.Raw;

			rc = resp->result;

			if (R_SUCCEEDED (rc)) {
				if (unk)
					*unk = resp->unk;
				if (data_size)
					*data_size = resp->data_size;
				if (timestamp)
					*timestamp = resp->timestamp;
			}
		}

		return rc;
	}

	// Borrowed from sysDVR
	Result _grcCmdNoIO (Service* srv, u64 cmd_id) {
		IpcCommand c;
		ipcInitialize (&c);

		struct {
			u64 magic;
			u64 cmd_id;
		} * raw;

		raw = serviceIpcPrepareHeader (srv, &c, sizeof (*raw));

		raw->magic  = SFCI_MAGIC;
		raw->cmd_id = cmd_id;

		rc = serviceIpcDispatch (srv);

		if (R_SUCCEEDED (rc)) {
			IpcParsedCommand r;
			struct {
				u64 magic;
				u64 result;
			} * resp;

			serviceIpcParse (srv, &r, sizeof (*resp));
			resp = r.Raw;

			rc = resp->result;
		}

		return rc;
	}

public:
	GetGameScreenshot () {
		Vbuf = aligned_alloc (0x1000, 0x32000);
		// Open video service if it is not already open
		rc = grcdServiceOpen (&grcdVideo);
		if (R_FAILED (rc))
			fatalSimple (rc);
		rc = grcdServiceBegin (&grcdVideo);
		if (R_FAILED (rc))
			fatalSimple (rc);
	}

	u8* readFrame () {
		// This function produces H264 frames in Vbuf with a size of 1280 x 720
		// I'm not exactly sure what this is, but it's always 0
		u32 unk       = 0;
		u32 VOutSz    = 0;
		u64 timestamp = 0;
		// GrcStream_Video is a constant from LibNX
		// 0x32000 is VBufZ, size of the video buffer
		int fails = 0;
		// Runs multiple times to make sure it works
		// From sysDVR
		/*
    while (true) {
            Result res = grcdServiceRead(&grcdVideo, GrcStream_Video, Vbuf,
    0x32000, &unk, &VOutSz, &timestamp) if (R_SUCCEEDED(res) && VOutSz > 0)
    break; VOutSz = 0; if (fails++ > 8 && !IsThreadRunning) break;
    }
    */
		rc = grcdServiceRead (&grcdVideo, GrcStream_Video, Vbuf, 0x32000, &unk,
			&VOutSz, &timestamp);
		// Return pointer to data
		return Vbuf;
	}

	~GetGameScreenshot () {
		grcdServiceClose (&grcdVideo);
		free (Vbuf);
	}
}