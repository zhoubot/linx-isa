#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <unistd.h>

enum {
	LINX_UART_BASE = 0x10000000ul,
};

static __thread int tls_counter = 7;

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

static void *worker(void *arg)
{
	(void)arg;
	tls_counter = 42;
	return (void *)(long)tls_counter;
}

int main(void)
{
	int cfd;
	pthread_t th;
	void *ret = NULL;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_PTHREAD_TLS_START");
	if (pthread_create(&th, NULL, worker, NULL) != 0) {
		emit_marker("MUSL_PTHREAD_TLS_FAIL: create");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}
	if (pthread_join(th, &ret) != 0) {
		emit_marker("MUSL_PTHREAD_TLS_FAIL: join");
		sync();
		reboot(RB_POWER_OFF);
		return 3;
	}
	if ((long)ret != 42 || tls_counter != 7) {
		emit_marker("MUSL_PTHREAD_TLS_FAIL: tls");
		sync();
		reboot(RB_POWER_OFF);
		return 4;
	}

	emit_marker("MUSL_PTHREAD_TLS_PASS");
	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
