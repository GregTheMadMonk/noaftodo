#include "qt_window.hpp"

#include <QPainter>

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

}
