#include <mictcp.h>
#include <api/mictcp_core.h>
#include <pthread.h>

#define MAX_ENVOI 100
#define TAILLE 10

/* Variables globales */
mic_tcp_sock mysock;
mic_tcp_sock_addr addr_sock_dest;
int PE = 0;
int PA = 0;
int perte_admissible = 10; // perte acceptee pour le Stop & Wait
int tab[TAILLE] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // fenetre glissante : 1 mesg recu / 0 mesg perdu
int indice_prochain_mesg = 0;
int pourcentage_perte;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm) {

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

  /* Pourcentage de pertes sur le reseau */
  set_loss_rate(20);

  if(initialize_components(sm)!=-1) { /* Appel obligatoire */
    mysock.fd = 0;
    mysock.state = IDLE;
    return mysock.fd;
  } else {
    return -1 ;
  }
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr) {

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__);printf("\n");

  if (mysock.fd == socket) {
    memcpy((char*)&mysock.addr, (char*)&addr, sizeof(mic_tcp_sock_addr)); // ça signifie : mysock.addr=addr;
    return 0;
  } else {
    return -1;
  }
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) { // addr du sock distant

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

	if (mysock.fd == socket) {
    mysock.state = CONNECTED;
    return 0;
  } else {
    return -1;
  }
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) {

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

  if(mysock.fd == socket){
    mysock.state = CONNECTED;
    addr_sock_dest = addr;
    return 0;
  } else {
    return -1;
  }
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size) {

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

  int taille_emis;
  int nb_envoi = 0;
  mic_tcp_pdu pdu_emis;
  mic_tcp_pdu ack;
  unsigned long timeout = 100; //100 ms
  int PDU_recu = 0;
  int nb_mesg_recus = 0;
  float perte = 0;
  int i;
  int resultat = -1;

  if ((mysock.fd == mic_sock) && (mysock.state == CONNECTED)) {

    /* Construction du PDU a emettre */
    // Header
    pdu_emis.header.source_port = mysock.addr.port;
    pdu_emis.header.dest_port = addr_sock_dest.port;
    pdu_emis.header.seq_num = PE;
    pdu_emis.header.syn = 0;
    pdu_emis.header.ack = 0;
    pdu_emis.header.fin = 0;

    // DU
    pdu_emis.payload.data = mesg;
    pdu_emis.payload.size = mesg_size;

    /* Incrémentation de PE */
    PE = (PE + 1) % 2;

    /* Envoi du PDU */
    taille_emis = IP_send(pdu_emis, addr_sock_dest);
    nb_envoi ++;

    /* Attente d'un ACK */
    mysock.state = WAIT_FOR_ACK;

    /* Malloc pour l'ACK */
    ack.payload.size = 2*sizeof(short)+2*sizeof(int)+3*sizeof(char);
    ack.payload.data = malloc(ack.payload.size);

    while (!PDU_recu) {
      /* Si on a recu l'ACK pour le PDU */
      if (((resultat=IP_recv(&(ack),&addr_sock_dest, timeout)) >= 0) && (ack.header.ack == 1) && (ack.header.ack_num == PE)) { // on a recu un PDU et num==PE
        PDU_recu = 1;
        /* Mise a jour de la fenetre d'analyse des pertes */
        tab[indice_prochain_mesg] = 1;
        indice_prochain_mesg = (indice_prochain_mesg + 1) % TAILLE;
      }

      /* Si le timer a expire */
      else if (resultat < 0) {
        printf("Expiration du timer : aucun ACK recu\n");
        /* Mise a jour de la fenetre d'analyse des pertes */
        tab[indice_prochain_mesg] = 0;

        nb_mesg_recus = 0;
        /* Parcours de la fenetre glissante */
        for (i=0; i<TAILLE; i++) {
          nb_mesg_recus += tab[i];
        }

        /* Calcul du pourcentage de perte alors obtenu */
        perte = (float)(TAILLE-nb_mesg_recus)/(float)TAILLE*100.0;

        printf("Pourcentage de perte = %f\n", perte);

        /* Si proportion de perte admissible */
        /* On ne renvoie pas le PDU */
        if (perte <= perte_admissible) {
          PDU_recu = 1;

          /* Incrémentation de PE */
          /* afin que PE=PA pour le prochain PDU envoye */
          PE = (PE + 1) % 2;

          indice_prochain_mesg = (indice_prochain_mesg + 1) % TAILLE;
          printf("Perte admissible : PDU non renvoye\n");

        } else {
          /* Sinon on renvoie le PDU */
          if (nb_envoi < MAX_ENVOI) {
            taille_emis = IP_send(pdu_emis, addr_sock_dest);
            nb_envoi++;
            printf("Perte non admissible : PDU renvoye\n");
          } else {
            printf("Erreur : nombre de renvoi depasse/n");
            exit(EXIT_FAILURE);
          }
        }
      } else { // sinon ACK avec num!=PE => on reste en attente du bon ACK
        printf("ACK recu mais PE!=PA/n");
      }

    }
  } else {
    printf("Erreur au niveau du numero de socket ou connexion non etablie\n");
    exit(EXIT_FAILURE);
  }

  mysock.state = CONNECTED;
  return taille_emis;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size) { //prend dans le buffer pour le donner à l'appli

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

  int nb_octets_lus = -1;
  mic_tcp_payload pdu;

  pdu.data = mesg;
  pdu.size = max_mesg_size;

  if (mysock.fd == socket && mysock.state == CONNECTED) {

    /* Attente d'un PDU */
    mysock.state = WAIT_FOR_PDU;

    /* Recuperation d'un PDU dans le buffer de reception */
    nb_octets_lus = app_buffer_get(pdu);

    mysock.state = CONNECTED;
  }

  return nb_octets_lus;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket) {

  printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");

  return -1;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr) { //met dans le buffer
  mic_tcp_pdu ack;

  printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

  if (pdu.header.seq_num == PA) { // n°seq == PA
    /** Acceptation de DT **/
    /* Ajout de la charge utile du PDU recu dans le buffer de reception */
    app_buffer_put(pdu.payload);

    /* Incrémentation de PA */
    PA = (PA + 1) % 2;
  }
  // sinon rejet de la DT => PA reste le même

  printf("Envoi d'un ACK (ack_num = %d)\n", PA);

  /* Construction d'un ACK */
  // Header
  ack.header.source_port = mysock.addr.port;
  ack.header.dest_port = addr.port;
  ack.header.ack_num = PA;
  ack.header.syn = 0;
  ack.header.ack = 1;
  ack.header.fin = 0;

  ack.payload.size = 0; // on n'envoie pas de DU

  /* Envoi de l'ACK */
  if (IP_send(ack, addr) == -1) {
    printf("Erreur : IP_send\n");
  }
}

