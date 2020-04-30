# QWinWidget (wangwenx190 version)

## Features

- The four frame borders are still there but the title bar is gone, so it's a full-featured window.
- High DPI scaling.
- Multi-monitor support (different resolution and DPI).

## Usage

```cpp
// include other files ...
#include "qwinwidget.h"
// include other files ...

// Anywhere you like, we use the main function here.
int main(int argc, char *argv[]) {
    // ...
    QWidget widget = new QWidget();
    QWinWidget winWidget(widget);
    winWidget.show();
    // ...
    const int exec = QApplication::exec();
    delete widget;
    return exec;
}
```

### Important Notes

For now, the content widget must be a pointer, the QWinWidget must not be a pointer, otherwise the application will report an error when closing. Still investigating. Sorry for the inconvenience.

## Supported Platforms

**Only Windows 10 is supported.**

Not because of using too new API, but the four frame borders in Windows 7 are too ugly that you definitely don't want to use it.

(Not tested on Windows 8/8.1 but I guess it will be no better than Windows 7)

## Problems to be solved

1. For a normal Win32 window, in light mode, the four window borders are semi-transparent white when it has focus, they will become semi-transparent light grayish when it loses focus.
2. For a normal Win32 window, in dark mode, the four window borders are semi-transparent dark grayish when it has focus, they will become semi-transparent light grayish (the same with when it's in light mode) when it loses focus. **[SOLVED]**
3. If the user choose to show the theme color on title bar and window border (only available on Windows 10), the border color will become the theme color with transparency when the window has focus, but when it loses focus, the border color is still semi-transparent light grayish. And when the user changes the theme color, the border color will also change, at the same time (notify signal: `WM_DWMCOLORIZATIONCOLORCHANGED`).

## Special Thanks

Thanks **Lucas** for testing this code in many various conditions.

Thanks [**Shujaat Ali Khan**](https://github.com/shujaatak) for searching so many useful articles and repositories for me.

## Licenses

### QWinWidget (modified version)

```text
Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
Contact: http://www.qt-project.org/legal

This file is part of the Qt Solutions component.

You may use this file under the terms of the BSD license as follows:

"Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
  * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
    of its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
```

### QWinNativeWindow

```text
MIT License

Copyright (C) 2020 by wangwenx190 (Yuhang Zhao)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
