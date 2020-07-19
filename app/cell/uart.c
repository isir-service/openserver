#include <unistd.h>
#include "uart.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "cell_pub.h"

int uart_init(char *dev_name)
{
	struct termios term;
	int fd = -1;
	if (!dev_name)
		goto out;

	fd = open(dev_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
		goto out;
	
	fcntl(fd, F_SETFL, O_RDWR);
	tcgetattr(fd, &term);
	term.c_lflag &= ~(ICANON | ECHO | ECHONL);
	term.c_lflag &= ~ISIG;
	term.c_lflag &= ~(IXON | ICRNL);
	term.c_oflag &= ~(ONLCR);
	term.c_iflag &= ~(IXOFF|IXON|IXANY|BRKINT|INLCR|ICRNL|IUCLC|IMAXBEL);
	cfsetspeed(&term, B9600);

	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSAFLUSH, &term);
out:
	return fd;
}

void uart_read(int s, short what, void *arg)
{
	(void)s;
	(void)what;

	int ret = 0;
	struct lte_module_info *module = (struct lte_module_info*)arg;
	if (!module)
		return;

	if (what & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		pthread_mutex_lock(&module->lock);
		lte_release_module(module);
		pthread_mutex_unlock(&module->lock);
		pthread_mutex_destroy(&module->lock);
		return;
	}

	ret = read(s, module->read_buf.buf+module->read_buf.index, module->read_buf.buf_size - module->read_buf.index);
	if (ret <= 0)
		return;

	pthread_mutex_lock(&module->lock);
	if (!module->current_map) {
		pthread_mutex_unlock(&module->lock);
		return;
	}

	if (!module->current_map->cb) {
		pthread_mutex_unlock(&module->lock);
		return;
	}

	module->current_map->cb(module->read_buf.buf,ret+module->read_buf.index,module);
	module->current_map = NULL;
	pthread_cond_broadcast(&module->cond);
	pthread_mutex_unlock(&module->lock);

	return;

}

