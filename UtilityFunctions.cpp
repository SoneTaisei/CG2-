#include "UtilityFunctions.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	// メッセージに応じてゲーム固有の処理を行う
	switch(msg) {
		// ウィンドウが破壊された
	case WM_DESTROY:
		// OSに応じて、アプリ固有の終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string &message) {
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string &str) {
	if(str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if(sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring &str) {
	if(str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if(sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

std::string str0{ "STRING" };

std::string str1{ std::to_string(10) };

LONG WINAPI ExportDump(EXCEPTION_POINTERS *exception) {
	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processId(exeID)とクラッシュ(例外)の発生したthreadIDを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// ほかに関連付けられているSEH例外ハンドラがあれば実行。通常プロセスを終了する。
	return EXCEPTION_EXECUTE_HANDLER;
}

IDxcBlob *CompileShader(
	// CompilerするShaderファイルへのパス
	const std::wstring &filePath,
	// Compilerに使用するProfile
	const wchar_t *profile,
	// 初期化で生成したものを3つ
	IDxcUtils *dxcUtils,
	IDxcCompiler3 *dxcCompiler,
	IDxcIncludeHandler *includeHandler) {

	/*********************************************************
	*1. hlslファイルを読む
	*********************************************************/

	// これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
	// hlslファイルを読む
	IDxcBlobEncoding *shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// あきらめなかったら止める
	assert(SUCCEEDED(hr));
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;// UTF8の文字コードであることを通知

	/*********************************************************
	*2.Compileする
	*********************************************************/

	LPCWSTR arguments[] = {
	  filePath.c_str(),// コンパイル対象のhlslファイル名
	  L"-E",L"main",// エントリーポイントの指定。基本的にmain以外にはしない
	  L"-T",profile,// ShaderProfileの設定
	  L"-Zi",L"-Qembed_debug",// デバッグ用の情報を埋め込む
	  L"-Od",// 最適化を外しておく
	  L"-Zpr",// メモリレイアウトは行優先
	};
	// 実際にShaderをコンパイルする
	IDxcResult *shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	// コンパイラエラーではなくdxcが起動できないなどの致命的なエラー
	assert(SUCCEEDED(hr));

	/*********************************************************
	*3.警告・エラーが出ていないか確認
	*********************************************************/

	IDxcBlobUtf8 *shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if(shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		//警告・エラー絶対ダメ
		assert(false);
	}

	/*********************************************************
	*4.Compile結果を受け取って返す
	*********************************************************/

	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob *shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功押したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded, path:{},profile:{}\n", filePath, profile)));
	// もう使わないリソースを開放
	shaderSource->Release();
	shaderResult->Release();
	// 実行用のバイナリを返却
	return shaderBlob;
}

ID3D12Resource *CreateBufferResource(ID3D12Device *device, size_t sizeInBytes) {
	assert(device != nullptr); // 安全チェック

	// アップロード用のヒープの設定（CPUからGPUにデータを送る用）
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際にリソース（バッファ）を作成
	ID3D12Resource *resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON, // 初期状態（読み取り用）
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr)); // 成功してるか確認

	return resource; // 作ったバッファを返す！
}

// DescriptorHeapの作成関数
ID3D12DescriptorHeap *CreateDescriptorHeap(
	ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = heapType;
	desc.NumDescriptors = numDescriptors;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap *heap = nullptr;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	assert(SUCCEEDED(hr));
	return heap;
}

// Textureデータを読む
DirectX::ScratchImage LoadTexture(const std::string &filePath) {
	// ★ ファイルパス確認用のログ
	OutputDebugStringA(("LoadTexture: " + filePath + "\n").c_str());
	
	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミニマップ付きのデータを返す
	return mipImages;

}

// DirectX12のTextureResourceを作る
ID3D12Resource *CreateTextureResource(ID3D12Device *device, const DirectX::TexMetadata &metadata) {
	// metadataをもとにResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);// 横幅
	resourceDesc.Height = UINT(metadata.height);// 高さ
	resourceDesc.MipLevels = UINT(metadata.mipLevels);// mipmapの数
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);// 奥行き ro 配列Textureの配列数
	resourceDesc.Format = metadata.format;// TextureのFormat
	resourceDesc.SampleDesc.Count = 1;// サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);// Textureの次元数。2次元

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// 細かい設定を行う

	// Resourceの生成
	ID3D12Resource *resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,// Heapの設定
		D3D12_HEAP_FLAG_NONE,// Heapの特殊な設定。今回はなし
		&resourceDesc,// Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,// 初回のResourceState。
		nullptr,//Clear最適値。今回は使わない
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}

// 戻り値を破損してはならないのでこれを付ける
[[nodiscard]]
// TextureResouorceにデータを転送する
ID3D12Resource *UploadTextureData(ID3D12Resource *texture, const DirectX::ScratchImage &mipImages, ID3D12Device *device, ID3D12GraphicsCommandList *commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA>subresources;
	// 読み込んだデータからDirectX12用のSubresourceの配列を作成
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	// IntermediateResourceに必要なサイズを計算する
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	// 計算したサイズでIntermediateResourceを作る
	ID3D12Resource *intermediateResource = CreateBufferResource(device, intermediateSize);
	// データ転送をコマンドに積む
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	// Tetureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

ID3D12Resource *CreateDepthStencilTextureResource(ID3D12Device *device, int32_t width, int32_t height) {
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;// 奥行き
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;// 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深層値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;// フォーマットをResourceと合わせる

	// Resourceの設定
	ID3D12Resource *resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);

	assert(SUCCEEDED(hr));
	return resource;
}

// DescriptorHandleを取得する(CPU)
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

// DescriptorHandleを取得する(GPU)
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap *descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}



void CreateSphereMesh(std::vector<VertexData> &vertices, std::vector<uint32_t> &indices, float radius, int latDiv, int lonDiv) {
	// 緯度の分割数: 上から下へ何段に分けるか
	// 経度の分割数: 横に何分割するか（赤道の輪切りみたいなイメージ）

	// 頂点の生成（緯度方向にループ）
	for(int lat = 0; lat <= latDiv; ++lat) {
		float theta = lat * float(M_PI) / float(latDiv); // 緯度の角度（0 ~ π）
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);

		// 経度方向にループ
		for(int lon = 0; lon <= lonDiv; ++lon) {
			float phi = lon * 2.0f * float(M_PI) / float(lonDiv); // 経度の角度（0 ~ 2π）
			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			// 球のx, y, z座標を求める
			float x = cosPhi * sinTheta;
			float y = cosTheta;
			float z = sinPhi * sinTheta;

			// 頂点データを作成
			VertexData v{};
			v.position = { radius * x, radius * y, radius * z, 1.0f }; // 球の表面上の点
			v.normal = TransformFunctions::Normalize({ v.position.x,v.position.y,v.position.z });
			v.texcoord = { float(lon) / lonDiv, float(lat) / latDiv }; // UV座標（テクスチャ用）

			vertices.push_back(v); // 頂点リストに追加
		}
	}
	// 三角形インデックスの生成（頂点をつなぐ）
	for(int lat = 0; lat < latDiv; ++lat) {
		for(int lon = 0; lon < lonDiv; ++lon) {
			// 現在の行・列から頂点の番号を計算
			int first = lat * (lonDiv + 1) + lon;
			int second = first + lonDiv + 1;

			// 二つの三角形を使って四角形を埋める
			indices.push_back(first);         // 左上
			indices.push_back(first + 1);     // 右上
			indices.push_back(second);        // 左下

			indices.push_back(second);        // 左下
			indices.push_back(first + 1);     // 右上
			indices.push_back(second + 1);    // 右下
		}
	}
}

ModelData LoadObjFile(const std::string &directoryPath, const std::string &filename) {
	ModelData modelData;// 構築するModelData
	std::vector<Vector4>positions;// 位置
	std::vector<Vector3>normals;// 法線
	std::vector<Vector2>texcoords;// テクスチャの座標
	std::string line;// ファイルから読んだ1行を格納する

	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());// 開けなかったら止める

	while(std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;// 先頭の識別子を読む

		// identifierに応じた処理
		if(identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		} else if(identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		} else if(identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		} else if(identifier == "f") {
			// 面は三角形限定。その他は未対応
			VertexData triangle[3];
			for(int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for(int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				normal.x *= -1.0f;
				texcoord.y = 1.0f - texcoord.y;
				triangle[faceVertex] = { position,texcoord,normal };
			}
			// 頂点を逆順で登録数rことで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);

		} else if(identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;

			OutputDebugStringA(("LoadMaterialTemplateFile: " + directoryPath + "/" + materialFilename + "\n").c_str());

			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;

}

MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
	MaterialData materialData;// 構築するMaterialData
	std::string line;//　ファイルから読んだ1行目を格納する
	std::ifstream file(directoryPath + "/" + filename);// ファイルを開く
	assert(file.is_open());// 開けなかったら止める

	while(std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if(identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		
			OutputDebugStringA(("Material Texture Path: " + materialData.textureFilePath + "\n").c_str());
		}
	}
	return materialData;
}
