#define _POSIX_C_SOURCE  200112L
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <supp.h>

static pthread_t *CAS;							//Puntatore ai thread delle casse

static pthread_mutex_t qmutex = PTHREAD_MUTEX_INITIALIZER;		//Mutex per la coda di uscita dei clienti
static pthread_cond_t  qcond = PTHREAD_COND_INITIALIZER;

static int TotCl=0, TotOb=0, ClIn=0, deskOP=1, *TT;
static DataCl **DCl;

static Cassa *Cas;							//Puntatore ai dati delle casse
static DataCassa **DCas;
static Cliente *Cl, *currCl;

static volatile sig_atomic_t dir_OP=1, TotDesk=0, QWm=0, exOP=1;

static Queue *wait_ex;							//Puntatore alla coda di uscita dei clienti
static QueueDesk **wait_desk;						//Puntatore alle code delle casse

static unsigned int seed;

//Gestione chiusura
void set_end(int sig){
	switch(sig){
		case SIGHUP: {
			dir_OP = 0;
			}break;
		default:{
			}
		}
	}

//Randomizzazione dati cliente
Cliente randomCL(Cliente Cl, DataStand *c){
	Cl.tempo = 10+(rand_r(&seed)%c->T);				//Randomizza tempo
	Cl.obj = rand_r(&seed)%c->P;					//Randomizza oggetti
	return Cl;
	}
//Randomizzazione scelta cassa
int randomCas(DataStand *conf){
	unsigned int dseed = time(NULL);			//Randomizza cassa
	int s = rand_r(&dseed)%conf->K;
	while(!Cas[s].ap)
		s--;
	return s;
	}

//CLIENTE
void *cl(void *arg){
	DataCl *D=arg;
	DataStand *conf = D->conf;
	Cliente data=randomCL(data, conf);			//Generazione dati cliente
	clock_t begin = clock();					//Ingresso cliente
	sleep(data.tempo/1000);
	int s = 0;
	float wait=0;
	if(data.obj>0){							//Richiesta Cassa
		Node *new_desk_ex = (Node*)malloc(sizeof(Node));
		new_desk_ex->req=0;	
		new_desk_ex->obj=data.obj;
		new_desk_ex->next = NULL;
		if(deskOP>1)
			s=randomCas(conf);
		clock_t begind = clock();				//Inizio coda
		pthread_mutex_lock(&wait_desk[s]->qdmutex);
		if(wait_desk[s]->head == NULL){
			wait_desk[s]->head = new_desk_ex;
			wait_desk[s]->tail = new_desk_ex;
			}
		else{
			wait_desk[s]->tail->next = new_desk_ex;
			wait_desk[s]->tail = new_desk_ex;
			}
		wait_desk[s]->len++;
		pthread_cond_signal(&wait_desk[s]->qdcond);
		while(new_desk_ex->req == 0)
			pthread_cond_wait(&wait_desk[s]->qdcond,&wait_desk[s]->qdmutex);
		clock_t endd = clock();
		pthread_mutex_unlock(&wait_desk[s]->qdmutex);
		free(new_desk_ex);
		wait = (double)(endd-begind)/CLOCKS_PER_SEC;		//Uscita dalla cassa
		}
	Node *new_ex = (Node*)malloc(sizeof(Node));			//Richiesta uscita
	new_ex->req=0;	
	new_ex->obj=data.obj;
	new_ex->next = NULL;
	pthread_mutex_lock(&qmutex);
	if(wait_ex->head == NULL){
		wait_ex->head = new_ex;
		wait_ex->tail = new_ex;
		}
	else{
		wait_ex->tail->next = new_ex;
		wait_ex->tail = new_ex;
		}
	wait_ex->len++;
	pthread_cond_signal(&qcond);
	while(new_ex->req == 0)
		pthread_cond_wait(&qcond,&qmutex);
	pthread_mutex_unlock(&qmutex);
	clock_t end = clock();	
	free(new_ex);
	Cliente *new=(Cliente*)malloc(sizeof(Cliente));			//Salvataggio dati cliente
	new->tempo = (double)(end-begin)/CLOCKS_PER_SEC;
	new->attesa = wait;
	new->obj=data.obj;
	new->next = NULL;
	if(Cl == NULL){
		Cl = new;
		currCl = Cl;
		}
	else{
		currCl->next = new;
		currCl = currCl->next;
		}
	D->run = 2;							//Aggiornamento stato cliente
	pthread_exit(0);
	}
//AGGIORNAMENTO DATI CODA CASSA
void *timer_desk(void *arg){
	DataCassa *DC = arg;
	int n = DC->n;
	int max = DC->MaxClD;
	while(TT[n]){							
		sleep(DC->agg/50);					//Attesa
		Cas[n].QW = wait_desk[n]->len;				//Aggiornamento lunghezza coda
		if(Cas[n].QW > max)
			QWm = 1;					//Aggiornamento flag se coda troppo lunga
		}
	pthread_exit(0);
	}
//CASSA
void *desk(void *arg){
	DataCassa *DC = arg;
	int n = DC->n, N_Ob;
	TT[n]=1;
	double l=0;
	unsigned int seed = time(NULL);
	int s = 20+(rand_r(&seed)%60);
	Cas[n].ap=1;
	pthread_t timer_cas;
	if (pthread_create(&timer_cas, NULL, &timer_desk, DC) != 0){	//Avvio thread di aggiornamento coda
			perror("timer");
			exit (EXIT_FAILURE);
			}
	clock_t begin = clock();					//Avvio cassa
	while(Cas[n].ap==1 && dir_OP){
		if(wait_desk[n]->head != NULL){
			clock_t beginc = clock();			//Avvio gestione cliente
			pthread_mutex_lock(&wait_desk[n]->qdmutex);
			sleep(s/1000+(DC->o/1000));
			N_Ob = wait_desk[n]->head->obj;
			wait_desk[n]->head->req = 1;			//Aggiornamento coda
			wait_desk[n]->len--;
			wait_desk[n]->head = wait_desk[n]->head->next;
			pthread_cond_signal(&wait_desk[n]->qdcond);
			pthread_mutex_unlock(&wait_desk[n]->qdmutex);
			clock_t endc = clock();				//Chiusura gestione cliente
			Cas[n].num_cl++;
			l = l+(double)(endc-beginc)/CLOCKS_PER_SEC;
			ServTime *new = malloc(sizeof(ServTime));	//Salvataggio dati operazione cliente
			new->tempo = (double)(endc-beginc)/CLOCKS_PER_SEC;
			new->obj = N_Ob;
			new->next = NULL;
			if(Cas[n].tempo_cl == NULL){
				Cas[n].tempo_cl = new;
				Cas[n].curr_cl = Cas[n].tempo_cl;
				}
			else{
				Cas[n].curr_cl->next = new;
				Cas[n].curr_cl = Cas[n].curr_cl->next;
				}
			}	
		}
	clock_t end = clock();						//Chiusura cassa e registrazione dati
	TT[n]=0;
	Cas[n].QW = 0;
	Cas[n].n_ap++;
	Cas[n].ap--;
	Cas[n].t_med = Cas[n].t_med+l;
	double t = (double) (end-begin)/CLOCKS_PER_SEC;
	Cas[n].tempo_ap = (double) Cas[n].tempo_ap+t;
	if(pthread_join(timer_cas, NULL) != 0) {			//Attesa chiusura thread di supporto
		printf("Fail join out\n");
		exit (EXIT_FAILURE);
		}
	if(dir_OP){							//Gestione coda rimanente
		pthread_mutex_lock(&wait_desk[0]->qdmutex);
		if(wait_desk[0]->head!=NULL){
			wait_desk[0]->tail->next = wait_desk[n]->head;
			}
		else
			wait_desk[0]->head = wait_desk[n]->head;
		wait_desk[0]->tail = wait_desk[n]->tail;
		wait_desk[0]->len += wait_desk[n]->len;
		pthread_cond_signal(&wait_desk[n]->qdcond);
		pthread_mutex_unlock(&wait_desk[n]->qdmutex);
		}
	else{
		pthread_mutex_lock(&wait_desk[n]->qdmutex);
		while(wait_desk[n]->head!=NULL){
			wait_desk[n]->head->req = 1;
			wait_desk[n]->head->obj = 0;
			wait_desk[n]->head = wait_desk[n]->head->next;
			pthread_cond_signal(&wait_desk[n]->qdcond);
			}
		pthread_mutex_unlock(&wait_desk[n]->qdmutex);
		}
	pthread_exit(0);
	}
//GESTIONE USCITA CLIENTI
void *out(){
	while(exOP || wait_ex->head != NULL){
		pthread_mutex_lock(&qmutex);
		while(wait_ex->head == NULL && dir_OP == 1) 
		    pthread_cond_wait(&qcond,&qmutex);
		if(wait_ex->head){					//Salvataggio e aggiornamento dati
			TotOb = TotOb+wait_ex->head->obj;
			TotCl++;
			ClIn--;
			wait_ex->head->req = 1;				//Permesso uscita
			wait_ex->len--;
			wait_ex->head = wait_ex->head->next;
			}
		pthread_cond_signal(&qcond);
		pthread_mutex_unlock(&qmutex);
		}
	pthread_exit(0);
	}
//DIRETTORE
void *dir(void *arg){
	handle_signals();
	DataStand *conf = arg;
	pthread_t EX;
	CAS = malloc(conf->K*sizeof(pthread_t));			//Creazione casse
	if (pthread_create(&EX, NULL, &out, conf) != 0){		//Avvio thread di supporto per uscita clienti
		perror("CLIENTE");
		exit (EXIT_FAILURE);
		}
	int *CassaAperta = calloc(conf->K, sizeof(int));		//Creazione array di supporto per casse aperte
	if (pthread_create(&CAS[0], NULL, &desk, DCas[0]) != 0){	//Generazione cassa 0 iniziale
		perror("CASSA");
		exit (EXIT_FAILURE);
		}
	CassaAperta[0]=1;

	int i=0, tr=0;
	while(dir_OP){							//Gestione casse da 1 a K
		if(QWm && deskOP<conf->K){				//Verifica su flag aggiornato da timer_desk
			i=1;
			tr=0;
			while(i<conf->K && !tr){			//Ricerca nuova cassa
				if(CassaAperta[i]==1)
					i++;
				else
					tr=1;
				}
			if(i<conf->K){					//Avvio nuova cassa
				deskOP++;
				if (pthread_create(&CAS[i], NULL, &desk, DCas[i]) != 0){
					perror("CASSA");
					exit (EXIT_FAILURE);
					}
				CassaAperta[i]++;
				}
			QWm=0;
			}
		else{							//Verifica chiusura cassa su dati da timer_desk
		int cnt=0;
		for(int j=0;j<conf->K;j++){
			if(CassaAperta[j]==1 && Cas[j].QW==1)
					cnt++;
			}
		tr=0;
		if(cnt>conf->V && !tr){					//Ricerca e selezione cassa da chiudere
				i=conf->K-1;
				while(!tr && i>0){
					if(CassaAperta[i] && Cas[i].QW>1)
						i--;
					else
						tr=1;
					}
				if(i>0 && CassaAperta[i]==1){		//Chiusura cassa
					CassaAperta[i]=0;
					Cas[i].ap=0;
					if(pthread_join(CAS[i], NULL) != 0) {
						printf("Fail join outD\n");
						exit (EXIT_FAILURE);
						}
					}
				tr=1;
			}
		}
	}
	for(int i=0;i<conf->K;i++){					//Chiusura di tutte le casse
		if(CassaAperta[i]){
			Cas[i].ap=0;
			CassaAperta[i]=0;
			if(pthread_join(CAS[i], NULL) != 0) {
				printf("Fail join out %d\n", i);
				exit (EXIT_FAILURE);
				}
			}
		}
	free(CassaAperta);
	exOP=0;
	if(pthread_join(EX, NULL) != 0) {				//Chiusura thread di supporto per uscita clienti
		printf("Fail join out\n");
		exit (EXIT_FAILURE);
		}
	pthread_exit(0);
	}

int main(int argc, char *argv[]){
	if (argc < 2){							//Errore se non vengono passati argomenti
		fprintf(stderr, "NO TEST\n");
		exit(EXIT_FAILURE);
   	}
		
	DataStand *conf=calloc(1, sizeof(DataStand));			//Lettura test
	conf = LeggiConf(conf, argv[1]);
	TotDesk = conf->K;

	Cas = calloc(conf->K, sizeof(Cassa));				//Creazione spazi dati casse
	DCas = malloc(conf->K*sizeof(DataCassa*));
	Cl=NULL;							//Puntatore a struttura cliente
	seed = time(NULL);						//Seme iniziale per randomizzare i dati clienti
	TT = calloc(conf->K, sizeof(int));				//Creazione struttura di supporto alle casse
	wait_ex = Start(wait_ex);					//Creazione coda uscita
	wait_desk = (QueueDesk**)malloc(conf->K*sizeof(QueueDesk*));	//Creazione code casse
	for(int i=0;i<conf->K;i++){					//Inizializzazione dati rimanenti
		Cas[i].tempo_cl=NULL;
		Cas[i].curr_cl=NULL;
		DCas[i] = calloc(1, sizeof(DataCassa));
		DCas[i]->n = i;
		DCas[i]->o = conf->O;
		DCas[i]->agg = conf->W;
		DCas[i]->MaxClD = conf->X;
		wait_desk[i] = (QueueDesk*)malloc(sizeof(QueueDesk));
		wait_desk[i]->head = NULL;
		wait_desk[i]->tail = NULL;
		wait_desk[i]->len = 0;
		if (pthread_mutex_init(&wait_desk[i]->qdmutex, NULL) != 0) {
			perror("mutex init");
			exit (EXIT_FAILURE);
		}
   		if (pthread_cond_init(&wait_desk[i]->qdcond, NULL) != 0) {
			perror("mutex cond");
			if (&wait_desk[i]->qdmutex) 
				pthread_mutex_destroy(&wait_desk[i]->qdmutex);
			exit (EXIT_FAILURE);
		    }   
		}
	DCl = calloc(conf->C, sizeof(DataCl*));				//Creazione struttura per stato cliente
	for(int i=0;i<conf->C;i++){
		DCl[i] = malloc(sizeof(DataCl));
		DCl[i]->conf = conf;
		DCl[i]->n = i;
		}

	pthread_t DIR;
	if (pthread_create(&DIR, NULL, &dir, conf) != 0){		//Apertura DIRETTORE
		perror("DIRETTORE");
		exit (EXIT_FAILURE);
		}

	pthread_t *CL = malloc(conf->C*sizeof(pthread_t));		//Creazione CLIENTE
	int at=0;
	while(dir_OP){							//Gestione clienti
		while(ClIn < conf->C){					//Verifica stato cliente
			if(at == conf->C)
				at=0;
			if(DCl[at]->run == 2){				//Chiusura se ciclo cliente completato
				if(pthread_join(CL[at], NULL) != 0) {
					printf("Fail join CL\n");
					exit (EXIT_FAILURE);
					}
				DCl[at]->run=0;
				}
			if(DCl[at]->run == 0){
				if (pthread_create(&CL[at], NULL, &cl, DCl[at]) != 0){		//Creazione singolo CLIENTE
					perror("CLIENTE");
					exit (EXIT_FAILURE);
					}
				DCl[at]->run=1;
				ClIn++;
				}
			at++;
			}
		while(ClIn >= (conf->C-conf->E))			//Attesa se numero clienti attivi soddisfatto
			sleep(1/100);
		}

	for(int i=0;i<conf->C;i++){					//Chiusura clienti rimanenti
		if(DCl[i]->run == 2){
			if(pthread_join(CL[i], NULL) != 0) {
				printf("Fail join CL\n");
				exit (EXIT_FAILURE);
				}
			}
		}

	if(pthread_join(DIR, NULL) != 0) {				//Chiusura DIRETTORE
		printf("Fail join DIRETTORE\n");
		exit (EXIT_FAILURE);
		}

	currCl = Cl;
	StampaDati(currCl, Cas, conf->K, TotCl, TotOb);			//Stampa dati
			
	while(Cl != NULL){						//Libera memoria
		currCl = Cl;
		Cl = Cl->next;
		free(currCl);
		}
	for(int i=0;i<conf->K;i++){
		for(int j=0;j<Cas[i].num_cl;j++){
			Cas[i].curr_cl = Cas[i].tempo_cl;
			Cas[i].tempo_cl = Cas[i].tempo_cl->next;
			free(Cas[i].curr_cl);
			}
		free(DCas[i]);
		free(wait_desk[i]);
		}
	for(int i=0;i<conf->C;i++)
		free(DCl[i]);
	free(TT);
	free(CL);
	free(DCl);
	free(CAS);
	free(conf);
	free(Cas);
	free(DCas);
	free(wait_ex);
	free(wait_desk);

	return 0;
}
