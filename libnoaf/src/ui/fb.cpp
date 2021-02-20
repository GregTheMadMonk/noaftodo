#ifdef __linux__

#include "fb.hpp"

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <log.hpp>

namespace noaf {

	backend_framebuffer::backend_framebuffer() :
		dev(-1),
		data(nullptr) {
	}

	void backend_framebuffer::init() {
		if (dev >= 0) return;

		log << "Initializing \"framebuffer\" backend..." << lend;

		dev = open(dev_name.c_str(), O_RDWR);
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
		if (dev == -1) return;
		munmap(data, w * h);
		data = nullptr;
		close(dev);
		dev = -1;

		blank(); // clear the screen
	}

	void backend_framebuffer::run() {
		while (true) {
			on_paint();
			system("sleep 1");
		}
	}

	int backend_framebuffer::width()	{ return w; }
	int backend_framebuffer::height()	{ return h; }

	void backend_framebuffer::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
		if (x1 == x2)
			for (int y = ((y1 < y2) ? y1 : y2); y <= ((y2 > y1) ? y2 : y1); y++)
				data[y * w + x1] = 0xffffff;
		else for (int x = ((x1 < x2) ? x1 : x2); x <= ((x2 > x1) ? x2 : x1); x++) {
			int y = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
			data[y * w + x] = 0xffffff;
		}
	}

	void backend_framebuffer::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
		draw_line(x1, y1, x2, y1);
		draw_line(x2, y1, x2, y2);
		draw_line(x2, y2, x1, y2);
		draw_line(x1, y2, x1, y1);
	}

	void backend_framebuffer::draw_text(const int& x, const int& y, const std::string& text) {
	}

	void backend_framebuffer::blank() {
		int blank = -1;
		if ((blank = open("/sys/classs/graphics/fb0/blank", O_WRONLY)) <= 0) {
			log << llev(VERR) << "Can't open \"blank\" file!" << lend;
			return;
		}

		write(blank, "1", sizeof(char));

		close(blank);
	}

}

#endif
