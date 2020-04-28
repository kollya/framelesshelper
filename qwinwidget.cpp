#include "qwinwidget.h"

/*
 * MIT License
 *
 * Copyright (C) 2020 by wangwenx190 (Yuhang Zhao)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dwmapi.h>
#include <shellapi.h>
#include <windowsx.h>

namespace {

LPCWSTR szWindowTitle = L"Window title";
LPCWSTR szWindowClassName = L"Window class name";

// The thickness of an auto-hide taskbar in pixels.
const int kAutoHideTaskbarThicknessPx = 2;
const int kAutoHideTaskbarThicknessPy = kAutoHideTaskbarThicknessPx;

UINT GetDotsPerInchForWindow(HWND handle) {
    if (handle && IsWindow(handle)) {
        return GetDpiForWindow(handle);
    }
    return GetSystemDpiForProcess(GetCurrentProcess());
}

qreal GetDevicePixelRatioForWindow(HWND handle) {
    return static_cast<qreal>(GetDotsPerInchForWindow(handle)) /
        static_cast<qreal>(USER_DEFAULT_SCREEN_DPI);
}

BOOL IsDwmCompositionEnabled() {
    // Since Win8, DWM composition is always enabled and can't be disabled.
    // In other words, DwmIsCompositionEnabled will always return TRUE on
    // systems newer than Win7.
    BOOL enabled = FALSE;
    return SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled;
}

BOOL IsFullScreen(HWND handle) {
    if (handle && IsWindow(handle)) {
        WINDOWINFO windowInfo;
        SecureZeroMemory(&windowInfo, sizeof(windowInfo));
        windowInfo.cbSize = sizeof(windowInfo);
        GetWindowInfo(handle, &windowInfo);
        MONITORINFO monitorInfo;
        SecureZeroMemory(&monitorInfo, sizeof(monitorInfo));
        monitorInfo.cbSize = sizeof(monitorInfo);
        const HMONITOR monitor =
            MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
        GetMonitorInfoW(monitor, &monitorInfo);
        // The only way to judge whether a window is fullscreen or not
        // is to compare it's size with the screen's size, there is no official
        // Win32 API to do this for us.
        return EqualRect(&windowInfo.rcWindow, &monitorInfo.rcMonitor) ||
            EqualRect(&windowInfo.rcClient, &monitorInfo.rcMonitor);
    }
    return FALSE;
}

BOOL IsTopLevel(HWND handle) {
    if (handle && IsWindow(handle)) {
        if (GetWindowLongPtrW(handle, GWL_STYLE) & WS_CHILD) {
            return FALSE;
        }
        const HWND parent = GetAncestor(handle, GA_PARENT);
        if (parent && (parent != GetDesktopWindow())) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

RECT GetFrameSizeForWindow(HWND handle, BOOL includingTitleBar = FALSE) {
    RECT frame = {0, 0, 0, 0};
    if (handle && IsWindow(handle)) {
        const auto style = GetWindowLongPtrW(handle, GWL_STYLE);
        AdjustWindowRectExForDpi(&frame,
                                 includingTitleBar ? (style | WS_CAPTION)
                                                   : (style & ~WS_CAPTION),
                                 FALSE, GetWindowLongPtrW(handle, GWL_EXSTYLE),
                                 GetDotsPerInchForWindow(handle));
        frame.top = -frame.top;
        frame.left = -frame.left;
    }
    return frame;
}

VOID UpdateFrameMarginsForWindow(HWND handle) {
    MARGINS margins = {0, 0, 0, 0};
    if (handle && IsWindow(handle)) {
        // We removed the whole top part of the frame (see handling of
        // WM_NCCALCSIZE) so the top border is missing now. We add it back here.
        // Note #1: You might wonder why we don't remove just the title bar
        // instead of removing the whole top part of the frame and then adding
        // the little top border back. I tried to do this but it didn't work:
        // DWM drew the whole title bar anyways on top of the window. It seems
        // that DWM only wants to draw either nothing or the whole top part of
        // the frame.
        // Note #2: For some reason if you try to set the top margin to just
        // the top border height (what we want to do), then there is a
        // transparency bug when the window is inactive, so I've decided to add
        // the whole top part of the frame instead and then we will hide
        // everything that we don't need (that is, the whole thing but the
        // little 1 pixel wide border at the top) in the WM_PAINT handler.
        // This eliminates the transparency bug and it's what a lot of Win32
        // apps that customize the title bar do so it should work fine.
        margins.cyTopHeight = GetFrameSizeForWindow(handle, TRUE).top;
    }
    DwmExtendFrameIntoClientArea(handle, &margins);
}

VOID RefreshWindowStyle(HWND handle) {
    if (handle && IsWindow(handle)) {
        SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSIZE |
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }
}

} // namespace

QWinNativeWindow::QWinNativeWindow(const int x, const int y, const int width,
                                   const int height) {
    const HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wcex;
    SecureZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClassName;
    wcex.hCursor = LoadCursorW(hInstance, IDC_ARROW);

    RegisterClassExW(&wcex);

    const qreal dpr = GetDevicePixelRatioForWindow(nullptr);

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW, szWindowClassName, szWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, x, y,
        width == CW_USEDEFAULT ? width
                               : qRound(static_cast<qreal>(width) * dpr),
        height == CW_USEDEFAULT ? height
                                : qRound(static_cast<qreal>(height) * dpr),
        nullptr, nullptr, hInstance, static_cast<LPVOID>(this));

    Q_ASSERT(m_hWnd);
}

QWinNativeWindow::~QWinNativeWindow() {
    ShowWindow(m_hWnd, SW_HIDE);
    DestroyWindow(m_hWnd);
};

void QWinNativeWindow::setMinimumSize(const QSize &size) {
    m_minimumSize = size;
}

void QWinNativeWindow::setMinimumSize(const int width, const int height) {
    m_minimumSize = QSize(width, height);
}

QSize QWinNativeWindow::getMinimumSize() const { return m_minimumSize; }

void QWinNativeWindow::setMaximumSize(const QSize &size) {
    m_maximumSize = size;
}

void QWinNativeWindow::setMaximumSize(const int width, const int height) {
    m_maximumSize = QSize(width, height);
}

QSize QWinNativeWindow::getMaximumSize() const { return m_maximumSize; }

void QWinNativeWindow::setGeometry(const QRect &geometry) {
    setGeometry(geometry.x(), geometry.y(), geometry.width(),
                geometry.height());
}

void QWinNativeWindow::setGeometry(const int x, const int y, const int width,
                                   const int height) {
    const qreal dpr = GetDevicePixelRatioForWindow(m_hWnd);
#if 0
    MoveWindow(m_hWnd, x - GetFrameSizeForWindow(m_hWnd).left - 1, y - 1,
               qRound(static_cast<qreal>(width) * dpr),
               qRound(static_cast<qreal>(height) * dpr) + 1, TRUE);
#else
    SetWindowPos(m_hWnd, nullptr, x, y, qRound(static_cast<qreal>(width) * dpr),
                 qRound(static_cast<qreal>(height) * dpr) + 1,
                 SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

QRect QWinNativeWindow::geometry() const {
    const QRect windowRect = frameGeometry();
    RECT clientRect = {0, 0, 0, 0};
    GetClientRect(m_hWnd, &clientRect);
    const int ww = clientRect.right;
    const int wh = clientRect.bottom;
    const qreal dpr = GetDevicePixelRatioForWindow(m_hWnd);
    return QRect{windowRect.left() + GetFrameSizeForWindow(m_hWnd).left + 1,
                 windowRect.top() + 1, qRound(static_cast<qreal>(ww) / dpr),
                 qRound(static_cast<qreal>(wh) / dpr) - 1};
}

QRect QWinNativeWindow::frameGeometry() const {
    RECT rect = {0, 0, 0, 0};
    GetWindowRect(m_hWnd, &rect);
    return QRect{rect.left, rect.top, rect.right - rect.left,
                 rect.bottom - rect.top};
}

HWND QWinNativeWindow::handle() const { return m_hWnd; }

void QWinNativeWindow::setContentWidget(QWidget *widget) { m_widget = widget; }

QWidget *QWinNativeWindow::contentWidget() const { return m_widget; }

LRESULT CALLBACK QWinNativeWindow::WndProc(HWND hWnd, UINT message,
                                           WPARAM wParam, LPARAM lParam) {
    const auto window = reinterpret_cast<QWinNativeWindow *>(
        GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (!window) {
        if (message == WM_NCCREATE) {
            const auto userData =
                reinterpret_cast<LPCREATESTRUCTW>(lParam)->lpCreateParams;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(userData));
            RefreshWindowStyle(hWnd);
        }
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    switch (message) {
    case WM_NCCALCSIZE: {
        const auto clientRect = static_cast<BOOL>(wParam)
            ? &(reinterpret_cast<LPNCCALCSIZE_PARAMS>(lParam)->rgrc[0])
            : reinterpret_cast<LPRECT>(lParam);
        // Store the original top before the default window proc applies the
        // default frame.
        const LONG originalTop = clientRect->top;
        // Apply the default frame
        const LRESULT ret = DefWindowProcW(hWnd, WM_NCCALCSIZE, wParam, lParam);
        if (ret != 0) {
            return ret;
        }
        // Re-apply the original top from before the size of the default
        // frame was applied.
        clientRect->top = originalTop;
        // We don't need this correction when we're fullscreen. We will have
        // the WS_POPUP size, so we don't have to worry about borders, and
        // the default frame will be fine.
        if (IsMaximized(hWnd) && !IsFullScreen(hWnd)) {
            // When a window is maximized, its size is actually a little bit
            // more than the monitor's work area. The window is positioned
            // and sized in such a way that the resize handles are outside
            // of the monitor and then the window is clipped to the monitor
            // so that the resize handle do not appear because you don't
            // need them (because you can't resize a window when it's
            // maximized unless you restore it).
            clientRect->top += GetFrameSizeForWindow(hWnd).top;
        }
        // Attempt to detect if there's an autohide taskbar, and if
        // there is, reduce our size a bit on the side with the taskbar,
        // so the user can still mouse-over the taskbar to reveal it.
        // Make sure to use MONITOR_DEFAULTTONEAREST, so that this will
        // still find the right monitor even when we're restoring from
        // minimized.
        if (IsMaximized(hWnd)) {
            APPBARDATA abd;
            SecureZeroMemory(&abd, sizeof(abd));
            abd.cbSize = sizeof(abd);
            const UINT taskbarState = SHAppBarMessage(ABM_GETSTATE, &abd);
            // First, check if we have an auto-hide taskbar at all:
            if (taskbarState & ABS_AUTOHIDE) {
                const HMONITOR windowMonitor =
                    MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO monitorInfo;
                SecureZeroMemory(&monitorInfo, sizeof(monitorInfo));
                monitorInfo.cbSize = sizeof(monitorInfo);
                GetMonitorInfoW(windowMonitor, &monitorInfo);
                // This helper can be used to determine if there's a
                // auto-hide taskbar on the given edge of the monitor
                // we're currently on.
                const auto hasAutohideTaskbar =
                    [&monitorInfo](const UINT edge) -> bool {
                    APPBARDATA _abd;
                    SecureZeroMemory(&_abd, sizeof(_abd));
                    _abd.cbSize = sizeof(_abd);
                    _abd.uEdge = edge;
                    _abd.rc = monitorInfo.rcMonitor;
                    const auto hTaskbar = reinterpret_cast<HWND>(
                        SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &_abd));
                    return hTaskbar != nullptr;
                };
                // If there's a taskbar on any side of the monitor, reduce
                // our size a little bit on that edge.
                // Note to future code archeologists:
                // This doesn't seem to work for fullscreen on the primary
                // display. However, testing a bunch of other apps with
                // fullscreen modes and an auto-hiding taskbar has
                // shown that _none_ of them reveal the taskbar from
                // fullscreen mode. This includes Edge, Firefox, Chrome,
                // Sublime Text, PowerPoint - none seemed to support this.
                // This does however work fine for maximized.
                if (hasAutohideTaskbar(ABE_TOP)) {
                    // Peculiarly, when we're fullscreen,
                    clientRect->top += kAutoHideTaskbarThicknessPy;
                } else if (hasAutohideTaskbar(ABE_BOTTOM)) {
                    clientRect->bottom -= kAutoHideTaskbarThicknessPy;
                } else if (hasAutohideTaskbar(ABE_LEFT)) {
                    clientRect->left += kAutoHideTaskbarThicknessPx;
                } else if (hasAutohideTaskbar(ABE_RIGHT)) {
                    clientRect->right -= kAutoHideTaskbarThicknessPx;
                }
            }
        }
        // We cannot return WVR_REDRAW when there is nonclient area, or
        // Windows exhibits bugs where client pixels and child HWNDs are
        // mispositioned by the width/height of the upper-left nonclient
        // area.
        return 0;
    } break;
    case WM_NCHITTEST: {
        // This will handle the left, right and bottom parts of the frame
        // because we didn't change them.
        const LRESULT originalRet =
            DefWindowProcW(hWnd, WM_NCHITTEST, wParam, lParam);
        if (originalRet != HTCLIENT) {
            return originalRet;
        }
        // At this point, we know that the cursor is inside the client area
        // so it has to be either the little border at the top of our custom
        // title bar or the drag bar. Apparently, it must be the drag bar or
        // the little border at the top which the user can use to move or
        // resize the window.
        RECT rcWindow = {0, 0, 0, 0};
        // Only GetWindowRect can give us the most accurate size of our
        // window which includes the invisible resize area.
        GetWindowRect(hWnd, &rcWindow);
        // Don't use HIWORD or LOWORD because they can only get positive
        // results, however, the cursor coordinates can be negative due to
        // in a different monitor.
        const LONG my = GET_Y_LPARAM(lParam);
        // The top of the drag bar is used to resize the window
        if (!IsMaximized(hWnd) &&
            (my < (rcWindow.top + GetFrameSizeForWindow(hWnd).top))) {
            return HTTOP;
        }
        if (my < (rcWindow.top + GetFrameSizeForWindow(hWnd, TRUE).top)) {
            return HTCAPTION;
        }
        // ###FIXME: Why?
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return HTCLIENT;
    } break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        const HDC hdc = BeginPaint(hWnd, &ps);
        const LONG topBorderHeight = 1;
        if (ps.rcPaint.top < topBorderHeight) {
            RECT rcTopBorder = ps.rcPaint;
            rcTopBorder.bottom = topBorderHeight;
            // To show the original top border, we have to paint on top
            // of it with the alpha component set to 0. This page
            // recommends to paint the area in black using the stock
            // BLACK_BRUSH to do this:
            // https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
            FillRect(hdc, &rcTopBorder, GetStockBrush(BLACK_BRUSH));
        }
        if (ps.rcPaint.bottom > topBorderHeight) {
            RECT rcRest = ps.rcPaint;
            rcRest.top = topBorderHeight;
            // To hide the original title bar, we have to paint on top
            // of it with the alpha component set to 255. This is a hack
            // to do it with GDI. See UpdateFrameMarginsForWindow for
            // more information.
            HDC opaqueDc;
            BP_PAINTPARAMS params;
            SecureZeroMemory(&params, sizeof(params));
            params.cbSize = sizeof(params);
            params.dwFlags = BPPF_NOCLIP | BPPF_ERASE;
            const HPAINTBUFFER buf = BeginBufferedPaint(
                hdc, &rcRest, BPBF_TOPDOWNDIB, &params, &opaqueDc);
            FillRect(opaqueDc, &rcRest,
                     reinterpret_cast<HBRUSH>(
                         GetClassLongPtrW(hWnd, GCLP_HBRBACKGROUND)));
            BufferedPaintSetAlpha(buf, nullptr, 255);
            EndBufferedPaint(buf, TRUE);
        }
        EndPaint(hWnd, &ps);
        return 0;
    } break;
    case WM_ACTIVATE:
    case WM_DWMCOMPOSITIONCHANGED:
        UpdateFrameMarginsForWindow(hWnd);
        break;
    case WM_SIZE: {
        QWidget *const widget = window->contentWidget();
        if (widget) {
            RECT rect = {0, 0, 0, 0};
            GetClientRect(hWnd, &rect);
            const qreal dpr = widget->devicePixelRatioF();
            // The one pixel height top frame border is drawn in the client area
            // by ourself, don't cover it.
            widget->setGeometry(
                0, 1, qRound(static_cast<qreal>(rect.right) / dpr),
                qRound(static_cast<qreal>(rect.bottom) / dpr) - 1);
        }
    } break;
    case WM_GETMINMAXINFO: {
        QWidget *const widget = window->contentWidget();
        if (widget) {
            auto &mmi = *reinterpret_cast<LPMINMAXINFO>(lParam);
            const qreal dpr = widget->devicePixelRatioF();
            if (!window->getMaximumSize().isEmpty()) {
                mmi.ptMaxSize.x = qRound(
                    static_cast<qreal>(window->getMaximumSize().width()) * dpr);
                mmi.ptMaxSize.y = qRound(
                    static_cast<qreal>(window->getMaximumSize().height()) *
                    dpr);
                mmi.ptMaxTrackSize.x = mmi.ptMaxSize.x;
                mmi.ptMaxTrackSize.y = mmi.ptMaxSize.y;
            }
            if (!window->getMinimumSize().isEmpty()) {
                mmi.ptMinTrackSize.x = qRound(
                    static_cast<qreal>(window->getMinimumSize().width()) * dpr);
                mmi.ptMinTrackSize.y = qRound(
                    static_cast<qreal>(window->getMinimumSize().height()) *
                    dpr);
            }
            return 0;
        }
    } break;
    case WM_DPICHANGED: {
        const auto prcNewWindow = reinterpret_cast<LPRECT>(lParam);
        SetWindowPos(hWnd, nullptr, prcNewWindow->left, prcNewWindow->top,
                     prcNewWindow->right - prcNewWindow->left,
                     prcNewWindow->bottom - prcNewWindow->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        RedrawWindow(hWnd, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);
    } break;
    case WM_CLOSE: {
        QWidget *const widget = window->contentWidget();
        if (widget) {
            SendMessageW(reinterpret_cast<HWND>(widget->winId()), WM_CLOSE, 0,
                         0);
            return 0;
        }
    } break;
    case WM_DESTROY:
        return 0;
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QFocusEvent>
#include <QVBoxLayout>

QWinWidget::QWinWidget(QWidget *widget) : QWidget() {
    m_winNativeWindow = new QWinNativeWindow(CW_USEDEFAULT, CW_USEDEFAULT,
                                             CW_USEDEFAULT, CW_USEDEFAULT);
    const HWND hParent = m_winNativeWindow->handle();
    Q_ASSERT(hParent);
    setProperty("_q_embedded_native_parent_handle",
                reinterpret_cast<WId>(hParent));
    const auto hWnd = reinterpret_cast<HWND>(winId());
    // Make the widget window style be WS_CHILD so SetParent will work
    SetWindowLongPtrW(hWnd, GWL_STYLE,
                      WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    RefreshWindowStyle(hWnd);
    SetParent(hWnd, hParent);
    setWindowFlags(Qt::FramelessWindowHint);
    QEvent event(QEvent::EmbeddingControl);
    QApplication::sendEvent(this, &event);
    setContentsMargins(0, 0, 0, 0);
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    setLayout(m_mainLayout);
    if (widget) {
        setContentWidget(widget);
    }
    m_winNativeWindow->setContentWidget(this);
    // Send the parent native window a WM_SIZE message to update the widget size
    SendMessageW(hParent, WM_SIZE, 0, 0);
}

QWinWidget::~QWinWidget() {
    setContentWidget(nullptr);
    if (m_mainLayout) {
        delete m_mainLayout;
        m_mainLayout = nullptr;
    }
}

void QWinWidget::setContentWidget(QWidget *widget) {
    if (m_widget) {
        m_mainLayout->removeWidget(m_widget);
    }
    m_widget = widget;
    if (m_widget) {
        m_mainLayout->addWidget(m_widget);
    }
}

QWidget *QWinWidget::contentWidget() const { return m_widget; }

HWND QWinWidget::parentWindow() const { return m_winNativeWindow->handle(); }

void QWinWidget::setIgnoreWidgets(const QVector<QWidget *> &widgets) {
    m_ignoreWidgets = widgets;
}

void QWinWidget::childEvent(QChildEvent *event) {
    QObject *const object = event->child();
    if (object->isWidgetType()) {
        if (event->added()) {
            object->installEventFilter(this);
        } else if (event->removed() && m_reEnableParent) {
            m_reEnableParent = false;
            EnableWindow(parentWindow(), TRUE);
            object->removeEventFilter(this);
        }
    }
    QWidget::childEvent(event);
}

void QWinWidget::saveFocus() {
    if (!m_prevFocus) {
        m_prevFocus = GetFocus();
    }
    if (!m_prevFocus) {
        m_prevFocus = parentWindow();
    }
}

void QWinWidget::show() {
    ShowWindow(parentWindow(), SW_SHOW);
    saveFocus();
    QWidget::show();
}

void QWinWidget::setGeometry(const int x, const int y, const int width,
                             const int height) {
    m_winNativeWindow->setGeometry(x, y, width, height);
}

void QWinWidget::setGeometry(const QRect &value) {
    setGeometry(value.x(), value.y(), value.width(), value.height());
}

QRect QWinWidget::geometry() const { return m_winNativeWindow->geometry(); }

void QWinWidget::move(const int x, const int y) {
    const QRect rect = geometry();
    m_winNativeWindow->setGeometry(x, y, rect.width(), rect.height());
}

void QWinWidget::move(const QPoint &point) { move(point.x(), point.y()); }

void QWinWidget::resize(const int width, const int height) {
    const QRect rect = geometry();
    setGeometry(rect.x(), rect.y(), width, height);
}

void QWinWidget::resize(const QSize &value) {
    resize(value.width(), value.height());
}

QSize QWinWidget::size() const { return geometry().size(); }

void QWinWidget::resetFocus() {
    if (m_prevFocus) {
        SetFocus(m_prevFocus);
    } else {
        SetFocus(parentWindow());
    }
}

bool QWinWidget::nativeEvent(const QByteArray &eventType, void *message,
                             long *result) {
    if (eventType == "windows_generic_MSG") {
        const auto msg = static_cast<LPMSG>(message);
        switch (msg->message) {
        case WM_SETFOCUS: {
            Qt::FocusReason reason = Qt::NoFocusReason;
            if ((GetKeyState(VK_LBUTTON) < 0) ||
                (GetKeyState(VK_RBUTTON) < 0)) {
                reason = Qt::MouseFocusReason;
            } else if (GetKeyState(VK_SHIFT) < 0) {
                reason = Qt::BacktabFocusReason;
            } else {
                reason = Qt::TabFocusReason;
            }
            QFocusEvent event(QEvent::FocusIn, reason);
            QApplication::sendEvent(this, &event);
        } break;
        case WM_NCHITTEST: {
            POINT point = {0, 0};
            point.x = GET_X_LPARAM(msg->lParam);
            point.y = GET_Y_LPARAM(msg->lParam);
            ScreenToClient(msg->hwnd, &point);
            if (point.y <= GetFrameSizeForWindow(msg->hwnd, TRUE).top) {
                bool shouldIgnore = false;
                if (!m_ignoreWidgets.isEmpty()) {
                    for (auto &&widget : qAsConst(m_ignoreWidgets)) {
                        if (widget->geometry().contains(
                                mapFromGlobal(QCursor::pos()))) {
                            shouldIgnore = true;
                            break;
                        }
                    }
                }
                if (!shouldIgnore) {
                    *result = HTTRANSPARENT;
                    return true;
                }
            }
        } break;
        default:
            break;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void QWinWidget::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    SetParent(reinterpret_cast<HWND>(winId()), nullptr);
    if (m_winNativeWindow) {
        delete m_winNativeWindow;
        m_winNativeWindow = nullptr;
    }
}

bool QWinWidget::eventFilter(QObject *object, QEvent *event) {
    const auto widget = qobject_cast<QWidget *>(object);
    switch (event->type()) {
    case QEvent::WindowDeactivate:
        if (widget->isModal() && widget->isHidden()) {
            BringWindowToTop(parentWindow());
        }
        break;
    case QEvent::Hide: {
        if (m_reEnableParent) {
            EnableWindow(parentWindow(), TRUE);
            m_reEnableParent = false;
        }
        resetFocus();
        if (widget->testAttribute(Qt::WA_DeleteOnClose) && widget->isWindow()) {
            deleteLater();
        }
    } break;
    case QEvent::Show:
        if (widget->isWindow()) {
            saveFocus();
            hide();
            if (widget->isModal() && !m_reEnableParent) {
                EnableWindow(parentWindow(), FALSE);
                m_reEnableParent = true;
            }
        }
        break;
    case QEvent::Close: {
        SetActiveWindow(parentWindow());
        if (widget->testAttribute(Qt::WA_DeleteOnClose)) {
            deleteLater();
        }
    } break;
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}

void QWinWidget::focusInEvent(QFocusEvent *event) {
    QWidget *candidate = this;
    switch (event->reason()) {
    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason: {
        while (!(candidate->focusPolicy() & Qt::TabFocus)) {
            candidate = candidate->nextInFocusChain();
            if (candidate == this) {
                candidate = nullptr;
                break;
            }
        }
        if (candidate) {
            candidate->setFocus(event->reason());
            if (event->reason() == Qt::BacktabFocusReason ||
                event->reason() == Qt::TabFocusReason) {
                candidate->setAttribute(Qt::WA_KeyboardFocusChange);
                candidate->window()->setAttribute(Qt::WA_KeyboardFocusChange);
            }
            if (event->reason() == Qt::BacktabFocusReason) {
                QWidget::focusNextPrevChild(false);
            }
        }
    } break;
    default:
        break;
    }
}

bool QWinWidget::focusNextPrevChild(bool next) {
    QWidget *curFocus = focusWidget();
    if (!next) {
        if (!curFocus->isWindow()) {
            QWidget *nextFocus = curFocus->nextInFocusChain();
            QWidget *prevFocus = nullptr;
            QWidget *topLevel = nullptr;
            while (nextFocus != curFocus) {
                if (nextFocus->focusPolicy() & Qt::TabFocus) {
                    prevFocus = nextFocus;
                    topLevel = nullptr;
                } else if (nextFocus->isWindow()) {
                    topLevel = nextFocus;
                }
                nextFocus = nextFocus->nextInFocusChain();
            }
            if (!topLevel) {
                return QWidget::focusNextPrevChild(false);
            }
        }
    } else {
        QWidget *nextFocus = curFocus;
        Q_FOREVER {
            nextFocus = nextFocus->nextInFocusChain();
            if (nextFocus->isWindow()) {
                break;
            }
            if (nextFocus->focusPolicy() & Qt::TabFocus) {
                return QWidget::focusNextPrevChild(true);
            }
        }
    }
    SetFocus(parentWindow());
    return true;
}
