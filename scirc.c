#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *host = "localhost";
static char *port = "6667";
static char *password;
static char *real;
static char *ident;
static char *caps;
static char *sasl;
static char nick[32];
static char bufin[4096];
static char bufout[4096];
static char channel[256];
static int state = 0;
static int waitjoin = 0;
static int quiet = 0;
static time_t trespond;
static FILE *srv;

#include "util.c"

static void
pout(char *channel, char *fmt, ...) {
	static char timestr[9];
	time_t t;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	t = time(NULL);
	strftime(timestr, sizeof timestr, "%T", localtime(&t));
	fprintf(stdout, "%-12s: %s %s\n", channel, timestr, bufout);
}

static void
sout(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	fprintf(srv, "%s\r\n", bufout);
}

static void
privmsg(char *channel, char *msg) {
	if(channel[0] == '\0') {
		pout("", "No channel to send to");
		return;
	}
	if(quiet == 0)
		pout(channel, "<%s> %s", nick, msg);
	sout("PRIVMSG %s :%s", channel, msg);
}

static void
parsein(char *s) {
	char c, *p;

	if(s[0] == '\0')
		return;
	skip(s, '\n');
	if(s[0] != '/') {
		privmsg(channel, s);
		return;
	}
	c = *++s;
	if(c != '\0' && isspace(s[1])) {
		p = s + 2;
		switch(c) {
		case 'j':
			sout("JOIN %s", p);
			if(channel[0] == '\0')
				strlcpy(channel, p, sizeof channel);
			return;
		case 'p':
			s = eat(p, isspace, 1);
			p = eat(s, isspace, 0);
			if(!*s)
				s = channel;
			if(*p)
				*p++ = '\0';
			if(!*p)
				p = "scirc - bad irc client";
			sout("PART %s :%s", s, p);
			return;
		case 'm':
			s = eat(p, isspace, 1);
			p = eat(s, isspace, 0);
			if(*p)
				*p++ = '\0';
			privmsg(s, p);
			return;
		case 's':
			strlcpy(channel, p, sizeof channel);
			return;
		}
	}
	sout("%s", s);
}

static void
parsesrv(char *cmd) {
	char *usr, *par, *txt;

	usr = host;
	if(!cmd || !*cmd)
		return;
	if(cmd[0] == ':') {
		usr = cmd + 1;
		cmd = skip(usr, ' ');
		if(cmd[0] == '\0')
			return;
		skip(usr, '!');
	}
	skip(cmd, '\r');
	par = skip(cmd, ' ');
	txt = skip(par, ':');
	trim(par);
	if(!strcmp("001", cmd)) {
		state = 2;
		if(channel[0] != '\0')
			sout("JOIN %s", channel);
	}
	if(!strcmp("366", cmd)) {
		state = 3;
	}
	if(state == 1) {
		if(!strcmp("CAP", cmd)) {
			char *dup[512];
			strcpy(dup, par);
			strtok(dup, " ");
			strcpy(dup, strtok(NULL, " "));
			if(!strcmp(dup,"ACK")) {
				if(sasl && strstr(txt,"sasl"))
					sout("AUTHENTICATE PLAIN");
			}
			if(!strcmp(dup,"NAK"))
				sout("CAP END");
		}
		if (!strcmp("AUTHENTICATE", cmd) && sasl)
			sout("AUTHENTICATE %s", sasl);
		if (!strcmp("903", cmd) && sasl)
			sout("CAP END");
		if (!strcmp("433", cmd) && strlen(nick) + 2 < sizeof(nick)) {
			strcat(nick, "_");
			sout("NICK %s", nick);
		}
	}
	if(!strcmp("PONG", cmd))
		return;
	if(!strcmp("PRIVMSG", cmd))
		pout(par, "<%s> %s", usr, txt);
	else if(!strcmp("PING", cmd))
		sout("PONG %s", txt);
	else {
		pout(usr, "%s %s :%s", cmd, par, txt);
		if(!strcmp("NICK", cmd) && !strcmp(usr, nick))
			strlcpy(nick, txt, sizeof nick);
	}
}

int
main(int argc, char *argv[]) {
	int i, c;
	struct timeval tv;
	const char *user = getenv("USER");
	fd_set rd;

	strlcpy(nick, user ? user : "unknown", sizeof nick);
	for(i = 1; i < argc; i++) {
		c = argv[i][1];
		if(argv[i][0] != '-' || argv[i][2])
			c = -1;
		switch(c) {
		case 'h':
			if(++i < argc) host = argv[i];
			break;
		case 'p':
			if(++i < argc) port = argv[i];
			break;
		case 'u':
			if(++i < argc) ident = argv[i];
			break;
		case 'r':
			if(++i < argc) real = argv[i];
			break;
		case 'a':
			if(++i < argc) caps = argv[i];
			break;
		case 's':
			if(++i < argc) sasl = argv[i];
			break;
		case 'n':
			if(++i < argc) strlcpy(nick, argv[i], sizeof nick);
			break;
		case 'j':
			if(++i < argc) strlcpy(channel, argv[i], sizeof channel);
			break;
		case 'k':
			if(++i < argc) password = argv[i];
			break;
		case 'w':
			waitjoin = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'v':
			eprint("scirc-"VERSION"\n");
		default:
			eprint("usage: scirc [-h host] [-p port] [-n nick] [-u username] [-r realname] [-a caps] [-s sasltoken] [-j channel] [-k keyword] [-w] [-q] [-v]\n");
		}
	}
	/* init */
	i = dial(host, port);
	srv = fdopen(i, "r+");
	/* login */
	state = 1;
	sout("CAP LS 302");
	if(password)
		sout("PASS %s", password);
	if(!ident)
		ident = nick;
	if(!real)
		real = nick;
	sout("NICK %s", nick);
	sout("USER %s 0 * :%s", ident, real);

	if(caps)
		sout("CAP REQ :%s", caps);
	else
		sout("CAP END");

	fflush(srv);
	setbuf(stdout, NULL);
	setbuf(srv, NULL);
	for(;;) { /* main loop */
		FD_ZERO(&rd);
		FD_SET(0, &rd);
		FD_SET(fileno(srv), &rd);
		tv.tv_sec = 120;
		tv.tv_usec = 0;
		i = select(fileno(srv) + 1, &rd, 0, 0, &tv);
		if(i < 0) {
			if(errno == EINTR)
				continue;
			eprint("scirc: error on select():");
		}
		else if(i == 0) {
			if(time(NULL) - trespond >= 300)
				eprint("scirc shutting down: parse timeout\n");
			sout("PING %s", host);
			continue;
		}
		if(FD_ISSET(fileno(srv), &rd)) {
			if(fgets(bufin, sizeof bufin, srv) == NULL)
				eprint("scirc: remote host closed connection\n");
			parsesrv(bufin);
			trespond = time(NULL);
		}
		if(FD_ISSET(0, &rd) && (!waitjoin || state == 3)) {
			if(fgets(bufin, sizeof bufin, stdin) == NULL)
				eprint("scirc: broken pipe\n");
			parsein(bufin);
		}
	}
	return 0;
}
