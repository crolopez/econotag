/*
 * Sistemas operativos empotrados
 * Driver para el controlador de interrupciones del MC1322x
 */

#include "system.h"

/*****************************************************************************/

/**
 * Acceso estructurado a los registros de control del ITC del MC1322x
 */
typedef struct
{
	/* Control del arbitraje de las interrupciones */
	uint32_t INTCNTL;

	/* Deshabilitación de interrupciones normales por debajo de un nivel */
	uint32_t NIMASK;

	/* Habilitación de una interrupción por su número */
	uint32_t INTENNUM;

	/* Deshabilitación de interrupción por su número */
	uint32_t INTDISNUM;

	/* Máscara de las interrupciones */
	uint32_t INTENABLE;

	/* Tipo de interrupción (IRQ/FIQ) */
	uint32_t INTTYPE;

	/* Reservado */
	uint32_t reserved[4];

	/* Vector de interrupción normal pendiente de más prioridad*/
	uint32_t NIVECTOR;

	/* Vector de interrupción rápida pendiente de más prioridad */
	uint32_t FIVECTOR;

	/* Fuentes de interrupción */
	uint32_t INTSRC;

	/* Para forzar interrupciones (para debug) */
	uint32_t INTFRC;

	/* Interrupciones normales pendientes */
	uint32_t NIPEND;

	/* Interrupciones rápidas pendientes */
	uint32_t FIPEND;

} itc_regs_t;

static volatile itc_regs_t* const itc_regs = ITC_BASE;

/**
 * Tabla de manejadores de interrupción.
 */
static itc_handler_t itc_handlers[itc_src_max];

/*****************************************************************************/

/**
 * Inicializa el controlador de interrupciones.
 * Deshabilita los bits I y F de la CPU, inicializa la tabla de manejadores a NULL,
 * y habilita el arbitraje de interrupciones Normales y rápidas en el controlador
 * de interupciones.
 */
inline void itc_init ()
{
	uint32_t i;

	/* Inicializamos la tabla de manejadores */
	for (i=0 ; i<itc_src_max ; itc_handlers[i++] = NULL);

	/* Anulamos la generación forzosa de interrupciones */
	itc_regs->INTFRC = 0;

	/* Habilitamos el arbitraje en el controlador de interrupciones */
	itc_regs->INTCNTL = 0;

        /* Al iniciar están todas las fuentes de interrupción deshabilitadas */
	itc_regs->INTENABLE = 0;
}

/*****************************************************************************/

/**
 * Variable local para guardar el estado de habilitación de las fuentes de
 * interrupción cuando son deshabilitadas, de cara a poder restaurarlo con
 * posterioridad.
 */
static int32_t intenable = 0;

/*****************************************************************************/

/**
 * Deshabilita el envío de peticiones de interrupción a la CPU
 * Permite implementar regiones críticas en modo USER
 */
inline void itc_disable_ints ()
{
	/* Deshabilitamos el arbitraje en el controlador de interrupciones */
        /* No funciona, aunque según el manuel, debería ...*/
//	itc_regs->INTCNTL = (1 << 19) | (1 << 20);

        /* Guardamos el estado de habilitación de interrupciones */
        intenable = itc_regs->INTENABLE;

        /* Las deshabilitamos todas */
	itc_regs->INTENABLE = 0;
}

/*****************************************************************************/

/**
 * Vuelve a habilitar el envío de peticiones de interrupción a la CPU
 * Permite implementar regiones críticas en modo USER
 */
inline void itc_restore_ints ()
{
	/* Habilitamos el arbitraje en el controlador de interrupciones */
//	itc_regs->INTCNTL = 0;

        /* Dejamos la habilitación de interrupciones como estaba */
        itc_regs->INTENABLE = intenable;
}

/*****************************************************************************/

/**
 * Asigna un manejador de interrupción
 * @param src		Identificador de la fuente
 * @param handler	Manejador
 */
inline void itc_set_handler (itc_src_t src, itc_handler_t handler)
{
	itc_handlers[src] = handler;
}

/*****************************************************************************/

/**
 * Asigna una prioridad (normal o fast) a una fuente de interrupción
 * @param src		Identificador de la fuente
 * @param priority	Tipo de prioridad
 */
inline void itc_set_priority (itc_src_t src, itc_priority_t priority)
{
        if (priority)
        	itc_regs->INTTYPE = (1 << src);
        else
                itc_regs->INTTYPE &= ~ (1 << src);
}

/*****************************************************************************/

/**
 * Habilita las interrupciones de una determinda fuente
 * @param src		Identificador de la fuente
 */
inline void itc_enable_interrupt (itc_src_t src)
{
	itc_regs->INTENNUM = src;
}

/*****************************************************************************/

/**
 * Deshabilita las interrupciones de una determinda fuente
 * @param src		Identificador de la fuente
 */
inline void itc_disable_interrupt (itc_src_t src)
{
	itc_regs->INTDISNUM = src;
}

/*****************************************************************************/

/**
 * Fuerza una interrupción con propósitos de depuración
 * @param src		Identificador de la fuente
 */
inline void itc_force_interrupt (itc_src_t src)
{
	itc_regs->INTFRC |= 1 << src;
}

/*****************************************************************************/

/**
 * Desfuerza una interrupción con propósitos de depuración
 * @param src		Identificador de la fuente
 */
inline void itc_unforce_interrupt (itc_src_t src)
{
	itc_regs->INTFRC &= ~(1 << src);
}

/*****************************************************************************/

/**
 * Da servicio a la interrupción normal pendiente de más prioridad.
 * Deshabilia las IRQ de menor prioridad hasta que se haya completado el servicio
 * de la IRQ para evitar inversiones de prioridad
 */
void itc_service_normal_interrupt ()
{
	itc_handlers[itc_regs->NIVECTOR]();		/* Servimos la IRQ */
}

/*****************************************************************************/

/**
 * Da servicio a la interrupción rápida pendiente de más prioridad
 */
void itc_service_fast_interrupt ()
{
	itc_handlers[itc_regs->FIVECTOR]();
}

/*****************************************************************************/
