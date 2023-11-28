#if !defined(SUPP_H_)
#define SUPP_H_

#include <pthread.h>

typedef struct _Node{
	int req;			//Flag di richiesta
	int obj;			//Numero oggetti
	struct _Node *next;
	}Node;

typedef struct _Queue{
	Node *head;
	Node *tail;
	int len;
	}Queue;

typedef struct _QueueDesk{
	Node *head;
	Node *tail;
	int len;
	pthread_mutex_t qdmutex;
	pthread_cond_t  qdcond;
	}QueueDesk;

typedef struct _ServiceTime{
	float tempo;			//Tempo in cassa
	int obj;			//Numero oggetti
	struct _ServiceTime *next;
	}ServTime;

typedef struct _Cliente{
	float tempo;			//Tempo totale
	float attesa;			//Tempo in coda
	int obj;			//Numero oggetti
	struct _Cliente *next;
	}Cliente;

typedef struct _Cassa{
	int ap;				//Flag di apertura
	int num_cl;			//Clienti serviti
	int n_ap;			//Numero aperture
	float tempo_ap;			//Tempo totale apertura
	double t_med;			//Tempo medio di servizio
	int QW;				//Clienti in coda
	ServTime *tempo_cl;		//Gestione coda
	ServTime *curr_cl;
	}Cassa;

typedef struct _DataStandard{
	int K;				//Numero Casse
	int C;				//Max clienti interni
	int E;				//Numero aggiornamento clienti (C-E)
	int T;				//Max tempo cliente pre-cassa
	int P;				//Max oggetti per cliente
	int W;				//Tempo aggiornamento coda per cassa
	int V;				//Numero massimo casse con 1 cliente in coda per cassa
	int X;				//Numero massimo clienti in coda per cassa
	int O;				//Tempo lavoro singolo oggetto in cassa (ms)
	}DataStand;

typedef struct _DataCl{
	DataStand *conf;		//Puntatore per passaggio dati di configurazione
	int n;				//Indice per thread
	int run;			//Flag di stato
	}DataCl;

typedef struct _DataDir{
	DataStand *conf;		//Puntatore per passaggio dati di configurazione
	}DataDir;

typedef struct _DataCassa{
	int n;				//Numero cassa
	int agg;			//Tempo aggiornamento coda
	int MaxClD;			//Numero massimo clienti in coda
	int o;				//Tempo lavoro singolo oggetto in cassa
	}DataCassa;

void set_end(int sig);
int handle_signals(void);
DataStand *LeggiConf(DataStand *conf, char *argv);
void StampaDati(Cliente *currCl, Cassa *Cas, int N_Cas, int TCl, int TOb);
Queue *Start(Queue *Q);

#endif /* SUPP_H_ */
