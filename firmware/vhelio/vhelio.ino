/*
 * vhelio.ino — Calculateur d'éclairage et de signalisation du VHélio.
 *
 * Arduino Nano + carte d'E/S rail DIN DN22D08.
 * Spécification complète dans ../../specs/.
 *
 * CE FICHIER EST VOLONTAIREMENT VIDE DE CODE.
 *
 * L'IDE Arduino réécrit tout .ino avant compilation : il y insère les
 * prototypes des fonctions qu'un ctags patché maison y détecte. Ce
 * prétraitement est fragile — avec un ctags légèrement différent, il insère
 * les prototypes AU MAUVAIS ENDROIT, et le code compilé n'est plus celui
 * qu'on a écrit. Un .cpp, lui, est compilé tel quel.
 *
 * Tout le code est donc dans src/ :
 *   src/scheduler.cpp   setup(), loop(), ordre d'appel des modules
 *   src/config.h        tous les réglages
 *   src/pins.h          brochage DN22D08 <-> Nano
 *
 * Rien à ajouter ici. Si une fonction doit être écrite, elle va dans src/.
 */
