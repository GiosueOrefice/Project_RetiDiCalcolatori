# Project_RetiDiCalcolatori

## Laboratorio di Reti di Calcolatori

A.A. 2020 - 2021

Food Delivery

## Descrizione

Progettare ed implementare un servizio di food delivery secondo le seguenti speciche. Il client si collega al server da cui riceve la lista dei ristoranti.
L'utente, usando l'interfaccia del client, sceglie il ristorante. Il server richiede al ristorante scelto una lista di cibi e bevande ordinabili e la invia al clientda cui l'utente effettua l'ordine. 
Una volta effettuato l'ordine, il server lo inoltra al ristorante che verifica la disponibilita di rider inviando la richiesta
ai rider connessi e selezionando il primo da cui riceve la conferma. Ricevuta la conferma, il server invia l'id del rider al client e l'id del client al rider.
Una volta effettuata la consegna il rider invia una notifica al ristorante che
a sua volta la inoltra al server. Tutte le comunicazioni avvengono tramite il server.

## Architettura 
<img src=

Si utilizzi il linguaggio C, Java o altro linguaggio su piattaforma UNIX utilizzando i socket per la comunicazione tra processi.
