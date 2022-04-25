#include "parametrage.h"

int ok=1, nbca=0;/*ok pour la boucle de création des clients et nbca le nombre de chefs d'atelier*/
pid_t pid_chef[1000];/*Le tableau contenant les pids des chefs d'ateliers*/
int file_mess_CO; /*ID de la file globale pour simplifier tout*/
int file_mess_CC; /* ID de la file CC*/
int *file_attente; // Adresse attachement

// Fonction pour donner l'usage

void usage(char *s){
    fprintf(stderr,"Usage : %s <nb_chefs> <nb_mecanos> <nb_1> <nb_2> <nb_3> <nb_4> [...] <nb_i>\n\tOù <nb_chefs> est le nombre de chefs d'atelier\n\t<nb_mecanos> le nombre d'ouvrier\n\ti est le nombre de catégorie d'outils, <nb_i> le nombre d'outils de cette catégorie\nTous ces nombres sont attendus en tant qu'entier\n<nb_chefs> > 2 et <nb_mecanos> > 3. Un nombre d'outils nul n'est pas accepté (minimum 4 types d'outils).\n",s);
    exit(-1);
}


// Fonction pour envoyer des signaux aux processus crées
void envoie_signal(int n, pid_t *tpid, int signal){

    int i;

    for(i=0;i<n;i++){
        kill(tpid[i],signal);
    }

  
}

//Réception d'un signal SIGUSR1 traîtée par cette fonction
void arret_signal(){

    fprintf(stderr, "\n\n<INIT> signal de fin reçus, extermination en cours\n\n");
    ok = 0;
  
}

//Réception d'un signal SIGINT (CTRL+C) permet l'affichage de cette info.
void info_occupation(){
    int i;
    fprintf(stderr,"\n---------------------------------------\nVoici l'état d'occupation des chefs d'ateliers à cet instant :\n");
    for( i = 1; i <= nbca ; i++){
        fprintf(stderr,"Chefs d'atelier %d [%d] : occupation clients = %d\n",i,pid_chef[i-1],file_attente[i-1]);
    }
    fprintf(stderr,"---------------------------------------\n\n");
    sleep(1);
}

// Fonction sigaction
void mon_sigaction(int s, void (*f)(int)){
    struct sigaction action;
 
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(s,&action,NULL);
}

//Fonction main
int main(int argc, char *argv[]){

    /* Gestion des processus fils */
  
    int nbo=0, type_outil[100]; // Var ligne commande, #Chef atelier, #ouvrier, tableau des outils
  
    int  parcours = 0, i, statut, nb_client=0; // Aide de camps
    pid_t pid; // pid des processus crées
  
    pid_t  pid_ouvrier[1000], pid_client[1000]; // Stock des pids
    sigset_t masque; // Masque des signaux
  
    char num_ordre_chef[1024], num_ordre_ouvrier[1024], *argca[100], val_cle[1000], id_seg_mem_part[1000]; // Argument des proc fils

    /*FILE DE MESSAGE*/
    key_t cle; /*clé de la file*/
    FILE *fich_cle_CO; /* Fichier pour la clé de file CO*/
    FILE *fich_cle_CC; /* Fichier pour la clé de file CC*/
  

    /*Segment mémoire partagée des clients et infos nécessaire au choix du moins occupé */

    int seg_mem_part; // ID SMP
    int sema_file_attente; // ID SEMA
    int sema_outils;//ID SEMAPHORE pour les outils
  

    srand(time(NULL));
    /* ----- Vérification de l'appel qu programme, identification des paramètres ---- */
    if (argc < 7){
        usage(argv[0]);
    }

    /*Version 1: On considère qu'on ne peut pas commencer avec un
      type d'outils pour lequel il n'y en a pas
    */

    nbca = atoi(argv[1]);
    nbo = atoi(argv[2]);
  
    if ((nbca < 2 || nbo < 3)){
        usage(argv[0]);
    }

    type_outil[0] = argc - 3;
  
    for (i = 1; i <= type_outil[0] ; i++){
        parcours = i + 2;
        type_outil[i] = atoi(argv[parcours]);
    
        if (type_outil[i] <= 0){
            usage(argv[0]);
        }
    }

  

    /*Test paramètres programme 
      printf("nb_chefs = %d\nnb_ouvrier = %d\n",nbca, nbo);
      for(i = 1; i < type_outil[0]; i++){
      printf("nb_outil%d : %d\n",i, type_outil[i]);
      }
    */

  
    /* ---Création de la file de message --- */
    /* ---Création de la file CO---*/
    /*Création de la clé :*/
    /*Test d'existence*/
    fich_cle_CO = fopen(FICHIER_CLE_CO,"r");
    if (fich_cle_CO == NULL){
        if (errno== ENOENT){
            /*Création*/
            fich_cle_CO = fopen(FICHIER_CLE_CO,"w");
            if (fich_cle_CO == NULL){
                fprintf(stderr, "Impossible de créer le fichier pour la clé de la file.\n");
                exit(-1);
            }
        }
        else {
            fprintf(stderr, "Lancement de la file de message CO impossible");
            exit(-1);
        }
    }
    cle = ftok(FICHIER_CLE_CO, 'a');
    if (cle ==1){
        fprintf(stderr, "Impossible de créer la clé CO.\n");
        exit(-1);
    }

    /*Création file de message*/
    file_mess_CO = msgget(cle,0);
    if (file_mess_CO != -1){
        /*Suppression si elle existait déjà avant le lancement du programme
          (En cas de fermeture précédente dégueulasse)*/
        fprintf(stderr, "File CO déjà existante avant le lancement. Suppression du parasite.\n");
        msgctl(file_mess_CO,IPC_RMID,NULL);
    }
    file_mess_CO = msgget(cle,IPC_CREAT | IPC_EXCL | 0660);
    if (file_mess_CO == -1){
        fprintf(stderr, "Problème lors de la création de la file de message CO\n");
        exit(-1);
    }

    /*------------------------------------------------------------------------------------------*/
    /*---Création de la file CC---*/
    /*Création de la clé :*/
    /*Test d'existence*/
    fich_cle_CC = NULL;
    fich_cle_CC = fopen(FICHIER_CLE_CC,"r");
    if (fich_cle_CC == NULL){
        if (errno== ENOENT){
            /*Création*/
            fich_cle_CC = fopen(FICHIER_CLE_CC,"w");
            if (fich_cle_CC == NULL){
                fprintf(stderr, "Impossible de créer le fichier pour la clé de la file CC.\n");
                exit(-1);
            }
        }
        else {
            fprintf(stderr, "Lancement de la file de message CC impossible.\n");
            exit(-1);
        }
    }
    cle = ftok(FICHIER_CLE_CC, 'a');
    if (cle ==1){
        fprintf(stderr, "Impossible de créer la clé.\n");
        exit(-1);
    }

    /*Création file de message*/
    file_mess_CC = msgget(cle,0);
    if (file_mess_CC != -1){
        /*Suppression si elle existait déjà avant le lancement du programme
          (En cas de fermeture précédente dégueulasse)*/
        fprintf(stderr, "File déjà existante avant le lancement. Suppression du parasite.\n");
        msgctl(file_mess_CC,IPC_RMID,NULL);
    }
    file_mess_CC = msgget(cle,IPC_CREAT | IPC_EXCL | 0660);
    if (file_mess_CC == -1){
        fprintf(stderr, "Problème lors de la création de la file de message CC.\n");
        exit(-1);
    }



    /* --- Création du segment de mémoire partagée client / chefs--- */
    /*Pour l'instant il utilise la même clé que la file CC, ça évite de devoir créer un nouveau fichier.*/
    /*On cree le SMP */
    seg_mem_part = shmget(cle,sizeof(int),0);
    if (seg_mem_part != -1){
        fprintf(stderr, "Le SMP existait déjà pour une raison inconnue, on le supprime avant de le recréer pour éviter des soucis.\n");
        shmctl(seg_mem_part,IPC_RMID,NULL);
    }
    seg_mem_part = shmget(cle,sizeof(int), IPC_CREAT | IPC_EXCL | 0660);
    if (seg_mem_part == -1){
        fprintf(stderr,"Problème lors de la création du SMP client/chef\n");
        ok = 0;
    }

    /* Attachement de la memoire */
  
    file_attente = shmat(seg_mem_part,NULL,0);
    if (file_attente == (int *) -1){
        fprintf(stderr,"Problème lors de l'attachement de mémoire\n");
        ok = 0;
    }

    /*On cree semaphore */
    sema_file_attente = semget(cle,1,IPC_CREAT | IPC_EXCL | 0660);
    if (sema_file_attente == -1){
        fprintf(stderr,"Problème création d'un sémaphore ou déjà existant.\n");
        ok = 0;
    }

    /*On initialise le sémaphore*/
    if (semctl(sema_file_attente,0,SETVAL,1) == -1){
        fprintf(stderr, "Pas pu initialisé le sémaphore liste d'attente.\n");
        ok = 0;
    }
  
    /*on remplit le smp de la file d'attente */
    for(i=0; (i < nbca) && ok ==1; i++){
        file_attente[i] = 0;
    }

    /*Création du sémaphore pour les outils. Il utilise la même clé que CO pour pas avoir plein de fichiers.*/
    cle = ftok(FICHIER_CLE_CO, 'a');
    sema_outils = semget(cle, type_outil[0], IPC_CREAT | IPC_EXCL | 0660);
    if (sema_outils == -1){
        fprintf(stderr, "Problème lors de création du sémaphore des outils ou déjà existant.\n");
        ok = 0;
    }
    /*Initialisation de ce sémaphore*/
    for (i = 1; i <= type_outil[0]; i++){
        if (semctl(sema_outils,i-1,SETVAL,type_outil[i]) == -1){
            fprintf(stderr, "Pas pu initialisé le sémaphore d'outils numéro %d\n",i);
            ok = 0;
            break;
        }
    }
    
  
  
    /* --- Création du masque des signaux --- */

    sigfillset(&masque);
    sigdelset(&masque, SIGUSR1);
    sigdelset(&masque, SIGINT);
    sigprocmask(SIG_SETMASK,&masque,NULL);
    mon_sigaction(SIGUSR1, arret_signal);
    mon_sigaction(SIGINT, info_occupation);
  
    /* --- Création des chefs d'atelier et des mécanos  --- */

    // Création chefs atelier

    for (i = 0; i < nbca && ok == 1; i++){
        sprintf(num_ordre_chef,"%d",i+1);
        argca[0]="chefs";
        argca[1] = num_ordre_chef;
        for (int j = 1; j <= type_outil[0];j++){
            argca[j+1] = argv[2+j];
        }
        /*Il faut compléter les arguments pour les outils et tout*/
        pid = fork();
    
        if (pid == -1){ //echec creation d'un des chefs d'ateliers
            ok = 0;
            break;
        }

        if ( pid == 0){ // lancement du chef d'atelier
            execvp("chefs",argca);
            fprintf(stderr,"Probleme execution chef d'atelier %d\n",i);
            kill(getppid(),SIGUSR1);
            exit(-1);
        }
    
        pid_chef[i] = pid;
        //fprintf(stdout,"Lancement du chef d'atelier %d\n",pid_chef[i]);
    
    }


    // Création ouvriers

 	 
    for(i = 0; i < nbo && ok == 1; i++){
        sprintf(num_ordre_ouvrier,"%d",i+1);
        pid = fork();
    
        if (pid == -1){ // echec creation ouvrier
            fprintf(stderr,"Echec de création d'un ouvrier\n");
            ok = 0;
            break;
        }
    
        if ( pid == 0){ // lancement de l'ouvrier
            execlp("ouvrier","ouvrier",num_ordre_ouvrier,NULL);
            fprintf(stderr,"Probleme execution ouvrier %d\n",i);
            kill(getppid(),SIGUSR1);
            exit(-1);
        }
    
        pid_ouvrier[i] = pid;
    
        //fprintf(stdout,"Lancement de l'ouvrier %d\n",pid_ouvrier[i]);
    
    }

    if ( ok ){
        fprintf(stderr,"<INIT> Le garage est grand ouvert.\n\n");
        sprintf(val_cle,"%d",cle);
        sprintf(id_seg_mem_part,"%d",seg_mem_part);
    }
    /* --- Boucle infinie tant que le garage est ouvert --- */

    while(ok) {    
        /* --- Création des clients à rythme régulier--- */
    
        pid = fork();
    
        if (pid == -1){ // echec creation client
            fprintf(stderr,"Echec création d'un client mise à terme de l'exécution\n");
            ok = 0;
        }
    
        if ( pid == 0){ // lancement du client
            execlp("client","client",argv[1], val_cle,id_seg_mem_part,NULL);
            fprintf(stderr,"Probleme execution client %d\n",pid);
            kill(getppid(),SIGUSR1);
            exit(-1);
        }
        pid_client[nb_client] = pid;
        nb_client++;
#ifdef CLIENT_CONSTANT
        sleep(CLIENT_CONSTANT);
#endif
#ifndef CLIENT_CONSTANT
        sleep(rand() % MAX_SLEEP_CLIENT);
#endif
    }
    
    /* --- Si le programme a reçus SIGUSR1 on arrête tout --- */

    fprintf(stderr,"\n");
    envoie_signal(nbca, pid_chef, SIGUSR1);
    envoie_signal(nbo, pid_ouvrier, SIGUSR1);
    envoie_signal(nb_client,pid_client,SIGUSR1);

    /*Attente de la fin des processus */
    while((pid=waitpid(-1, &statut,0)!= -1)){
        fprintf(stderr,"<INIT> Fin d'un processus\n");
    }
    fprintf(stderr, "\n\nTous les processus ont été terminé\n");

    /*Suppression de la file*/
    fprintf(stderr, "\nSuppression de la file de message CO.\n");
    if ( msgctl(file_mess_CO,IPC_RMID,NULL) ==-1){
        fprintf(stderr,"J'ai pas réussi à fermer la file CO ouin ouin\n");
    }

    fprintf(stderr, "\nSuppression de la file de message CC.\n");
    if ( msgctl(file_mess_CC,IPC_RMID,NULL) ==-1){
        fprintf(stderr,"J'ai pas réussi à fermer la file CC ouin ouin\n");
    }

    /*Suppression segment de mémoire partagée client/chef */
    fprintf(stderr, "\nSuppression des sémaphores et du SMP\n");
  
    // Detachement
    if(file_attente != (int *) -1){
        shmdt(file_attente);
    }
    // Destruction SMP
    if ( shmctl(seg_mem_part,IPC_RMID,NULL) == -1){
        fprintf(stderr, "J'ai pas pu fermer le SMP.\n");
    }
    // Destruction semaphores
    if (semctl(sema_file_attente,1,IPC_RMID,NULL) == -1){
        fprintf(stderr, "J'ai pas pu détruire un sémaphore (liste d'attente clients).\n");
    }
    if (semctl(sema_outils,1,IPC_RMID,NULL) == -1){
        fprintf(stderr, "J'ai pas pu détruire un sémaphore (outils).\n");
    }
    fprintf(stderr,"\n\nTout a été supprimé, fin d'initial, le garage est fermé.\n");
    exit(0);
}
