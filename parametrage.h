#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

#ifndef TYPE_H
#define TYPE_H
/*Pour la file de message*/

/*Type utilisé pour le mécano qui veut dire à son chef que le travail est fini*/
typedef struct
{
    long type;/*Numéro du chef concerné par le travail fini*/
    int mecano; /*Numéro du mécano qui a fait le taf pour 
                  communiquer au client*/
                
} 
reponse_t;

/*Type utilisé par le chef d'atelier qui veut envoyer du taf aux mécanos*/
typedef struct
{
    long type; /*TOUJOURS 1*/
    int chef; /*PID du chef à l'origine de la demande*/
    int duree; /*sleep du mécano*/
    int nboutils[100];/*Case i indique le nombre d'outils de numéro i nécessaires*/
} 
requete_t;


/*Type utilisé par les clients qui contactent les chefs.*/
typedef struct
{
    long type; /*Identité du chef demandé par le client*/
    int client; /*Identité du client qui fait la demande*/
} 
c_requete_t;

/*Type utilisé par les chefs pour répondre aux clients*/
typedef struct
{
    long type; /*Identité du client qui avait demandé un service*/
    int mecano; /*Numéro du mécano qui a fait le taf*/
} 
c_reponse_t;

#define FICHIER_CLE_CO "cle.file.co" /*Chefs ouvriers ET sémaphore outils*/
#define FICHIER_CLE_CC "cle.file.cc" /*Chefs Clients ET sémaphore liste d'attente*/

/*PARAMETRES A CHANGER SELON VOS GOUTS. NE PAS RETIRER*/
#define MAX_SLEEP_CLIENT 15 /*temps maximal -1 entre deux clients*/
#define TRAVAIL_MINIMUM 0 /*Travail minimum pour un ouvrier*/

/*COMMENTEZ / DECOMMENTEZ POUR CHANGER L'AFFICHAGE / LE FONCTIONNEMENT*/
//Précision sur les outils utilisés
//#define AFFICHAGE_OUTIL_CHEF
#define AFFICHAGE_OUTIL_OUVRIER
//#define PAS_BESOIN_OUTIL

//Durée de travail constante pour chaque ouvrier
//#define TRAVAIL_CONSTANT 10

//Temps constant entre deux clients
//#define CLIENT_CONSTANT 5
#endif