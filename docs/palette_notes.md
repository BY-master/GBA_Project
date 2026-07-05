# GBA / Butano 调色板笔记

这份笔记用来记录本项目里 sprite 调色板、palette swap 和 `cavegirl_alt.bmp` 的相关知识。

## 1. Sprite 调色板是什么？

在 GBA 上，sprite 图片的像素通常不是直接保存 RGB 颜色，而是保存 **颜色索引**。

可以理解为：

```text
像素值 0 -> 调色板第 0 个颜色
像素值 1 -> 调色板第 1 个颜色
像素值 2 -> 调色板第 2 个颜色
...
```

真正的颜色存在 palette，也就是调色板表里。

如果是 4bpp sprite，一套 palette 通常有 16 个颜色：

```text
index 0  -> 通常是透明色
index 1  -> 颜色 1
index 2  -> 颜色 2
...
index 15 -> 颜色 15
```

所以 sprite 里面的像素值不是“红色/绿色/蓝色”，而是“使用第几个颜色”。

## 2. Palette swap 是什么？

Palette swap，意思是 **调色板替换**。

它的核心是：

```text
不替换 sprite 图片
不替换 sprite 像素数据
只替换调色板里的颜色
```

例如原来：

```text
index 5 = 红色衣服
```

替换 palette 后：

```text
index 5 = 蓝色衣服
```

那么所有使用 index `5` 的像素，都会从红色变成蓝色。

Sprite 的形状不会变，因为像素索引没有变；变的只是索引对应的颜色。

## 3. `cavegirl.bmp` 和 `cavegirl_alt.bmp` 的区别

本项目里有：

```text
graphics/cavegirl.bmp
graphics/cavegirl.json
```

其中 `cavegirl.json` 是：

```json
{
    "type": "sprite",
    "height": 64
}
```

这表示 `cavegirl.bmp` 是真正的 sprite 图片资源。

它会提供：

```text
1. sprite 图形 / 像素索引数据
2. 默认 sprite 调色板
```

而：

```text
graphics/cavegirl_alt.bmp
graphics/cavegirl_alt.json
```

其中 `cavegirl_alt.json` 是：

```json
{
    "type": "sprite_palette"
}
```

这表示 `cavegirl_alt.bmp` **不是一张 sprite 图片**，而是一个 **sprite 调色板资源**。

也就是说，Butano 不会拿 `cavegirl_alt.bmp` 的图案来显示，只会读取它里面的 palette 数据。

## 4. 为什么 `cavegirl_alt.bmp` 可以是一张纯绿色图片？

对于：

```json
{
    "type": "sprite_palette"
}
```

这种资源来说，BMP 的画面内容基本不重要。

重要的是 BMP 文件内部保存的 **indexed palette 调色板数据**。

也就是说：

```text
画布上看起来是什么图案，不是重点。
BMP 里面的 palette 顺序和颜色，才是重点。
```

所以 `cavegirl_alt.bmp` 看起来是一整块绿色也没关系，只要它内部的 palette 是正确的。

## 5. 调色板数据存在哪里？

构建项目后，Butano / grit 会在 `build/` 目录生成资源文件。

例如：

```text
build/bn_sprite_palette_items_cavegirl_alt.h
build/cavegirl_alt_bn_gfx.s
```

Header 文件里会把它包装成 Butano 的资源对象：

```cpp
bn::sprite_palette_items::cavegirl_alt
```

真正的颜色数据在 `.s` 文件里，例如：

```asm
cavegirl_alt_bn_gfxPal:
    .hword 0x03E0,0x0C64,0x4000,0x0010,0x5000,0x0014,0x6000,0x0018
    .hword 0x7000,0x19DD,0x1E9F,0x7FFF,0x001F,0x001F,0x001F,0x001F
```

这些 `.hword` 就是 16 个 GBA 颜色值。

每个颜色是 16-bit 数据，GBA 实际使用的是 15-bit BGR 颜色格式。

编译后，这些调色板数据会放在 ROM 的只读数据区。

运行时调用 `set_colors()` 时，Butano 会把这些颜色复制到 GBA 的 OBJ Palette RAM，也就是 sprite 调色板显存。

## 6. `set_colors()` 做了什么？

代码：

```cpp
bn::sprite_palette_ptr cavegirl_palette = cavegirl_sprite.palette();
cavegirl_palette.set_colors(bn::sprite_palette_items::cavegirl_alt);
```

可以理解为：

```text
读取 cavegirl_alt 的调色板颜色
复制到 cavegirl 当前使用的 sprite palette 槽位
GBA 用新的颜色表重新显示同一张 sprite
```

它不会替换 `cavegirl.bmp` 的像素图案。

它只是替换颜色表。

## 7. 如何自定义一个 alt palette？

假设要做一个蓝色版本：

```text
graphics/cavegirl_blue.bmp
graphics/cavegirl_blue.json
```

### 第一步：复制原始 BMP

复制：

```text
graphics/cavegirl.bmp
```

改名为：

```text
graphics/cavegirl_blue.bmp
```

这样做最安全，因为可以保留原来的 palette index 顺序。

### 第二步：用软件修改 palette

用支持 indexed color 的软件打开 `cavegirl_blue.bmp`，比如：

- Aseprite
- Usenti
- GIMP
- GraphicsGale
- LibreSprite

重点是：

```text
修改调色板面板里的颜色
不要随便重排 palette index
```

例如：

```text
index 5 = 衣服亮色
index 6 = 衣服暗色
```

你可以把 index 5 和 index 6 改成蓝色系。

但是不要把 index 5 移动到 index 2。

### 第三步：创建 JSON 文件

创建：

```text
graphics/cavegirl_blue.json
```

内容：

```json
{
    "type": "sprite_palette"
}
```

### 第四步：在代码中 include

重新编译后，Butano 会生成类似：

```cpp
bn_sprite_palette_items_cavegirl_blue.h
```

然后在 `src/main.cpp` 里 include：

```cpp
#include "bn_sprite_palette_items_cavegirl_blue.h"
```

### 第五步：使用新 palette

```cpp
cavegirl_palette.set_colors(bn::sprite_palette_items::cavegirl_blue);
```

然后重新编译：

```powershell
make -j8
```

## 8. 重要注意事项

- `sprite_palette` 类型的 BMP，画面内容基本不重要。
- 真正重要的是 BMP 内部的 indexed palette 数据。
- 原始 sprite 和 alt palette 的 index 顺序要一致。
- 不要让图片编辑器自动重排 palette。
- sprite 的 index `0` 通常是透明色。
- `set_colors()` 是整套调色板替换，不是只替换某一个 index。
- 如果换色后颜色乱了，通常是 palette index 顺序乱了。

## 9. 一句话总结

```text
cavegirl.bmp 负责 sprite 的形状和像素索引。
cavegirl_alt.bmp 负责提供另一套颜色表。
palette swap 的本质是：像素索引不变，只替换 index 对应的颜色。
```
