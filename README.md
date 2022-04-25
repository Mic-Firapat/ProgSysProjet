# ProgSysProjet
University project about system programmation. The goal was to create 4 types of process, one (initial) that launches semaphores, other process and shared memory spaces. One (client) that asks for a task to be performed using certain tools (the task simply being a "sleep"). One (ouvrier) that awaits orders from the chefs to perform the tasks and the last one (chefs) begin a process that makes the link between the clients and the ouvriers.
This project was done in 2 days for university purposes by 2 persons.

Next is the subject given to the students by our teacher (in French), here's a link to his personal website used for the classes : https://perso.univ-st-etienne.fr/ezequel/.
# The project (in French)

 On se propose de simuler l'activité d'un garage. Dans ce garage, deux catégories d'employés, des chefs d'ateliers et des mécaniciens, s'activent pour satisfaire les requêtes de clients (donc, d'ores et déjà, il y aura au moins trois programmes à écrire ...). Plus précisément :

    chaque client contacte le chef d'atelier le moins occupé (comme dans la vraie vie), et attend une réponse de sa part.
    Les chefs d'ateliers sont chargés de faire le lien entre les demandes des clients et les mécaniciens: à la réception d'un client, un chef d'atelier poste une demande de travail aux mécaniciens; lorsqu'il reçoit une réponse des mécaniciens, il la répercute au client concerné.
    Les mécaniciens sont chargés du travail. Ils récupèrent les demandes de travaux des chefs d'atelier, exécutent les travaux, et préviennent les chefs d'ateliers lorsque le travail est terminé.

L'application sera composée d'un processus initial qui va se charger de créer et d'initialiser les IPC nécessaires à l'application, et d'un certain nombre de processus fils, décrits ci-dessous.

Règles du jeu, à lire attentivement

    Cet énoncé a été rendu disponible jeudi 16 décembre 2021 à 8 h. Vous devrez réaliser l'application, en binome ou monome uniquement, et rendre votre travail par messagerie au plus tard vendredi 17 décembre 2021 à 23h59 (heure du serveur de messagerie utilisé faisant foi).

    Tout travail rendu en $n$-nome, avec $n\geq 3$, vaudra 0/20.

    Vous serez noté sur le projet lui-même ainsi que sur une soutenance, prévue vendredi 7 janvier 2021.

    J'attire votre attention sur les points suivants, dont la non-observance sera éliminatoire :
        la compilation de votre programme doit se faire en silence avec l'option -Wall de gcc;
        l'exécution ne doit pas générer d'erreur (en particulier pas d'erreur de segmentation).

    Un critère essentiel de la note sera l'adéquation à la norme POSIX pour la partie système, et la qualité de la programmation pour la partie C.

    Rendre le projet va consister à
        faire un tar (voir man tar) du répertoire contenant le projet. Le nom du répertoire doit être constitué du ou des noms des auteurs (vous serez pénalisés si ce n'est pas le cas);
        l'envoyer comme fichier attaché à ezequel(at)univ-st-etienne.fr (inutile de le compresser, vu la petite taille des fichiers demandés);
        avec comme sujet impérativement exam-unix-2021 (vous serez pénalisés si ce n'est pas le cas).

    Le répertoire doit contenir 1, 3 ou 4 programmes C (voir plus bas) ainsi qu'un Makefile en ordre de marche, avec une cible clean ou nettoyer.

Le processus initial
C'est lui qui est chargé de lancer et d'arrêter proprement l'application. Ses spécifications sont les suivantes :

    il prend 6 entiers en argument sur la ligne de commande :

    initial nb_chefs nb_mecanos nb_1 nb_2 nb_3 nb_4

    où
        nb_chefs (resp. nb_mecanos) représente le nombre de chefs d'atelier (resp. mécaniciens) à créer et à lancer; nb_chefs (resp. nb_mecanos) vaudra au moins 2 (resp. 3);
        pour $i=1,2,3,4$, nb_i représente le nombre d'outils de catégorie $i$ à la disposition des mécaniciens.
    il doit créer et initialiser les IPC nécessaires à l'application (ensembles de sémaphores, files de message);
    il doit créer et lancer les chefs d'atelier et les mécanos;
    c'est lui qui crée les clients : indéfiniment, il crée et lance un client (prévoir une temporisation entre le lancement de chaque client, pour ne pas trop saturer le système);
    à la réception du signal SIGUSR1, il doit terminer les chefs d'atelier et les mécaniciens (ainsi que les clients encore en attente), puis supprimer les IPC avant de se terminer : ceci constitue l'unique façon d'arrêter l'application.

Le garage

L'application est donc constituée (entre autres) de processus « clients », de processus « chefs » et d'un certain nombre de processus « mécanos ».

Les chefs d'atelier reçoivent les requêtes des clients et leur répondent.

Chaque mécanicien, indéfiniment, reçoit une demande en provenance d'un des chefs d'atelier, exécute la tâche demandée, prévient le chef d'atelier à la source de la demande de l'achèvement de la tâche, et recommence. Il faudra donc un instrument de communication entre les chefs et les mécaniciens : vous devrez utiliser une IPC de type file de messages pour le dialogue entre les chefs d'atelier et les mécaniciens.

Voyons maintenant les spécifications précises des 3 types de processus.

Les mécaniciens
Chaque mécanicien, lancé par le processus initial à l'exclusion de toute autre méthode, reçoit 1 paramètre, son numéro d'ordre.

Après récupération des IPC utilisées (sémaphores et file de message), il doit exécuter indéfiniment la boucle suivante :

    réception du travail à faire, par relevage d'un message dans la file de messages. Un travail est déterminé par une durée, et le nombre d'outils de chacune des 4 catégories nécessaire pour son exécution;
    réservation des outils (au moyen d'un ensemble de sémaphores);
    exécution du travail (en fait un appel à la fonction sleep, voir le man à ce sujet);
    notification au chef d'atelier à l'origine de la demande de la fin du traitement de la requête, par envoi d'un message de type judicieusement choisi dans la file de messages (il pourra être nécessaire d'ajouter des informations à ce message).

NB1 : en aucun cas le nombre de paramètres du mécanicien ne doit être différent de 1

NB2 : vous aurez à résoudre en particulier le problème de l'identité du chef d'atelier à l'origine d'une requête

NB3 : en aucun cas un mécanicien ne peut commencer à travailler s'il n'a pas obtenu tous les outils nécessaires

L'arrêt d'un mécanicien est assuré par le captage par celui-ci du signal SIGUSR1 : on veillera donc, dans la partie «initialisations» des mécaniciens, à positionner une fonction de capture de ce signal.

Les chefs d'atelier
Chaque chef d'atelier, lancé par le processus initial à l'exclusion de toute autre méthode, reçoit 5 paramètres, son numéro d'ordre, et le nombre d'outils de chaque catégorie disponible (ce sont les nb_i du processus initial).

Après récupération des IPC qu'il utilise, il doit exécuter indéfiniment la boucle suivante :

    attente d'un client, par un moyen à votre convenance;
    envoi aux mécaniciens du travail à faire, par postage dans la file de message: le chef d'atelier détermine la durée et le nombre d'outils de chaque catégorie dont aura besoin le mécanicien qui fera le travail;
    attente de la réalisation du travail: le chef d'atelier est prévenu par la réception d'un message de type judicieusement choisi;
    notification au client du résultat de la requête, par un moyen à votre convenance;

L'arrêt d'un chef d'atelier est assuré par le captage par celui-ci du signal SIGUSR1 : on veillera donc, dans la partie «initialisations», à positionner une fonction de capture de ce signal.

Les clients
Chaque client, lancé par le processus initial à l'exclusion de toute autre méthode, reçoit en paramètre

    le nombre de chefs d'atelier,
    la ou les clés qui ont servi à fabriquer les IPC nécessaires à sa communication avec les chefs d'atelier.

Après récupération des IPC qu'il utilise, il doit

    se mettre en attente du chef d'atelier le moins occupé;
    transmettre une demande à ce chef d'atelier (par un moyen à votre convenance);
    se mettre en attente de la réponse du chef d'atelier (par un moyen à votre convenance);
    afficher la requête et son résultat.
