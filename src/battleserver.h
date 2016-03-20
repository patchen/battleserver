/*
 * battleserver.h
 *
 *  Created on: Aug 4, 2012
 *      Author: Patrick
 */

#ifndef BATTLESERVER_H_
#define BATTLESERVER_H_

struct client {
	int fd, inbuf, isName, isYell, isTurn, inMatch;

	struct in_addr ipaddr;
	struct client *next, *opponent, *lastOpp;
	int life, powermove;

	char buf[300];
	char name[30];
};
typedef struct client Client;

/*Given fd and in_addr add the client
 * to the server
 */
void addclient(int fd, struct in_addr addr);

/*Client p left, clean his game*/
void cleangame(Client *p);

/*Print stat menu*/
void printstat(Client *p);

/*Given a fd remove the client from
 * from the server
 */
void removeclient(int fd);

/*Given a character and size brodcast a message
 * accross the server
 */
void broadcast(char *s, int size);

/* accept connection, prepare client response */
void newconnection();

/* select() said activity; check it out */
void whatsup(Client *p);

/* Set up a match if possible */
void match(Client *p);

/* Ready Client for game */
void readyclient(Client *p);

/* Return random number between lower and upper bounds */
int rnd(int lower, int upper);

/* Write message to the fd*/
void say(int fd, char *s);

/* Attack the client with or without powermove*/
int myread(Client *p, char *temp);

/* Attack the client with or without powermove*/
void attack(Client *p, Client *q, int power);

#endif /* BATTLESERVER_H_ */
