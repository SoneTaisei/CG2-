#include "D3DResourceLeakChacker.h"
#include "Utility/Structs.h"

D3DResourceLeakChacker::~D3DResourceLeakChacker() {
	// リソースチェック
	Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
	if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(
			DXGI_DEBUG_ALL,
			DXGI_DEBUG_RLO_DETAIL     // 詳細情報を含めて列挙
		);
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}
}
