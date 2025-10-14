# エンジンの使い方

## キーボード入力
### `IsKeyDown(BYTE keyCode)`
- **説明**  
  指定したキーが現在押され続けているかどうかを判定。  

- **引数**  
  - `BYTE keyCode` : 判定するキーの仮想キーコード（`VK_*` 定数を使用）。  

- **戻り値**  
  - `true` : キーが押されている。  
  - `false` : キーが押されていない。  

- **使用例**
  ```cpp
  if (KeyboardInput::GetInstance()->IsKeyDown(DIK_SPACE)) {
      // 処理
  }

### `IsKeyPressed(BYTE keyCode)`
- **説明**  
  指定したキーが「押された瞬間」かどうかを判定。  

- **引数**  
  - `BYTE keyCode` : 判定するキーの仮想キーコード（`VK_*` 定数を使用）。  

- **戻り値**  
  - `true` : キーが押されている。  
  - `false` : キーが押されていない。  

- **使用例**
  ```cpp
  if (KeyboardInput::GetInstance()->IsKeyPressed(DIK_SPACE)) {
      // 処理
  }

### `IsKeyReleased(BYTE keyCode);`
- **説明**  
  指定したキーが「離された瞬間」かどうかを判定。  

- **引数**  
  - `BYTE keyCode` : 判定するキーの仮想キーコード（`VK_*` 定数を使用）。  

- **戻り値**  
  - `true` : キーが押されている。  
  - `false` : キーが押されていない。  

- **使用例**
  ```cpp
  if (KeyboardInput::GetInstance()->IsKeyReleased(DIK_SPACE)) {
      // 処理
  }

## スプライト描画
### `PreDraw(ID3D12GraphicsCommandList *commandList)`
**説明**  
  スプライト描画の共通処理を行う。Draw関数を呼び出す前に、フレームごとに一度だけ呼び出す必要がある。主にグラフィックスパイプラインの設定などを行う。  

- **引数**  
  - `ID3D12GraphicsCommandList *commandList` : 描画コマンドを積むためのコマンドリスト。  

- **戻り値**  
- なし

- **使用例**
  ```cpp
  // 描画処理の最初で呼び出す
  Sprite::PreDraw(commandList);
  // この後に個別のスプライトを描画する
  Sprite::Draw(...);
  Sprite::Draw(...);
  
### `static void Draw(float destX, float destY, float width, float height, uint32_t textureHandle, float scaleX = 1.0f, float scaleY = 1.0f, float angle = 0.0f, const Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f });`
- **説明**
  指定したテクスチャを使用して、画面上の指定した位置にスプライトを描画する。拡大・縮小、回転、色付けも可能。

- **引数**
  - `float destX` : 描画先の左上X座標。
  - `float destY` : 描画先の左上Y座標。
  - `float width` : スプライトの幅。
  - `float height` : スプライトの高さ。
  - `uint32_t textureHandle` : 描画に使用するテクスチャのハンドル（インデックス）。
  - `float scaleX` (オプション) : X軸方向の拡縮率。デフォルトは`1.0f`。
  - `float scaleY` (オプション) : Y軸方向の拡縮率。デフォルトは`1.0f`。
  - `float angle` (オプション) : Z軸周りの回転角度（ラジアン）。デフォルトは`0.0f`。
  - `const Vector4 &color` (オプション) : スプライトに乗算する色（RGBA）。デフォルトは`{ 1.0f, 1.0f, 1.0f, 1.0f }`（白）。

- **戻り値**
  なし

- **使用例**
  ```cpp
  // 基本的な描画
  Sprite::Draw(100.0f, 150.0f, 64.0f, 64.0f, playerTexture);

  // 拡大・回転・色付けをして描画
  Sprite::Draw(
      300.0f, 250.0f, 128.0f, 128.0f, enemyTexture,
      1.5f, 1.5f, // 1.5倍に拡大
      3.14f / 4.0f, // 45度回転
      {1.0f, 0.5f, 0.5f, 1.0f} // 少し赤みがかった色にする
  );

