# My_secmalloc

Ce projet consiste à développer une bibliothèque d'allocation de mémoire sécurisée en C, qui remplace les fonctions standards telles que `malloc`, `free`, `calloc` et `realloc` en utilisant le principe de TDD.

## Auteurs

- Nathan Journo - nathan.journo@ecole2600.com
- Thibault Juillard - thibault.juillard@ecole2600.com

## Fonctionnalités

Le projet met l'accent sur la sécurité plutôt que sur la performance, avec des fonctions d'allocation qui intègrent des vérifications de sécurité pour détecter les erreurs courantes de gestion de mémoire. Les fonctionnalités incluent :

- Allocation (`malloc`), libération (`free`), allocation zéro-initialisée (`calloc`) et redimensionnement (`realloc`)
- Rapports d'exécution qui tracent les appels de fonction, les tailles des blocs alloués et les adresses.

## Pré-requis
Pour installer les pré-requis nécessaires à la compilation et aux tests, exécutez les commandes suivantes :

```bash
sudo apt install gcovr
sudo apt install criterion
```

## Compilation, Tests et Utilisations

Pour compiler le projet et exécuter les tests, utilisez les commandes suivantes :

```bash
make clean all
make clean test
make clean testcovr
make clean test_lucien
```

Les tests utilisent la bibliothèque Criterion pour les assertions et sont configurés pour couvrir divers scénarios d'allocation et de libération de mémoire.

Pour stocker les logs de l'executions dans un fichier `log_file.txt` :

```bash
export MSM_OUTPUT="log_file.txt"
```

Pour compiler une bibliotèque dynamique et lancer `ls` ou `sh` avec les implementations des fonctions d'allocations de ce projet.

```bash
make clean dynamic
LD_PRELOAD=./build/lib/libmy_secmalloc.so ls
LD_PRELOAD=./build/lib/libmy_secmalloc.so sh
```



