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

//Fonction qui gère la réception d'un signal USR1
void arret(){
    fprintf(stderr,"\t\t<CLIENT> [%d], réception SIGUSR1 on s'arrête\n",getpid());
  
    exit(-1);
}

void mon_sigaction(int s, void (*f)(int)){
    struct sigaction action;
 
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(s,&action,NULL);
}

//Fonction main (quelle surprise !)
int main(int argc, char *argv[]){
    sigset_t masque;
  
    key_t cle; // cle IPC argv[2]
    int seg_mem_part; // ID SMP argv[3]
    int *adr; //adresse smp
    FILE *fich_cle = NULL;//Pour la file CC
    int file_mess_cc;//ID de la file CC
    long contact;
    int i;
    c_requete_t c_requete; /*demande du client au chef*/
    c_reponse_t c_reponse; /*réponse du chef*/

    /*Ca ne devrait jamais arriver mais au cas où, erreur très grave */
    if (argc < 4){
        fprintf(stderr, "\t\tPROBLEMES lors de la création du client %d, nombre d'arguments incorrect %d reçu\n",getpid(), argc);
        kill(getppid(),SIGUSR1);
    }
    // Recup arguments
    cle = atoi(argv[2]);
    seg_mem_part = atoi(argv[3]);
  
    if (cle <= 0 || (seg_mem_part == -1)){
        fprintf(stderr,"\t\tErreur récup clef client ou ID SMP fin programme\n");
        kill(getppid(),SIGUSR1);
    }  
  
  
    /*Préparation du masque, seul SIGUSR1 est reconnu*/

    sigfillset(&masque);
    sigdelset(&masque,SIGUSR1);
    sigprocmask(SIG_SETMASK,&masque,NULL);
    

    /*On donne le traitement de SIGUSR1 à arret() */
    mon_sigaction(SIGUSR1, arret);

    /*Récupération du semaphore */

    //Attachement du SMP:
    adr = shmat(seg_mem_part,NULL,0);
    if ( adr == (int *)-1){
        fprintf(stderr,"<CLIENT> [%d] : Je n'ai pas réussi à attacher le sémaphore\n",getpid());
        kill(getppid(),SIGUSR1);
    }

    /*Recuperation semaphore */
    sema = semget(cle,1,0);
    if ( sema == -1) {
        fprintf(stderr,"\t\t<CLIENT> [%d] : Je n'ai pu récupérer le sémaphore",getpid());
        kill(getppid(),SIGUSR1);
    }

    /*---FILE DE MESSAGE CC---*/
    /*Récupération de la clé*/
    fich_cle = NULL;
    fich_cle = fopen(FICHIER_CLE_CC,"r");
    if (fich_cle==NULL){
	fprintf(stderr,"\t\tLe fichier pour créer la clé CC n'existe pas. Le chef meurt.\n");
        exit(-1);
    }
    
    cle = ftok(FICHIER_CLE_CC,'a');
    if (cle==-1){
	fprintf(stderr,"\t\tCréation de clé impossible chef.\n");
	exit(-1);
    }

    /* Recuperation file de message :    */
    file_mess_cc = msgget(cle,0);
    if (file_mess_cc==-1){
	fprintf(stderr,"\t\tImpossible de récupérer la file de message\n");
	exit(-1);
    }


    /*Attente semaphore*/
    P(0,1);
    /*Cherche le chef le moins occupé*/
    //fprintf(stderr,"<CLIENT> [%d] : Recherche du chef le moins occupé\n",getpid());
    contact = 1;
    for (i = 1 ; i < atoi(argv[1]); i ++){
        if (adr[contact-1] > adr[i]){
            contact = i+1;
        }
    }
    /*Contact est le numéro d'ordre du chef le moins occupé */ 
    fprintf(stderr,"\t\t<CLIENT> [%d] contacte le Chef %ld.\n",getpid(),contact);
    /*Incrémente le nombre de clients en attente de ce chef*/
    adr[contact-1] = adr[contact-1] + 1;
    /*Envoie son message au chef*/
    c_requete.type = contact;
    c_requete.client = getpid();
    if (msgsnd(file_mess_cc, &c_requete, sizeof(c_requete_t),0) == -1){
        fprintf(stderr, "\t\t<CLIENT> [%d] N'a pas pu faire sa demande.\n",getpid());
        exit(-1);
    }
    /*Libère le sémaphore*/
    V(0,1);

    /*Attente du message du chef*/
    if (msgrcv(file_mess_cc,&c_reponse,sizeof(c_reponse_t),getpid(),0) == -1){
         fprintf(stderr, "\t\t<CLIENT> [%d] N'a pas pu lire sa réponse.\n",getpid());
         exit(-1);
    }

    fprintf(stderr,"\t\t<CLIENT> [%d] J'ai reçu ma réponse du chef %ld.\n\t\tLe mécano %d a fait le travail.\n",getpid(),contact,c_reponse.mecano);


    /*Demande le sémaphore pour partir du garage*/
    P(0,1);
    adr[contact-1] = adr[contact-1] - 1;
    V(0,1);

    /*Detachement SMP*/
    shmdt(adr);
    /*Fin normale*/
    exit(0);
  
}
