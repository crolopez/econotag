/*
 * Sistemas operativos empotrados
 * Driver de las uart
 */

#include <fcntl.h>
#include <errno.h>
#include "system.h"
#include "circular_buffer.h"

/*****************************************************************************/

/**
 * Acceso estructurado a los registros de control de las uart del MC1322x
 */

typedef struct
{
	/* Registro de control de la uart */
	union
	{
		struct
		{
			uint32_t TxE		: 1;
			uint32_t RxE		: 1;
			uint32_t PEN		: 1;
			uint32_t EP			: 1;
			uint32_t ST2		: 1;
			uint32_t SB			: 1;
			uint32_t conTx		: 1;
			uint32_t Tx_oen_b	: 1;
			uint32_t			: 2;
			uint32_t xTIM		: 1;
			uint32_t FCp		: 1;
			uint32_t FCe		: 1;
			uint32_t mTxR		: 1;
			uint32_t mRxR		: 1;
			uint32_t TST		: 1;
		};
		uint32_t CON;
	};

	/* Registro de estado de la uart */
	union
	{
		struct
		{
			uint32_t SE			: 1;
			uint32_t PE			: 1;
			uint32_t FE			: 1;
			uint32_t TOE		: 1;
			uint32_t ROE		: 1;
			uint32_t RUE		: 1;
			uint32_t RxRdy		: 1;
			uint32_t TxRdy		: 1;
		};
		uint32_t STAT;
	};

	/* Registro de datos de la uart */
	union
	{
		uint8_t Rx_data;
		uint8_t Tx_data;
		uint32_t DATA;
	};

	/* Registro de control del búfer de recepción de la uart */
	union
	{
		uint32_t RxLevel			: 5;
		uint32_t Rx_fifo_addr_diff	: 6;
		uint32_t RxCON;
	};

	/* Registro de control del búfer de transmisión de la uart */
	union
	{
		uint32_t TxLevel			: 5;
		uint32_t Tx_fifo_addr_diff	: 6;
		uint32_t TxCON;
	};

	/* Registro de control del nivel de CTS de la uart */
	union
	{
		uint32_t CTS_Level			: 5;
		uint32_t CTS;
	};

	/* Registro de control la frecuencia de la uart */
	union
	{
		struct
		{
			uint32_t BRMOD          : 16;
			uint32_t BRINC          : 16;
		};
		uint32_t BR;
	};
} uart_regs_t;

/*****************************************************************************/

/**
 * Acceso estructurado a los pines de las uart del MC1322x
 */
typedef struct
{
	gpio_pin_t tx,rx,cts,rts;
} uart_pins_t;

/*****************************************************************************/

/**
 * Definición de las UARTS
 */
static volatile uart_regs_t* const uart_regs[uart_max] = {UART1_BASE, UART2_BASE};

static const uart_pins_t uart_pins[uart_max] = {
		{gpio_pin_14, gpio_pin_15, gpio_pin_16, gpio_pin_17},
		{gpio_pin_18, gpio_pin_19, gpio_pin_20, gpio_pin_21} };

static void uart_1_isr (void);
static void uart_2_isr (void);
static const itc_handler_t uart_irq_handlers[uart_max] = {uart_1_isr, uart_2_isr};

/*****************************************************************************/

/**
 * Tamaño de los búferes circulares
 */
#define __UART_BUFFER_SIZE__	256

/* Reservamos espacio para los buffers "circulares"*/
static volatile uint8_t uart_rx_buffers[uart_max][__UART_BUFFER_SIZE__];
static volatile uint8_t uart_tx_buffers[uart_max][__UART_BUFFER_SIZE__];

/* Instanciamos las estructuras de buffers que harán uso de la reserva anterior*/
static volatile circular_buffer_t uart_circular_rx_buffers[uart_max];
static volatile circular_buffer_t uart_circular_tx_buffers[uart_max];


/*****************************************************************************/

/**
 * Gestión de las callbacks
 */
typedef struct
{
	uart_callback_t tx_callback;
	uart_callback_t rx_callback;
} uart_callbacks_t;

static volatile uart_callbacks_t uart_callbacks[uart_max];

/*****************************************************************************/

/**
 * Inicializa una uart
 * @param uart	Identificador de la uart
 * @param br	Baudrate
 * @param name	Nombre del dispositivo
 * @return		Cero en caso de éxito o -1 en caso de error.
 * 				La condición de error se indica en la variable global errno
 */
int32_t uart_init (uart_id_t uart, uint32_t br, const char *name)
{
	//comprobamos errores
	if (uart > uart_2) {
		errno=ENODEV;
		return -1;
	}

	if (name == NULL) {
		errno=EFAULT;
		return -1;
	}


	uint32_t inc, mod;

    /* Fijamos los parámetros por defecto y deshabilitamos la uart */
	/* La uart debe estar deshabilitada para fijar la frecuencia */
	uart_regs[uart]->CON =	(1 << 13) |		/* MTxR = 1 - Enmascaramos las interrupciones */
							(1 << 14);		/* MRxR = 1 */

	/* Una vez fijado xTIM, y con la UART desabilitada, fijamos la frecuencia */
	mod = 9999;
	inc = br*mod/(CPU_FREQ>>4);			/* Asumimos un oversampling de 8x */
	uart_regs[uart]->BR = ( inc << 16 ) | mod;

	/* Hay que habilitar el periférico antes fijar el modo de funcionamiento de sus pines en GPIO_FUNC_SEL */
	/* Consultar la sección 11.5.1.2 Alternate Modes del datasheet: */
	/* "The peripheral function will control operation of the pad IF THE PERIPHERAL IS ENABLED." */
	uart_regs[uart]->CON |=	(1 << 0) |		/* TxE = 1 - Habilitamos la transmisión */
				            (1 << 1);		/* RxE = 1 - Habilitamos la recepción */

	/* Cambiamos el modo de funcionamiento de los pines */
	gpio_set_pin_func (uart_pins[uart].tx, gpio_func_alternate_1);
	gpio_set_pin_func (uart_pins[uart].rx, gpio_func_alternate_1);
	gpio_set_pin_func (uart_pins[uart].cts, gpio_func_alternate_1);
	gpio_set_pin_func (uart_pins[uart].rts, gpio_func_alternate_1);

	/* Fijamos TX y CTS como salidas y RX y RTS como entradas*/
	gpio_set_pin_dir_output (uart_pins[uart].tx);
	gpio_set_pin_dir_output (uart_pins[uart].cts);
	gpio_set_pin_dir_input (uart_pins[uart].rx);
	gpio_set_pin_dir_input (uart_pins[uart].rts);

	/*código driver nivel 1*/

	/*instanciamos la estructura de buffers (para rx y tx) indicandoles
	la zona de memoria donde pueden existir y su tamaño*/
	circular_buffer_init( & uart_circular_rx_buffers[uart],
		(uint8_t *)uart_rx_buffers[uart], __UART_BUFFER_SIZE__);
	circular_buffer_init( & uart_circular_tx_buffers[uart],
		(uint8_t *)uart_tx_buffers[uart], __UART_BUFFER_SIZE__);

	/*indicamos el máximo número de bytes vacíos que puede tener la cola
	de envío antes de avisar a la cpu*/
	uart_regs[uart]->TxLevel = 31;
	/*indicamos los bytes que debe haber recibido el buffer de recepción
	antes de avisar a la cpu para que los retiren*/
	uart_regs[uart]->RxLevel = 1;


		/*le decimos al controlador de interrupciones que asigne
		la uart a la entrada IRQ de la CPU*/
		itc_set_priority(itc_src_uart1+uart, itc_priority_normal);
		/*le indicamos el manejador de la interrupción*/
		itc_set_handler(itc_src_uart1+uart, uart_irq_handlers[uart]);
		/*activamos las interrupciones del uart*/
		itc_enable_interrupt(itc_src_uart1 + uart);

		/*sin funciones callback en primera instancia*/
		uart_callbacks[uart].tx_callback = NULL;
		uart_callbacks[uart].rx_callback = NULL;

		/*rehabilitamos interrupciones por recepción*/
		uart_regs[uart]->mRxR = 0;

		/*para L2*/
		bsp_register_dev (name, uart, NULL, NULL, uart_receive, uart_send, NULL, NULL, NULL);


	return 0;
}

/*****************************************************************************/

/**
 * Transmite un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que transmite el byte
 * @param uart	Identificador de la uart
 * @param c		El carácter
 */

/*envío bloqueante de 1 byte*/
void uart_send_byte (uart_id_t uart, uint8_t c)
{
	/* driver L0
	while (uart_regs[uart]->Tx_fifo_addr_diff == 0);
	uart_regs[uart]->Tx_data = c;*/

	uint32_t aux = uart_regs[uart]->mTxR;
	/*desactivamos interrupciones del transmisor*/
	uart_regs[uart]->mTxR = 1;

	/*terminamos las escrituras pendientes ya que tienen
	mayor prioridad que la que vamos a hacer ahora*/
	while (!circular_buffer_is_empty(& uart_circular_tx_buffers[uart])) {
		while (! uart_regs[uart]->Tx_fifo_addr_diff){
			/*esperamos a que haya algún hueco libre en la cola de envío*/
		};
		uart_regs[uart]->Tx_data = circular_buffer_read(& uart_circular_tx_buffers[uart]);
	}


	while (!uart_regs[uart]->Tx_fifo_addr_diff){
		/*ahora ya podemos mandar el dato. Volvemos a esperar a que haya
		sitio en la cola de envío antes de hacerlo*/
	}
	uart_regs[uart]->Tx_data = c;

	//restauramos el valor que tuviese mtxr
	uart_regs[uart]->mTxR = aux;


}

/*****************************************************************************/

/**
 * Recibe un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que recibe el byte
 * @param uart	Identificador de la uart
 * @return		El byte recibido
 */

/*recepción bloqueante de un byte*/
uint8_t uart_receive_byte (uart_id_t uart)
{
		uint8_t recep;
		uint32_t aux = uart_regs[uart]->mRxR;
		/*desactivamos interrupciones del receptor*/
		uart_regs[uart]->mRxR = 1;

		if(!circular_buffer_is_empty(& uart_circular_rx_buffers[uart])){
			recep = circular_buffer_read(& uart_circular_rx_buffers[uart]);
		}
		else{
			while (uart_regs[uart]->Rx_fifo_addr_diff == 0);
			recep= uart_regs[uart]->Rx_data;
		}

		//restauramos el valor que tuviese mtxr
		uart_regs[uart]->mRxR = aux;

		return recep;
}

/*****************************************************************************/

/**
 * Transmisión de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer con los caracteres
 * @param count	Número de caracteres a escribir
 * @return	El número de bytes almacenados en el búfer de transmisión en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_send (uint32_t uart, char *buf, size_t count)
{
	//comprobación de errores
	if (uart > uart_2) {
		errno = ENODEV;
	return -1;
	}
	if (buf == NULL || count < 0) {
		errno = EFAULT;
		return -1;
	}

	uart_regs[uart]->mTxR = 1;
	size_t i;
	for (i = 0; i < count && !circular_buffer_is_full( & uart_circular_tx_buffers[uart] ); i++)
		circular_buffer_write(& uart_circular_tx_buffers[uart], buf[i]);

	uart_regs[uart]->mTxR = 0;

	//indicamos cuánto se ha mandado
  return i;
}

/*****************************************************************************/

/**
 * Recepción de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer para almacenar los bytes
 * @param count	Número de bytes a leer
 * @return	El número de bytes realmente leídos en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_receive (uint32_t uart, char *buf, size_t count)
{
	//comprobación de errores
	if (uart > uart_2) {
		errno = ENODEV;
	return -1;
	}
	if (buf == NULL || count < 0) {
		errno = EFAULT;
		return -1;
	}
	uart_regs[uart]->mRxR = 1;
	size_t i;
	for (i = 0; i < count && !circular_buffer_is_empty( & uart_circular_rx_buffers[uart] ); i++)
		buf[i] = circular_buffer_read(& uart_circular_rx_buffers[uart]);

		uart_regs[uart]->mRxR = 0;

	//indicamos cuánto se ha recibido
  return i;

}

/*****************************************************************************/

/**
 * Fija la función callback de recepción de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_receive_callback (uart_id_t uart, uart_callback_t func)
{
		if (uart > uart_2) {
			errno = ENODEV;
			return -1;
		}
		uart_callbacks[uart].rx_callback = func;

		return 0;
}

/*****************************************************************************/

/**
 * Fija la función callback de transmisión de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_send_callback (uart_id_t uart, uart_callback_t func)
{
		if (uart > uart_2) {
			errno = ENODEV;
			return -1;
		}
		uart_callbacks[uart].tx_callback = func;

		return 0;
}

/*****************************************************************************/

/**
 * Manejador genérico de interrupciones para las uart.
 * Cada isr llamará a este manejador indicando la uart en la que se ha
 * producido la interrupción.
 * Lo declaramos inline para reducir la latencia de la isr
 * @param uart	Identificador de la uart
 */
static inline void uart_isr (uart_id_t uart)
{
/* Limpiamos los bits de error, de momento no gestionamos errores */
	uint32_t status = uart_regs[uart]->STAT;

	if (uart_regs[uart]->RxRdy) {
		/* Mientras podamos cargar datos en nuestra estructura
		y queden bytes por leer (indicado por Rx_fifo_addr_diff)*/
		while(!circular_buffer_is_full(& uart_circular_rx_buffers[uart])
		&& uart_regs[uart]->Rx_fifo_addr_diff) {
			circular_buffer_write(&uart_circular_rx_buffers[uart], uart_regs[uart]->Rx_data);
		}

		if (uart_callbacks[uart].rx_callback)
			uart_callbacks[uart].rx_callback();
		if (circular_buffer_is_full(& uart_circular_rx_buffers[uart]))
			uart_regs[uart]->mRxR = 1;
	}

	if (uart_regs[uart]->TxRdy) {
		/* Tx_fifo_addr_diff =número de bytes libres en la cola de envío
		es por ello que tendremos que llenar este buffer mientras tengamos
		con qué llenarlo*/
		while (!circular_buffer_is_empty(&uart_circular_tx_buffers[uart])
		&& uart_regs[uart]->Tx_fifo_addr_diff  )
			/*lo escrito va directamente al buffer de envío (TX FIFO)*/
			uart_regs[uart]->Tx_data = circular_buffer_read(&uart_circular_tx_buffers[uart]);

		/*si se ha elegido controlador de envío, se le da el control*/
		if (uart_callbacks[uart].tx_callback)
			uart_callbacks[uart].tx_callback();

			/*si la estructura intermedia de envío está vacía
			pedimos a la UART por favor que nos deje en paz a la hora de pedir cosas*/
		if (circular_buffer_is_empty(&uart_circular_tx_buffers[uart]))
			uart_regs[uart]->mTxR = 1;
	}



}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart1
 */
static void uart_1_isr (void)
{
	uart_isr(uart_1);
}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart2
 */
static void uart_2_isr (void)
{
	uart_isr(uart_2);
}

/*****************************************************************************/
