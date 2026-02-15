#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
	unsigned char *buf;
	size_t i;
	const size_t n = 1024;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_SMOKE_START");

	buf = (unsigned char *)malloc(n);
	if (!buf) {
		emit_marker("MUSL_SMOKE_FAIL: malloc returned NULL");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}

	for (i = 0; i < n; ++i)
		buf[i] = (unsigned char)((i * 17u) ^ 0x5au);

	for (i = 0; i < n; ++i) {
		unsigned char want = (unsigned char)((i * 17u) ^ 0x5au);
		if (buf[i] != want) {
			printf("MUSL_SMOKE_FAIL: memory mismatch at %zu\n", i);
			fflush(stdout);
			uart_puts("MUSL_SMOKE_FAIL: memory mismatch\n");
			free(buf);
			sync();
			reboot(RB_POWER_OFF);
			return 3;
		}
	}

	free(buf);
	emit_marker("MUSL_SMOKE_PASS");

	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
