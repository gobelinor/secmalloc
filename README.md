# My_secmalloc

Ce projet consiste à développer une bibliothèque d'allocation de mémoire sécurisée en C, qui remplace les fonctions standards telles que `malloc`, `free`, et `calloc` en utilisant le principe de TDD.

## Auteurs

- Nathan Journo - nathan.journo@ecole2600.com
- Thibault Juillard - thibault.juillard@ecole2600.com

## Fonctionnalités

Le projet met l'accent sur la sécurité plutôt que sur la performance, avec des fonctions d'allocation qui intègrent des vérifications de sécurité pour détecter les erreurs courantes de gestion de mémoire. Les fonctionnalités incluent :

- Allocation (`malloc`), libération (`free`) et allocation zéro-initialisée (`calloc`).
- Détournement d'`malloc` pour sécuriser les allocations mémoire.
- Rapports d'exécution qui tracent les appels de fonction, les tailles des blocs alloués et les adresses.

## Compilation et Tests

Pour compiler le projet et exécuter les tests, utilisez les commandes suivantes :

```bash
make clean all
make test
```

Les tests utilisent la bibliothèque Criterion pour les assertions et sont configurés pour couvrir divers scénarios d'allocation et de libération de mémoire.
