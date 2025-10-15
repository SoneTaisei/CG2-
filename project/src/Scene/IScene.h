#pragma once

// 前方宣言
class SceneManager;

class IScene {
public:
    virtual ~IScene() = default;

    // 初期化
    virtual void Initialize() = 0;

    // 更新
    virtual void Update(SceneManager *sceneManager) = 0;

    // 描画
    virtual void Draw() = 0;
};