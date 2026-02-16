#include <fcntl.h>
#include <stdio.h>
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

static long add3(long x) { return x + 3; }
static long mul2(long x) { return x * 2; }

static long nested(long x)
{
	long a = add3(x);
	long b = mul2(a);
	return add3(b);
}

static long sum_to_n(long n)
{
	if (n <= 0)
		return 0;
	return n + sum_to_n(n - 1);
}

typedef long (*u64_fn)(long);

static long indirect_call(u64_fn fn, long x) { return fn(x); }

static long tail_target(long x) { return x + 9; }
static u64_fn g_tail_fn = tail_target;

static long tail_direct(long x)
{
	__attribute__((musttail)) return tail_target(x);
}

static long tail_indirect(long x)
{
	u64_fn fn = g_tail_fn;
	__attribute__((musttail)) return fn(x);
}

int main(void)
{
	int cfd;
	long r1, r2, r3, r4, r5;

	cfd = open("/dev/console", O_RDWR);
	if (cfd >= 0) {
		(void)dup2(cfd, STDIN_FILENO);
		(void)dup2(cfd, STDOUT_FILENO);
		(void)dup2(cfd, STDERR_FILENO);
		if (cfd > STDERR_FILENO)
			(void)close(cfd);
	}

	emit_marker("MUSL_CALLRET_START");

	r1 = nested(4);
	r2 = sum_to_n(8);
	r3 = indirect_call(add3, 6);
	r4 = tail_direct(11);
	r5 = tail_indirect(2);

	if (r1 != 17 || r2 != 36 || r3 != 9 || r4 != 20 || r5 != 11) {
		printf("MUSL_CALLRET_FAIL: r1=%ld r2=%ld r3=%ld r4=%ld r5=%ld\n",
		       r1, r2, r3, r4, r5);
		fflush(stdout);
		uart_puts("MUSL_CALLRET_FAIL\n");
		sync();
		reboot(RB_POWER_OFF);
		return 2;
	}

	emit_marker("MUSL_CALLRET_PASS");

	sync();
	reboot(RB_POWER_OFF);
	return 0;
}
