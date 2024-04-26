1. Au démarrage la fréquence de clignotement sera réglée à 2 Hz
2. Utilisation des boutons-poussoirs
    - k1 pour augmenter la fréquence
    - k2 pour remettre la fréquence à sa valeur initiale
    - k3 pour diminuer la fréquence
    - optionnel : une pression continue exercée sur un bouton indiquera une auto incrémentation ou décrémentation de la fréquence.
3. Tous les changements de fréquence seront logger avec syslog de Linux. Il est possible de voir les message avec la commande `tail -f /var/log/messages`
4. Le multiplexage des entrées/sorties devra être utilisé. epoll a été choisi pour cette tâche.