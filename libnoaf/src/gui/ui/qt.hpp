#pragma once
#ifndef NOAF_UI_QT_H
#define NOAF_UI_QT_H

#include <QApplication>
#include <QEvent>
#include <QMainWindow>
#include <QPen>
#include <QPixmap>
#include <QWidget>

#include <memory>

#include <ui.hpp>

namespace noaf {

	class QNOAFMainWidget : public QWidget {
			Q_OBJECT

			QPixmap target;
		public:
			QNOAFMainWidget(QWidget* parent);
			~QNOAFMainWidget();
			friend class QNOAFWindow;
			friend class backend_qt;
		protected:
			void paintEvent(QPaintEvent* e);
	};

	class QNOAFWindow : public QMainWindow {
			Q_OBJECT
			std::unique_ptr<QNOAFMainWidget> main_widget;
		public:
			QNOAFWindow();

			friend class backend_qt;
	};

	class backend_qt : public backend {
			std::unique_ptr<QApplication> app = nullptr;
			std::unique_ptr<QNOAFWindow> win = nullptr;

			QPen fg;
			QColor bg;

			int argc;
			char** argv;
		public:
			backend_qt(const int& ac, char**& av);
			~backend_qt();

			void init();
			void pause();
			void resume();
			void kill();

			void run();

			int width();
			int height();

			void clear();

			void draw_line(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_box(const int& x1, const int& y1, const int& x2, const int& y2);
			void draw_text(const int& x, const int& y, const std::string& text);

			void set_fg(const uint32_t& color);
			void set_bg(const uint32_t& color);
	};

}

#endif
