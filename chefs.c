#include "parametrage.h"

/*Fonction qui gère l'arrêt lors de la réception d'un SIGUSR1*/
void arret(){
    fprintf(stderr,"\t<CHEFS ATELIER> [%d], réception SIGUSR1 on s'arrête\n",getpid());
  
    exit(-1);
}

void mon_sigaction(int s, void (*f)(int)){
    struct sigaction action;
 
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(s,&action,NULL);
}

//Yet Another Main Function
int main(int argc, char *argv[]){
    int type_outil[100];/*Première case = nombre d'outils différents. Cases suivantes = nb max d'outils par type*/
    sigset_t masque;
    key_t cle;/*cle des files*/
    int file_mess_co;/*ID de cette file chefs/ouvriers*/
    int file_mess_cc;/*ID de la file clients/Chefs*/
    FILE *fich_cle = NULL;
    /*Pour les échanges avec le client dans la file cc*/
    c_requete_t c_requete;
    c_reponse_t c_reponse;
    /*Pour les échanges avec les ouvriers dans la file co*/
    requete_t requete;
    reponse_t reponse;

    srand(getpid());/*On utilise le pid pour pas que tous les chefs aient les mêmes rand()*/
    /*Ca ne devrait jamais arriver mais au cas où, erreur très grave */
    if(argc < 3){
      fprintf(stderr,"\n\n\tPROBLEMES lors de la création du chef d'atelier %d, nombre d'arguments incorrect %d reçu\n\n",getpid(), argc);
        kill(getppid(),SIGUSR1);
    }
    type_outil[0] = argc - 2;
    for(int i = 1; i <= type_outil[0];i++){
        type_outil[i] = atoi(argv[i+1]);
    }
    /*Préparation du masque, seul SIGUSR1 est reconnu*/

    sigfillset(&masque);
    sigdelset(&masque,SIGUSR1);
    sigprocmask(SIG_SETMASK,&masque,NULL);
    

    /*On donne le traitement de SIGUSR1 à arret() */
    mon_sigaction(SIGUSR1, arret);

    /*---FILE DE MESSAGE CO---*/
    /*Récupération de la clé*/
    fich_cle = fopen(FICHIER_CLE_CO,"r");
    if (fich_cle==NULL){
	fprintf(stderr,"\tLe fichier pour créer la clé CO n'existe pas. Le chef meurt.\n");
        exit(-1);
    }
    
    cle = ftok(FICHIER_CLE_CO,'a');
    if (cle==-1){
	fprintf(stderr,"\tCréation de clé impossible chef.\n");
	exit(-1);
    }

    
    /* Recuperation file de message :    */
    file_mess_co = msgget(cle,0);
    if (file_mess_co==-1){
	fprintf(stderr,"\tImpossible de récupérer la file de message\n");
	exit(-1);
    }




    /*---FILE DE MESSAGE CC---*/
    /*Récupération de la clé*/
    fich_cle = NULL;
    fich_cle = fopen(FICHIER_CLE_CC,"r");
    if (fich_cle==NULL){
	fprintf(stderr,"\tLe fichier pour créer la clé CC n'existe pas. Le chef meurt.\n");
        exit(-1);
    }
    
    cle = ftok(FICHIER_CLE_CC,'a');
    if (cle==-1){
	fprintf(stderr,"\tCréation de clé impossible chef.\n");
	exit(-1);
    }

    /* Recuperation file de message :    */
    file_mess_cc = msgget(cle,0);
    if (file_mess_cc==-1){
	fprintf(stderr,"\tImpossible de récupérer la file de message\n");
	exit(-1);
    }


    /*Boucle principale*/
    for(;;){
        /*Attente du client*/
        if (msgrcv(file_mess_cc,&c_requete, sizeof(c_requete_t),atoi(argv[1]),0) == -1){
            fprintf(stderr,"\t<CHEF> [%d] Pas pu lire la requête du client.\n",atoi(argv[1]));
        }
        /*Envoi aux mécanos*/
        requete.type = 1;
        requete.chef = getpid();
#ifdef TRAVAIL_CONSTANT
        requete.duree = TRAVAIL_CONSTANT;
#endif
#ifndef TRAVAIL_CONSTANT
        requete.duree = c_requete.client % (rand()%20 + 10) + TRAVAIL_MINIMUM; /*Durée entre TRAVAIL_MINIMUM
                                                                                 et TRAVAIL_MINIMUM+28*/
#endif
        //La durée prend en compte le pid du client parce que chaque client est unique chez nous.
#ifdef PAS_BESOIN_OUTIL
        requete.nboutils[0] = 0;
#endif
#ifndef PAS_BESOIN_OUTIL
        requete.nboutils[0] = type_outil[0];
#endif
        for (int i = 0;i < requete.nboutils[0];i++){
            /*On choisit le nombre d'outils de chaque catégorie. (Jamais le nombre max parce que le garage
              est bien fait, sauf si il est pauvre et a moins de 2 outils dans une catégorie.)*/
            requete.nboutils[i+1] = (type_outil[i+1] > 2) ? rand()%(type_outil[i+1]-1) : rand()%(type_outil[i+1]+1);
        }
        #ifdef AFFICHAGE_OUTIL_CHEF
        fprintf(stderr,"\t<CHEF> [%d] Offre d'emploi postée pour le client %d.Besoin en outils :\n",atoi(argv[1]),c_requete.client);
        for (int i =0; i < type_outil[0];i++){
            if(requete.nboutils[i+1]!=0){
                fprintf(stderr,"\t\t%d outils de type %d.\n",requete.nboutils[i+1],i+1);
            }
        }
        #endif
        if (msgsnd(file_mess_co,&requete,sizeof(requete),0) == -1){
            fprintf(stderr,"\t<CHEF> [%d] Pas pu poster dans la file pour donner des consignes aux ouvriers.\n",atoi(argv[1]));
            kill(getppid(),SIGUSR1);
        }
       
        /*Attente de la fin du taf du mécano*/
        if (msgrcv(file_mess_co, &reponse,sizeof(reponse_t),getpid(),0) == -1){
            fprintf(stderr,"\t<CHEF> [%d] Pas pu récupérer dans la file la réponse des ouvriers.\n",atoi(argv[1]));
            kill(getppid(),SIGUSR1);
        }

        /*Information donnée au client*/
        c_reponse.type = c_requete.client;
        c_reponse.mecano = reponse.mecano;
        if (msgsnd(file_mess_cc,&c_reponse,sizeof(c_reponse_t),0) == -1){
            fprintf(stderr,"\t<CHEF> [%d] Pas pu poster dans la file pour donner le résultat au client.\n",atoi(argv[1]));
            kill(getppid(),SIGUSR1);
        }
    }
    exit(0);
}
