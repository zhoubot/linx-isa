#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

enum {
	LINX_UART_BASE = 0x10000000ul,
};

static volatile sig_atomic_t got_sigusr1;

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

static void rt_handler(int sig, siginfo_t *si, void *ctx)
{
	(void)ctx;
	if (sig == SIGUSR1 && si && si->si_signo == SIGUSR1)
		got_sigusr1 = 1;
}

extern void __restore_rt(void);

int main(void)
{
	int cfd;
	struct sigaction sa;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_SIGNAL_RT_START");
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = rt_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTORER;
	sa.sa_restorer = __restore_rt;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGUSR1, &sa, NULL) != 0) {
		emit_marker("MUSL_SIGNAL_RT_FAIL: sigaction");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}
	if (raise(SIGUSR1) != 0 || !got_sigusr1) {
		emit_marker("MUSL_SIGNAL_RT_FAIL: handler");
		sync();
		reboot(RB_POWER_OFF);
		return 3;
	}

	emit_marker("MUSL_SIGNAL_RT_PASS");
	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
