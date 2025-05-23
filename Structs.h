#pragma once

#include <Windows.h>

#define _USE_MATH_DEFINES // M_PIなどの定数を使うために必要
#include <cmath>          // sin, cos, M_PI など数学関数

#include<format>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <chrono>

#include <d3d12.h>
#pragma comment(lib,"d3d12.lib")
#include <Dxgi1_6.h>

#pragma comment(lib,"dxgi.lib")
#include <cassert>

// debug用の処理を作るときに使う呼び出し
#include <dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

// StringCcPrintfWを使うときに作る呼び出し
#include<strsafe.h>

// 最後の警告を作るときに使う呼び出し
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include<dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

// Transformするための呼び出し
#include "TransformFunctions.h"

// ImGuiを使うための宣言
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DirectXを使うための宣言
#include "externals/DirectXTex/DirectXTex.h"  // パスはプロジェクト構成により調整
#pragma comment(lib, "windowscodecs.lib")

#include"externals/DirectXTex/d3dx12.h"
#include"vector"




/*********************************************************
*構造体
*********************************************************/

struct Vector2 {
	float x;
	float y;
};

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

struct  Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};