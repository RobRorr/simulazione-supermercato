#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <supp.h>

int handle_signals(void) {						//Gestione segnali
	sigset_t set;
	struct sigaction sa;
	sigfillset(&set);
	pthread_sigmask(SIG_SETMASK,&set,NULL);
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = set_end;
	sigaction(SIGHUP,&sa,NULL);
	sigemptyset(&set);
	pthread_sigmask(SIG_SETMASK,&set,NULL);
	return 0;
	}

Queue *Start(Queue *Q){
	Q = (Queue*)malloc(sizeof(Queue));				//Creazione coda uscita
	Q->head = NULL;
	Q->tail = NULL;
	Q->len = 0;
	return Q;
	}

DataStand *LeggiConf(DataStand *conf, char *argv){			//Leggi configurazione
	FILE *fp;
	int n;
	char c;
	if ((fp = fopen(argv, "r")) == NULL) {
		perror ("errore read");
		exit(EXIT_FAILURE);
   		}
	while (fscanf(fp, "%c %d\n", &c, &n) != EOF){
		switch (c){
			case 'K':{
				conf->K = n;				//Numero Casse
				}break;
			case 'C':{
				conf->C = n;				//Max clienti interni
				}break;
			case 'E':{
				conf->E = n;				//Numero aggiornamento clienti (C-E)
				}break;
			case 'T':{
				conf->T = n;				//Max tempo cliente pre-cassa
				}break;
			case 'P':{
				conf->P = n;				//Max oggetti per cliente
				}break;
			case 'V':{
				conf->V = n;				//Numero massimo casse con 1 cliente in coda per cassa
				}break;
			case 'W':{
				conf->W = n;				//Tempo aggiornamento coda per cassa
				}break;
			case 'X':{
				conf->X = n;				//Numero massimo clienti in coda per cassa
				}break;
			case 'O':{
				conf->O = n;				//Tempo lavoro singolo oggetto in cassa (ms)
				}break;
			default:
				break;
			}
		}
	if (fp) 
		fclose(fp);
	return conf;
	}

void StampaDati(Cliente *currCl, Cassa *Cas, int N_Cas, int TCl, int TOb){	//Stampa dati
	int i = 0;
	while(currCl != NULL){							//Stampa dati clienti
		printf("Cl: %d %.3f %.3f %d\n", i, currCl->tempo, currCl->attesa, currCl->obj);
		currCl=currCl->next;
		i++;
		}

	for(int i=0;i<N_Cas;i++){						//Stampa dati casse
		if(Cas[i].t_med > 0 && Cas[i].num_cl)
			Cas[i].t_med = Cas[i].t_med/Cas[i].num_cl;
		printf("Cas: %d %d %.3f %d %.3f\n", i, Cas[i].num_cl, Cas[i].tempo_ap, Cas[i].n_ap, Cas[i].t_med);
		Cas[i].curr_cl = Cas[i].tempo_cl;
		for(int j=0;j<Cas[i].num_cl;j++){
			printf("timeServ: %.3f %d\n", Cas[i].curr_cl->tempo, Cas[i].curr_cl->obj);
			Cas[i].curr_cl = Cas[i].curr_cl->next;
			}
		}
	printf("TotCl:%d\n", TCl);						//Stampa dati generali
	printf("TotOb:%d\n", TOb);
	//printf("END LOG\n");
	}
