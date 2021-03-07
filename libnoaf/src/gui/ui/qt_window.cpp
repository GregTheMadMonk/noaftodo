#include "qt_window.hpp"

#include <QKeyEvent>
#include <QPainter>

#include <ui.hpp>

using namespace std;

namespace noaf {

	QNOAFMainWidget::QNOAFMainWidget(QWidget* parent) : QWidget(parent) {
		target = QPixmap(100, 100);
	}

	void QNOAFMainWidget::paintEvent(QPaintEvent* e) {
		QPainter p(this);
		p.drawPixmap(0, 0, target);
	}

	QNOAFWindow::QNOAFWindow() : QMainWindow(0) {
		main_widget = new QNOAFMainWidget(this);
		setCentralWidget(main_widget);
	}

	void QNOAFWindow::keyPressEvent(QKeyEvent* event) {
		input_event ret;

		switch (event->key()) {
			case Qt::Key_Escape:
				ret.key = 27;
				ret.name = "esc";
				break;
			default:
				ret.name = string(1, event->key());
				transform(ret.name.begin(), ret.name.end(), ret.name.begin(), ::tolower);
				ret.key = ret.name.at(0);
		}

		on_input(ret);
		main_widget->target = QPixmap(size());
		on_paint();
		main_widget->repaint();
	}

}
