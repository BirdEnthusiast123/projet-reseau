# Projet d'Algorithmes des Réseaux 



## Description
Ce projet entre dans le cadre de notre formation L3 d'Informatique à l'Université de Strasbourg, de l'Unité d'Enseignement: Algorithmes des Réseaux. Il est le résultat de modifications de code procuré par l'Université de Strasbourg par Raphaël TOSCANO et Florent HARDY. Il est question de créer une simulation du jeu `Light Cycles` de la franchise `Tron` en multijoueur en ligne ou local via Cloud Computing. (voir sujet: [page moodle](https://moodle.unistra.fr/mod/resource/view.php?id=703877))

## Attentes du sujet / Cahier des charges
Rendre une archive `.zip` dont le nom contiendra nos noms et prénoms. Cette archive contiendra un rapport, du code C ainsi qu'un `makefile`.
- Rapport: pdf qui servira à commenter/expliquer les choix d’implémentation lorsque nécessaire. Ainsi qu'une réponse aux questions: 
    - Discutez des avantages et inconvénients de ce genre d’architecture. En particulier, examinez l’impact des caractéristiques physiques de la connexion (délai, gigue, taux de pertes...). 
    - L’utilisation du protocole TCP est-elle souhaitable pour les jeux en ligne? Qu’en est-il du Cloud Gaming? Idéalement, quel(s) protocoles utiliseriez-vous dans le cas présent?
- Code C: 
    - gestion d'une connexion à 1 joueur⸱euse voulant jouer avec un⸱e autre joueur⸱euse en ligne;
    - gestion d'une connexion à 2 joueurs⸱euses voulant jouer sur un unique clavier;
    - gestion du serveur responsable du Cloud Computing.
- `makefile`: doit compiler du code commenté et indenté.

## Serveur
- Sujet: `Le serveur maintient l’état actuel du jeu et est chargé de calculer et mettre à jour les nouvelles positions des joueurs.`
- Usage: `./serveur [port_serveur] [refresh_rate]` où `refresh_rate` est le temps en millisecondes entre chaque envoi aux client⸱e⸱s. `refresh_rate` n'influe pas sur la gestion des inputs reçus par le serveur, envoyés par les client⸱e⸱s. (e.g. Si le/la client⸱e envoie les inputs `'z', 's' et 'd'` les uns après les autres, tous devront être pris en compte. Le dernier input en mémoire après écoulement des `refresh_rate`ms sera celui renvoyé au client).
- Doit également accepter les commandes :
    - `restart` qui permet de redémarrer;
    - `quit` qui permet de quitter.

## Client⸱e⸱s
- Usage: `./client [IP_serveur] [port_serveur] [nb_joueurs]`
- Doit surveiller les inputs claviers de la machine locale et les messages envoyés par le serveur.

## Détails d'implémentation
Structures de données à respecter présentes dans `common.h`:
- `client_init`: contient les informations nécessaires pour indiquer le nombre de joueurs derrière un même client. Elle doit être envoyée dès que la tentative de connexion d’un client a été acceptée.
- `client_input`: contient les informations envoyées par le client lorsqu’une commande est passée (parmi: `'z','q','s','d',' ','i','j','k','l','m'`).
- `display_info`: contient toutes les informations actuelles du plateau de jeu. `board` entre autres.
- `board`: fait partie de `display_info`, est un tableau de `char` tel que:
    - `0,1,2,3,4`: sont des identifiants de joueurs;
    - `50,51,52,53,54`: sont réservés pour les mûrs de lumière créés par les joueurs. Correspond à `id_joueur + 50`;
    - pas compris la partie sur les couleurs :/ on verra bien.

## Idées d'améliorations
**/!\ ATTENTION /!\ : IL FAUT GARDER UNE VERSION BASIQUE DU PROJET QUI RÉPOND UNIQUEMENT AU CAHIER DES CHARGES ET CRÉER UN DIFFÉRENT PROJET QUI SERA ENVOYÉ À PART.**

Le sujet propose des idées à implémenter pour des points bonus:
- l'un⸱e des deux joueurs⸱euses est choisi⸱e pour héberger la partie, iel s'occupe de faire les calculs;
- passer de TCP à UDP;
- permettre des parties à plus de 2 joueurs⸱euses (FFA ou par équipes);
- autre.


