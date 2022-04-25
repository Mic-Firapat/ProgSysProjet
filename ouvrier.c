#include "parametrage.h"
int sema;//ID du sémaphore

int P(int sem, int n){
    struct sembuf op = {sem, -n, SEM_UNDO};
    
    return semop(sema, &op,1);
}

int V(int sem, int n){
    struct sembuf op = {sem, n, SEM_UNDO};
    
    return semop(sema, &op,1);
}

void arret(){
    fprintf(stderr,"\t<OUVRIER> [%d], réception SIGUSR1 on s'arrête\n",getpid());
    exit(-1);
}

void mon_sigaction(int s, void (*f)(int)){
    struct sigaction action;
 
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(s,&action,NULL);
}

int main(int argc, char *argv[]){

    sigset_t masque;
    key_t cle;/*cle de la file entre ouvriers/chefs*/
    int file_mess;/*ID de cette même file*/
    FILE *fich_cle;
    reponse_t reponse;
    requete_t requete;
  

    /*Ca ne devrait jamais arriver mais au cas où, erreur très grave */
    if(argc != 2){
        printf("\tPROBLEME lors de la création d'ouvrier %d, nombre d'arguments incorrect %d reçu\n",getpid(), argc);
        kill(getppid(),SIGUSR1);
    }
    
    /*Préparation du masque, seul SIGUSR1 est reconnu*/

    sigfillset(&masque);
    sigdelset(&masque,SIGUSR1);
    sigprocmask(SIG_SETMASK,&masque,NULL);

    /*On donne le traitement de SIGUSR1 à arret() */
    mon_sigaction(SIGUSR1, arret);

    /*Récupération de la clé*/
    fich_cle = fopen(FICHIER_CLE_CO,"r");
    if (fich_cle==NULL){
	fprintf(stderr,"\tLe fichier pour créer la clé n'existe pas. L'ouvrier meurt.\n");
        exit(-1);
    }
    
    cle = ftok(FICHIER_CLE_CO,'a');
    if (cle==-1){
	fprintf(stderr,"\tCréation de clé impossible ouvrier.\n");
	exit(-1);
    }
    
    /* Recuperation file de message :    */
    file_mess = msgget(cle,0);
    if (file_mess==-1){
	fprintf(stderr,"\tImpossible de récupérer la file de message\n");
	exit(-1);
    }


    /*Boucle principale de travail*/
    for(;;){
        /*Réception du message de travail*/
        if (msgrcv(file_mess,&requete,sizeof(requete_t),1,0) == -1){
            fprintf(stderr, "\t<OUVRIER> [%d] Erreur j'ai pas pu lire un message chef !\n",atoi(argv[1]));
        }
        /*Réservation des outils avec sémaphores*/
        /*Récupération du sémaphore des outils*/
        sema = semget(cle,requete.nboutils[0],0);
        if (sema == -1){
            fprintf(stderr,"\t<OUVRIER> [%d] : Je n'ai pas pu récupérer le sémaphore.\n",atoi(argv[1]));
            exit(-1);
        }
        /*Réservation des outils nécessaires*/
        for(int i = 0; i < requete.nboutils[0];i++){
            if (requete.nboutils[i+1] > 0){
                P(i,requete.nboutils[i+1]);
                #ifdef AFFICHAGE_OUTIL_OUVRIER
                fprintf(stderr,"\t<OUVRIER> [%d] a réservé %d outils de type %d.\n",atoi(argv[1]),requete.nboutils[i+1],i+1);
                #endif
            }
        }
        /*Le travail en question*/
        fprintf(stdout,"\t<OUVRIER> [%d] Au travail pour %d secondes!\n",atoi(argv[1]),requete.duree);
        sleep(requete.duree);
        /*Il rend les outils*/
        for (int i = 0; i < requete.nboutils[0];i++){
            if (requete.nboutils[i+1] > 0){
                V(i,requete.nboutils[i+1]);
            }
        }
        fprintf(stderr,"\t<OUVRIER> [%d] A rendu ses outils et fini de travailler.\n",atoi(argv[1]));

        /*Fin du travail, notification au chef d'atelier*/
        reponse.type = requete.chef;
        reponse.mecano = atoi(argv[1]);
        if (msgsnd(file_mess,&reponse,sizeof(reponse_t),0) == -1){
            fprintf(stderr, "\t<OUVRIER> [%d] Oula je peux pas dire que j'ai fini : on arrête tout!\n",atoi(argv[1]));
            kill(getppid(),SIGUSR1);
        }
    }
    exit(0);
}
