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
      // スペースキーが押され続けているときの処理
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
      // スペースキーが押され続けているときの処理
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
      // スペースキーが押され続けているときの処理
  }