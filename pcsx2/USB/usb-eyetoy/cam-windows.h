// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "videodev.h"
#include "common/RedtapeWindows.h"
#include "common/RedtapeWilCom.h"

#include <dshow.h>
#include <mutex>
#include <wrl/client.h>
#ifndef _MSC_VER
#include <qedit.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "strmiids")
#endif

extern "C" {
#ifdef _MSC_VER
extern GUID IID_ISampleGrabberCB;
extern GUID CLSID_SampleGrabber;
#endif
extern GUID CLSID_NullRenderer;
}

#ifdef _MSC_VER
#pragma region qedit.h
struct __declspec(uuid("0579154a-2b53-4994-b0d0-e773148eff85")) ISampleGrabberCB : IUnknown
{
	virtual HRESULT __stdcall SampleCB(double SampleTime, struct IMediaSample* pSample) = 0;
	virtual HRESULT __stdcall BufferCB(double SampleTime, unsigned char* pBuffer, long BufferLen) = 0;
};

struct __declspec(uuid("6b652fff-11fe-4fce-92ad-0266b5d7c78f")) ISampleGrabber : IUnknown
{
	virtual HRESULT __stdcall SetOneShot(long OneShot) = 0;
	virtual HRESULT __stdcall SetMediaType(struct _AMMediaType* pType) = 0;
	virtual HRESULT __stdcall GetConnectedMediaType(struct _AMMediaType* pType) = 0;
	virtual HRESULT __stdcall SetBufferSamples(long BufferThem) = 0;
	virtual HRESULT __stdcall GetCurrentBuffer(long* pBufferSize, long* pBuffer) = 0;
	virtual HRESULT __stdcall GetCurrentSample(struct IMediaSample** ppSample) = 0;
	virtual HRESULT __stdcall SetCallback(struct ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
};

struct __declspec(uuid("c1f400a0-3f08-11d3-9f0b-006008039e37")) SampleGrabber;

#pragma endregion
#endif


#ifndef MAXLONGLONG
#define MAXLONGLONG 0x7FFFFFFFFFFFFFFF
#endif

#ifndef MAX_DEVICE_NAME
#define MAX_DEVICE_NAME 80
#endif

#ifndef BITS_PER_PIXEL
#define BITS_PER_PIXEL 24
#endif

namespace usb_eyetoy
{
	namespace windows_api
	{
		std::vector<std::pair<std::string, std::string>> getDevList();

		typedef void (*DShowVideoCaptureCallback)(unsigned char* data, int len, int bitsperpixel);

		struct buffer_t
		{
			void* start = NULL;
			size_t length = 0;
		};

		class DirectShow : public VideoDevice
		{
		public:
			DirectShow();
			~DirectShow();
			int Open(int width, int height, FrameFormat format, int mirror);
			int Close();
			int GetImage(uint8_t* buf, size_t len);
			void SetMirroring(bool state);
			int Reset() { return 0; };

		protected:
			void SetCallback(DShowVideoCaptureCallback cb) { callbackhandler->SetCallback(cb); }
			bool Start();
			void Stop();
			int InitializeDevice(const std::wstring& selectedDevice);

		private:
			wil::unique_couninitialize_call dshowCoInitialize{false};
			Microsoft::WRL::ComPtr<ICaptureGraphBuilder2> pGraphBuilder;
			Microsoft::WRL::ComPtr<IFilterGraph2> pGraph;
			Microsoft::WRL::ComPtr<IMediaControl> pControl;

			Microsoft::WRL::ComPtr<IBaseFilter> sourcefilter;
			Microsoft::WRL::ComPtr<IAMStreamConfig> pSourceConfig;
			Microsoft::WRL::ComPtr<IBaseFilter> samplegrabberfilter;
			Microsoft::WRL::ComPtr<ISampleGrabber> samplegrabber;
			Microsoft::WRL::ComPtr<IBaseFilter> nullrenderer;

			class CallbackHandler : public ISampleGrabberCB
			{
			public:
				CallbackHandler() { callback = 0; }
				~CallbackHandler() {}

				void SetCallback(DShowVideoCaptureCallback cb) { callback = cb; }

				HRESULT __stdcall SampleCB(double time, IMediaSample* sample) override;
				HRESULT __stdcall BufferCB(double time, BYTE* buffer, long len) override { return S_OK; }
				HRESULT __stdcall QueryInterface(REFIID iid, LPVOID* ppv) override;
				ULONG __stdcall AddRef() override { return 1; }
				ULONG __stdcall Release() override { return 2; }

			private:
				DShowVideoCaptureCallback callback;

			} * callbackhandler;
		};

	} // namespace windows_api
} // namespace usb_eyetoy
