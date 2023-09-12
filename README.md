# Projet Compilateur (Hexanôme H4221)

Conception d’un compilateur pour un sous-ensemble du langage C. Le compilateur
sera écrit en C++ et utilisera ANTLR4

## Lancement

### Compilation du projet à executer depuis le dossier **/compiler/**:   
``` ./runmake_ubuntu.sh```


### Lancement des tests depuis le dossier **/tests/**:   

``` python3 ifcc-test.py testfiles/```

Lors du développement de notre compilateur, nous avons suivi les grandes étapes du sujet. Ainsi, notre nomenclature de test est la suivante: ```<étape_sujet>_<infos_sur_test>_<id_à_2_chiffres>```

Pour n'excuter que certains tests, on peut utiliser ```*```. Par exemple, pour n'exécuter que les tests de la partie 4.9:

``` python3 ifcc-test.py testfiles/4_9*```

### Execution d'un fichier test depuis le dossier **/compiler/**:   

``` ./ifcc ../tests/testfiles/<fichier_de_test>```

exemple:``` ./ifcc ../tests/testfiles/4_03_global_01.c```


### Obtenir l'arbre graphique d'un fichier test depuis le dossier **/compiler/**:

``` ./runmake_ubuntu.sh gui FILE="../tests/testfiles/<fichier_de_test>"```

exemple:```./runmake_ubuntu.sh gui FILE="../tests/testfiles/4_03_global_01.c"```

### Les fonctionnalités implémentées
Avant de présenter les fonctionnalités du compilateur, commençons par en présenter ses limites :
* Il ne fonctionne qu'avec des variables entières (char n'est pas implémenté)
* Les variables globales ne sont pas implémentées
* Une fonction a forcément un type de retour entier (pas de gestion des void ou des char)
* Un programme est forcément composé d'au moins une fonction (notre compilateur ne rejète pas un programme sans fonction main, il rejettera un programme vide)
* Les boucles for ne sont pas implémentées
* La gestion des retours partout n'est pas gérée
* Les opérations logiques bit à bit ne sont pas gérées

Maintenant, voilà la liste de toutes les fonctionnalités implémentées :
*  Un seul fichier source sans pré-processing
* Les commentaires sont ignorées
* Type de données : uniquement les entiers
* Variables
* Constantes entières
* Opérations arithmétiques de base: +, -, *
* Opérations unaires: ! et -
* Déclaration de variables n'importe où (tant qu'elle ne sont pas globales !)
* Affectation
* Possibilité d'initialiser une variable lors de sa déclaration
* Utilisation des fonctions standard ```putchar``` et ```getchar``` pour les entrées/sorties
* Définition de fonctions avec forcément le retour d'un entier (les autres cas ne sont pas gérées, s'il n'y a pas de retour dans la fonction non plus)
* Structures de blocs grâce à { et }
* Support des portées de variables et du shadowing
* Les structures de contrôles (if, else et while)
* Vérification qu'une variable utilisée dans une expression a été déclarée
* Vérification qu'une variable n'est pas déclarée plusieurs fois
* Division (pas le modulo)
* Appel de fonction (avec 6 entiers en paramètres au maximum, pas de passage de paramètres sur la pile)

Il est possible que dans certains messages d'erreur sur les redéclarations / utilisation de variables non-initialisées dans les expressions, le nom d'une variable soit précédé d'un nombre et d'un tiret. Ce nombre et ce tiret vienne de notre manière de gérer les portées. Par faute de temps, nous ne pouvons pas garantir la propreté des messages d'erreurs (cf test 4_9_while_04.c et ```2-t```).

## Hexanôme H4421

- BERNARD Dorian
- DELAPORTE Tom
- DUBILLOT Elise
- FLANDRE Corentin
- SADOUN Nathan
- THOMAS Colin
- TRIMBORN Charley