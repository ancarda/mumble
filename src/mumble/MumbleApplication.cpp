// Copyright 2014-2021 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "MumbleApplication.h"

#include "EnvUtils.h"
#include "MainWindow.h"
#include "GlobalShortcut.h"
#include "Global.h"

#if defined(Q_OS_WIN)
#	include "GlobalShortcut_win.h"
#endif

#include <QtGui/QFileOpenEvent>

MumbleApplication *MumbleApplication::instance() {
	return static_cast< MumbleApplication * >(QCoreApplication::instance());
}

MumbleApplication::MumbleApplication(int &pargc, char **pargv) : QApplication(pargc, pargv) {
	connect(this, SIGNAL(commitDataRequest(QSessionManager &)), SLOT(onCommitDataRequest(QSessionManager &)),
			Qt::DirectConnection);
}

QString MumbleApplication::applicationVersionRootPath() {
	QString versionRoot = EnvUtils::getenv(QLatin1String("MUMBLE_VERSION_ROOT"));
	if (versionRoot.count() > 0) {
		return versionRoot;
	}
	return this->applicationDirPath();
}

void MumbleApplication::onCommitDataRequest(QSessionManager &) {
	// Make sure the config is saved and supress the ask on quit message
	if (Global::get().mw) {
		Global::get().s.save();
		Global::get().mw->bSuppressAskOnQuit = true;
		qWarning() << "Session likely ending. Suppressing ask on quit";
	}
}

bool MumbleApplication::event(QEvent *e) {
	if (e->type() == QEvent::FileOpen) {
		QFileOpenEvent *foe = static_cast< QFileOpenEvent * >(e);
		if (!Global::get().mw) {
			this->quLaunchURL = foe->url();
		} else {
			Global::get().mw->openUrl(foe->url());
		}
		return true;
	}
	return QApplication::event(e);
}

#ifdef Q_OS_WIN
/// gswForward forwards a native Windows keyboard/mouse message
/// into GlobalShortcutWin's event stream.
///
/// @return  Returns true if the forwarded event was suppressed
///          by GlobalShortcutWin. Otherwise, returns false.
static bool gswForward(MSG *msg) {
	GlobalShortcutWin *gsw = static_cast< GlobalShortcutWin * >(GlobalShortcutEngine::engine);
	if (!gsw) {
		return false;
	}
	switch (msg->message) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			return gsw->injectMouseMessage(msg);
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			return gsw->injectKeyboardMessage(msg);
	}
	return false;
}

bool MumbleApplication::nativeEventFilter(const QByteArray &, void *message, long *) {
	MSG *msg = reinterpret_cast< MSG * >(message);
	if (QThread::currentThread() == thread()) {
		bool suppress = gswForward(msg);
		if (suppress) {
			return true;
		}
	}
	return false;
}
#endif
