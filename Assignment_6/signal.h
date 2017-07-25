#include "threads/malloc.h"
#include <stdio.h>
#include "threads/thread.h"
#include <list.h>
typedef int tid_t;
enum sigtype {SIG_CHLD,SIG_KILL,SIG_CPU,SIG_USR,SIG_UNBLOCK};
enum action {SIG_DFL,SIG_IGN};
enum how {SIG_BLOCK, SIG_UNBLOCK1, SIG_SETMASK};

typedef struct{
		struct list_elem signal_elem;
		enum sigtype sig;
		tid_t pid;
		tid_t cid;
} signal;

typedef struct{
	unsigned short int mask;
} sigset_t;

struct list signal_list;

void handler(struct thread *);
void SIG_CPU_handler(struct thread *);
void SIG_KILL_handler(struct thread *,signal *);
void SIG_USR_handler(signal *);
void SIG_CHLD_handler(struct thread *,signal *);
void SIG_UNBLOCK_handler(signal * );

int Signal(enum sigtype ,enum action );
int kill(enum sigtype ,tid_t );

int sigemptyset(sigset_t *);
int sigfillset(sigset_t *);
int sigaddset(sigset_t *,enum sigtype);
int sigdelset(sigset_t *,enum sigtype);
int sigprocmask(enum how,sigset_t *, sigset_t *);