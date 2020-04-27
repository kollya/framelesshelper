#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <QWidget>
#include <qt_windows.h>

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

class QWinNativeWindow {
    Q_DISABLE_COPY_MOVE(QWinNativeWindow)

public:
    explicit QWinNativeWindow(const int x, const int y, const int width,
                              const int height);
    ~QWinNativeWindow();

    void setMinimumSize(const QSize &size);
    void setMinimumSize(const int width, const int height);
    QSize getMinimumSize() const;

    void setMaximumSize(const QSize &size);
    void setMaximumSize(const int width, const int height);
    QSize getMaximumSize() const;

    void setGeometry(const QRect &geometry);
    void setGeometry(const int x, const int y, const int width,
                     const int height);
    QRect geometry() const;
    QRect frameGeometry() const;

    HWND handle() const;

    void setContentWidget(QWidget *widget);
    QWidget *contentWidget() const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam);

private:
    QSize m_minimumSize = {0, 0}, m_maximumSize = {0, 0};
    QWidget *m_widget = nullptr;
    HWND m_hWnd = nullptr;
};

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

class QVBoxLayout;

class QWinWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QWinWidget)

public:
    explicit QWinWidget(QWidget *widget = nullptr);
    ~QWinWidget() override;

    void setContentWidget(QWidget *widget);
    QWidget *contentWidget() const;

    QRect geometry() const;

    QSize size() const;

    HWND parentWindow() const;

    void setIgnoreWidgets(const QVector<QWidget *> &widgets);

    void setMinimumSize(const int width, const int height);
    void setMinimumSize(const QSize &value);

    void setMaximumSize(const int width, const int height);
    void setMaximumSize(const QSize &value);

public Q_SLOTS:
    void show();

    void setGeometry(const int x, const int y, const int width,
                     const int height);
    void setGeometry(const QRect &value);

    void move(const int x, const int y);
    void move(const QPoint &point);

    void resize(const int width, const int height);
    void resize(const QSize &value);

protected:
    void childEvent(QChildEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;

    void closeEvent(QCloseEvent *event) override;

private:
    void saveFocus();
    void resetFocus();

private:
    QWinNativeWindow *m_winNativeWindow = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
    QWidget *m_widget = nullptr;
    QVector<QWidget *> m_ignoreWidgets;
    HWND m_prevFocus = nullptr;
    bool m_reEnableParent = false;
};
