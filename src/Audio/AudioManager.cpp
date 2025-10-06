#include "AudioManager.h"
#include <cassert>

// 静的メンバ変数の実体を定義
Microsoft::WRL::ComPtr<IXAudio2> AudioManager::xAudio2_ = nullptr;
IXAudio2MasteringVoice *AudioManager::masterVoice_ = nullptr;
std::list<std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>> AudioManager::playingVoices_;

void AudioManager::Initialize() {
    HRESULT result;
    // XAudioエンジンのインスタンスを生成
    result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスの生成
    result = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(result));
}

void AudioManager::Finalize() {
    // playingVoices_はunique_ptrが自動で解放してくれるので、ここでは何もしなくて良い
    xAudio2_.Reset();
}

void AudioManager::Play(const SoundData &soundData) {
    HRESULT result;

    // 1. SourceVoiceを生成
    IXAudio2SourceVoice *pSourceVoiceRaw = nullptr;
    result = xAudio2_->CreateSourceVoice(&pSourceVoiceRaw, &soundData.wfex);
    assert(SUCCEEDED(result));

    // 2. 作成したボイスをカスタムデリータ付きのunique_ptrでラップ
    std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter> pSourceVoice(pSourceVoiceRaw);

    // 3. 再生する波形データの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // 4. 波形データを登録して再生開始
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    assert(SUCCEEDED(result));
    result = pSourceVoice->Start();
    assert(SUCCEEDED(result));

    // 5. 所有権をリストに移動して管理対象にする
    playingVoices_.push_back(std::move(pSourceVoice));
}

void AudioManager::Update() {
    // 再生が終わったボイスをリストから削除する
    playingVoices_.remove_if([](const auto &voice) {
        XAUDIO2_VOICE_STATE state{};
        voice->GetState(&state);
        // バッファのキューが空になったら再生終了とみなす
        return state.BuffersQueued == 0;
                             });
}