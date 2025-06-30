// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GSTexture11.h"
#include "GS/GSVector.h"
#include "GS/Renderers/Common/GSDevice.h"
#include "GS/Renderers/DX11/D3D11ShaderCache.h"

#include <string_view>
#include <unordered_map>

#include <wil/com.h>
#include <dxgi1_5.h>
#include <d3d11_1.h>
#include <wrl/client.h>

struct GSVertexShader11
{
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> il;
};

class GSDevice11 final : public GSDevice
{
public:
	using VSSelector = GSHWDrawConfig::VSSelector;
	using PSSelector = GSHWDrawConfig::PSSelector;
	using PSSamplerSelector = GSHWDrawConfig::SamplerSelector;
	using OMDepthStencilSelector = GSHWDrawConfig::DepthStencilSelector;

	union OMBlendSelector
	{
		struct
		{
			GSHWDrawConfig::ColorMaskSelector colormask;
			u8 pad[3];
			GSHWDrawConfig::BlendState blend;
		} s;
		u64 key;

		constexpr OMBlendSelector() : key(0) {}
		constexpr OMBlendSelector(GSHWDrawConfig::ColorMaskSelector colormask_, GSHWDrawConfig::BlendState blend_)
		{
			key = 0;
			s.colormask = colormask_;
			s.blend = blend_;
		}
	};
	static_assert(sizeof(OMBlendSelector) == sizeof(u64));

	class ShaderMacro
	{
		struct mcstr
		{
			const char *name, *def;
			mcstr(const char* n, const char* d)
				: name(n)
				, def(d)
			{
			}
		};

		struct mstring
		{
			std::string name, def;
			mstring(const char* n, std::string d)
				: name(n)
				, def(d)
			{
			}
		};

		std::vector<mstring> mlist;
		std::vector<mcstr> mout;

	public:
		void AddMacro(const char* n, int d);
		void AddMacro(const char* n, std::string d);
		D3D_SHADER_MACRO* GetPtr();
	};

private:
	enum : u32
	{
		MAX_TEXTURES = 4,
		MAX_SAMPLERS = 1,
		VERTEX_BUFFER_SIZE = 32 * 1024 * 1024,
		INDEX_BUFFER_SIZE = 16 * 1024 * 1024,
		NUM_TIMESTAMP_QUERIES = 5,
	};

	void SetFeatures(IDXGIAdapter1* adapter);

	u32 GetSwapChainBufferCount() const;
	bool CreateSwapChain();
	bool CreateSwapChainRTV();
	void DestroySwapChain();

	bool CreateTimestampQueries();
	void DestroyTimestampQueries();
	void PopTimestampQuery();
	void KickTimestampQuery();

	void DoMerge(GSTexture* sTex[3], GSVector4* sRect, GSTexture* dTex, GSVector4* dRect, const GSRegPMODE& PMODE, const GSRegEXTBUF& EXTBUF, u32 c, const bool linear) override;
	void DoInterlace(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, ShaderInterlace shader, bool linear, const InterlaceConstantBuffer& cb) override;
	void DoFXAA(GSTexture* sTex, GSTexture* dTex) override;
	void DoShadeBoost(GSTexture* sTex, GSTexture* dTex, const float params[4]) override;

	bool CreateCASShaders();
	bool DoCAS(GSTexture* sTex, GSTexture* dTex, bool sharpen_only, const std::array<u32, NUM_CAS_CONSTANTS>& constants) override;

	bool CreateImGuiResources();
	void RenderImGui();

	Microsoft::WRL::ComPtr<IDXGIFactory5> m_dxgi_factory;
	Microsoft::WRL::ComPtr<ID3D11Device1> m_dev;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_ctx;
	Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_annotation;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_swap_chain_rtv;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vb;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ib;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_expand_vb;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_expand_ib;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_expand_vb_srv;

	D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_10_0;
	u32 m_vb_pos = 0; // bytes
	u32 m_ib_pos = 0; // indices/sizeof(u32)
	u32 m_structured_vb_pos = 0; // bytes

	bool m_allow_tearing_supported = false;
	bool m_using_flip_model_swap_chain = true;
	bool m_using_allow_tearing = false;
	bool m_is_exclusive_fullscreen = false;

	struct
	{
		D3D11_PRIMITIVE_TOPOLOGY topology;
		std::array<ID3D11ShaderResourceView*, MAX_TEXTURES> ps_sr_views;
		std::array<ID3D11ShaderResourceView*, MAX_TEXTURES> ps_cached_sr_views;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> layout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vs_cb;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
		Microsoft::WRL::ComPtr<ID3D11Buffer> ps_cb;
		std::array<ID3D11SamplerState*, MAX_SAMPLERS> ps_ss;
		std::array<ID3D11SamplerState*, MAX_SAMPLERS> ps_cached_ss;
		GSVector2i viewport;
		GSVector4i scissor;
		u32 vb_stride;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss;
		u8 sref;
		Microsoft::WRL::ComPtr<ID3D11BlendState> bs;
		u8 bf;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rt_view;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
		GSTexture* cached_rt_view;
		GSTexture* cached_dsv;
	} m_state;

	std::array<std::array<Microsoft::WRL::ComPtr<ID3D11Query>, 3>, NUM_TIMESTAMP_QUERIES> m_timestamp_queries = {};
	float m_accumulated_gpu_time = 0.0f;
	u8 m_read_timestamp_query = 0;
	u8 m_write_timestamp_query = 0;
	u8 m_waiting_timestamp_queries = 0;
	bool m_timestamp_query_started = false;
	bool m_gpu_timing_enabled = false;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11InputLayout> il;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps[static_cast<int>(ShaderConvert::Count)];
		Microsoft::WRL::ComPtr<ID3D11SamplerState> ln;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> pt;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss_write;
		std::array<Microsoft::WRL::ComPtr<ID3D11BlendState>, 16> bs;
	} m_convert;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11InputLayout> il;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps[static_cast<int>(PresentShader::Count)];
		Microsoft::WRL::ComPtr<ID3D11Buffer> ps_cb;
	} m_present;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps[2];
		Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
		Microsoft::WRL::ComPtr<ID3D11BlendState> bs;
	} m_merge;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps[NUM_INTERLACE_SHADERS];
		Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
	} m_interlace;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_fxaa_ps;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
		Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
	} m_shadeboost;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> dss;
		Microsoft::WRL::ComPtr<ID3D11BlendState> bs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> primid_init_ps[4];
	} m_date;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs_upscale;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs_sharpen;
	} m_cas;

	struct
	{
		Microsoft::WRL::ComPtr<ID3D11InputLayout> il;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;
		Microsoft::WRL::ComPtr<ID3D11BlendState> bs;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vs_cb;
	} m_imgui;

	// Shaders...

	std::unordered_map<u32, GSVertexShader11> m_vs;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vs_cb;
	std::unordered_map<u32, Microsoft::WRL::ComPtr<ID3D11GeometryShader>> m_gs;
	std::unordered_map<PSSelector, Microsoft::WRL::ComPtr<ID3D11PixelShader>, GSHWDrawConfig::PSSelectorHash> m_ps;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ps_cb;
	std::unordered_map<u32, Microsoft::WRL::ComPtr<ID3D11SamplerState>> m_ps_ss;
	std::unordered_map<u32, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> m_om_dss;
	std::unordered_map<u64, Microsoft::WRL::ComPtr<ID3D11BlendState>> m_om_bs;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rs;

	GSHWDrawConfig::VSConstantBuffer m_vs_cb_cache;
	GSHWDrawConfig::PSConstantBuffer m_ps_cb_cache;

	D3D11ShaderCache m_shader_cache;
	std::string m_tfx_source;

protected:
	virtual void DoStretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect,
		GSHWDrawConfig::ColorMaskSelector cms, ShaderConvert shader, bool linear) override;

public:
	GSDevice11();
	~GSDevice11() override;

	static void SetD3DDebugObjectName(ID3D11DeviceChild* obj, std::string_view name);

	__fi static GSDevice11* GetInstance() { return static_cast<GSDevice11*>(g_gs_device.get()); }
	__fi ID3D11Device1* GetD3DDevice() const { return m_dev.Get(); }
	__fi ID3D11DeviceContext1* GetD3DContext() const { return m_ctx.Get(); }

	bool Create(GSVSyncMode vsync_mode, bool allow_present_throttle) override;
	void Destroy() override;

	RenderAPI GetRenderAPI() const override;

	bool UpdateWindow() override;
	void ResizeWindow(u32 new_window_width, u32 new_window_height, float new_window_scale) override;
	bool SupportsExclusiveFullscreen() const override;
	bool HasSurface() const override;
	void DestroySurface() override;
	std::string GetDriverInfo() const override;

	void SetVSyncMode(GSVSyncMode mode, bool allow_present_throttle) override;

	PresentResult BeginPresent(bool frame_skip) override;
	void EndPresent() override;

	bool SetGPUTimingEnabled(bool enabled) override;
	float GetAndResetAccumulatedGPUTime() override;

	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void DrawIndexedPrimitive(int offset, int count);

	void PushDebugGroup(const char* fmt, ...) override;
	void PopDebugGroup() override;
	void InsertDebugMessage(DebugMessageCategory category, const char* fmt, ...) override;

	GSTexture* CreateSurface(GSTexture::Type type, int width, int height, int levels, GSTexture::Format format) override;
	std::unique_ptr<GSDownloadTexture> CreateDownloadTexture(u32 width, u32 height, GSTexture::Format format) override;

	void CommitClear(GSTexture* t);

	void CopyRect(GSTexture* sTex, GSTexture* dTex, const GSVector4i& r, u32 destX, u32 destY) override;

	void DoStretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear);
	void DoStretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear);
	void PresentRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, PresentShader shader, float shaderTime, bool linear) override;
	void UpdateCLUTTexture(GSTexture* sTex, float sScale, u32 offsetX, u32 offsetY, GSTexture* dTex, u32 dOffset, u32 dSize) override;
	void ConvertToIndexedTexture(GSTexture* sTex, float sScale, u32 offsetX, u32 offsetY, u32 SBW, u32 SPSM, GSTexture* dTex, u32 DBW, u32 DPSM) override;
	void FilteredDownsampleTexture(GSTexture* sTex, GSTexture* dTex, u32 downsample_factor, const GSVector2i& clamp_min, const GSVector4& dRect) override;
	void DrawMultiStretchRects(const MultiStretchRect* rects, u32 num_rects, GSTexture* dTex, ShaderConvert shader) override;
	void DoMultiStretchRects(const MultiStretchRect* rects, u32 num_rects, const GSVector2& ds);

	void SetupDATE(GSTexture* rt, GSTexture* ds, SetDATM datm, const GSVector4i& bbox);

	void* IAMapVertexBuffer(u32 stride, u32 count);
	void IAUnmapVertexBuffer(u32 stride, u32 count);
	bool IASetVertexBuffer(const void* vertex, u32 stride, u32 count);
	bool IASetExpandVertexBuffer(const void* vertex, u32 stride, u32 count);

	u16* IAMapIndexBuffer(u32 count);
	void IAUnmapIndexBuffer(u32 count);
	bool IASetIndexBuffer(const void* index, u32 count);
	void IASetIndexBuffer(ID3D11Buffer* buffer);

	void IASetInputLayout(ID3D11InputLayout* layout);
	void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);

	void VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb);

	void PSSetShaderResource(int i, GSTexture* sr);
	void PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb);
	void PSUpdateShaderState(const bool sr_update, const bool ss_update);
	void PSUnbindConflictingSRVs(GSTexture* tex1 = nullptr, GSTexture* tex2 = nullptr);
	void PSSetSamplerState(ID3D11SamplerState* ss0);

	void OMSetDepthStencilState(ID3D11DepthStencilState* dss, u8 sref);
	void OMSetBlendState(ID3D11BlendState* bs, u8 bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = nullptr, ID3D11DepthStencilView* read_only_dsv = nullptr);
	void SetViewport(const GSVector2i& viewport);
	void SetScissor(const GSVector4i& scissor);

	void SetupVS(VSSelector sel, const GSHWDrawConfig::VSConstantBuffer* cb);
	void SetupPS(const PSSelector& sel, const GSHWDrawConfig::PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, u8 afix);

	void RenderHW(GSHWDrawConfig& config) override;
	void SendHWDraw(const GSHWDrawConfig& config, GSTexture* draw_rt_clone, GSTexture* draw_rt, const bool one_barrier, const bool full_barrier, const bool skip_first_barrier);

	void ClearSamplerCache() override;

	ID3D11Device1* operator->() { return m_dev.Get(); }
	operator ID3D11Device1*() { return m_dev.Get(); }
	operator ID3D11DeviceContext1*() { return m_ctx.Get(); }
};
