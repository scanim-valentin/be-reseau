#include <mictcp.h>
#include <api/mictcp_core.h>
#define TAILLE_BUFFER 500 //Taille du buffer
#define TIME_OUT 50 //Timer de réception de l'ack (IP_recv)
#define RETRY_LIMIT 100 //Limite le nombre de nouvelles tentatives d'envoi lors de l'échec de réception d'un ack
#define LOSS_RATE 30


//VARIABLES GLOBALES
int PE = 0; // Prochaine Trame à Emettre
int PA = 0; // Prochaine Trame à Attendre

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(LOSS_RATE);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    int size; //Taille du PDU envoyé
    int retry_nb = 0; //Nombre de tentative d'envoi d'un PDU

    //Construction du PDU
    mic_tcp_pdu pdu;
    mic_tcp_sock_addr addr;
    pdu.header.seq_num = PE;
    //pdu.header.source_port = 666;
    //pdu.header.dest_port = 666;
    pdu.payload.size = mesg_size;
    pdu.payload.data = mesg;
    mic_tcp_pdu ack; //PDU d'acquittement
    ack.header.ack_num = -1;
    char next_frame = 0; //Variable de sortie de la boucle
    
    //Émission du PDU
    if(-1 == (size = IP_send(pdu, addr)) ){
        printf("Erreur IP_Send\n");
    }

    while(!next_frame){
        printf("(*) ack.header.ack_num = %d ;  ack.header.ack  = %d;  PE = %d\n",ack.header.ack_num, ack.header.ack,PE);
        if( IP_recv(&ack, &addr, TIME_OUT) < 0 || ack.header.ack != 1 || ack.header.ack_num != (PE+1)%2 ){
            printf("(**) ack.header.ack_num = %d ;  ack.header.ack  = %d;  PE = %d\n",ack.header.ack_num, ack.header.ack,PE);
        //Echec dans la réception de l'ack : on renvoi le PDU
            printf("Echec dans la réception de l'ack : on renvoi le PDU\n");
            if( (retry_nb++) >= RETRY_LIMIT ){ //Dépassement de la limite de tentatives
                printf("Depassement du nombre de tentatives d'envoi autorisé (%d) pour un PDU\n",RETRY_LIMIT);
                exit(-1);
            }
            printf("[retry_nb = %d / %d] - Reemission du PDU\n",retry_nb,RETRY_LIMIT);
            if(-1 == (size = IP_send(pdu, addr)) ){
                printf("Erreur IP_Send\n");
                exit(-1);
            }

        }else{
            PE = (PE+1)%2;
            next_frame = 1;
            printf("Passage à l'attente du prochain PDU\n");
        }

    }
    return 0;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload payload;
    payload.data = mesg;
    payload.size = max_mesg_size;

    return app_buffer_get(payload);
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    mic_tcp_pdu ack;
    ack.header.ack = 1;
    ack.payload.size = 0;

    if(pdu.header.seq_num == PA) {
        app_buffer_put(pdu.payload);
        PA = (PA+1)%2;
    }

    ack.header.ack_num = PA;

    printf("succes seq_num=%d\n",pdu.header.seq_num);

    //printf("PA=%d\n",PA);
    printf("Emission du PDU ack\n");
    if(IP_send(ack, addr) < 0) {
        //erreur send
        printf("erreur send\n");
        exit(-1);
    }
}
