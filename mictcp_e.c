#include <mictcp.h>
#include <api/mictcp_core.h>

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */

struct mic_tcp_sock tab_sock[100];
int i = 0;
unsigned long timeout = 50;
unsigned int PE = 0;
unsigned int PA = 0;

int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);

   tab_sock[i].fd = i;
   //tab_sock[i].protocol_state = "CONNECTED";
   i++;

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   tab_sock[socket].addr = addr;   

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
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); 
    printf("\n");
    
    //unsigned long t1 = get_now_time_usec();
    //unsigned long t2;
    //unsigned int PE = 0;

    mic_tcp_pdu pdu;
    mic_tcp_sock_addr addr;
    int size;
    
    pdu.header.seq_num = PE;
    pdu.header.source_port = tab_sock[mic_sock].addr.port;
    pdu.header.dest_port = tab_sock[mic_sock].addr.port;

    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;
    size = IP_send(pdu, addr);  //renvoi la taille ou -1 si erreur
    //pp_buffer_put(pdu.payload); // stockage de DT dans les buffers
    //printf("debug8\n");
    //PE = (PE+1)%2;
    
    mic_tcp_pdu ack;
    //mic_tcp_sock_addr addr;
    int recvResult;

    while(1){
        //printf("PE=%d\n",PE);
        recvResult = IP_recv(&ack, &addr, timeout);
        printf("recv=%d\n",recvResult);
        if(recvResult>=0){
            //if(ack.header.ack_num == PE) {
            if(ack.header.ack_num == (PE+1)%2) {
                // stop timer et libération du buffer de DT(M)
                //app_buffer_get(pdu.payload); //libération du buffer
                PE = (PE+1)%2;
                return size;
            } 
        } else {
            printf("perte ack_num=%d\n",ack.header.ack_num);
            IP_send(pdu, addr) ; //renvoi du PDU de données
        }
    }

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

    int effective_size;
    effective_size = app_buffer_get(payload);

    return effective_size;

}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return -1;
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

