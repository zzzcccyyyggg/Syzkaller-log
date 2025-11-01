#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 31
#endif
#ifndef BTPROTO_SCO
#define BTPROTO_SCO 2
#endif

typedef struct {
	unsigned char b[6];
} bdaddr_t;
struct sockaddr_sco {
	sa_family_t sco_family;
	bdaddr_t sco_bdaddr;
};

static void sco_addr_any(struct sockaddr_sco *a)
{
	memset(a, 0, sizeof(*a));
	a->sco_family = AF_BLUETOOTH;
}

static void sco_addr_fixed(struct sockaddr_sco *a)
{
	memset(a, 0, sizeof(*a));
	a->sco_family = AF_BLUETOOTH;
	a->sco_bdaddr.b[0] = 0x11;
	a->sco_bdaddr.b[1] = 0x22;
	a->sco_bdaddr.b[2] = 0x33;
	a->sco_bdaddr.b[3] = 0x44;
	a->sco_bdaddr.b[4] = 0x55;
	a->sco_bdaddr.b[5] = 0x66;
}

static int nb_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	int ret = connect(fd, addr, len);
	if (ret == 0)
		return 0;
	if (ret < 0 && (errno == EINPROGRESS || errno == EALREADY))
		return 0;
	return -1;
}

struct worker_arg {
	int fd;
	int us;
};

static void *racer(void *arg)
{
	struct worker_arg *wa = arg;
	int fd = wa->fd;
	int us = wa->us;

	struct sockaddr_sco a_none;
	struct sockaddr_sco a_fixed;
	sco_addr_any(&a_none);
	sco_addr_fixed(&a_fixed);
	const struct sockaddr *sa;
	socklen_t slen;

	sa = (struct sockaddr *)&a_none;
	slen = sizeof(a_none);
	printf("racer %p,tid: %d\n", pthread_self(), gettid());
	if (us > 0)
		usleep(us);
	if (nb_connect(fd, sa, slen) < 0) {
	}
	return NULL;
}

static void *racee(void *arg)
{
	struct worker_arg *wa = arg;
	int fd = wa->fd;
	int us = wa->us;

	struct sockaddr_sco a_none;
	struct sockaddr_sco a_fixed;
	sco_addr_any(&a_none);
	sco_addr_fixed(&a_fixed);
	const struct sockaddr *sa;
	socklen_t slen;

	sa = (struct sockaddr *)&a_fixed;
	slen = sizeof(a_fixed);

	if (us > 0)
		usleep(us);
	if (nb_connect(fd, sa, slen) < 0) {
	}
	close(fd);
	return NULL;
}

int main(void)
{
	int fd = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	int fl = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);

	struct sockaddr_sco bind_addr;
	sco_addr_any(&bind_addr);
	bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

	pthread_t th1, th2;
	struct worker_arg wa1 = { .fd = fd, .us = 0 };
	struct worker_arg wa2 = { .fd = fd, .us = 0 };
	if (pthread_create(&th1, NULL, racer, &wa1) != 0) {
		perror("pthread_create th1");
		close(fd);
		return 1;
	}
	if (pthread_create(&th2, NULL, racee, &wa2) != 0) {
		perror("pthread_create th2");
		close(fd);
		return 1;
	}

	pthread_join(th1, NULL);
	pthread_join(th2, NULL);

	sleep(5);
	close(fd);

	return 0;
}
