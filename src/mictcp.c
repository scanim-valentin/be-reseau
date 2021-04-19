#include <mictcp.h>
#include <api/mictcp_core.h>

/* le temps */
#include <sys/time.h>

#define LOSS_RATE 0 //Loss rate used in set_loss_rate
#define TAILLE_BUFFER 500 //Size of the message buffer
#define TIME_OUT 50 //Time out for IP_recv

#define SEUIL_NB_PERTES 5 //Seuil de tolérance de nombre de perte (on doi avoir Losses < SEUIL_NB_PERTES)

#define SEUIL_TEMPOREL 500000 // (microseconde) Seuil de tolérance de l'écart de temps entre 2 pertes (on doit avoir delta > SEUIL_TEMPOREL)

 //Variable globale
int PE = 0; //Token Envoi
int PA = 0; //Token Reception

//Gestion du seuil de tolérance des pertes
int Losses = 0;
int sock_id = 0;
unsigned long tf, td;
struct timeval* restrict tp;


struct mic_tcp_sock tab_sock[TAILLE_BUFFER];
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{ 
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   tp = (struct timeval*)malloc(sizeof(struct timeval));
   gettimeofday(tp,NULL);
   int result = -1;
   tab_sock[sock_id].fd = sock_id;
   sock_id++;
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
    int i = 0;
    printf("L0\n");
     while(1){
        printf("i = %d\n",i);
        printf("L1\n");
        if(IP_recv(&ack, &addr, TIME_OUT)>=0){
            printf("L2\n");
            if(ack.header.ack_num == (PE+1)%2) {
                printf("L3\n");
                PE = (PE+1)%2;
            } 
        printf("L4\n");
        } else {
            printf("L5\n");
            printf("perte ack_num=%d\n",ack.header.ack_num);
            Losses++;
            if(gettimeofday(tp,NULL)){
                printf("Echec getitmeofday\n");
            }
            printf("L6.0.0\n");
            printf("usec = %lu\n",tp->tv_usec);
            printf("L6.0.1\n");
            tf = tp->tv_usec;
            printf("L6.1\n");
            printf("Losses : %d\n",Losses);
            printf("L6.2\n");
            if(Losses > SEUIL_NB_PERTES){
                printf("L7\n");
                printf("/_\\ = %lu\n",tf - td);
                if(tf - td < SEUIL_TEMPOREL){
                    printf("L8\n");
                    Losses = 0;
                    if(-1 == (size = IP_send(pdu, addr)) ){ //On renvoie le PDU
                        printf("L9\n");
                        printf("Erreur IP_Send\n");
                    }
                }
                else{
                    printf("L10\n");
                    td = tf;
                    Losses = 0;
                    
                }
            }
        }
     }
    printf("L11\n");
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
    free(tp);
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

    //PA = 0;

    mic_tcp_pdu ack;
    ack.header.ack = 1;
    ack.payload.size = 0;

    if(pdu.header.seq_num == PA) {

        app_buffer_put(pdu.payload);
        PA = (PA+1)%2; //PA = 1
    }
    ack.header.ack_num = PA;
    printf("succes seq_num=%d\n",pdu.header.seq_num);
    //printf("PA=%d\n",PA);
    if(IP_send(ack, addr) < 0) {
        //erreur send
        printf("erreur send\n");
        exit(-1);
    }
}
