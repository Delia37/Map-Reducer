Pentru inceput, mi-am definit structura thread_arg pentru a retine datele necesare executiei:
-fileInfo: numele fisierelor si ID-urile corespunzatoare
-thread_id, num_mappers, num_reducers, type: rolul si numarul fiecărui thread
-allWords: un map ce stocheaza toate cuvintele agregate de fiecare mapper.
      Cheia (string) reprezinta un cuvant,iar valoarea o lista de id-uri,
      pe care am declarat-o folosind set de numere intregi pentru a ma asigura ca elimin 
      duplicatele si numerele vor fi sortate crescator
-fileMutex: necesar pentru sincronizarea accesului la fileInfo (pentru ca un fisier sa nu 
      fie preluat simultan de mai multi mapperi)
-wordsMutex: necesar pentru ca un singur mapper ar trebui sa scrie in lista agregata
      la un moment de timp
-barieră: pentru ca toti mapperii trebuie sa-si termine executia

Am implementat o singura functie pentru threaduri, unde tratez ambele tipuri de threaduri:
Astfel, mapperii se vor ocupa de prelucrarea fisierelor de input conform cerintei.
Pentru a ma asigura ca nu exista acces concurent la acelasi fisier, am folosit un mutex pe
fileInfo (partajat intre threaduri), variabila ce retine numele si id-ul fisierelor. 
Dupa sincronizare, pot trece la prelucrarea datelor -> pe masura ce citesc cuvinte, 
ma asigur ca ele sunt valide si le adaug la listele partiale in functia process_file.
Pentru a valida cuvintele, am folosit o functie auxiliara - check, care converteste literele
mari dintr-un cuvant in litere mici si pastreaza doar cuvintele formate exclusiv din litere.
Pentru agregarea listelor partiale am definit functia merge_words, care verifica dacă un cuvant 
este deja inregistrat - daca da -> adaugă ID-ul fisierului la intrarea existenta, iar în caz contrar 
creeaza o noua intrare. Aici a fost necesar un mutex pentru a asigura faptul ca nu sunt mai multi
mapperi care lucreaza cu lista totala in acelasi timp(verificarile nu ar fi  fost corecte).

Inainte de implementarea pentru reduceri, e nevoie de o bariera care se asigura ca nu apar 
race conditions intre cele 2 tipuri de threaduri. Reducerii au nevoie ca rezultatele date de 
mapperi sa fie "complete" inainte de a incepe.
In continuare, am inceput paralelizarea pentru reduceri. Am ales sa impart impart cele 26 de
litere ale alfabetului threadurilor intr-un mod cat mai echitabil. Daca am un numar de reduceri
la care 26 nu se imparte exact, restul de caractere pana la z ii revin ultumui thread.
Toate aceste cuvinte trebuie insa grupate dupa litera cu care incep. Asadar, pentru fiecare 
litera din alfabet, am luat un vector de perechi (chWords) in care stochez cuvantul si 
fileIds unde se gaseste.
Mai departe, sortez chWords descrescator dupa numarul de fileIds in primul rand si alfabetic
in al doilea rand. Iar acum a ramas doar sa pun vectorul in fisierul de output corespunzator
literii cu care incepe.

In main:
Citesc din linia de comanda, numarul de threaduri mapper si reducer, precum si fisierul 
ce contine numele fisierelor de input. Le citesc si pe acestea si le adaug in fileInfo.
Mai departe, initializez primitivele de sincronizare si un vector de vectori ce retine 
lista totala de cuvinte gasite de mapperi, si folosind aceste argumente creez toate threadurile
odata, urmand sa le dau si join-urile aferente.
