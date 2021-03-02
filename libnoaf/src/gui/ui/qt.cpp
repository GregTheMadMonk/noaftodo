#include "qt.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>

using namespace std;

namespace noaf {

	QNOAFMainWidget::QNOAFMainWidget(QWidget* parent) : QWidget(parent) {
		target = QPixmap(100, 100);
	}

	QNOAFMainWidget::~QNOAFMainWidget() {
	}

	void QNOAFMainWidget::paintEvent(QPaintEvent* e) {
		QPainter p(this);
		p.drawPixmap(0, 0, target);
	}

	QNOAFWindow::QNOAFWindow() : QMainWindow(0) {
		main_widget = make_unique<QNOAFMainWidget>(this);
		setCentralWidget(main_widget.get());
	}

	backend_qt::backend_qt(const int& ac, char**& av) : argc(ac) {
		argv = new char*[ac];
		for (int i = 0; i < ac; i++) argv[i] = av[i];
	}

	backend_qt::~backend_qt() {
		delete argv;
		kill();
	}

	void backend_qt::init() {
		app = make_unique<QApplication>(argc, argv);
		win = make_unique<QNOAFWindow>();
	}

	// we don't pause Qt interface for logging
	void backend_qt::pause() {}
	void backend_qt::resume() {}

	void backend_qt::kill() {
		app.reset();
		win.reset();
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
		path.addRect(x1, y1, x2 - x1, y2 - y1);
		if (draw_stroke) {
			p.setPen(fg);
			p.drawPath(path);
		}
		if (draw_fill) p.fillPath(path, bg);
		p.end();
	}

	void backend_qt::draw_text(const int& x, const int& y, const string& text) {
		QPainter p(&win->main_widget->target);
		p.setPen(fg);
		p.drawText(x, y, text.c_str());
		p.end();
	}

	void backend_qt::set_fg(const uint32_t& color) {
		fg = QPen(QColor::fromRgba(color), 1, Qt::SolidLine);
	}

	void backend_qt::set_bg(const uint32_t& color) {
		bg = QColor::fromRgba(color);
	}

}
