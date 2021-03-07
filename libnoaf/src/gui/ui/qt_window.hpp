#pragma once
#ifndef NOAF_UI_QT_WINDOW_H
#define NOAF_UI_QT_WINDOW_H

#include <QMainWindow>
#include <QPen>
#include <QPixmap>
#include <QWidget>

namespace noaf {

	class QNOAFMainWidget : public QWidget {
			Q_OBJECT

			QPixmap target;
		public:
			QNOAFMainWidget(QWidget* parent);
			friend class QNOAFWindow;
			friend class backend_qt;
		protected:
			void paintEvent(QPaintEvent* e);
	};

	class QNOAFWindow : public QMainWindow {
			Q_OBJECT
			QNOAFMainWidget* main_widget;

			QPen fg;
			QColor bg;
		public:
			QNOAFWindow();

			friend class backend_qt;

		protected:
			void keyPressEvent(QKeyEvent *event);
	};

}

#endif
