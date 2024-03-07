#ifndef RELAY_H
#define RELAY_H

typedef struct {
    int pin;
    int state;
} Relay;

void relay_init(Relay *relay, int pin);
void relay_turn_on(Relay *relay);
void relay_turn_off(Relay *relay);
int relay_get_status(Relay *relay);

#endif
