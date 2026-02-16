#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

enum {
	LINX_UART_BASE = 0x10000000ul,
};

static void uart_puts(const char *s)
{
	while (*s)
		*(volatile unsigned char *)LINX_UART_BASE = (unsigned char)*s++;
}

static void emit_marker(const char *s)
{
	printf("%s\n", s);
	fflush(stdout);
	uart_puts(s);
	uart_puts("\n");
}

int main(void)
{
	int cfd;
	void *self;
	void *sym_default;
	void *sym_next;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_DLOPEN_TLSDESC_START");
	self = dlopen(NULL, RTLD_NOW | RTLD_LOCAL);
	if (!self) {
		emit_marker("MUSL_DLOPEN_TLSDESC_FAIL: dlopen");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}

	dlerror();
	sym_default = dlsym(RTLD_DEFAULT, "printf");
	if (!sym_default) {
		emit_marker("MUSL_DLOPEN_TLSDESC_FAIL: dlsym default");
		sync();
		reboot(RB_POWER_OFF);
		return 3;
	}
	dlerror();
	sym_next = dlsym(RTLD_NEXT, "malloc");
	if (!sym_next) {
		const char *err = dlerror();
		if (!err || strstr(err, "undefined") == NULL) {
			emit_marker("MUSL_DLOPEN_TLSDESC_FAIL: dlsym next");
			sync();
			reboot(RB_POWER_OFF);
			return 4;
		}
	}

	emit_marker("MUSL_DLOPEN_TLSDESC_PASS");
	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
