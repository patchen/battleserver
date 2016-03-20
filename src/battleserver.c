/*Patrick Chen ~ August 4 2012
 *Some Code taken from Alan J Rosenthal and lecture slides
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "battleserver.h"

#define MAXLIFE 30
#define MINLIFE 20
#define MAXPOWER 3
#define MAXATTACK 6

#ifndef PORT
#define PORT 32512
#endif

int listenfd;
int maxfd = 2;
fd_set set;
Client *top = NULL;

static char stat[] = "Hitpoints: %d PowerMoves: %d\r\n";
static char menu[] = "[A] ttack \n[P] owermove\n[Y] ell\n";

void say(int fd, char *s) {
	char buf[300];
	sprintf(buf, "%s", s);
	if (write(fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "write\n");
	}
}

void printstat(Client *p) {
	char buf[30];
	sprintf(buf, stat, p->life, p->powermove);
	if (write(p->fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "write\n");
	}
}

void setup(void) {
	struct sockaddr_in r;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	memset(&r, '\0', sizeof(r));
	r.sin_family = AF_INET;
	r.sin_addr.s_addr = INADDR_ANY;
	r.sin_port = htons(PORT);

	if (bind(listenfd, (struct sockaddr *) &r, sizeof(r)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(listenfd, 10) == -1) {
		perror("listen");
		exit(1);
	}
	printf("Patrick's Battle Server!\n");
}

//From Alan Rosenthal
void broadcast(char *s, int size) {
	Client *p;
	for (p = top; p; p = p->next)
		write(p->fd, s, size);
}

//From Alan Rosenthal
void addclient(int fd, struct in_addr addr) {
	Client *p = malloc(sizeof(Client));
	if (!p) {
		fprintf(stderr, "out of memory!\n"); /* highly unlikely to happen */
		exit(1);
	}
	printf("Adding client %s\n", inet_ntoa(addr));
	fflush(stdout);
	p->opponent = NULL;
	p->fd = fd;
	p->ipaddr = addr;
	p->next = top;
	p->isName = 1;
	p->isYell = 0;
	p->isTurn = 0;
	p->inMatch = 0;
	top = p;

}

void cleangame(Client *p) {
	say(p->opponent->fd, "You WIN, he ran away!");
	// not in match, last opponent, turn
	p->opponent->inMatch = 0;
	p->opponent->lastOpp = p;
	p->opponent->isTurn = 0;
	match(p->opponent);
}

//From Alan Rosenthal
void removeclient(int fd) {
	char buf[50];
	Client **p;
	for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next)
		;
	if (*p) {
		Client *t = (*p)->next;
		printf("Removing client %s\n", inet_ntoa((*p)->ipaddr));
		sprintf(buf, "\r\nChallenger %s has left\r\n", (*p)->name);
		broadcast(buf, strlen(buf));
		if ((*p)->opponent != NULL )
			cleangame(*p);
		fflush(stdout);
		free(*p);
		*p = t;
	} else {
		fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
		fflush(stderr);
	}

}

//From Dan
int myread(Client *p, char *temp) {
	char *startbuf = p->buf + p->inbuf;
	int room = sizeof(p->buf) - p->inbuf;
	int crlf;
	char *tok, *cr, *lf;

	if (room <= 1) { // overflow
		removeclient(p->fd);
		return -1;
	}
	int len = read(p->fd, startbuf, room - 1);
	if (len <= 0) { // dropped
		removeclient(p->fd);
		return -1;
	}

	p->inbuf += len;
	p->buf[p->inbuf] = '\0';
	lf = strchr(p->buf, '\n');
	cr = strchr(p->buf, '\r');
	if (!lf && !cr)
		return 0; //No complete line
	tok = strtok(p->buf, "\r\n");
	if (tok)
		// use tok (complete string)
		strcpy(temp, tok);

	if (!lf)
		crlf = cr - p->buf;
	else if (!cr)
		crlf = lf - p->buf;
	else
		crlf = (lf > cr) ? lf - p->buf : cr - p->buf;
	crlf++;
	p->inbuf -= crlf;
	memmove(p->buf, p->buf + crlf, p->inbuf);
	return 1;
}

int rnd(int lower, int upper) {
	return (rand() % (upper - lower)) + lower;
}

void attack(Client *p, Client *q, int power) {
	char buf[50];
	int attack = rnd(1, MAXATTACK);
	if (power == 1 && p->powermove > 0) { // power move enough
		p->powermove -= 1;
		if (rnd(0, 2) == 1) // worked
			attack = attack * 3;
		else
			// failed
			attack = attack * 0;
	} else if (power == 1 && p->powermove <= 0) { // power move not enough
		say(p->fd, "You have no more Power Moves. Doing Regular Attack\n");
	}

	// how much damage was done
	if (attack == 0) {
		say(p->fd, "\r\nMISS\n");
		say(p->opponent->fd, "\r\nHe tried to powermove you but missed\n");
	} else {
		sprintf(buf, "\r\nYou did %d damage \r\n", attack);
		if (write(p->fd, buf, strlen(buf)) < 0)
			fprintf(stderr, "write\n");
		sprintf(buf, "\r\nYou got hit with %d damage \r\n", attack);
		if (write(q->fd, buf, strlen(buf)) < 0)
			fprintf(stderr, "write\n");
		}

	q->life -= attack;

	if (q->life <= 0) { //dead check
		say(p->fd, "You WON\r\nFinding Opponent...\r\n");
		say(q->fd, "You LOST\r\nFinding Opponent...\r\n");

		// not in match, last opponent, turn
		p->inMatch = 0;
		q->inMatch = 0;
		p->lastOpp = q;
		q->lastOpp = p;
		p->isTurn = 0;
		q->isTurn = 0;

		match(p);
		match(q);
	} else {
		//set turns
		p->isTurn = 0;
		q->isTurn = 1;

		printstat(q);
		say(q->fd, menu);

	}
}

void readyclient(Client *p) {
	p->life = rnd(MINLIFE, MAXLIFE);
	p->powermove = rnd(1, MAXPOWER);
}

void match(Client *p) {
	Client *q;
	char buf[50];

	for (q = top; q; q = q->next)
		if (q != p && p->inMatch == 0 && q->inMatch == 0 && p->isName == 0
				&& q->isName == 0 && p->lastOpp != q) { // not yourself ,not in match, with name, not last oppoennt = ready
				// in match
			p->inMatch = 1;
			q->inMatch = 1;

			// oppoents
			p->opponent = q;
			q->opponent = p;

			//alert them
			sprintf(buf, "Now fighting with %s \r\n", p->name);
			if (write(q->fd, buf, strlen(buf)) < 0)
				fprintf(stderr, "write\n");
			sprintf(buf, "Now fighting with %s \r\n", q->name);
			if (write(p->fd, buf, strlen(buf)) < 0) {
				fprintf(stderr, "write\n");
			}
				//give p the turn
			p->isTurn = 1;
			q->isTurn = 0;

			readyclient(p);
			readyclient(q);

			//print stats
			printstat(p);
			printstat(q);

			//print menu
			say(p->fd, menu);
		}
}

//Based on code by Alan Rosenthal
void whatsup(Client *p) /* select() said activity; check it out */
{
	char msg[300];
	char temp[50];
	char buf[2];
	int len;
	if (p->isName == 1) {/*set name */  //fix so wait for new line
		if (myread(p, msg) == 1) {
			strcpy(p->name, msg);
			p->isName = 0;
			printf("fd: %d is using name '%s'\n", p->fd, p->name);
			sprintf(temp, "\r\nChallenger %s has entered the arena\r\n",
					p->name);
			broadcast(temp, strlen(temp));
			say(p->fd, "Awaiting Opponent...\r\n");
			readyclient(p);
			match(p);
		}
	} else if (p->isTurn == 1) {  //their turn
		if (p->isYell == 1) { /* yell content */
			if (myread(p, msg) == 1) {
				say(p->opponent->fd, "Opponent Says: ");
				say(p->opponent->fd, msg);
				p->isYell = 0;
				printstat(p);
				say(p->fd, menu);
			}
		} else {
			len = read(p->fd, buf, sizeof buf);
			if (len <= 0) {
				removeclient(p->fd);
			} else if ((buf[0] == 'y' || buf[0] == 'Y')) { /* want to yell*/
				p->isYell = 1;
				say(p->fd, "\r\nYell:\r\n");
			} else if (buf[0] == 'a' || buf[0] == 'A') {/* normal */
				attack(p, p->opponent, 0);
			} else if ((buf[0] == 'p' || buf[0] == 'P') && p->powermove > 0) {/* Power */
				attack(p, p->opponent, 1);
			}
		}
	} else if (p->isTurn == 0 && read(p->fd, buf, sizeof buf) <= 0) // Remove client gone
		removeclient(p->fd);
}

//From Alan Rosenthal
void newconnection() {
	int newfd;
	struct sockaddr_in r;

	socklen_t socklen = sizeof r;

	//accept
	if ((newfd = accept(listenfd, (struct sockaddr *) &r, &socklen)) < 0)
		perror("accept");

	say(newfd, "Patrick's Battle Server!\n");
	say(newfd, "Welcome to the Arena. What is your name?");

	FD_SET(newfd, &set);	// add to sets
	if (newfd > maxfd)		// update fd max
		maxfd = newfd;
	addclient(newfd, r.sin_addr);

}

//Based on code by Alan Rosenthal
int main(void) {
	srand((unsigned) time(NULL ));

	fd_set tempset;
	Client *p;
	int yes = 1;
	FD_ZERO(&set);
	FD_ZERO(&tempset);

	setup();
	FD_SET(listenfd, &set);
	maxfd = listenfd;

	while (1) {
		tempset = set;
		if (select(maxfd + 1, &tempset, NULL, NULL, NULL ) == -1) {
			perror("select");
			exit(1);
		}
		for (p = top; p; p = p->next)
			if (FD_ISSET(p->fd, &tempset)) // if in set
				break;

		if (p) //handle client
			whatsup(p);

		if (FD_ISSET(listenfd, &tempset)) // new connection
			newconnection();
	}

	//fix port
	broadcast("Server Closed", 14);
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	return (0);
}
