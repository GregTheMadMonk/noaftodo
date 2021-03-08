#include "qt.hpp"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <ui/qt_window.hpp>

#ifdef __linux__
// Qt fails to reset terminal back to normal
// after running a framebuffer application
// for some reason.
// Fine, we'll do it ourselves
#include <cstdlib>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace std;

namespace noaf {

	backend_qt::backend_qt(const int& ac, char**& av) : argc(ac) {
		argv = new char*[ac];
		for (int i = 0; i < ac; i++) argv[i] = av[i];
	}

	backend_qt::~backend_qt() {
		kill();
		delete app;
		delete win;
		delete argv;
	}

	void backend_qt::init() {
		#ifdef __linux__
		if (linuxfb) {
			setenv("QT_QPA_PLATFORM", "linuxfb:tty=/dev/tty", 1);
			int tfd = open("/dev/tty", O_RDWR);
			if (tfd == -1) throw runtime_error("Cannot open TTY");
			ioctl(tfd, KDGKBMODE, &k_mode);
			if (k_mode == -1) throw runtime_error("Cannot recieve KDBMODE");
			close(tfd);
		}
		#endif
		app = new QApplication(argc, argv);
		win = new QNOAFWindow();
	}

	// we don't pause Qt interface for logging
	void backend_qt::pause() {}
	void backend_qt::resume() {}

	void backend_qt::kill() {
		app->quit();
		#ifdef __linux__
		int tfd = open("/dev/tty", O_RDWR);
		if (tfd == -1) throw runtime_error("Cannot open TTY");
		ioctl(tfd, KDSETMODE, KD_TEXT); // return to text mode from graphics mode
		ioctl(tfd, 0x4b51, 0); // 0x4b51=KDKBMUTE
		ioctl(tfd, KDSKBMODE, k_mode);
		k_mode = -1;
		const auto blink_on = "\033[9;15]\033[?33h\033[?25h\033[?0c";
		write(tfd, blink_on, strlen(blink_on) + 1);
		close(tfd);
		#endif
	}

	void backend_qt::run() {
		win->show();
		QTimer timer;

		app->connect(&timer, &QTimer::timeout, [&] () {
			win->main_widget->target = QPixmap(width(), height());
			on_paint();
			win->main_widget->repaint();
		});
		timer.start(frame_time);
		app->exec();
	}

	int backend_qt::width() {
		return win->size().width();
	}

	int backend_qt::height() {
		return win->size().height();
	}

	void backend_qt::clear() {
		QPainter p(&win->main_widget->target);
		p.fillRect(0, 0, width(), height(), QColor::fromRgb(0));
		p.end();
	}

	void backend_qt::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
		QPainter p(&win->main_widget->target);
		p.drawLine(x1, y1, x2, y2);
		p.end();
	}

	void backend_qt::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
		QPainter p(&win->main_widget->target);
		QPainterPath path;
		path.addRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
		if (draw_fill) p.fillPath(path, win->bg);
		if (draw_stroke) {
			p.setPen(win->fg);
			p.drawPath(path);
		}
		p.end();
	}

	void backend_qt::draw_text(const int& x, const int& y, const string& text) {
		QPainter p(&win->main_widget->target);
		p.setPen(win->fg);
		p.drawText(x, y + QFontMetrics(p.font()).height(), text.c_str());
		p.end();
	}

	void backend_qt::set_fg(const uint32_t& color) {
		win->fg = QPen(QColor::fromRgba(color), 1, Qt::SolidLine);
	}

	void backend_qt::set_bg(const uint32_t& color) {
		win->bg = QColor::fromRgba(color);
	}


	extern "C" backend* backend_create(const int& ac, char**& av) {
		return new backend_qt(ac, av);
	}

}
