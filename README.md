# REST-API-Web-Client
Implemented HTTP communication between client and REST API server

Calugaritoiu Ion-Victor

PROTOCOALE DE COMUNICAŢIE - Client Web - Comunicaţie cu REST API.

client.c: Contine logica principala a programului.
Programul primeste comenzi de la stdin, pana la intalnirea comenzii exit.
- login: sunt citite username-ul si password-ul, parsate in format json
    folosind Parson, si trimise catre server; se cauta coduri de eroare in 
    response si sunt afisate mesaje corespunzatoare; in cazul in care codul
    primit este 200 OK, se salveaza cookie-ul de sesiune si se marcheaza
    incrementand variabila logged_in_flag;

- register: asemanator comenzii "login", se parseaza username-ul si password-ul
    si sunt trimise catre server; in cazul in care exista deja un utilizator cu
    numele trimis, se afiseza mesajul de eroare corespunzator;

- enter_library: trimite catre server cookie-ul de sesiune primit la login;
    in cazul in care este valid, in response este primit un JWT, care este
    parsat si salvat;

- get_books: in mesajul catre server este inclus JWT-ul primit anterior;
    daca response-ul este valid, se parseaza lista de carti si se afiseaza;
    in cazul in care lista este goala, se afieaza un mesaj corespunzator;

- get_book: se citeste id-ul dorit de la stdin si se concateneaza la path-ul
    bibliotecii; analog comenzii get_books, se trimite catre server(inclusiv
    jwt-ul) si se verifica daca response-ul este valid; se afiseaza cartea
    dorita / mesaj de eroare daca id-ul nu este valid / cartea nu exista;

- add_book: se citesc de la stdin toate informatiile referitoare la carte si
    se valideaza datele; sunt permise litere, cifre, spatii si caractere
    speciale pentru toate campurile mai putin numarul de pagini, care trebuie
    sa fie format doar din cifre; se verifica response-ul si se afiseaza
    mesajele corespunzatoare;

- delete_book: se citeste id-ul si se concateneaza la path-ul bibliotecii;
    se trimite request-ul de DELETE catre server si se afiseaza mesajele
    corespunzatoare;

- logout: trimite cerere catre server, elibereaza memoria pentru cookie si 
    pentru jwt (daca sunt prezente);

- exit: se elibereaza memoria pentru cookie si jwt si se termina executia 
    aplicatiei;

requests.c: contine functiile de request de tip GET, POST (preluate din
    rezolvarea laboratorului, la care am adaugat implementarea pentru 
    includerea header-ului de JWT) si DELETE (implementata analog celor din
    laborator);

Fisierul parson.c ccontine biblioteca de parsare JSON.
