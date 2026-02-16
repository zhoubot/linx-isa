#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

enum {
	LINX_UART_BASE = 0x10000000ul,
};

static jmp_buf jb;
static sigjmp_buf sjb;

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
	int r;
	sigset_t set, old, cur;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_SETJMP_START");
	r = setjmp(jb);
	if (r == 0) {
		longjmp(jb, 0);
	}
	if (r != 1) {
		emit_marker("MUSL_SETJMP_FAIL: longjmp zero-normalization");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &set, &old) != 0) {
		emit_marker("MUSL_SETJMP_FAIL: sigprocmask block");
		sync();
		reboot(RB_POWER_OFF);
		return 3;
	}

	r = sigsetjmp(sjb, 1);
	if (r == 0) {
		(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
		siglongjmp(sjb, 9);
	}
	if (r != 9) {
		emit_marker("MUSL_SETJMP_FAIL: siglongjmp value");
		sync();
		reboot(RB_POWER_OFF);
		return 4;
	}
	if (sigprocmask(SIG_SETMASK, NULL, &cur) != 0 || !sigismember(&cur, SIGUSR1)) {
		emit_marker("MUSL_SETJMP_FAIL: mask restore");
		sync();
		reboot(RB_POWER_OFF);
		return 5;
	}
	(void)sigprocmask(SIG_SETMASK, &old, NULL);

	emit_marker("MUSL_SETJMP_PASS");
	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
