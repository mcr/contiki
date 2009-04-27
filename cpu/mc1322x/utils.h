#ifndef UTILS_H
#define UTILS_H

#define CAT2(x, y, z)  x##y##z

#define reg32(x) (*(volatile uint32_t *)(x))
#define reg16(x) (*(volatile uint16_t *)(x))

#endif /* UTILS_H */
