# Dependencies and research

- SDL 3.4.16, zlib license: https://github.com/libsdl-org/SDL/tree/release-3.4.16
- nlohmann/json 3.12.0, MIT: `third_party/json.LICENSE.MIT`.
- dr_mp3, commit `dfe8377631000664666519fdb83da193fd8037f4`, public domain or MIT-0; license text included at the end of `third_party/dr_mp3.h`.
- Emscripten SDK 6.0.10, MIT/University of Illinois licenses; copied notices for Emscripten, libc++, libc++abi and musl are in `third_party/licenses`. SDK tools are not shipped with the prototype.
- Source Han Serif SC 2.003R, OFL 1.1. Reuses the prepared CaoCaoSong font from the previous project. The font and its OFL notice are imported together. No game artwork was generated or replaced.

Research references (not authorities for this edition's simulation):

- Original LS format research: https://www.xycq.org.cn/forum/thread-34612-1-1.html
- Original EEX research: https://xycq.org.cn/forum/viewthread.php?authoruid=25461&tid=35258
- Original-era numeric instruction definitions: https://www.xycq.org.cn/forum/attachment.php?aid=23574
- cczEditor, commit `eb0f07daf43c4d5e7a854f4fe5060ae0621e8bec`: https://gitee.com/BulletHermit/cczEditor . MIT. Consulted for EEX nesting and parameter meanings.
- ccz-for-iOS, commit `0d322d44953dfe073b4261e9d19ddeaf2ecfee14`: https://gitee.com/BulletHermit/ccz-for-iOS . MIT. Consulted for sprite frames and the provisional scene projection. Its random generator differs from the supplied executable and is not used.

The two research repositories are cached under `.cache/research` and are not build dependencies or distributed artwork. The engine's numeric random generator comes from the user's executable; bounded x86 probes provide reproducible fixtures without Wine or a Windows loader. Original game resources remain the original publisher's material and are imported from the user's local directory.

## MIT research notices

Copyright (c) 2015 訾飞
Copyright (c) 2015 子弹大仙

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
