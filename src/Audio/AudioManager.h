#pragma once
#include "Utility/Structs.h" // SoundDataなどを使うために必要
#include <list>
#include <memory> // std::unique_ptr を使うために必要

// IXAudio2SourceVoiceを自動で破棄するためのカスタムデリータ
struct SourceVoiceDeleter {
    void operator()(IXAudio2SourceVoice *p) const {
        if(p) {
            // Release()ではなくDestroyVoice()を呼ぶ
            p->DestroyVoice();
        }
    }
};

class AudioManager {
public:
    // 初期化
    static void Initialize();
    // 終了処理
    static void Finalize();

    // 音声再生
    static void Play(const SoundData &soundData);

    // 毎フレームの更新処理（再生が終わったボイスを破棄する）
    static void Update();

private:
    // XAudio2の本体など
    static Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    static IXAudio2MasteringVoice *masterVoice_;

    // 再生中のボイスを管理するリスト
    static std::list<std::unique_ptr<IXAudio2SourceVoice, SourceVoiceDeleter>> playingVoices_;
};