#include "threads/signal.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include <stdio.h>
#include <stddef.h>

void SIG_CPU_handler(struct thread *t) {
	printf("CPU life of %d thread ticks expired, thread exiting\n",MAXLIFE );
	thread_exit2(t);
}

void SIG_CHLD_handler(struct thread *t,signal *f)
{
		t->rem=t->rem-1;
	 	printf("Parent thread ( id = %d ) recieved SIG_CHLD for child thread ( id = %d ) : total children created by %d: %d  total child remaining : %d\n",f->pid,f->cid,f->pid,t->chld,t->rem);	
}

void SIG_KILL_handler(struct thread *t,signal *f)
{
		printf("SIG KILL received for thread %d by parent thread %d\n",f->pid,f->cid);
		thread_exit2(t);
}

void SIG_USR_handler(signal *f)
{
	printf("SIG_USR sent to thread %d by thread %d\n",f->pid,f->cid );
}

void SIG_UNBLOCK_handler(signal *f)
{
	printf("SIG_UNBLOCK sent to thread %d by thread %d\n",f->pid,f->cid );
	struct thread *t;
	t=thread_foreach2(f->pid);
	if ( t->status == THREAD_BLOCKED ) {
		printf("Thread %d blocked, unblocking it\n",t->tid);
		thread_unblock(t);
	} 
}

int Signal(enum sigtype a,enum action b){		
	struct thread *t = thread_current();
	unsigned short mask_new = 0;//   0010   0110       

	if(a==SIG_CPU)	
	{			
		if(b==SIG_DFL) {			
			mask_new = 15-1; 
			t->mask = t->mask & mask_new ;
		}		
		else{
			mask_new = 1;
			t->mask = t->mask | mask_new;
		}				
	}			
	else if(a==SIG_CHLD)		
	{		
		if(b==SIG_DFL) {			
			mask_new = 15-2; 
			t->mask = t->mask & mask_new ;
		}		
		else{
			mask_new = 2;
			t->mask = t->mask | mask_new;
		}		
	}		
	else if(a==SIG_USR)		
	{		
		if(b==SIG_DFL) {			
			mask_new = 15-4; 
			t->mask = t->mask & mask_new ;
		}		
		else{
			mask_new = 4;
			t->mask = t->mask | mask_new;
		}		
	}		
	else if(a==SIG_UNBLOCK)		
	{		
		if(b==SIG_DFL) {			
			mask_new = 15-8; 
			t->mask = t->mask & mask_new ;
		}		
		else{
			mask_new = 8;
			t->mask = t->mask | mask_new;
		}
	}		

return 1;
}

int kill(enum sigtype a,tid_t id)
{	
	if(a==SIG_CHLD || a== SIG_CPU) return -1; 

	if(a==SIG_KILL){   
		struct thread *f;
		f=thread_foreach2(id);
		if(f->ppid == thread_current()->tid){
			int p=0;
			struct list_elem *e;
			signal *f;
			for (e = list_begin (&signal_list); e != list_end (&signal_list);
  			 e = list_next (e))
			{
				f = list_entry (e, signal, signal_elem);
				if(f->pid == id && f->sig == a ) {
					p=1;
					break;
				}
			}
			if(p==0){
				signal *s=(signal *)malloc(sizeof(signal));
			    s->sig=SIG_KILL;
			    s->pid=id;
			    s->cid=thread_current()->tid;
			    list_push_back(&signal_list,&(s->signal_elem));
			}
			else f->cid=id;
		}
		return 0;
	}

	else if(a==SIG_USR){   
		int p=0;
		struct list_elem *e;
		signal *f;
		for (e = list_begin (&signal_list); e != list_end (&signal_list);
			 e = list_next (e))
		{
			f = list_entry (e, signal, signal_elem);
			if(f->pid == id && f->sig == a && f->cid == thread_current()->tid) {
				p=1;
				break;
			}
		}
		if(p==0){
			signal *s=(signal *)malloc(sizeof(signal));
		    s->sig=SIG_USR;
		    s->pid=id;
		    s->cid=thread_current()->tid;
		    list_push_back(&signal_list,&(s->signal_elem));
		}
		else f->cid=id;
		return 0;
	}

	else if(a==SIG_UNBLOCK){
		int p=0;
		struct list_elem *e;
		signal *f;
		for (e = list_begin (&signal_list); e != list_end (&signal_list);
			 e = list_next (e))
		{
			f = list_entry (e, signal, signal_elem);
			if( f->pid == id && f->sig == a ) {
				p=1;
				break;
			}
		}
		if(p==0){
			signal *s=(signal *)malloc(sizeof(signal));
		    s->sig=SIG_UNBLOCK;
		    s->pid=id;
		    s->cid=thread_current()->tid;
		    list_push_back(&signal_list,&(s->signal_elem));
		}
		else f->cid=id;
		return 0;
	}

	return -1;
}

void handler(struct thread *t)
{	
	struct thread *t1 = thread_current();
	struct list_elem *e,*prev;
	prev=NULL;
	for (e = list_begin (&signal_list); e != list_end (&signal_list);
	   e = list_next (e)){ 
		
		if(prev!=NULL) {
			list_remove(prev);
			prev=NULL;
		}

		signal *f = list_entry (e, signal, signal_elem);
		if(f->pid == t->tid){

			if(f->sig == SIG_CHLD){ 
				if ( ( (t1->mask>>1) % 2 )==0) SIG_CHLD_handler(t,f);
				prev=e;
			}
			else if(f->sig==SIG_USR)
			{ 
				if ( ( (t1->mask>>2) % 2 )==0) SIG_USR_handler(f);
				prev=e;

			}
			else if(f->sig == SIG_KILL)
			{
				SIG_KILL_handler(t,f);
				prev=e;
			}
		}

		if(f->sig == SIG_UNBLOCK)
		{
			if ( ( (t1->mask>>3) % 2 )==0) SIG_UNBLOCK_handler(f);
			prev=e;
		}
	}

	if(prev!=NULL)
	{
		list_remove(prev);
		prev=NULL;
	}

}

int sigemptyset(sigset_t *s){
	s->mask = 0;
	return 0;
}

int sigfillset(sigset_t *s){
	s->mask = 8+4+2+1;
	return 0;
}

int sigaddset(sigset_t *t, enum sigtype a) {
	unsigned short mask_new = 0;  

	if(a==SIG_CPU)	
	{			
		mask_new = 1;
		t->mask = t->mask | mask_new;					
	}			
	else if(a==SIG_CHLD)		
	{		
		mask_new = 2;
		t->mask = t->mask | mask_new;				
	}		
	else if(a==SIG_USR)		
	{		
		mask_new = 4;
		t->mask = t->mask | mask_new;	
	}		
	else if(a==SIG_UNBLOCK){		
		mask_new = 8;
		t->mask = t->mask | mask_new;	
	}	
	return 0;
}

int sigdelset(sigset_t *t, enum sigtype a) {
	unsigned short mask_new = 0;  

	if(a==SIG_CPU)	
	{						
		mask_new = 15-1; 
		t->mask = t->mask & mask_new ;		
	}			
	else if(a==SIG_CHLD)		
	{				
		mask_new = 15-2; 
		t->mask = t->mask & mask_new ;				
	}		
	else if(a==SIG_USR)		
	{					
		mask_new = 15-4; 
		t->mask = t->mask & mask_new ;	
	}		
	else if(a==SIG_UNBLOCK)		
	{				
		mask_new = 15-8; 
		t->mask = t->mask & mask_new ;
	}	
	return 0;
}

int sigprocmask(enum how h, sigset_t * n, sigset_t * o) {

struct thread *t1 = thread_current();
if (h==SIG_UNBLOCK1) {
	t1->mask = t1->mask ^ (t1->mask & n->mask) ;
	return 0;
}
else if( h==SIG_BLOCK) { 
	t1->mask = t1->mask | n->mask;
	return 0;
}

else if ( h == SIG_SETMASK) {
	if(o!=NULL) o->mask = t1->mask;
	if(n!=NULL) t1->mask = n->mask;
	return 0;
}
else return -1;
}