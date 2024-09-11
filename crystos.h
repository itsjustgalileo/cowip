#ifndef CRYSTOS_H_
#define CRYSTOS_H_

typedef struct Board Board;
typedef struct cpu cpu;

typedef struct {
    long frequency; // Frequency in Hertz (cycles per second)
} Clock;

typedef void (*update_t)(struct cpu *c);

/**
 * Simulates a clock tick by sleeping for the duration based on the specified frequency
 * and then calling the provided update function.
 * 
 * @param clock Pointer to a Clock structure containing the clock frequency.
 * @param update Function pointer to the update function to be called on each tick.
 */
void tick(Clock *clock, update_t update, struct Board *b);
#endif // !CRYSTOS_H_
