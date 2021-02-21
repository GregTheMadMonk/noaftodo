#ifdef __linux__

#include "fb.hpp"

#include <algorithm>
#include <fcntl.h>
#include <curses.h>	// we will use ncurses for input
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <log.hpp>
#include <ui/nc.hpp>

namespace noaf {

	backend_framebuffer::backend_framebuffer() {
		features = COLOR;
	}

	void backend_framebuffer::init() {
		if (dev >= 0) return;

		log << "Initializing \"framebuffer\" backend..." << lend;

		initscr();
		if (halfdelay_time == 0) cbreak();
		else halfdelay(halfdelay_time);

		dev = open(("/dev/" + dev_name).c_str(), O_RDWR);
		if (dev < 0) return;

		struct fb_var_screeninfo vinfo;

		ioctl(dev, FBIOGET_VSCREENINFO, &vinfo);
		w = vinfo.xres;
		h = vinfo.yres;

		data = (uint32_t*)mmap(NULL, w * h * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, dev, (off_t)0);
	}

	void backend_framebuffer::pause() { }	// no need to pause or
	void backend_framebuffer::resume() { }	// resume the framebuffer (for now)

	void backend_framebuffer::kill() {
		running = false;
		if (dev == -1) return;
		munmap(data, w * h);
		data = nullptr;
		close(dev);
		dev = -1;

		blank(); // clear the screen

		endwin();
	}

	void backend_framebuffer::run() {
		char c;
		running = true;
		for (wint_t c = 0; running; (get_wch(&c) != ERR) ? : (c = 0)) {
			if (c != 0) on_input(nc_process_input(c));
			if (!running) break;
			on_paint();
		}
	}

	int backend_framebuffer::width()	{ return w; }
	int backend_framebuffer::height()	{ return h; }

	void backend_framebuffer::clear() {
		std::fill(data, data + w * h, 0);
	}

	void backend_framebuffer::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
		if (x1 == x2)
			for (int y = ((y1 < y2) ? y1 : y2); y <= ((y2 > y1) ? y2 : y1); y++)
				data[y * w + x1] = fg;
		else for (int x = ((x1 < x2) ? x1 : x2); x <= ((x2 > x1) ? x2 : x1); x++) {
			int y = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
			data[y * w + x] = fg;
		}
	}

	void backend_framebuffer::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
		if (draw_stroke) {
			draw_line(x1, y1, x2, y1);
			draw_line(x2, y1, x2, y2);
			draw_line(x2, y2, x1, y2);
			draw_line(x1, y2, x1, y1);
		}

		if (draw_fill) for (int y = y1 + 1; y < y2; y++)
				std::fill(data + y * w + x1 + 1, data + y * w + x2, bg);
	}

	void backend_framebuffer::draw_text(const int& x, const int& y, const std::string& text) {
	}

	void backend_framebuffer::set_fg(const uint32_t& color) { fg = color; }
	void backend_framebuffer::set_bg(const uint32_t& color) { bg = color; }

	void backend_framebuffer::blank() {
		int blank = -1;
		if ((blank = open(("/sys/class/graphics/" + dev_name + "/blank").c_str(), O_WRONLY)) <= 0) {
			log << llev(VERR) << "Can't open \"blank\" file!" << lend;
			return;
		}

		write(blank, "1", sizeof(char));

		close(blank);
	}

}

#endif
