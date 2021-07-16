#include<sys/types.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define MAXPRODOTTI 25

struct prodotto { 
  char nome[4096];
  int prezzo;
};
typedef struct prodotto Prodotto;

struct ristorante {
  char nome[4096];
  char tipo[4096];
};
typedef struct ristorante Ristorante;

struct ListRistoranti {
  Ristorante ristorante;
  int descriptor;
  struct ListRistoranti * nxt;
};
typedef struct ListRistoranti listRistoranti;

struct ordine {
  int quantita[MAXPRODOTTI];
};
typedef struct ordine Ordine;

struct ListReq { //Rappresenta la lista delle richieste 
  int client;
  int ristorante;
  int stato;
  struct ListReq * nxt;
  struct ListReq * prec;
};
typedef struct ListReq listReq;

void FullRead(int fd, void * buf, size_t count); //funzione per la gestione del valore di ritorno della fullRead
void FullWrite(int fd, const void * buf, size_t count); //funzione per la gestione del valore di ritorno della fullWrite
ssize_t fullRead(int fd, void * buf, size_t count);//funzione effettiva della fullRead
ssize_t fullWrite(int fd, const void * buf, size_t count);//funzione effettiva della fullWrite

void aggiungiRistoranti(listRistoranti ** head, Ristorante y, int fd);
listRistoranti * creaListRistoranti();
listRistoranti * trova(listRistoranti ** head, int pos);
void deleteRistorante(listRistoranti ** head, int fd);
void deleteHeadRistorante(listRistoranti ** head);

void aggiungiHeadReq(int client, int ristorante);
void aggiungereReq(int client, int ristorante);
listReq * creaListReq();
//trovaReq:Parametro 1:passa la testa della lista delle richieste Parametro 2: passa il socket,Parametro 3:indica se cerchiamo il descrittore del client o del ristorante,Parametro 4: stato della richiesta
listReq * trovaReq(listReq ** head, int fd, int identificativo, int stato);
//deleteReq:Parametro unico:richiesta da cancellare
void deleteReq(listReq ** punt);
void deleteHeadReq();

int nRistoranti;
int nProdotti;

listReq * headReq, * puntReq, * fineReq;

int main(int argc, char ** argv) {

  int i, sync, j, invia_prodotti, ricevi_ordine;
  int list_fd, fd_client, fd_ristorante;
  int id_client, id_rider;
  int consegna;

  Ristorante ristoranti;
  Prodotto prodotti;
  Ordine ordine;

  listRistoranti * head, * punt;
  head = creaListRistoranti();//creiamo la lista ristoranti

  headReq = creaListReq();//crea lista delle richieste
  fineReq = headReq;

  if ((list_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(0);
  }

  struct sockaddr_in serv_add;
  serv_add.sin_family = AF_INET;
  serv_add.sin_port = htons(1024);
  serv_add.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY viene usato come indirizzo del server. L'applicazione accetterà connessioni da qualsiasi indirizzo associato al server

  int enable = 1;
  if (setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, & enable, sizeof(int)) < 0) {//permette di riutilizzare lo stesso indirizzo
    perror("setsockopt");
    exit(1);
  }
  if (setsockopt(list_fd, SOL_SOCKET, SO_REUSEPORT, & enable, sizeof(int)) < 0) {//permette di riutilizzare lo stesso numero di porta
    perror("setsockopt");
    exit(1);
  }

  if (bind(list_fd, (struct sockaddr * ) & serv_add, sizeof(serv_add)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(list_fd, 1024) < 0) {
    perror("listen");
    exit(1);
  }

  fd_set rset;
  fd_set allset;
  int client[FD_SETSIZE];

  int maxd = list_fd;
  int maxi = -1;

  for (i = 1; i < FD_SETSIZE; i++)
    client[i] = -1;

  client[0] = list_fd;

  struct sockaddr_in cliaddr;//struct utilizzata nella accept
  socklen_t cliaddr_len;
  ssize_t n;

  int connd;
  int sockd;
  int ready;

  FD_ZERO( & allset);
  FD_SET(list_fd, & allset);

  while (1) {
    // setta  l’insieme  dei  descrittori  da  controllare  in  lettura
    rset = allset;

    /* 
      chiama la select: 
        - esce quando un descrittore è pronto
        - restituisce il numero di descrittori pronti
    */

    if ((ready = select(maxd + 1, & rset, NULL, NULL, NULL)) < 0)
      perror("errore nella select");

    // controlla se il socket di ascolto è leggibile 
    if (FD_ISSET(list_fd, & rset)) {
      cliaddr_len = sizeof(cliaddr);

      // invoca la accept
      if ((connd = accept(list_fd, (struct sockaddr * ) & cliaddr, & cliaddr_len)) < 0)
        perror("errore nella accept");

      // inserisce il socket di connessione in un posto libero di client

      for (i = 0; i < FD_SETSIZE; i++)

        if (client[i] < 0) {
          client[i] = connd;
          break;
        }

      // se non ci sono posti segnala errore
      if (i == FD_SETSIZE)
        perror("troppi client");

      FD_SET(connd, & allset);

      // registra il socket ed aggiorna maxd
      if (connd > maxd)
        maxd = connd;

      if (i > maxi)
        maxi = i;

      if (--ready <= 0)
        continue;
    }

    // controlla tutti i socket di ascolto se sono leggibili
    for (i = 0; i <= maxi; i++) {

      if ((sockd = client[i]) < 0)
        continue;

      if (FD_ISSET(sockd, & rset)) {
        FullRead(sockd, & sync, sizeof(int));
        switch (sync) {
	        case 0: {
	          //Riceve la struct ristorante e aggiunge il ristorante alla lista
	          FullRead(sockd, & ristoranti, sizeof(Ristorante));
	          aggiungiRistoranti( & head, ristoranti, sockd);
	          printf("Si è connesso il Ristorante: %s \n", ristoranti.nome);
	          break;
	        }
	        case 1: {
	          //invia al client le informazioni sui ristoranti
	          FullWrite(sockd, & nRistoranti, sizeof(int));
	          if (nRistoranti > 0) {
	            punt = head;
	            for (i = 0; i < nRistoranti; i++) {
	              FullWrite(sockd, & punt->ristorante, sizeof(Ristorante));
	              if (i != nRistoranti - 1)
	                punt = (listRistoranti * ) punt -> nxt;
	            }
	          }
	          printf("Sono stati inviati i nomi dei ristoranti al client.\n");
	          break;
	        }
	        case 2: {
	          //il cliente ha inviato l'indice del ristorante, il server ricerca il ristorante richiesto e lo aggiunge alla lista delle richieste
	          FullRead(sockd, & i, sizeof(i));
	          punt = trova( & head, i);

	          //crea richiesta
	          //aggiungere richiesta
	          //imposta stato case 3 
	          aggiungereReq(sockd, punt -> descriptor);//gli passiamo il canale di comunicazione del client e del ristorante

	          invia_prodotti = -1;//avvisa il ristorante di inviare i prodotti
	          FullWrite(punt -> descriptor, & invia_prodotti, sizeof(int));
	          printf("Il cliente ha selezionato il ristorante %s.\n", punt -> ristorante.nome);
	          break;

	        }
	        case 3: { //invia tutti i prodotti disponibili in un determinato ristorante al client
	          FullRead(sockd, & nProdotti, sizeof(int));
	          //trova la richiesta che sta in stato 3 e prenditi il file descriptor del client
	          puntReq = trovaReq( & headReq, sockd, 1, 3);
	          //invia il numero di prodotti 
	          FullWrite(puntReq -> client, & nProdotti, sizeof(int));

	          for (j = 0; j < nProdotti; j++) {
	            FullRead(sockd, & prodotti, sizeof(Prodotto));
	            FullWrite(puntReq -> client, & prodotti, sizeof(Prodotto));

	          }

	          puntReq -> stato = 4;
	          //imposta richiesta stato a 4
	          printf("Prodotti del ristorante mostrati al client.\n");
	          break;
	        }
	        case 4: { //riceve l'ordine dal client e lo invia al ristorante

	          FullRead(sockd, & ordine, sizeof(Ordine));
	          // trova req con stato 4 e prendi il file descriptor del ristorante 
	          puntReq = trovaReq( & headReq, sockd, 0, 4);

	          //inviare -2 prima per avvisare il ristorante che sta arrivando l'ordine

	          ricevi_ordine = -2;
	          FullWrite(puntReq -> ristorante, & ricevi_ordine, sizeof(int));
	          FullWrite(puntReq -> ristorante, & ordine, sizeof(Ordine));
	          puntReq -> stato = 5;
	          //imposta stato a 5
	          printf("Il client ha effettuato l'ordinazione ed è stata inviata al ristorante.\n");
	          break;
	        }
	        case 5: { //gestione della consegna dell'ordine 
	          FullRead(sockd, & id_rider, sizeof(int));
	          //trova req stato 5 e prendi il client
	          puntReq = trovaReq( & headReq, sockd, 1, 5);
	          FullWrite(puntReq -> client, & id_rider, sizeof(int));
	          FullRead(puntReq -> client, & id_client, sizeof(int));
	          FullWrite(sockd, & id_client, sizeof(int));
	          FullRead(sockd, & consegna, sizeof(int));
	          printf("Consegna effettuata dal rider: %d per il cliente %d.\n", id_rider, id_client);
	          deleteReq( & puntReq); //elimina req
	          break;
	        }
	        case 6: {
	          //il client e' uscito o ha killato il processo
	          if (close(sockd) == -1)
	            perror("Errore nella close");
	          FD_CLR(sockd, & allset);
	          client[i] = -1;
	          break;
	        }
	        case 7: {
	          //il ristorante ha killato il processo
	          if (close(sockd) == -1)
	            perror("Errore nella close");
	          FD_CLR(sockd, & allset);
	          client[i] = -1;
	          deleteRistorante( & head, sockd);
	          break;
	        }

	        default:
	          break;
	        }

        // se la coda finisce chiudi
        sync = -1;
        if (--ready <= 0)
          break;
      }
    }
  }

} //fine main

ssize_t fullRead(int fd, void * buf, size_t count) {
  size_t nleft;
  ssize_t nread;
  nleft = count;
  while (nleft > 0) {
    if ((nread = read(fd, buf, nleft)) < 0) {

      if (errno == EINTR)
        continue;
      else
        exit(nread);
    } else if (nread == 0) {
      break;
    } else {
      nleft -= nread;
      buf += nread;
    }
  }
  buf = 0;
  return nleft;
}

ssize_t fullWrite(int fd,
  const void * buf, size_t count) {
  size_t nleft;
  ssize_t nwritten;
  nleft = count;
  while (nleft > 0) {
    /* repeat until no left */
    if ((nwritten = write(fd, buf, nleft)) < 0) {
      if (errno == EINTR)
        continue;
      else exit(nwritten);

    }

    nleft -= nwritten;
    buf += nwritten;
  }
  return nleft;
}

void FullRead(int fd, void * buf, size_t count) {
  if ((fullRead(fd, buf, count)) == -1)
    perror("Errore nella read.\n");
}

void FullWrite(int fd,
  const void * buf, size_t count) {
  if ((fullWrite(fd, buf, count)) == -1)
    perror("Errore nella write.\n");
}

listRistoranti * creaListRistoranti() {
  listRistoranti * x;
  x = NULL;
  nRistoranti = 0;
  return x;
}

void aggiungiRistoranti(listRistoranti ** head, Ristorante y, int fd) {
  listRistoranti * x;
  x = malloc(sizeof(listRistoranti));
  strcpy(x -> ristorante.nome, y.nome);
  strcpy(x -> ristorante.tipo, y.tipo);

  x -> descriptor = fd;
  x -> nxt = (listRistoranti * )( * head);
  * head = x;
  nRistoranti++;
}

listRistoranti * trova(listRistoranti ** head, int pos) {
  listRistoranti * visita;
  visita = * head;
  int i = 0;
  while (i < pos - 1) {
    visita = (listRistoranti * ) visita -> nxt;
    i++;
  }
  return visita;

}

void deleteRistorante(listRistoranti ** head, int fd) {
  listRistoranti * visita, * prec;
  visita = * head;

  if (visita -> descriptor == fd)
    deleteHeadRistorante(head);

  else {
    while ((visita -> nxt) != NULL) {
      if (visita -> descriptor == fd)
        break;
      prec = visita;
      visita = (listRistoranti * ) visita -> nxt;
    }
    printf("Il ristorante %s non è piu' disponibile.\n", ( * head) -> ristorante.nome);
    prec -> nxt = visita -> nxt;
  }
  nRistoranti--;
}

void deleteHeadRistorante(listRistoranti ** head) {
  listRistoranti * pt;
  pt = (listRistoranti * )( * head) -> nxt;
  printf("Il ristorante %s non e` piu' disponibile.\n", ( * head) -> ristorante.nome);
  free( * head);
  * head = pt;
}

void deleteReq(listReq ** punt) {

  if (( * punt) == headReq)
    deleteHeadReq();

  else {
    ( * punt) -> prec -> nxt = ( * punt) -> nxt;
    ( * punt) -> nxt -> prec = ( * punt) -> prec;
    free(punt);
  }

}

void deleteHeadReq() {
  listReq * pt;
  pt = (headReq) -> nxt;
  free(headReq);
  headReq = pt;
}

void aggiungereReq(int client, int ristorante) {
  if (headReq == NULL)
    aggiungiHeadReq(client, ristorante);
  else {
    listReq * c;
    c = malloc(sizeof(listReq));
    c -> client = client;
    c -> ristorante = ristorante;
    c -> stato = 3;
    fineReq -> nxt = c;

    c -> nxt = NULL;
    c -> prec = fineReq;
    fineReq = c;
  }
}

void aggiungiHeadReq(int client, int ristorante) {
  listReq * x;
  x = malloc(sizeof(listReq));
  x -> client = client;
  x -> ristorante = ristorante;
  x -> stato = 3;
  x -> nxt = headReq;
  x -> prec = NULL;
  headReq = x;
  fineReq = headReq;
}

listReq * creaListReq() {
  listReq * x;
  x = NULL;
  return (listReq * ) x;
}

listReq * trovaReq(listReq ** head, int fd, int identificativo, int stato) { //l'identificativo serve per indicare di chi e' fd, stato indica lo stato dell'ordine
  listReq * visita;
  visita = * head;

  while (visita != NULL) {
    if (identificativo == 1) {
      if (visita -> ristorante == fd && visita -> stato == stato)
        return visita;

    } else {
      if (visita -> client == fd && visita -> stato == stato)
        return visita;

    }

    visita = visita -> nxt;

  }

}