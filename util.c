#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

static void eprint(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s", bufout);
	if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s\n", strerror(errno));
	exit(1);
}

static int dial(char *host, char *port) {
	static struct addrinfo hints;
	int sock = 0;
	struct addrinfo *res, *r;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		eprint("error: cannot resolve hostname '%s':", host);
	for (r = res; r; r = r->ai_next) {
		if ((sock = socket(r->ai_family, r->ai_socktype,
				   r->ai_protocol)) == -1)
			continue;
		if (connect(sock, r->ai_addr, r->ai_addrlen) == 0)
			break;
		close(sock);
	}
	freeaddrinfo(res);
	if (!r)
		eprint("error: cannot connect to host '%s'\n", host);
	return sock;
}

#define strlcpy _strlcpy
static void strlcpy(char *to, const char *from, int l) {
	memccpy(to, from, '\0', l);
	to[l - 1] = '\0';
}

static char *eat(char *s, int (*p)(int), int r) {
	while (*s != '\0' && p(*s) == r)
		s++;
	return s;
}

static char *skip(char *s, char c) {
	while (*s != c && *s != '\0')
		s++;
	if (*s != '\0')
		*s++ = '\0';
	return s;
}

static char *skips(char *s, char c) {
	if (*s == c) {
		*s++ = '\0';
		return s;
	}
	do
		do
			s++;
		while (*(s - 1) != ' ' && *(s - 1) != '\0');
	while (*(s - 1) != '\0' && *s != c && *s != '\0');
	if (*s != '\0')
		*s++ = '\0';
	return s;
}

static void trim(char *s) {
	char *e;

	e = s + strlen(s) - 1;
	while (isspace(*e) && e > s)
		e--;
	*(e + 1) = '\0';
}
