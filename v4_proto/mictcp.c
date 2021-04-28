#include <mictcp.h>
#include <api/mictcp_core.h>
#define TAILLE_BUFFER 500 //Taille du buffer

//VARIABLES GLOBALES
int PE = 0; // Prochaine Trame à Emettre
int PA = 0; // Prochaine Trame à Attendre
int sock_id = 0; //Index dans tab_sock
struct mic_tcp_sock tab_sock[TAILLE_BUFFER];
struct mic_tcp_sock_addr tab_addr[TAILLE_BUFFER];

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   if(-1 != (result = initialize_components(sm))){
    tab_sock[sock_id].fd = sock_id+1;
    sock_id++;
    set_loss_rate(0);
   }
   return result;
}

/*
 * Retourne 0 si succès, et -1 en cas d’échec
 * Permet d’attribuer une adresse à un socket.
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int R = -1;
    if (tab_sock[socket-1].fd==socket){;
        memcpy((char*)&tab_sock[socket-1].addr, (char*)&addr, sizeof(mic_tcp_sock_addr));
        R = 0;
    }
    return R;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int R = -1;
    if ( (tab_sock[socket-1].fd==socket) ){
        R = 0;
        tab_sock[socket-1].state = ESTABLISHED;
    }
    return R;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int R = -1;
    if ( (tab_sock[socket-1].fd==socket) && memcpy((char*)&tab_addr[socket-1], (char*)&addr, sizeof(mic_tcp_sock_addr)) ){
        R = 0;
        tab_sock[socket-1].state = ESTABLISHED;
    }
    return R;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    int size; //Taille du PDU envoyé

    //Construction du PDU
    mic_tcp_pdu pdu;
    mic_tcp_sock_addr addr;
    pdu.header.seq_num = PE;
    pdu.header.source_port = tab_sock[mic_sock-1].addr.port;
    pdu.header.dest_port = tab_sock[mic_sock-1].addr.port;
    pdu.payload.size = mesg_size;
    pdu.payload.data = mesg;

    //Émission du PDU
    if(-1 == (size = IP_send(pdu, addr)) ){
        printf("Erreur IP_Send\n");
    }

    //(Aucun mécanisme de reprise des pertes dans cette version)
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
    return close(socket);
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
    app_buffer_put(pdu.payload);
}
