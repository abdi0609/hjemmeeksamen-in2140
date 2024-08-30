
jeg har implementert alle funksjoner i d1_udp.c i tillegg til å ha laget to hjelpe funksjoner compute_checksum() og compute_ack_checksum()
for å finne checksum til både datapakker og ack pakker



Prosjektet består av følgende viktige funksjoner:

d1_create_client()
Denne metoden oppretter en UDP-klient ved å opprette en socket og allokere minne for en D1Peer-struktur ved hjelp av malloc(). Den returnerer en peker til den opprettede D1Peer-strukturen.

d1_delete()
Metoden d1_delete() tar en peker til en D1Peer-struktur som input, lukker socketen assosiert med peeren hvis den er åpen, frigjør minnet som er allokeret for D1Peer-strukturen, og returnerer NULL.

d1_get_peer_info()
Denne metoden henter adresseinformasjon for en spesifisert peer ved å bruke DNS-oppslag og fyller D1Peer-strukturen med denne informasjonen. Den tar en peker til en D1Peer-struktur, navnet på peeren (peername), og portnummeret til serveren (server_port) som inputparametere.

compute_checksum()
Metoden compute_checksum() beregner en sjekksum for en gitt databuffer sammen med flagg og størrelse. Den tar databufferen (data), flaggene, størrelsen på databufferen (size), og den faktiske størrelsen på dataene i bufferen (size_of_data) som inputparametere.

d1_recv_data()
Denne funksjonen mottar data fra en peer over nettverket. Den beregner den totale størrelsen på pakken som skal mottas, allokerer minne for å lagre den mottatte pakken, mottar pakken fra socketen, ekstrakterer headeren fra den mottatte pakken, kopierer databufferen fra den mottatte pakken til den angitte bufferen, beregner sjekksummen for den mottatte databufferen, sammenligner størrelsen på den mottatte pakken med størrelsen som er spesifisert i headeren, sammenligner den beregnede sjekksummen med sjekksummen i headeren, sender en bekreftelse med gjeldende sekvensnummerbit, og returnerer antall bytes mottatt i bufferen.

d1_wait_ack()
Metoden d1_wait_ack() venter på en bekreftelse (ACK) fra den spesifiserte peeren. Den venter i 1 sekund for å gi tid til at ACKen skal mottas, mottar ACK-headeren fra peeren, sjekker om den mottatte meldingen er en ACK, sjekker om ACKen har riktig sekvensnummer, og oppdaterer det neste sekvensnummeret for peeren.

d1_send_data()
Denne metoden sender data til en peer. Den beregner størrelsen på hele pakken (header + nyttelast), setter flaggene i henhold til neste sekvensnummer, beregner sjekksummen for pakken, fyller ut feltene i D1Header med riktige verdier, kopierer headeren og bufferen inn i en pakke for sending, sender pakken via socketen til den tilkoblede peeren, og venter på en bekreftelse fra peeren.

compute_ack_checksum()
Metoden compute_ack_checksum() beregner sjekksummen for en bekreftelse (ACK)-pakke. Den tar flagg og størrelse som inputparametere og returnerer den beregnede sjekksummen.

d1_send_ack()
Denne funksjonen sender en bekreftelse (ACK) til den spesifiserte peeren. Den oppretter en D1Header for ACK-pakken, bestemmer flaggene basert på sekvensnummeret, beregner sjekksummen for ACK-pakken, sender ACK-pakken til peeren, og skriver ut en bekreftelsesmelding etter at ACKen er sendt.


videre i d2_lookup.c implementerte jeg alle metodene samtidig som jeg også lagde en hjelpe metode for å rekursivt printe ut nodene i treet


d2_client_create(const char* server_name, uint16_t server_port)
Denne metoden oppretter en klient som skal kommunisere med en ekstern server. Den allokerer minne for en D2Client struktur og oppretter en D1Peer struktur ved hjelp av d1_create_client-metoden. Deretter hentes og lagres informasjon om serveren i klientstrukturen ved hjelp av d1_get_peer_info-metoden.

d2_client_delete(D2Client* client)
Denne metoden sletter en klient og frigjør ressursene som er tildelt til den. Det sørger for å slette den tilhørende D1Peer-strukturen og deretter frigjøre minnet som er tildelt til klientstrukturen.

d2_send_request(D2Client* client, uint32_t id)
Denne metoden sender en forespørsel til serveren med en angitt ID. Den oppretter en PacketRequest-struktur med forespørselstype og ID, konverterer den til nettverksbyteorden, og sender pakken til serveren ved hjelp av d1_send_data-metoden.

d2_recv_response_size(D2Client* client)
Denne metoden mottar størrelsen på responsdataene fra serveren. Den leser en PacketResponseSize-pakke fra serveren, konverterer størrelsen til vertens byteorden og returnerer den.

d2_recv_response(D2Client *client, char *buffer, size_t sz)
Denne metoden mottar responsdata fra serveren og lagrer dem i en buffer. Den mottar en PacketResponse-pakke fra serveren, ekstraherer størrelsen av responsdataene, sjekker om bufferstørrelsen er tilstrekkelig, og kopierer responsdataene til den gitte bufferen.

d2_alloc_local_tree(int num_nodes)
Denne metoden allokerer minne for lagring av lokale trededatastrukturer med et gitt antall noder. Den oppretter en LocalTreeStore struktur og tildeler minne til et array av NetNode strukturer basert på det angitte antallet noder.

d2_free_local_tree(LocalTreeStore* nodes)
Denne metoden frigjør minnet som er tildelt for lagring av lokale trededatastrukturer. Den frigjør minnet som er tildelt til arrayet av NetNode strukturer og deretter frigjør minnet til LocalTreeStore strukturen selv.

d2_add_to_local_tree(LocalTreeStore* nodes, int node_idx, char* buffer, int buflen)
Denne metoden legger til noder i den lokale trededatastrukturen basert på mottatte data. Den kopierer data fra bufferet til midlertidige variabler, beregner størrelsen av hver node, kopierer nodene til riktig plass i arrayet, og oppdaterer bufferpekeren og lengden for å hoppe til neste node.

d2_print_tree(LocalTreeStore* nodes)
Denne metoden skriver ut den lokale trededatastrukturen til konsollen. Den itererer over alle nodene i LocalTreeStore, finner rotnoden, og deretter rekursivt skriver ut alle barnenodene til roten.

jeg modifiserte også header filen d2_lookup_mod.h hvor jeg la til mulighet for å lagre netnode strukturer.