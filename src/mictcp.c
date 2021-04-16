#include <mictcp.h>
#include <api/mictcp_core.h>

/* le temps */
#include <sys/time.h>

#define TAILLE_BUFFER 500
#define TIME_OUT 50
#define SEUIL 1

 //Variable globale
int PE = 0; //Token Envoi
int PA = 0; //Token Reception

//Gestion du seuil de tolérance des pertes par un fenêtrage glissant
int Losses = 0;
int sock_id = 0;
struct timeval* restrict tp;
unsigned long tf; 
unsigned long td = 0;

struct mic_tcp_sock tab_sock[TAILLE_BUFFER];
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{ 
    printf("truc a la con\n");
   int result = -1;
   tab_sock[sock_id].fd = sock_id;
   sock_id++;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(30);
   
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int R = socket;
    
    return R;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    //Construction du PDU

        //Construction du PDU
    mic_tcp_pdu pdu;
    mic_tcp_sock_addr addr;
    int size;
    
    // Construction du header
    pdu.header.seq_num = PE; //Variable globale
    pdu.header.source_port = tab_sock[mic_sock].addr.port;
    pdu.header.dest_port = tab_sock[mic_sock].addr.port;

    //Construction du payload
    pdu.payload.size = mesg_size;
    pdu.payload.data = mesg;

    // !! Envoi du PDU
    if(-1 == (size = IP_send(pdu, addr)) ){
        printf("Erreur IP_Send\n");
    }
    mic_tcp_pdu ack;
     while(1){
        if(IP_recv(&ack, &addr, TIME_OUT)>=0){
            if(ack.header.ack_num == (PE+1)%2) {
                PE = (PE+1)%2;
            } 
        } else {
            printf("perte ack_num=%d\n",ack.header.ack_num);
            Losses++;
            gettimeofday(tp,NULL);
            tf = tp->tv_usec;
            printf("Losses : %d\n",Losses);
            if(Losses > SEUIL){
                if(tf - td < 1000000){
                    Losses = 0;
                    if(-1 == (size = IP_send(pdu, addr)) ){ //On renvoie le PDU
                        printf("Erreur IP_Send\n");
                    }
                }
                else{
                    td = tf;
                    Losses = 0;
                    
                }
            }
        }
     }
    return size;
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
    int R = 0;
    if(-1 == ( R = close(socket)) );
    return R;
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
}
