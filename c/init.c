/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern char	*maxaddr;	/* max memory address (set in i386.c)	*/

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc( void )				/* The beginning */
{
	kprintf( "\n\nCPSC 415, 2013W1 \n32 Bit Xeros 1.1\nLocated at: %x to %x\n", &entry, &end ); 

  /* Your code goes here */
  kmeminit(); // initialize memory manager
  dispinit(); // initialize dispatcher
  contextinit();  // initialize context switcher
  di_init();  // initialize devices
  create(&root, 1024 * 8);
  create(&idleproc, 1024 * 8);
  dispatch();

  kprintf("Return to initproc(). Shouldn't happen!!!\n");
  /* This code should never be reached after you are done */
	for(;;) ; /* loop forever */
}


