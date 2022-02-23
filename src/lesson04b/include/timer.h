#ifndef	_TIMER_H
#define	_TIMER_H

void timer_init ( void );
void handle_timer_irq ( void );

void generic_timer_init ( void );
void handle_generic_timer_irq ( void );

extern void gen_timer_init();
extern void gen_timer_reset();
extern int get_sys_time();
int elapsed_time();

#endif  /*_TIMER_H */
